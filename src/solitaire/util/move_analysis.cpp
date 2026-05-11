#include "solitaire/util/move_analysis.h"
#include "solitaire/game_state.h"
#include "solitaire/types.h"
#include "solitaire/constants.h"
#include "solitaire/util/move_notation.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

#define DEBUG_MOVE_ANALYSIS 1

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

// If reason code < 0, it's productive due to new empty tableau
//     With value = (index of new empty tableau) - 1
// If the reason code > 0, it's productive due to access to a new foundation
//     With value = index of the newly accessible foundation + 1
// If the reason code = 0, thats a default value somehow passing through due to a bug
using ReasonCode = int8_t;

struct BoardStateInfo {
    std::string myhash;

    uint8_t min_depth;
    solitaire::Move initial_move;
    bool productive;
    ReasonCode productive_reason;
    std::unordered_set<std::string> children;

    BoardStateInfo(uint8_t depth, const solitaire::Move& move)
        : min_depth(depth), initial_move(move), 
        productive(false), productive_reason(0),
        children() {}
};

using BoardStateCache = std::unordered_map<std::string, std::shared_ptr<BoardStateInfo>>;
using ProductiveStateSet = std::unordered_map<ReasonCode, std::string>; 

using FoundationsArray = std::array<bool, solitaire::NUM_FOUNDATIONS>;

void propagate_shorter_path(const std::shared_ptr<BoardStateInfo>& current_info,
                            uint8_t new_depth,
                            BoardStateCache& cache,
                            ProductiveStateSet& productive_states,
                            const solitaire::Move& initial_move) {

    if (new_depth >= current_info->min_depth) {
        return;
    }

    current_info->min_depth = new_depth;
    current_info->initial_move = initial_move;

    if (current_info->productive){
        assert(current_info->productive_reason != 0);
        assert(productive_states.count(current_info->productive_reason) > 0);
        if (cache[productive_states[current_info->productive_reason]]->min_depth > new_depth) {
            productive_states[current_info->productive_reason] = current_info->myhash;
        }
    }

    for (auto& child_fingerprint : current_info->children) {
        propagate_shorter_path(
            cache[child_fingerprint], 
            new_depth + 1, 
            cache, 
            productive_states, 
            initial_move
        );
    }
}

std::pair<std::shared_ptr<BoardStateInfo>, bool> get_or_create_node(
    BoardStateCache& cache,
    ProductiveStateSet& productive_states,
    const std::string& fingerprint,
    uint8_t depth,
    const solitaire::Move& initial_move) {

    auto it = cache.find(fingerprint);
    if (it == cache.end()) {
        auto node = std::make_shared<BoardStateInfo>(depth, initial_move);
        cache.emplace(fingerprint, node);
        cache[fingerprint]->myhash = fingerprint;
        return {node, true};
    }

    auto& node = it->second;
    if (depth < node->min_depth) {
        propagate_shorter_path(
            node, 
            depth, 
            cache, 
            productive_states, 
            initial_move
        );
    }
    return {node, false};
}

void DFS_worker(const solitaire::GameState& current_state,
                const solitaire::Move& causing_move,
                const FoundationsArray& foundations_accessible,
                const bool empty_tableaus_productive,
                const uint8_t original_empty_tableaus,
                const uint8_t depth,
                const solitaire::Move& initial_move,
                const std::shared_ptr<BoardStateInfo>& current_node,
                BoardStateCache& cache,
                ProductiveStateSet& productive_states) {

    const solitaire::MoveList avail_moves = current_state.legal_moves();

    bool productive = false;
    ReasonCode productive_reason = 0; // Default to invalid reason code

    if (empty_tableaus_productive) {
        int8_t current_empty = 0;
        for (int i = 0; i < solitaire::NUM_TABLEAU_PILES; ++i) {
            if (current_state.tableau_face_up_count(i) == 0) {
                current_empty++;
            }
        }
        if (current_empty > original_empty_tableaus) {
            productive = true;
            // Use negative reason codes for new empty tableau, 
            // with value = -(index of new empty tableau + 1) 
            productive_reason = -(causing_move.source.index() + 1); 
        }
    }

    if (!productive) {
        for (const auto& move : avail_moves) {
            if (move.kind == solitaire::MoveKind::TableauToFoundation &&
                !foundations_accessible[move.target.index()]) {
                productive = true;
                // Use positive reason codes for new foundation access, 
                // with value = (index of newly accessible foundation + 1)
                productive_reason = move.target.index() + 1; 
                break;
            }
        }
    }

    if (productive) {
        cache[current_state.board_fingerprint()]->productive = true;
        cache[current_state.board_fingerprint()]->productive_reason = productive_reason;

        if (productive_states.count(productive_reason) == 0 || depth < cache[productive_states[productive_reason]]->min_depth) {
            productive_states[productive_reason] = current_state.board_fingerprint();
        } 
        return;
    }

    for (const auto& move : avail_moves) {
        if (move.kind != solitaire::MoveKind::TableauToTableau) {
            continue;
        }
        if (obviously_productive_TtoT_move(move, current_state)) {
            continue;
        }
        if (empty_tableau_to_empty_tableau(current_state, move)) {
            continue;
        }

        solitaire::GameState next = current_state.apply_move(move);
        const std::string next_fingerprint = next.board_fingerprint();
        const auto [child_node, created] = get_or_create_node(
            cache, productive_states,
             next_fingerprint, 
             depth + 1, 
             initial_move
        );
        current_node->children.insert(next_fingerprint);

        if (created) {
            DFS_worker(
                next,
                move,
                foundations_accessible,
                empty_tableaus_productive,
                original_empty_tableaus,
                depth + 1,
                initial_move,
                child_node,
                cache,
                productive_states
            );
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
    solitaire::GameState current = original_state;
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


        const auto legal = current.legal_moves();

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
        ProductiveStateSet productive_states;

        for (const auto& move : nontrivial_ttt) {
            GameState next = state.apply_move(move);

            std::string next_repr = next.board_fingerprint();
            const auto [root_node, created] = get_or_create_node(
                cache, productive_states, 
                next_repr, 1, 
                move
            );
            if (!created) {
                continue;
            }

            DFS_worker(
                next,
                move,
                foundations_accessible,
                empty_tableaus_productive,
                current_empty,
                1,
                move,
                root_node,
                cache,
                productive_states
            );
        }

        // Collect all moves that lead to productive states
        std::unordered_set<std::string> emitted_moves;
        for (const auto& productive : productive_states) {
            const auto& initial_move = cache[productive.second]->initial_move;

            const std::string move_key = util::move_to_notation(initial_move);
            if (emitted_moves.insert(move_key).second) {
                non_no_ops.push_back(initial_move);
            }
        }
    }
    
    return non_no_ops;
}

}  // namespace solitaire::util
