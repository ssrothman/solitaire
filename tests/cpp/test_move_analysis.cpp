#include <catch2/catch_test_macros.hpp>
#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"
#include "solitaire/util/move_analysis.h"

using namespace solitaire;
using namespace solitaire::util;

TEST_CASE("Move analysis: legal moves are not conservative no-ops in sample states", "[move_analysis]") {
    for (unsigned seed = 1; seed <= 10; ++seed) {
        GameState state = GameState::from_deck(shuffle_deck(seed), GameConfig(3, true));
        MoveList moves = state.legal_moves();

        for (const auto& move : moves) {
            REQUIRE_FALSE(is_no_op_move(state, move));
        }
    }
}

TEST_CASE("Move analysis: illegal moves are not treated as no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(shuffle_deck(42), GameConfig(3, true));

    Move illegal;
    illegal.source = PileId(PileKind::Tableau, 0);
    illegal.target = PileId(PileKind::Tableau, 0);
    illegal.kind = MoveKind::TableauToTableau;
    illegal.card_count = 1;

    REQUIRE_FALSE(is_no_op_move(state, illegal));
}

TEST_CASE("Move analysis: helper is deterministic on repeated calls", "[move_analysis]") {
    GameState state = GameState::from_deck(shuffle_deck(123), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    for (const auto& move : moves) {
        REQUIRE(is_no_op_move(state, move) == is_no_op_move(state, move));
    }
}
