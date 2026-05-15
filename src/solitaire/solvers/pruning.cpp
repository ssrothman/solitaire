#include "solitaire/solvers/pruning.h"
#include <unordered_set>
namespace solitaire::solvers {

MoveList CompoundPruningStrategy::filter(const GameState& state,
                                         const MoveList& moves) const {
    MoveList current = moves;
    for (const auto& strategy : _strategies) {
        current = strategy->filter(state, current);
    }
    return current;
}

class ProductiveMovePruningStrategy : public IPruningStrategy {
public:
    MoveList filter(const GameState& state, const MoveList& moves) const override {
        MoveList productive_moves = util::all_non_no_op_moves(state);
        std::unordered_set<size_t> productive_hashes;
        for (const auto& move : productive_moves) {
            productive_hashes.insert(move.hash());
        }
        
        MoveList filtered;
        for (const auto& move : moves) {
            if (productive_hashes.count(move.hash()) > 0) {
                filtered.push_back(move);
            }
        }
        return filtered;
    }
};

std::unique_ptr<CompoundPruningStrategy> make_pruning_strategy(const SolverConfig& cfg) {
    auto strategy = std::make_unique<CompoundPruningStrategy>();

    if (cfg.enable_productive_move_pruning) {
        strategy->add_strategy(std::make_unique<ProductiveMovePruningStrategy>());
    }

    if (strategy->empty()) {
        return nullptr;
    }

    return strategy;
}

}  // namespace solitaire::solvers