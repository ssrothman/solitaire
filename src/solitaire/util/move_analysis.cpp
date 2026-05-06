#include "solitaire/util/move_analysis.h"
#include "solitaire/game_state.h"
#include "solitaire/types.h"
#include "solitaire/constants.h"
#include "solitaire/util/move_notation.h"

#include <algorithm>
#include <unordered_set>
#include <cassert>

namespace {

// ============================================================================
// Helper: Check if move is obviously productive (exposes face-down)
// ============================================================================

bool obviously_productive_TtoT_move(const solitaire::Move& move, const solitaire::GameState& state) {
    return move.card_count == state.tableau_face_up_count(move.source.index()) 
           && state.tableau_face_down_count(move.source.index()) > 0;
}

// ============================================================================
// Helper: Check if move is from an empty tableau to another empty tableau
// ============================================================================

bool empty_tableau_to_empty_tableau(const solitaire::GameState& state, const solitaire::Move& move) {
    return state.tableau_face_up_count(move.source.index()) == 0 &&
           state.tableau_face_up_count(move.target.index()) == 0;
}

// ============================================================================
// DFS to explore T-to-T move chains with accumulated move tracking
// ============================================================================
// 
// Recursively explores tableau-to-tableau move chains, tracking moves seen so far
// in the chain (accumulated_moves) to prevent re-exploration of the same move.
// Only explores newly-discovered T-to-T moves at each level.
//
// Returns true if DFS discovers new T-to-F or productive empty tableau.

struct BoardStateInfo {
    int min_depth;
    solitaire::Move* initial_move;
    std::unordered_map<std::string, BoardStateInfo*> children;

    BoardStateInfo(int depth, const solitaire::Move* move) :
        min_depth(depth), initial_move(move), children() {}
};
using BoardStateCache = std::unordered_map<std::string, BoardStateInfo>;
using BoardStateSet = std::unordered_set<BoardStateInfo*>;

using FoundationsArray = std::array<bool, solitaire::NUM_FOUNDATIONS>;

void propagate_shorter_path(BoardStateInfo* current_info, int new_depth, const solitaire::Move* initial_move) {
    if (current_info->min_depth > new_depth) {

        current_info->min_depth = new_depth;
        current_info->initial_move = initial_move;

        for (auto& child_pair : current_info->children) {
            propagate_shorter_path(child_pair.second, new_depth + 1, initial_move);
        }
    }
}

void DFS_worker(const solitaire::GameState& current_state,
                const FoundationsArray& foundations_accessible,
                const bool empty_tableaus_productive,
                const int original_empty_tableaus,
                const int depth,
                const solitaire::Move* initial_move,
                BoardStateCache& cache,
                BoardStateSet& productive_states){

    // Precondition: current_state has not already been visited (not in cache)

    std::string board_repr = current_state.board_fingerprint();

    const MoveList avail_moves = current_state.legal_moves();

    // First check if this state is productive
    // Because if it is we are already done, and don't need to explore further
    
    bool productive = false;

    if (empty_tableaus_productive) {
        int current_empty = 0;
        for (int i = 0; i < solitaire::NUM_TABLEAU_PILES; ++i) {
            if (current_state.tableau_face_up_count(i) == 0) {
                current_empty++;
            }
        }
        if (current_empty > original_empty_tableaus) {
            productive = true;
        }
    }

    if (!productive) {
        for (const auto& move : avail_moves){
            if (move.kind == solitaire::MoveKind::TableauToFoundation 
                && !foundations_accessible[move.target.index()]) {
                // Found a new T-to-F move, so this state is productive
                // We can stop exploring further from this state
                productive = true;
                break;
            }
        }
    }

    if (productive) {
        cache[board_repr] = BoardStateInfo(depth, initial_move);
        productive_states.insert(cache[board_repr]);
        return;
    }

    // If not productive, explore further
    for (const auto& move : avail_moves){
        // skip non-T-to-T moves (productivity already obvious)
        if (move.kind != solitaire::MoveKind::TableauToTableau) continue;
        // skip obviously productive moves (already counted as productive, no need to explore)
        if (obviously_productive_TtoT_move(move, current_state)) continue;
        // skip empty tableau to empty tableau moves (never productive, would bloat runtime)
        if (empty_tableau_to_empty_tableau(current_state, move)) continue;

        solitaire::GameState next = current_state.apply_move(move);
        std::string next_repr = next.board_fingerprint();

        if (cache.count(next_repr) == 0) {
            // If we've never seen this state before, explore further with DFS
            DFS_worker(
                next, 
                foundations_accessible, empty_tableaus_productive,
                original_empty_tableaus, depth + 1, 
                initial_move, 
                cache, productive_states
            );
        } else {
            // If we have seen this state before, propagate shorter path if aplicable
            // (the relevant check happens inside the propagate_shorter_path function)
            propagate_shorter_path(*cache[next_repr], depth + 1, initial_move);
        }
    }
}            

// ============================================================================
// Stock cycle detection
// ============================================================================

bool is_stock_cycle_no_op(const solitaire::GameState& original_state) {

    if (original_state.waste_size() % original_state.config().cards_per_draw != 0) {
        return false;  // Not aligned with draw cycle, can't be a no-op
    }
    
    int initial_position = original_state.waste_size();
    bool once_through = false;

    
    // Loop until we return to the original position (cycle detected) 
    // Or we discover a waste-origin move (proving it's not a no-op)
    GameState current = original_state;
    while (!once_through || current.waste_size() < initial_position) {
        if (current.stock_size() > 0) {
            solitaire::Move draw(
                solitaire::PileId(solitaire::PileKind::Stock, 0),
                solitaire::PileId(solitaire::PileKind::Waste, 0),
                solitaire::MoveKind::StockDraw,
                current.config().cards_per_draw);
                
            // Should always be legal because we have already checked stock_size > 0
            assert(current.is_legal(draw));  
            current = current.apply_move(draw);
        } else {
            // We've been through the stock at least once
            once_through = true;

            solitaire::Move recycle(
                solitaire::PileId(solitaire::PileKind::Waste, 0),
                solitaire::PileId(solitaire::PileKind::Stock, 0),
                solitaire::MoveKind::StockRecycle,
                current.waste_size());

            // Should always be legal 
            // If unlimited recycle is disabled, we should have already returned
            assert(current.is_legal(recycle));  
            current = current.apply_move(recycle);
        }


        auto legal = current.legal_moves();
        
        // Check for waste-origin moves (would enable progress)
        bool has_waste_moves = false;
        for (const auto& m : legal) {
            if (m.kind == solitaire::MoveKind::WasteToTableau ||
                m.kind == solitaire::MoveKind::WasteToFoundation) {

                return false;  // Found a move that can make progress, so not a no-op
            }
        }

        // If we do not have unlimited recycle and we've run out of stock
        // Then we can not make progress, so this is a no-op
        if (current.waste_size() == 0 && !current.config().unlimited_recycle) {
            return true;
        }        
    }

    // If I've set up the logic correctly, we have now returned to the original position 
    // And we haven't found any waste-origin moves, so this is a no-op

    assert(current.same_position(original_state));
    return true;
}

}  // namespace

