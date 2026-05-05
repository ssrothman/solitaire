#include "solitaire/util/move_analysis.h"

#include <algorithm>
#include <optional>
#include <unordered_set>

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

// ============================================================================

// Helper: Extract all T-to-T moves from a state, excluding those that expose face-down cards
// (moves that expose face-down are already handled as early returns in is_non_stock_no_op)
solitaire::MoveList get_tableau_to_tableau_moves(const solitaire::GameState& state) {
    auto legal_moves = state.legal_moves();
    solitaire::MoveList ttt_moves;
    for (const auto& move : legal_moves) {
        if (move.kind == solitaire::MoveKind::TableauToTableau) {
            // Only consider moves that don't expose face-down cards
            if (move.card_count < state.tableau_face_up_count(move.source.index()) || state.tableau_face_down_count(move.source.index()) == 0) {
                ttt_moves.push_back(move);
            }
        }
    }
    return ttt_moves;
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

// Forward declaration
bool dfs_tableau_moves(const solitaire::GameState& original_state,
                       const solitaire::GameState& current_state,
                       const solitaire::MoveList& original_ttf_moves,
                       const solitaire::MoveList& accumulated_moves,
                       std::unordered_set<std::size_t>& visited);

// Check if current state has a new T-to-F move not in the original state
bool has_new_ttf_move(const solitaire::GameState& current_state,
                      const solitaire::MoveList& original_ttf_moves) {
    auto current_moves = current_state.legal_moves();
    
    // Find all T-to-F moves in current state
    for (const auto& move : current_moves) {
        if (move.kind != solitaire::MoveKind::TableauToFoundation) {
            continue;
        }
        
        // Check if this T-to-F was NOT in the original state
        // Only the foundation destination matters for "newness" — ignore
        // the source tableau index to avoid false positives when moving
        // stacks that contain a card movable to the foundation.
        bool found_in_original = false;
        for (const auto& orig : original_ttf_moves) {
            if (orig.target.kind() == solitaire::PileKind::Foundation &&
                move.target.kind() == solitaire::PileKind::Foundation &&
                orig.target.index() == move.target.index()) {
                found_in_original = true;
                break;
            }
        }
        
        if (!found_in_original) {
            return true;  // Found a new T-to-F move
        }
    }
    
    return false;
}

// DFS helper: explore only newly-exposed T-to-T moves from current state
// Returns true if a new T-to-F move is discovered
// 
// accumulated_moves: All T-to-T moves seen from the start of the chain to this point
//                    (we only explore moves not in this set, to avoid re-exploring)
bool dfs_tableau_moves(const solitaire::GameState& original_state,
                       const solitaire::GameState& current_state,
                       const solitaire::MoveList& original_ttf_moves,
                       const solitaire::MoveList& accumulated_moves,
                       std::unordered_set<std::size_t>& visited) {
    std::size_t current_hash = compute_board_hash(current_state);
    
    // If we've already visited this state in this DFS, skip it (prevents infinite loops)
    if (visited.count(current_hash) > 0) {
        return false;
    }
    visited.insert(current_hash);
    
    // Check if current state has a new T-to-F move
    if (has_new_ttf_move(current_state, original_ttf_moves)) {
        return true;
    }
    
    // Get all legal T-to-T moves from current state
    auto current_ttt_moves = get_tableau_to_tableau_moves(current_state);
    
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
        
        if (dfs_tableau_moves(original_state, next, original_ttf_moves, 
                              new_accumulated, visited)) {
            return true;
        }
    }
    
    // No new T-to-F moves found
    return false;
}

// Check if a T-to-T move is a no-op by DFS analysis
// Note: Early returns for progress/face-down exposure are handled in is_non_stock_no_op()
bool is_tableau_to_tableau_no_op(const solitaire::GameState& state,
                                 const solitaire::GameState& after) {
    // Get the original T-to-F moves available
    auto original_moves = state.legal_moves();
    solitaire::MoveList original_ttf_moves;
    for (const auto& m : original_moves) {
        if (m.kind == solitaire::MoveKind::TableauToFoundation) {
            original_ttf_moves.push_back(m);
        }
    }
    
    // Get the T-to-T moves from the original state (starting accumulated set)
    auto original_ttt_moves = get_tableau_to_tableau_moves(state);
    
    // DFS from the after state to find new T-to-F moves
    // Only explore T-to-T moves not in original_ttt_moves (cumulative across chain)
    std::unordered_set<std::size_t> visited;
    return !dfs_tableau_moves(state, after, original_ttf_moves, original_ttt_moves, visited);
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
    if (move.kind != solitaire::MoveKind::TableauToTableau) {
        return false;  // Shouldn't reach here, but be safe
    }

    // Check if this move exposes a face-down card
    // This happens when card_count equals the number of face-up cards on the source
    // (moving all visible cards reveals the hidden cards beneath)
    if (move.card_count == state.tableau_face_up_count(move.source.index()) && state.tableau_face_down_count(move.source.index()) > 0) {
        return false;  // Exposes face-down cards, not a no-op
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
