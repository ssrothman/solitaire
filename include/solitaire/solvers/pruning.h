#ifndef SOLITAIRE_SOLVERS_PRUNING_H
#define SOLITAIRE_SOLVERS_PRUNING_H

#include "../game_state.h"
#include "../types.h"
#include "../util/move_analysis.h"
#include "../util/move_equivalence.h"

#include <memory>
#include <vector>

namespace solitaire::solvers {

// ============================================================================
// Pruning Strategy Interface
// ============================================================================

class IPruningStrategy {
public:
    virtual ~IPruningStrategy() = default;

    // Return a filtered move list for the given state.
    virtual MoveList filter(const GameState& state, const MoveList& moves) const = 0;
};

class EquivalencePruningStrategy : public IPruningStrategy {
public:
    MoveList filter(const GameState& state, const MoveList& moves) const override;
};

class NoOpPruningStrategy : public IPruningStrategy {
public:
    MoveList filter(const GameState& state, const MoveList& moves) const override;
};

class CompoundPruningStrategy : public IPruningStrategy {
public:
    CompoundPruningStrategy() = default;

    void add_strategy(std::unique_ptr<IPruningStrategy> strategy);

    bool empty() const { return _strategies.empty(); }

    MoveList filter(const GameState& state, const MoveList& moves) const override;

private:
    std::vector<std::unique_ptr<IPruningStrategy>> _strategies;
};

std::unique_ptr<CompoundPruningStrategy> make_pruning_strategy(const SolverConfig& cfg);

}  // namespace solitaire::solvers

#endif  // SOLITAIRE_SOLVERS_PRUNING_H