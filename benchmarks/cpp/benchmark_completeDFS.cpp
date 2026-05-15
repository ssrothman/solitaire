#include "solitaire/shuffle.h"
#include "solitaire/solvers/complete_dfs.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

struct BenchmarkResult {
    std::size_t nodes = 0;
    int max_depth = 0;
    double seconds = 0.0;
    bool solvable = false;
    solitaire::SolverStatus status = solitaire::SolverStatus::InvalidState;
};

BenchmarkResult run_benchmark(unsigned int seed, bool enable_pruning) {
    solitaire::solvers::CompleteDfsSolver solver;

    solitaire::SolverConfig cfg;
    cfg.max_nodes = 1'000'000;
    cfg.enable_productive_move_pruning = enable_pruning;

    const auto state = solitaire::GameState::from_deck(
        solitaire::shuffle_deck(seed), solitaire::GameConfig(3, true));

    const auto start = std::chrono::steady_clock::now();
    const auto result = solver.solve(state, cfg);
    const auto end = std::chrono::steady_clock::now();

    BenchmarkResult benchmark_result;
    benchmark_result.nodes = static_cast<std::size_t>(result.stats.nodes_explored);
    benchmark_result.max_depth = result.stats.max_depth;
    benchmark_result.seconds = std::chrono::duration<double>(end - start).count();
    benchmark_result.solvable = result.solvable;
    benchmark_result.status = result.status;
    return benchmark_result;
}

void print_row(unsigned int seed,
               const BenchmarkResult& result) {
    std::cout << std::left << std::setw(8) << seed
              << std::setw(12) << result.nodes
              << std::setw(12) << result.max_depth
              << std::setw(12) << solitaire::to_string(result.status) 
              << std::setw(12) << std::fixed << std::setprecision(4) << result.seconds
              << std::setw(12) << result.solvable << '\n';
}

}  // namespace

int main() {
    const std::vector<unsigned int> seeds = {1U, 42U, 123U, 456U, 789U};

    std::cout << "Move-equivalence pruning benchmark\n";
    std::cout << "==================================\n\n";
    std::cout << std::left << std::setw(8) << "Seed"
              << std::setw(12) << "Nodes"
              << std::setw(12) << "Max Depth"
              << std::setw(12) << "Status"
              << std::setw(12) << "Time (s)"
              << std::setw(12) << "Solvable" << '\n';
    std::cout << std::string(66, '-') << '\n';

    for (unsigned int seed : seeds) {
        const auto result = run_benchmark(seed, true);
        print_row(seed, result);
    }

    return 0;
}