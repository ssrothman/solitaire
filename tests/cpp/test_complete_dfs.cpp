#include <catch2/catch_test_macros.hpp>
#include "solitaire/solvers/complete_dfs.h"
#include "solitaire/shuffle.h"

using namespace solitaire;
using namespace solitaire::solvers;

TEST_CASE("DFS solver basic [seed 0]") {
    auto deck_ids = shuffle_deck(0);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    CompleteDfsSolver solver;
    SolverConfig cfg;
    cfg.max_nodes = 1000000;  // Limit for testing

    SolverResult result = solver.solve(state, cfg);
    REQUIRE(result.solvable == 1);
    REQUIRE(result.stats.nodes_explored > 0);
}

TEST_CASE("DFS solver basic [seed 111]") {
    auto deck_ids = shuffle_deck(111);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    CompleteDfsSolver solver;
    SolverConfig cfg;
    cfg.max_nodes = 100000;  // Limit for testing

    SolverResult result = solver.solve(state, cfg);
    REQUIRE(result.solvable == 0);
    REQUIRE(result.stats.nodes_explored > 0);
}