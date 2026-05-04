#ifndef SOLITAIRE_UTIL_MOVE_EQUIVALENCE_H
#define SOLITAIRE_UTIL_MOVE_EQUIVALENCE_H

#include "../types.h"
#include "../game_state.h"
#include <vector>

namespace solitaire::util {

// ============================================================================
// Move Equivalence Detection
// ============================================================================

// Check if two moves produce identical game states
// Returns true if applying m1 and m2 to the same state result in equivalent states
bool moves_equivalent(const GameState& state, const Move& m1, const Move& m2);

// Group moves into equivalence classes; returns indices of canonical moves to keep
// The canonical move for each equivalence class is the one with the lowest index
std::vector<std::size_t> find_move_equivalence_classes(const GameState& state,
                                                        const MoveList& moves);

}  // namespace solitaire::util

#endif  // SOLITAIRE_UTIL_MOVE_EQUIVALENCE_H
