#ifndef SOLITAIRE_SOLVERS_RANDOM_POLICY_H
#define SOLITAIRE_SOLVERS_RANDOM_POLICY_H

#include "solver.h"
#include <random>

namespace solitaire::solvers {

// ============================================================================
// Random Policy
// ============================================================================
// Randomly selects from available legal moves

class RandomPolicy : public IPolicy {
public:
    explicit RandomPolicy(unsigned int seed = 0);

    Move choose_move(const GameState& state, const MoveList& legal_moves) override;

private:
    mutable std::mt19937 rng_;
};

}  // namespace solitaire::solvers

#endif  // SOLITAIRE_SOLVERS_RANDOM_POLICY_H
