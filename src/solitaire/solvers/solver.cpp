#include "solitaire/solvers/solver.h"
#include <chrono>

namespace solitaire::solvers {

PolicyResult HeuristicRunner::run(const GameState& initial, IPolicy& policy,
                                   [[maybe_unused]] const SolverConfig& cfg) const {
    PolicyResult result;
    auto start_time = std::chrono::high_resolution_clock::now();

    GameState current = initial;
    int iterations = 0;
    int max_iterations = 1000000;  // Prevent infinite loops

    while (iterations < max_iterations) {
        iterations++;

        // Check win/loss
        if (current.is_won()) {
            result.won = true;
            break;
        }

        if (current.is_lost()) {
            result.won = false;
            break;
        }

        // Get legal moves
        MoveList legal = current.legal_moves();
        if (legal.empty()) {
            result.won = false;
            break;
        }

        // Choose move using policy
        Move chosen = policy.choose_move(current, legal);

        // Verify move is legal
        bool found = false;
        for (const auto& m : legal) {
            if (m.source == chosen.source && m.target == chosen.target &&
                m.kind == chosen.kind) {
                found = true;
                break;
            }
        }

        if (!found) {
            // Policy returned illegal move, stop
            result.won = false;
            break;
        }

        // Apply move
        result.moves.push_back(chosen);
        current = current.apply_move(chosen);

        // Track stock cycles (turns)
        if (chosen.kind == MoveKind::StockRecycle) {
            result.turns++;
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    result.stats.time_seconds =
        std::chrono::duration<double>(end_time - start_time).count();
    result.stats.nodes_explored = iterations;

    return result;
}

}  // namespace solitaire::solvers
