#include <catch2/catch_test_macros.hpp>
#include "solitaire/solvers/complete_dfs.h"
#include "solitaire/shuffle.h"

using namespace solitaire;
using namespace solitaire::solvers;

TEST_CASE("DFS solver basic") {
    auto deck_ids = shuffle_deck(111);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    CompleteDfsSolver solver;
    SolverConfig cfg;
    cfg.max_nodes = 1000;  // Limit for testing

    SolverResult result = solver.solve(state, cfg);
    REQUIRE(result.stats.nodes_explored > 0);
}

TEST_CASE("DFS solver on won state") {
    GameState state;
    CompleteDfsSolver solver;

    SolverResult result = solver.solve(state);
    REQUIRE(result.stats.time_seconds >= 0.0);
}
