#include <catch2/catch_test_macros.hpp>

#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"
#include "solitaire/util/board_equivalence.h"

using namespace solitaire;
using namespace solitaire::util;

namespace {

bool is_no_reveal_tableau_move(const GameState& before, const Move& move) {
    if (move.kind != MoveKind::TableauToTableau) {
        return false;
    }

    const int src = move.source.index();
    return move.card_count < before.tableau_face_up_count(src);
}

}  // namespace

TEST_CASE("Board equivalence: canonicalization is idempotent", "[board_equivalence]") {
    GameState state = GameState::from_deck(shuffle_deck(42), GameConfig(3, true));
    GameState canonical = canonicalize_board_state(state);

    REQUIRE(canonicalize_board_state(canonical) == canonical);
    REQUIRE(board_states_equivalent(state, canonical));
}

TEST_CASE("Board equivalence: zero-reveal tableau moves stay in the same class", "[board_equivalence]") {
    GameState state = GameState::from_deck(shuffle_deck(42), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    bool found = false;
    for (const auto& move : moves) {
        if (!is_no_reveal_tableau_move(state, move)) {
            continue;
        }

        GameState next = state.apply_move(move);
        REQUIRE(board_states_equivalent(state, next));
        REQUIRE(canonicalize_board_state(state) == canonicalize_board_state(next));
        found = true;
        break;
    }

    if (!found) {
        SKIP("No zero-reveal tableau move available in this state");
    }
}

TEST_CASE("Board equivalence: transitive closure is respected", "[board_equivalence]") {
    GameState state = GameState::from_deck(shuffle_deck(123), GameConfig(3, true));
    MoveList first_moves = state.legal_moves();

    bool found_chain = false;
    for (const auto& first_move : first_moves) {
        if (!is_no_reveal_tableau_move(state, first_move)) {
            continue;
        }

        GameState first = state.apply_move(first_move);
        MoveList second_moves = first.legal_moves();
        for (const auto& second_move : second_moves) {
            if (!is_no_reveal_tableau_move(first, second_move)) {
                continue;
            }

            GameState second = first.apply_move(second_move);
            REQUIRE(board_states_equivalent(state, first));
            REQUIRE(board_states_equivalent(first, second));
            REQUIRE(board_states_equivalent(state, second));
            found_chain = true;
            break;
        }

        if (found_chain) {
            break;
        }
    }

    if (!found_chain) {
        SKIP("No two-step zero-reveal tableau chain found in this state");
    }
}
