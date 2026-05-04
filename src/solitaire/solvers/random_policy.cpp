#include "solitaire/solvers/random_policy.h"

namespace solitaire::solvers {

RandomPolicy::RandomPolicy(unsigned int seed) : rng_(seed) {}

Move RandomPolicy::choose_move(const GameState& state,
                                const MoveList& legal_moves) {
    if (legal_moves.empty()) {
        return Move();
    }

    // Select random move
    std::uniform_int_distribution<int> dist(0, legal_moves.size() - 1);
    int idx = dist(rng_);
    return legal_moves[idx];
}

}  // namespace solitaire::solvers
