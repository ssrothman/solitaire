#ifndef SOLITAIRE_SOLVERS_SOLVER_H
#define SOLITAIRE_SOLVERS_SOLVER_H

#include "../types.h"
#include "../game_state.h"
#include <memory>

namespace solitaire::solvers {

// ============================================================================
// Solver Interface
// ============================================================================

class ISolver {
public:
    virtual ~ISolver() = default;

    // Solve the game starting from initial state
    virtual SolverResult solve(const GameState& initial,
                               const SolverConfig& cfg = SolverConfig()) const = 0;
};

// ============================================================================
// Policy Interface for Heuristic Play
// ============================================================================

class IPolicy {
public:
    virtual ~IPolicy() = default;

    // Choose a move from the given legal moves
    // Returns a Move, or an empty move if no choice
    virtual Move choose_move(const GameState& state,
                             const MoveList& legal_moves) = 0;

    // Optional: evaluate the consequence of a move (for advanced policies)
    virtual double evaluate_move(const GameState& state, const Move& move) {
        return 0.0;  // Default: no evaluation
    }
};

// ============================================================================
// Heuristic Runner - applies a policy with no backtracking
// ============================================================================

class HeuristicRunner {
public:
    HeuristicRunner() = default;

    // Run policy-based play from initial state
    PolicyResult run(const GameState& initial,
                     IPolicy& policy,
                     const SolverConfig& cfg = SolverConfig()) const;
};

}  // namespace solitaire::solvers

#endif  // SOLITAIRE_SOLVERS_SOLVER_H
