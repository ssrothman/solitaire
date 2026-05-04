#include "solitaire/solvers/pruning.h"

namespace solitaire::solvers {

MoveList EquivalencePruningStrategy::filter(const GameState& state,
                                            const MoveList& moves) const {
    auto canonical_indices = util::find_move_equivalence_classes(state, moves);
    MoveList filtered;
    filtered.reserve(canonical_indices.size());
    for (std::size_t idx : canonical_indices) {
        filtered.push_back(moves[idx]);
    }
    return filtered;
}

MoveList NoOpPruningStrategy::filter(const GameState& state,
                                       const MoveList& moves) const {
    MoveList filtered;
    filtered.reserve(moves.size());

    for (const auto& move : moves) {
        if (!util::is_no_op_move(state, move)) {
            filtered.push_back(move);
        }
    }

    return filtered;
}

void CompoundPruningStrategy::add_strategy(std::unique_ptr<IPruningStrategy> strategy) {
    if (strategy) {
        _strategies.push_back(std::move(strategy));
    }
}

MoveList CompoundPruningStrategy::filter(const GameState& state,
                                         const MoveList& moves) const {
    MoveList current = moves;
    for (const auto& strategy : _strategies) {
        current = strategy->filter(state, current);
    }
    return current;
}

std::unique_ptr<CompoundPruningStrategy> make_pruning_strategy(const SolverConfig& cfg) {
    auto strategy = std::make_unique<CompoundPruningStrategy>();

    if (cfg.enable_move_equivalence_pruning) {
        strategy->add_strategy(std::make_unique<EquivalencePruningStrategy>());
    }
    if (cfg.enable_no_op_pruning) {
        strategy->add_strategy(std::make_unique<NoOpPruningStrategy>());
    }

    if (strategy->empty()) {
        return nullptr;
    }

    return strategy;
}

}  // namespace solitaire::solvers