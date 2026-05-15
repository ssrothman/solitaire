#include "solitaire/solvers/complete_dfs.h"
#include "solitaire/solvers/pruning.h"
#include <chrono>
#include <stack>

namespace solitaire::solvers {

SolverResult CompleteDfsSolver::solve(const GameState& initial,
                                       const SolverConfig& cfg) const {

    SolverResult result;
    SearchStats stats;
    MoveList solution;

    auto start_time = std::chrono::high_resolution_clock::now();

    // Check if already won
    if (initial.is_won()) {
        result.solvable = true;
        result.status = SolverStatus::Success;
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

    dfs_explore_(root_state, visited, cfg, stats, solution, pruning.get());
    
    if (!solution.empty()) {
        result.solvable = true;
        result.status = SolverStatus::Success;
    } else if (stats.nodes_explored >= cfg.max_nodes) {
        result.status = SolverStatus::ReachedMaxNodes;
    } else if (stats.max_depth >= cfg.max_depth) {
        result.status = SolverStatus::ReachedMaxDepth;
    } else {
        result.status = SolverStatus::Success;  // Unsolvable, but fully explored
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    stats.time_seconds =
        std::chrono::duration<double>(end_time - start_time).count();
    result.stats = stats;

    return result;
}

void CompleteDfsSolver::dfs_explore_(SearchState& initial_state,
                                      std::unordered_set<std::size_t>& visited,
                                      const SolverConfig& cfg, SearchStats& stats,
                                      MoveList& solution,
                                      const IPruningStrategy* pruning) const {

    std::stack<SearchState> work_stack;
    work_stack.push(initial_state);

    while (!work_stack.empty() && solution.empty()) {
        SearchState current = work_stack.top();
        work_stack.pop();

        // Check depth and node limits
        if (current.depth > cfg.max_depth || stats.nodes_explored > cfg.max_nodes) {
            continue;
        }

        // Check if visited
        std::size_t state_hash = current.state.hash();
        if (visited.count(state_hash)) {
            continue;
        }
        visited.insert(state_hash);
        stats.nodes_explored++;
        stats.max_depth = std::max(stats.max_depth, current.depth);

        // Check win condition
        if (current.state.is_won()) {
            solution = current.path;
            return;
        }

        // Check loss condition
        if (current.state.is_lost()) {
            continue;
        }

        // Generate legal moves
        MoveList legal = current.state.legal_moves();

        if (pruning) {
            const auto before = legal.size();
            legal = pruning->filter(current.state, legal);
            stats.moves_pruned += static_cast<int>(before - legal.size());
        }

        // Push each move as a new state onto the stack (in reverse order to maintain DFS order)
        for (int i = static_cast<int>(legal.size()) - 1; i >= 0; --i) {
            const auto& move = legal[i];
            GameState next = current.state.apply_move(move);

            SearchState next_state;
            next_state.state = next;
            next_state.path = current.path;
            next_state.path.push_back(move);
            next_state.depth = current.depth + 1;

            work_stack.push(next_state);
        }
    }
}

}  // namespace solitaire::solvers