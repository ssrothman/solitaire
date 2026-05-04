#ifndef SOLITAIRE_SOLVERS_GREEDY_POLICY_H
#define SOLITAIRE_SOLVERS_GREEDY_POLICY_H

#include "solver.h"

namespace solitaire::solvers {

// ============================================================================
// Greedy Policy
// ============================================================================
// Implements exact priority order from plan.tex:
// 1. Move cards to foundation from tableau
// 2. Move cards to foundation from waste
// 3. Move cards from tableau to tableau (to reveal face-down)
// 4. Move cards from waste to tableau
// 5. Draw from stock

class GreedyPolicy : public IPolicy {
public:
    GreedyPolicy() = default;

    Move choose_move(const GameState& state, const MoveList& legal_moves) override;
};

}  // namespace solitaire::solvers

#endif  // SOLITAIRE_SOLVERS_GREEDY_POLICY_H
