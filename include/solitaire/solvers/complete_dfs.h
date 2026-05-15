#ifndef SOLITAIRE_SOLVERS_COMPLETE_DFS_H
#define SOLITAIRE_SOLVERS_COMPLETE_DFS_H

#include "solver.h"
#include "pruning.h"
#include <unordered_set>

namespace solitaire::solvers {

// ============================================================================
// Complete DFS Solver
// ============================================================================

class CompleteDfsSolver : public ISolver {
public:
    CompleteDfsSolver() = default;

    // Solve using depth-first search with state hashing
    SolverResult solve(const GameState& initial,
                       const SolverConfig& cfg = SolverConfig()) const override;

private:
    // DFS exploration with backtracking
    struct SearchState {
        GameState state;
        std::vector<Move> path;
        int depth = 0;
    };

    // Iterative DFS exploration using explicit stack
    void dfs_explore_(SearchState& initial_state,
                      std::unordered_set<std::size_t>& visited,
                      const SolverConfig& cfg,
                      SearchStats& stats,
                      MoveList& solution,
                      const IPruningStrategy* pruning) const;
};

}  // namespace solitaire::solvers

#endif  // SOLITAIRE_SOLVERS_COMPLETE_DFS_H
