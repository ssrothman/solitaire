#include <catch2/catch_test_macros.hpp>
#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"

using namespace solitaire;

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

TEST_CASE("Move structural validation") {
    SECTION("Valid move shapes") {
        REQUIRE(Move(PileId(PileKind::Tableau, 0),
                     PileId(PileKind::Tableau, 1),
                     MoveKind::TableauToTableau,
                     2)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Tableau, 3),
                     PileId(PileKind::Foundation, 2),
                     MoveKind::TableauToFoundation,
                     1)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Waste, 0),
                     PileId(PileKind::Tableau, 4),
                     MoveKind::WasteToTableau,
                     1)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Waste, 0),
                     PileId(PileKind::Foundation, 1),
                     MoveKind::WasteToFoundation,
                     1)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Stock, 0),
                     PileId(PileKind::Waste, 0),
                     MoveKind::StockDraw,
                     3)
                    .is_valid());
        REQUIRE(Move(PileId(PileKind::Waste, 0),
                     PileId(PileKind::Stock, 0),
                     MoveKind::StockRecycle,
                     0)
                    .is_valid());
    }

    SECTION("Invalid move shapes") {
        REQUIRE_FALSE(Move(PileId(PileKind::Tableau, 1),
                           PileId(PileKind::Tableau, 1),
                           MoveKind::TableauToTableau,
                           1)
                          .is_valid());
        REQUIRE_FALSE(Move(PileId(PileKind::Tableau, 0),
                           PileId(PileKind::Foundation, 0),
                           MoveKind::TableauToFoundation,
                           2)
                          .is_valid());
        REQUIRE_FALSE(Move(PileId(PileKind::Waste, 1),
                           PileId(PileKind::Tableau, 0),
                           MoveKind::WasteToTableau,
                           1)
                          .is_valid());
        REQUIRE_FALSE(Move(PileId(PileKind::Stock, 0),
                           PileId(PileKind::Waste, 0),
                           MoveKind::StockDraw,
                           0)
                          .is_valid());
    }
}
