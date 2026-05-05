#include "solitaire/util/move_analysis.h"
#include "solitaire/game_state.h"
#include "solitaire/types.h"
#include "solitaire/constants.h"
#include "solitaire/util/move_notation.h"

#include <algorithm>
#include <optional>
#include <unordered_set>
#include <cassert>

namespace {

// ============================================================================
// Helper: Compute board-only hash (ignores stock position and turn)
// ============================================================================

std::size_t compute_board_hash(const solitaire::GameState& state) {
    // Use a board-only fingerprint provided by GameState which excludes
    // solver metadata such as turn counts and stock cycle position.
    std::string board_repr = state.board_fingerprint();
    std::hash<std::string> hasher;
    return hasher(board_repr);
}

// ============================================================================
// DFS for T-to-T move chains
// Helper: Compare two moves for equality (source, target, kind)
bool are_moves_equal(const solitaire::Move& a, const solitaire::Move& b) {
    return a.kind == b.kind && a.source == b.source && a.target == b.target
           && a.card_count == b.card_count;
}

// Helper: check if a move is "obviously productive"
// This short circuits the DFS for simple cases
// where a T-to-T move clearly exposes a face-down card, which is always productive.
bool obviously_productive_TtoT_move(const solitaire::Move& move, const solitaire::GameState& state) {
    if (move.card_count == state.tableau_face_up_count(move.source.index()) 
       && (state.tableau_face_down_count(move.source.index()) > 0)) {
            return true; 
    } 

    return false;
}

bool empty_tableau_to_empty_tableau(const solitaire::GameState& state, const solitaire::Move& move) {
    return state.tableau_face_up_count(move.source.index()) == 0 &&
           state.tableau_face_up_count(move.target.index()) == 0;
}

// ============================================================================

// Helper: Filter all T-to-T moves that need DFS analysis
// This excludes moves that obviously expose face-down cards, which are always productive and don't need DFS analysis.
// This also excludes moves that are empty tableau to empty tableau, which are always not productive and don't need DFS analysis.

solitaire::MoveList filter_tableau_to_tableau_moves_for_DFS(const solitaire::MoveList& moves, const solitaire::GameState& state) {
    solitaire::MoveList ttt_moves;
    for (const auto& move : moves) {
        if (move.kind == solitaire::MoveKind::TableauToTableau && !obviously_productive_TtoT_move(move, state) && !empty_tableau_to_empty_tableau(state, move)) {
            ttt_moves.push_back(move);
        }
    }
    return ttt_moves;
}

// Helper: Extract all T-to-T moves that need DFS analysis
solitaire::MoveList get_tableau_to_tableau_moves_for_DFS(const solitaire::GameState& state) {
    auto legal_moves = state.legal_moves();
    return filter_tableau_to_tableau_moves_for_DFS(legal_moves, state);
}

// Helper: Check if a move is in a move list
bool move_in_list(const solitaire::Move& move, const solitaire::MoveList& moves) {
    for (const auto& m : moves) {
        if (m.source == move.source &&
            m.target == move.target &&
            m.card_count == move.card_count) {
            return true;
        }
    }
    return false;
}

using foundations_array_t = std::array<bool, solitaire::NUM_FOUNDATIONS>;

// Forward declaration
bool dfs_tableau_moves(
    const solitaire::GameState& current_state,
    const foundations_array_t& foundations_accessible,
    const bool empty_tableaus_productive,
    const int original_empty_tableaus,
    const solitaire::MoveList& accumulated_moves,
    std::unordered_set<std::size_t>& visited
);

// Check if current state has a new T-to-F move not in the original state
bool has_new_ttf_move(const solitaire::MoveList& current_moves,
                      const foundations_array_t& foundations_accessible) {
        
    // Find all T-to-F moves in current state
    for (const auto& move : current_moves) {
        if (move.kind == solitaire::MoveKind::TableauToFoundation && !foundations_accessible[move.target.index()]) {
            return true;
        }        
    }
    
    return false;
}

bool has_new_empty_tableau_move(const solitaire::GameState& current_state,
                              const int original_empty_tableaus) {
    int current_empty_tableaus = 0;
    for (int i = 0; i < solitaire::NUM_TABLEAU_PILES; ++i) {
        if (current_state.tableau_face_up_count(i) == 0) {
            current_empty_tableaus++;
        }
    }
    return (current_empty_tableaus > original_empty_tableaus);
}

// DFS helper: explore only newly-exposed T-to-T moves from current state
// Returns true if a new T-to-F move is discovered
// 
// accumulated_moves: All T-to-T moves seen from the start of the chain to this point
//                    (we only explore moves not in this set, to avoid re-exploring)
bool dfs_tableau_moves(const solitaire::GameState& current_state,
                       const foundations_array_t& foundations_accessible,
                       const bool empty_tableaus_productive,
                       const int original_empty_tableaus,
                       const solitaire::MoveList& accumulated_moves,
                       std::unordered_set<std::size_t>& visited) {
    std::size_t current_hash = compute_board_hash(current_state);
    
    // If we've already visited this state in this DFS, skip it (prevents infinite loops)
    if (visited.count(current_hash) > 0) {
        return false;
    }
    visited.insert(current_hash);
    
    auto current_moves = current_state.legal_moves();

    // Check if current state has a new T-to-F move
    if (has_new_ttf_move(current_moves, foundations_accessible)) {
        return true;
    }
    if (empty_tableaus_productive && has_new_empty_tableau_move(current_state, original_empty_tableaus)) {
        return true;
    }
    
    // Get all legal T-to-T moves from current state
    auto current_ttt_moves = filter_tableau_to_tableau_moves_for_DFS(current_moves, current_state);
    
    // Explore only moves not in the accumulated set
    // (moves that haven't been seen anywhere in the chain so far)
    for (const auto& move : current_ttt_moves) {
        // Skip moves that have already been seen in this chain
        if (move_in_list(move, accumulated_moves)) {
            continue;
        }
        
        // Apply this newly-exposed T-to-T move and continue DFS
        solitaire::GameState next = current_state.apply_move(move);
        
        // Build new accumulated set: previous accumulated moves + current moves
        solitaire::MoveList new_accumulated = accumulated_moves;
        for (const auto& m : current_ttt_moves) {
            if (!move_in_list(m, new_accumulated)) {
                new_accumulated.push_back(m);
            }
        }
        
        if (dfs_tableau_moves(next, foundations_accessible,
                              empty_tableaus_productive,
                              original_empty_tableaus, 
                              new_accumulated, visited)) {
            return true;
        }
    }
    
    // No new T-to-F moves found
    return false;
}

// Check if a T-to-T move is a no-op by DFS analysis
bool is_tableau_to_tableau_no_op(const solitaire::GameState& state,
                                 const solitaire::GameState& after) {

    // Get the original T-to-F moves available
    auto original_moves = state.legal_moves();

    // We need to track which foundation piles are accessible 
    // from the tableau in the initial state.
    // This allows us to ignore T-to-F moves 
    // that were already available in the DFS, 
    // even if the specific source tableau index changes due to T-to-T moves.
    foundations_array_t foundations_accessible;
    foundations_accessible.fill(false);
    for (const auto& m : original_moves) {
        if (m.kind == solitaire::MoveKind::TableauToFoundation) {
            foundations_accessible[m.target.index()] = true;
        }
    }

    // The other kind of productive move that can be revealed by T-to-T moves
    // is a new king-to-empty-tableau move.
    // We track this by:
    // 1) counting the available kings in the original state (including tableau and waste)
    // 2) counting the empty tableaus in the original state
    // If there are more available kings than empty tableaus, 
    // then any newly exposed empty tableaus are automatically productive
    // we will check for this in the DFS
    int available_kings = 0;
    int empty_tableaus = 0;
    if (state.has_waste() && state.waste_top().rank() == solitaire::Rank::King) {
        available_kings++;
    }
    for (int i = 0; i < solitaire::NUM_TABLEAU_PILES; ++i) {
        if (state.tableau_face_up_count(i) == 0) {
            empty_tableaus++;
        }
        if (state.tableau_face_up_count(i) > 0 && state.tableau_top(i).rank() == solitaire::Rank::King) {
            available_kings++;
        }
    }
    bool empty_tableaus_productive = (available_kings > empty_tableaus);
    
    // Get the T-to-T moves from the original state
    // This will be both the list of moves to run DFS exploration on,
    auto original_ttt_moves = get_tableau_to_tableau_moves_for_DFS(state);
    
    // DFS from the after state to find new T-to-F moves
    // Only explore T-to-T moves not in original_ttt_moves (cumulative across chain)
    std::unordered_set<std::size_t> visited;
    // TODO: This will be the piece that I rework by hand
    // I need to plumb through empty_tableaus_productive()
    // And to make sure the treatment of which moves to ignore is correct
    // I think I'll replace this direct call with an explicit outer loop
    // It would be nice to have some notion of the fewest steps to arrive at a productive
    // move, so that we can reject moves which are not along the shortest path to a productive move, 
    // Well see if I feel like that's worth iT
    return !dfs_tableau_moves(
        after, 
        foundations_accessible, 
        empty_tableaus_productive,
        empty_tableaus,
        original_ttt_moves, 
        visited
    );
}

bool is_waste_origin_move(const solitaire::Move& move) {
    return move.kind == solitaire::MoveKind::WasteToTableau ||
           move.kind == solitaire::MoveKind::WasteToFoundation;
}

bool has_waste_origin_moves(const solitaire::MoveList& moves) {
    return std::any_of(moves.begin(), moves.end(), [](const solitaire::Move& move) {
        return is_waste_origin_move(move);
    });
}

bool is_non_stock_no_op(const solitaire::GameState& state, const solitaire::Move& move) {
    // Waste-to-Foundation, Tableau-to-Foundation, Waste-to-Tableau moves are never no-ops
    if (move.kind == solitaire::MoveKind::WasteToFoundation ||
        move.kind == solitaire::MoveKind::TableauToFoundation ||
        move.kind == solitaire::MoveKind::WasteToTableau) {
        return false;
    }

    // Only Tableau-to-Tableau moves reach here
    // Check with an assertion
    assert(move.kind == solitaire::MoveKind::TableauToTableau);

    // Check if this move exposes a face-down card
    // This happens when card_count equals the number of face-up cards on the source
    // (moving all visible cards reveals the hidden cards beneath)
    if (obviously_productive_TtoT_move(move, state)) {
        return false;  // Exposes face-down cards, not a no-op
    }
    if (empty_tableau_to_empty_tableau(state, move)) {
        return true;  // Empty tableau to empty tableau, always a no-op
    }
    
    // Apply the move for DFS analysis
    solitaire::GameState after = state.apply_move(move);
    
    // Use DFS for remaining analysis
    return is_tableau_to_tableau_no_op(state, after);
}

bool is_stock_cycle_no_op(const solitaire::GameState& original_state,
                          const solitaire::GameState& start_after_move) {
    solitaire::GameState current = start_after_move;
    std::vector<solitaire::GameState> seen_positions;
    seen_positions.push_back(current);

    while (true) {
        auto legal = current.legal_moves();
        if (has_waste_origin_moves(legal)) {
            return false;
        }

        if (current.stock_size() > 0) {
            solitaire::Move draw(
                solitaire::PileId(solitaire::PileKind::Stock, 0),
                solitaire::PileId(solitaire::PileKind::Waste, 0),
                solitaire::MoveKind::StockDraw,
                current.config().cards_per_draw);
            if (!current.is_legal(draw)) {
                return false;
            }
            current = current.apply_move(draw);
            continue;
        }

        if (current.waste_size() == 0 || !current.config().unlimited_recycle) {
            return false;
        }

        solitaire::Move recycle(
            solitaire::PileId(solitaire::PileKind::Waste, 0),
            solitaire::PileId(solitaire::PileKind::Stock, 0),
            solitaire::MoveKind::StockRecycle,
            current.waste_size());
        if (!current.is_legal(recycle)) {
            return false;
        }

        current = current.apply_move(recycle);
        if (current.same_position(original_state)) {
            return true;
        }

        for (const auto& seen : seen_positions) {
            if (current.same_position(seen)) {
                return true;
            }
        }
        seen_positions.push_back(current);
    }
}

}  // namespace

namespace solitaire::util {

bool is_no_op_move(const GameState& state, const Move& move) {
    // If the move is illegal, it's not a no-op.
    if (!state.is_legal(move)) {
        return false;
    }

    GameState next = state.apply_move(move);

    // If the move leaves the position unchanged, it's a no-op.
    if (next == state) {
        return true;
    }


    // If the move is a stock draw/recycle then we need a more complicated check...
    if (move.kind == MoveKind::StockDraw || move.kind == MoveKind::StockRecycle) {
        return is_stock_cycle_no_op(state, next);
    }
    
    return is_non_stock_no_op(state, move);
}

}  // namespace solitaire::util
