#include "solitaire/solvers/greedy_policy.h"

namespace solitaire::solvers {

Move GreedyPolicy::choose_move([[maybe_unused]] const GameState& state,
                               const MoveList& legal_moves) {
    // Priority order from plan.tex:
    // 1. Move cards to foundation from tableau
    // 2. Move cards to foundation from waste
    // 3. Move cards from tableau to tableau (to reveal face-down)
    // 4. Move cards from waste to tableau
    // 5. Draw from stock

    // Level 1: Tableau to foundation
    for (const auto& move : legal_moves) {
        if (move.kind == MoveKind::TableauToFoundation) {
            return move;
        }
    }

    // Level 2: Waste to foundation
    for (const auto& move : legal_moves) {
        if (move.kind == MoveKind::WasteToFoundation) {
            return move;
        }
    }

    // Level 3: Tableau to tableau (prefer moves that reveal cards)
    for (const auto& move : legal_moves) {
        if (move.kind == MoveKind::TableauToTableau) {
            return move;
        }
    }

    // Level 4: Waste to tableau
    for (const auto& move : legal_moves) {
        if (move.kind == MoveKind::WasteToTableau) {
            return move;
        }
    }

    // Level 5: Stock draw or recycle
    for (const auto& move : legal_moves) {
        if (move.kind == MoveKind::StockDraw ||
            move.kind == MoveKind::StockRecycle) {
            return move;
        }
    }

    // Fallback (shouldn't reach here if moves is non-empty)
    return legal_moves.empty() ? Move() : legal_moves[0];
}

}  // namespace solitaire::solvers