namespace solitaire::util {

// ============================================================================
// Main API: Return list of all NON-no-op moves (moves that make progress)
// ============================================================================

MoveList all_non_no_op_moves(const GameState& state) {
    MoveList non_no_ops;

    MoveList moves = state.legal_moves();
    
    // ========================================================================
    // Phase 1: Early returns for obviously productive moves
    // ========================================================================
    
    std::vector<Move> nontrivial_ttt;  // T-to-T moves requiring DFS analysis
    
    for (const auto& move : moves) {
        // Skip if move is illegal
        assert(state.is_legal(move));
        
        // Waste-to-* moves: always progress
        // And so do tableau-to-foundation moves
        if (move.kind == MoveKind::WasteToTableau || 
            move.kind == MoveKind::WasteToFoundation ||
            move.kind == MoveKind::TableauToFoundation) {

            non_no_ops.push_back(move);
            continue;
        }
        
        
        // Stock Draw/Recycle: check for cycles
        if (move.kind == MoveKind::StockDraw || move.kind == MoveKind::StockRecycle) {
            if (!is_stock_cycle_no_op(state)) {
                non_no_ops.push_back(move);
            }
            continue;
        }
        
        // Tableau-to-Tableau: check for obvious productivity
        if (move.kind == MoveKind::TableauToTableau) {
            // Exposes face-down: always progress
            if (obviously_productive_TtoT_move(move, state)) {
                non_no_ops.push_back(move);
                continue;
            }
            
            // Empty to empty: never progress (skip)
            if (empty_tableau_to_empty_tableau(state, move)) {
                continue;
            }
            
            // Nontrivial: requires DFS analysis
            nontrivial_ttt.push_back(move);
            continue;
        }
    }
    
    // ========================================================================
    // Phase 2: DFS analysis for nontrivial T-to-T moves
    // ========================================================================
    
    if (!nontrivial_ttt.empty()) {
        // Pre-compute: which T-to-F destinations are accessible from initial state?
        std::array<bool, NUM_FOUNDATIONS> foundations_accessible;
        foundations_accessible.fill(false);
        for (const auto& move : moves) {
            if (move.kind == MoveKind::TableauToFoundation) {
                foundations_accessible[move.target.index()] = true;
            }
        }
        
        // Pre-compute: are new empty tableaus useful?
        // (Yes if there are more available Kings than current empty tableaus)
        int available_kings = 0;
        int current_empty = 0;
        if (state.has_waste() && state.waste_top().rank() == Rank::King) {
            available_kings++;
        }
        for (int i = 0; i < NUM_TABLEAU_PILES; ++i) {
            if (state.tableau_face_up_count(i) == 0) {
                current_empty++;
            }
            if (state.tableau_face_up_count(i) > 0 && state.tableau_top(i).rank() == Rank::King) {
                available_kings++;
            }
        }
        bool empty_tableaus_productive = (available_kings > current_empty);
        
        // Run DFS for each nontrivial T-to-T move
        BoardStateCache cache;
        BoardStateSet productive_states;

        for (const auto& move : nontrivial_ttt) {
            GameState next = state.apply_move(move);

            std::string next_repr = next.board_fingerprint();
            if (cache.count(next_repr) != 0) {
                continue;
            }

            DFS_worker(
                next,
                foundations_accessible,
                empty_tableaus_productive,
                current_empty,
                1,
                &move,
                cache,
                productive_states
            );
        }

        // Collect all moves that lead to productive states
        for (BoardStateInfo* info : productive_states) {
            assert(info->initial_move != nullptr);

            // Need to check if the initial move is already in non_no_ops
            // (One move could lead to multiple productive states??)
            bool already_in_non_no_ops = false;
            for (const auto& m : non_no_ops) {
                if (m == *info->initial_move) {
                    already_in_non_no_ops = true;
                    break;
                }
            }

            if (already_in_non_no_ops) {
                printf("DEBUG: Move %s can result in multiple productive board states. I wasn't sure this was possible\n", util::move_to_notation(*info->initial_move).c_str());
                continue;
            } else {
                non_no_ops.push_back(*info->initial_move);
            }
        }
    }
    
    return non_no_ops;
}

}  // namespace solitaire::util
