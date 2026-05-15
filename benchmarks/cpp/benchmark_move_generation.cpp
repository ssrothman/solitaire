#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>

namespace {

struct GenerationResult {
    std::size_t total_moves = 0;
    double seconds = 0.0;
};

GenerationResult benchmark_generation(unsigned int seed, int iterations) {
    const auto state = solitaire::GameState::from_deck(
        solitaire::shuffle_deck(seed), solitaire::GameConfig(3, true));

    std::size_t total_moves = 0;
    const auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < iterations; ++i) {
        const auto moves = state.legal_moves();
        total_moves += moves.size();
    }

    const auto end = std::chrono::steady_clock::now();

    GenerationResult result;
    result.total_moves = total_moves;
    result.seconds = std::chrono::duration<double>(end - start).count();
    return result;
}

}  // namespace

int main() {
    const std::vector<unsigned int> seeds = {1U, 42U, 123U, 456U, 789U};
    const int iterations = 2000;

    std::cout << "Move generation benchmark\n";
    std::cout << "=========================\n\n";
    std::cout << std::left << std::setw(8) << "Seed"
              << std::setw(16) << "Total Moves"
              << std::setw(14) << "Time (s)"
              << std::setw(16) << "Moves / s" << '\n';
    std::cout << std::string(54, '-') << '\n';

    for (unsigned int seed : seeds) {
        const auto result = benchmark_generation(seed, iterations);
        const double moves_per_second = result.total_moves / std::max(result.seconds, 0.000001);

        std::cout << std::left << std::setw(8) << seed
                  << std::setw(16) << result.total_moves
                  << std::setw(14) << std::fixed << std::setprecision(4) << result.seconds
                  << std::setw(16) << std::setprecision(2) << moves_per_second << '\n';
    }

    return 0;
}