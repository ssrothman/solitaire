#include <catch2/catch_test_macros.hpp>
#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"

using namespace solitaire;

TEST_CASE("State hashing consistency") {
    auto deck_ids = shuffle_deck(999);
    GameState s1 = GameState::from_deck(deck_ids, GameConfig());
    GameState s2 = GameState::from_deck(deck_ids, GameConfig());

    REQUIRE(s1.hash() == s2.hash());
}

TEST_CASE("State equality") {
    auto deck_ids = shuffle_deck(555);
    GameState s1 = GameState::from_deck(deck_ids, GameConfig());
    GameState s2 = GameState::from_deck(deck_ids, GameConfig());

    REQUIRE(s1 == s2);
}

TEST_CASE("Move application") {
    auto deck_ids = shuffle_deck(777);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    MoveList moves = state.legal_moves();
    if (!moves.empty()) {
        GameState next = state.apply_move(moves[0]);
        REQUIRE(next.stock_size() >= 0);
    }
}
