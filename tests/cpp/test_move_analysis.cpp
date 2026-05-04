#include <catch2/catch_test_macros.hpp>
#include <array>
#include <vector>

#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"
#include "solitaire/util/move_analysis.h"

using namespace solitaire;
using namespace solitaire::util;

namespace {

std::vector<Card> make_stock_cycle_no_op_deck() {
    std::array<Card, 7> tableau_tops = {
        Card(Suit::Hearts, Rank::Ten),
        Card(Suit::Diamonds, Rank::Ten),
        Card(Suit::Hearts, Rank::Jack),
        Card(Suit::Diamonds, Rank::Jack),
        Card(Suit::Hearts, Rank::Queen),
        Card(Suit::Diamonds, Rank::Queen),
        Card(Suit::Hearts, Rank::King),
    };

    std::vector<Card> stock_candidates;
    std::vector<Card> hidden_candidates;
    stock_candidates.reserve(24);
    hidden_candidates.reserve(21);

    std::array<bool, 52> used{};
    auto mark_used = [&](const Card& card) {
        used[static_cast<std::size_t>(card.raw_data())] = true;
    };

    for (const auto& card : tableau_tops) {
        mark_used(card);
    }

    for (int suit = 0; suit < 4; ++suit) {
        for (int rank = 1; rank <= 13; ++rank) {
            Card card(static_cast<Suit>(suit), static_cast<Rank>(rank));
            if (used[static_cast<std::size_t>(card.raw_data())]) {
                continue;
            }
            const bool blocks_tableau = card.color() == Color::Black &&
                                        static_cast<int>(card.rank()) >= 9 &&
                                        static_cast<int>(card.rank()) <= 12;
            const bool blocks_foundation = card.rank() == Rank::Ace || card.rank() == Rank::Two;

            if (rank >= 3 && !blocks_tableau && !blocks_foundation) {
                stock_candidates.push_back(card);
            } else {
                hidden_candidates.push_back(card);
            }
        }
    }

    std::vector<Card> deck(52);
    const std::array<int, 7> top_positions = {0, 2, 5, 9, 14, 20, 27};
    std::size_t top_idx = 0;
    std::size_t stock_idx = 0;
    std::size_t hidden_idx = 0;

    for (int position = 0; position < 52; ++position) {
        bool is_top = false;
        for (int top_position : top_positions) {
            if (position == top_position) {
                is_top = true;
                break;
            }
        }

        if (is_top) {
            deck[position] = tableau_tops[top_idx++];
        } else if (position >= 28) {
            deck[position] = stock_candidates[stock_idx++];
        } else {
            deck[position] = hidden_candidates[hidden_idx++];
        }
    }

    return deck;
}

std::vector<Card> make_progress_deck() {
    std::vector<Card> deck = make_stock_cycle_no_op_deck();
    const Card removed = deck[0];
    const Card ace_of_spades(Suit::Spades, Rank::Ace);
    deck[0] = ace_of_spades;

    for (std::size_t i = 1; i < deck.size(); ++i) {
        auto& card = deck[i];
        if (card == ace_of_spades) {
            card = removed;
            break;
        }
    }

    return deck;
}

std::vector<Card> make_useful_stock_draw_deck() {
    std::vector<Card> deck = make_stock_cycle_no_op_deck();
    const Card useful(Suit::Hearts, Rank::Two);
    const std::size_t useful_position = 49;
    const Card displaced = deck[useful_position];

    deck[useful_position] = useful;

    for (std::size_t i = 0; i < deck.size(); ++i) {
        if (i != useful_position && deck[i] == useful) {
            deck[i] = displaced;
            break;
        }
    }

    return deck;
}

}  // namespace

TEST_CASE("Move analysis: useless stock milling is detected as a no-op", "[move_analysis]") {
    GameState state = GameState::from_deck(make_stock_cycle_no_op_deck(), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    REQUIRE(moves.size() == 1);
    REQUIRE(moves.front().kind == MoveKind::StockDraw);
    REQUIRE(is_no_op_move(state, moves.front()));
}

TEST_CASE("Move analysis: progress moves are not treated as no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(make_progress_deck(), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    bool checked_progress_move = false;
    for (const auto& move : moves) {
        if (move.kind == MoveKind::TableauToFoundation) {
            REQUIRE_FALSE(is_no_op_move(state, move));
            checked_progress_move = true;
            break;
        }
    }

    REQUIRE(checked_progress_move);
}

TEST_CASE("Move analysis: useful stock draws are not no-ops", "[move_analysis]") {
    GameState state = GameState::from_deck(make_useful_stock_draw_deck(), GameConfig(3, true));
    MoveList moves = state.legal_moves();

    REQUIRE_FALSE(moves.empty());
    REQUIRE(moves.front().kind == MoveKind::StockDraw);
    REQUIRE_FALSE(is_no_op_move(state, moves.front()));
}

TEST_CASE("Move analysis: stock cycle recurrence is treated as no-op", "[move_analysis]") {
    GameState start = GameState::from_deck(make_stock_cycle_no_op_deck(), GameConfig(3, true));
    GameState state = start;

    while (state.stock_size() > 0) {
        MoveList moves = state.legal_moves();
        REQUIRE_FALSE(moves.empty());
        REQUIRE(moves.front().kind == MoveKind::StockDraw);
        state = state.apply_move(moves.front());
    }

    Move recycle(
        PileId(PileKind::Waste, 0),
        PileId(PileKind::Stock, 0),
        MoveKind::StockRecycle,
        state.waste_size());

    REQUIRE(state.is_legal(recycle));
    REQUIRE(is_no_op_move(state, recycle));

    GameState recycled = state.apply_move(recycle);
    REQUIRE(recycled.same_position(start));
    REQUIRE(recycled != start);
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
