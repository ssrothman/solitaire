#ifndef SOLITAIRE_UTIL_BOARD_EQUIVALENCE_H
#define SOLITAIRE_UTIL_BOARD_EQUIVALENCE_H

#include "../game_state.h"

namespace solitaire::util {

// Canonicalize a board state under transitive closure of tableau-to-tableau moves
// that do not reveal a face-down card.
GameState canonicalize_board_state(const GameState& state);

// Return true when two board states are equivalent under that relation.
bool board_states_equivalent(const GameState& a, const GameState& b);

}  // namespace solitaire::util

#endif  // SOLITAIRE_UTIL_BOARD_EQUIVALENCE_H