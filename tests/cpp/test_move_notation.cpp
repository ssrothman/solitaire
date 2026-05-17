#include <catch2/catch_test_macros.hpp>

#include "solitaire/util/move_notation.h"

using namespace solitaire;

TEST_CASE("Move notation round trips") {
    const Move moves[] = {
        Move(PileId(PileKind::Tableau, 0), PileId(PileKind::Tableau, 1), 3),
        Move(PileId(PileKind::Tableau, 0), PileId(PileKind::Foundation, 2), 1),
        Move(PileId(PileKind::Waste, 0), PileId(PileKind::Tableau, 1), 1),
        Move(PileId(PileKind::Waste, 0), PileId(PileKind::Foundation, 1), 1),
        Move(PileId(PileKind::Stock, 0), PileId(PileKind::Waste, 0), 3),
        Move(PileId(PileKind::Waste, 0), PileId(PileKind::Stock, 0), 0),
    };

    for (const auto& move : moves) {
        const std::string notation = util::move_to_notation(move);
        INFO(notation);
        auto parsed = util::move_from_notation(notation);
        REQUIRE(parsed.has_value());
        REQUIRE(parsed->source == move.source);
        REQUIRE(parsed->target == move.target);
        REQUIRE(parsed->card_count == move.card_count);
    }
}

TEST_CASE("Move notation rejects invalid input") {
    REQUIRE(!util::move_from_notation("Draw(0)").has_value());
    REQUIRE(!util::move_from_notation("X1").has_value());
}