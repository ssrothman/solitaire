#include <catch2/catch_test_macros.hpp>
#include "solitaire/solvers/greedy_policy.h"
#include "solitaire/solvers/random_policy.h"
#include "solitaire/shuffle.h"
#include "solitaire/game_state.h"

using namespace solitaire;
using namespace solitaire::solvers;

TEST_CASE("Greedy policy basic") {
    auto deck_ids = shuffle_deck(222);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    GreedyPolicy policy;
    HeuristicRunner runner;
    SolverConfig cfg;
    cfg.max_nodes = 10000;

    PolicyResult result = runner.run(state, policy, cfg);
    REQUIRE(result.stats.nodes_explored > 0);
}

TEST_CASE("Random policy basic") {
    auto deck_ids = shuffle_deck(333);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    RandomPolicy policy(42);
    HeuristicRunner runner;

    PolicyResult result = runner.run(state, policy);
    REQUIRE(result.stats.nodes_explored > 0);
}

TEST_CASE("Greedy policy determinism") {
    auto deck_ids = shuffle_deck(444);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    GreedyPolicy policy;
    HeuristicRunner runner;

    PolicyResult result1 = runner.run(state, policy);
    PolicyResult result2 = runner.run(state, policy);

    REQUIRE(result1.moves.size() == result2.moves.size());
}
