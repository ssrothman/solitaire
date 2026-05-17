#include <catch2/catch_test_macros.hpp>

#include <string>
#include <utility>
#include <vector>

#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"

using namespace solitaire;

namespace {

std::vector<Card> make_ordered_deck() {
    std::vector<Card> deck;
    deck.reserve(DECK_SIZE);
    for (int suit = 0; suit < NUM_SUITS; ++suit) {
        for (int rank = 1; rank <= NUM_RANKS; ++rank) {
            deck.emplace_back(static_cast<Suit>(suit), static_cast<Rank>(rank));
        }
    }
    return deck;
}

}  // namespace

TEST_CASE("Card creation and comparison") {
    Card c1(Suit::Hearts, Rank::Ace);
    Card c2(Suit::Hearts, Rank::Ace);
    Card c3(Suit::Spades, Rank::King);

    REQUIRE(c1 == c2);
    REQUIRE(c1 != c3);
}

TEST_CASE("Tableau move validation") {
    auto deck_ids = shuffle_deck(12345);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    MoveList moves = state.legal_moves();
    REQUIRE(!moves.empty());
}

TEST_CASE("Game won condition") {
    GameState state;
    REQUIRE(!state.is_won());
}

TEST_CASE("Stock draw") {
    auto deck_ids = shuffle_deck(42);
    GameState state = GameState::from_deck(deck_ids, GameConfig());

    int initial_stock = state.stock_size();
    REQUIRE(initial_stock > 0);
}

TEST_CASE("GameState string prints all four foundation piles") {
    GameState state = GameState::from_deck(shuffle_deck(0), GameConfig());
    const std::string text = state.to_string();

    REQUIRE(text.find("Foundation: -- -- -- --") != std::string::npos);
}

TEST_CASE("Removing last face-up tableau card reveals hidden card") {
    auto deck = make_ordered_deck();

    // T1 layout after dealing: deck[1] is hidden, deck[2] is face-up.
    std::swap(deck[2], deck[0]);   // put A♥ at deck[2] so it can move to foundation
    std::swap(deck[1], deck[38]);  // put K♣ at deck[1] as the hidden card to reveal

    GameState state = GameState::from_deck(deck, GameConfig());
    REQUIRE(state.tableau_face_down_count(1) == 1);
    REQUIRE(state.tableau_face_up_count(1) == 1);
    REQUIRE(state.tableau_top(1) == Card(Suit::Hearts, Rank::Ace));

    Move move(PileId(PileKind::Tableau, 1),
              PileId(PileKind::Foundation, static_cast<int>(Suit::Hearts)),
              1);
    REQUIRE(state.is_legal(move));

    GameState next = state.apply_move(move);
    REQUIRE(next.foundation_top(static_cast<int>(Suit::Hearts)) == Card(Suit::Hearts, Rank::Ace));
    REQUIRE(next.tableau_face_down_count(1) == 0);
    REQUIRE(next.tableau_face_up_count(1) == 1);
    REQUIRE(next.tableau_top(1) == Card(Suit::Clubs, Rank::King));
}

TEST_CASE("Moving multiple tableau cards preserves their order") {
    auto deck = make_ordered_deck();

    auto place = [&](std::size_t position, Suit suit, Rank rank) {
        const Card desired(suit, rank);
        for (std::size_t i = 0; i < deck.size(); ++i) {
            if (deck[i] == desired) {
                std::swap(deck[position], deck[i]);
                return;
            }
        }
    };

    place(0, Suit::Hearts, Rank::Four);   // T0 top: 4♥
    place(2, Suit::Clubs, Rank::Three);    // T1 top: 3♣
    place(5, Suit::Spades, Rank::Five);    // T2 top: 5♠

    GameState state = GameState::from_deck(deck, GameConfig());

    Move move_to_tableau(
        PileId(PileKind::Tableau, 1),
        PileId(PileKind::Tableau, 0),
        1);
    REQUIRE(state.is_legal(move_to_tableau));
    state = state.apply_move(move_to_tableau);

    Move move_run(
        PileId(PileKind::Tableau, 0),
        PileId(PileKind::Tableau, 2),
        2);
    REQUIRE(state.is_legal(move_run));

    GameState next = state.apply_move(move_run);
    const std::string text = next.to_string();
    REQUIRE(text.find("T2: 3♣ 4♥ 5♠ [2 hidden]") != std::string::npos);
}

TEST_CASE("Move structural validation") {
    SECTION("Valid move shapes") {
        REQUIRE(Move(PileId(PileKind::Tableau, 0),
                     PileId(PileKind::Tableau, 1),
                     2)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Tableau, 3),
                     PileId(PileKind::Foundation, 2),
                     1)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Waste, 0),
                     PileId(PileKind::Tableau, 4),
                     1)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Waste, 0),
                     PileId(PileKind::Foundation, 1),
                     1)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Stock, 0),
                     PileId(PileKind::Waste, 0),
                     3)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Waste, 0),
                     PileId(PileKind::Stock, 0),
                     0)
                    .is_valid());
    }

    SECTION("Invalid move shapes") {
        REQUIRE_FALSE(Move(PileId(PileKind::Tableau, 1),
                           PileId(PileKind::Tableau, 1),
                           1)
                          .is_valid());
        REQUIRE_FALSE(Move(PileId(PileKind::Tableau, 0),
                           PileId(PileKind::Foundation, 0),
                           2)
                          .is_valid());
        REQUIRE_FALSE(Move(PileId(PileKind::Waste, 1),
                           PileId(PileKind::Tableau, 0),
                           1)
                          .is_valid());
        REQUIRE_FALSE(Move(PileId(PileKind::Stock, 0),
                           PileId(PileKind::Waste, 0),
                           0)
                          .is_valid());
    }
}
