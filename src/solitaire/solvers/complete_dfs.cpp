#include "solitaire/solvers/complete_dfs.h"
#include "solitaire/solvers/pruning.h"
#include <chrono>
#include <stack>

namespace solitaire::solvers {

SolverResult CompleteDfsSolver::solve(const GameState& initial,
                                       const SolverConfig& cfg) const {

    printf("Starting DFS solver with max_depth=%d, max_nodes=%d\n", cfg.max_depth, cfg.max_nodes);

    SolverResult result;
    SearchStats stats;
    MoveList solution;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Check if already won
    if (initial.is_won()) {
        result.solvable = true;
        auto end_time = std::chrono::high_resolution_clock::now();
        stats.time_seconds =
            std::chrono::duration<double>(end_time - start_time).count();
        result.stats = stats;
        return result;
    }

    std::unordered_set<std::size_t> visited;
    auto pruning = make_pruning_strategy(cfg);
    SearchState root_state;
    root_state.state = initial;
    root_state.depth = 0;

    if (dfs_explore_(root_state, visited, cfg, stats, solution, pruning.get())) {
        result.solvable = true;
        result.solution = solution;
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    stats.time_seconds =
        std::chrono::duration<double>(end_time - start_time).count();
    result.stats = stats;

    return result;
}

bool CompleteDfsSolver::dfs_explore_(SearchState& current,
                                      std::unordered_set<std::size_t>& visited,
                                      const SolverConfig& cfg, SearchStats& stats,
                                      MoveList& solution,
                                      const IPruningStrategy* pruning) const {

    printf("Exploring depth %d, nodes explored %d\n", current.depth, stats.nodes_explored);

    // Check depth and node limits
    if (current.depth > cfg.max_depth || stats.nodes_explored > cfg.max_nodes) {
        return false;
    }

    // Check if visited
    std::size_t state_hash = current.state.hash();
    if (visited.count(state_hash)) {
        return false;
    }
    visited.insert(state_hash);
    stats.nodes_explored++;
    stats.max_depth = std::max(stats.max_depth, current.depth);
    printf("\t Visited states: %zu (current depth: %d)\n", visited.size(), current.depth);

    // Check win condition
    if (current.state.is_won()) {
        solution = current.path;
        return true;
    }
    printf("\tNot a winning state\n");

    // Check loss condition
    if (current.state.is_lost()) {
        return false;
    }
    printf("\tNot a losing state\n");

    // Generate legal moves
    MoveList legal = current.state.legal_moves();
    printf("\tGenerated %zu legal moves\n", legal.size());

    if (pruning) {
        const auto before = legal.size();
        legal = pruning->filter(current.state, legal);
        stats.moves_pruned += static_cast<int>(before - legal.size());
    }
    printf("\tAfter pruning, %zu moves remain\n", legal.size());

    // Explore each move
    for (const auto& move : legal) {
        GameState next = current.state.apply_move(move);

        SearchState next_state;
        next_state.state = next;
        next_state.path = current.path;
        next_state.path.push_back(move);
        next_state.depth = current.depth + 1;

        if (dfs_explore_(next_state, visited, cfg, stats, solution, pruning)) {
            return true;
        }
    }

    return false;
}

}  // namespace solitaire::solvers