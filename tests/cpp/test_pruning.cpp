#include <catch2/catch_test_macros.hpp>

#include "solitaire/solvers/pruning.h"
#include "solitaire/shuffle.h"

using namespace solitaire;
using namespace solitaire::solvers;

namespace {

class KeepFirstMoveStrategy : public IPruningStrategy {
public:
    MoveList filter(const GameState&, const MoveList& moves) const override {
        MoveList filtered;
        if (!moves.empty()) {
            filtered.push_back(moves.front());
        }
        return filtered;
    }
};

class KeepLastMoveStrategy : public IPruningStrategy {
public:
    MoveList filter(const GameState&, const MoveList& moves) const override {
        MoveList filtered;
        if (!moves.empty()) {
            filtered.push_back(moves.back());
        }
        return filtered;
    }
};

}  // namespace

TEST_CASE("Pruning: compound strategy applies filters in sequence", "[pruning]") {
    GameState state = GameState::from_deck(shuffle_deck(42), GameConfig(3, true));
    MoveList moves = state.legal_moves();
    REQUIRE(moves.size() >= 2);  // Ensure we have enough moves to test
    
    CompoundPruningStrategy compound;
    compound.add_strategy(std::make_unique<KeepFirstMoveStrategy>());
    compound.add_strategy(std::make_unique<KeepLastMoveStrategy>());

    MoveList filtered = compound.filter(state, moves);
    REQUIRE(filtered.size() == 1);
    REQUIRE(filtered.front().source == moves.front().source);
    REQUIRE(filtered.front().target == moves.front().target);
    REQUIRE(filtered.front().kind == moves.front().kind);
    REQUIRE(filtered.front().card_count == moves.front().card_count);
}

TEST_CASE("Pruning: factory builds enabled strategies", "[pruning]") {
    SolverConfig cfg;
    cfg.enable_productive_move_pruning = true;

    auto strategy = make_pruning_strategy(cfg);
    REQUIRE(strategy != nullptr);

    GameState state = GameState::from_deck(shuffle_deck(123), GameConfig(3, true));
    MoveList moves = state.legal_moves();
    MoveList filtered = strategy->filter(state, moves);

    REQUIRE(filtered.size() <= moves.size());
}

TEST_CASE("Pruning: factory returns no strategy when disabled", "[pruning]") {
    SolverConfig cfg;
    cfg.enable_productive_move_pruning = false;

    auto strategy = make_pruning_strategy(cfg);
    REQUIRE(strategy == nullptr);
}