#ifndef SOLITAIRE_UTIL_MOVE_ANALYSIS_H
#define SOLITAIRE_UTIL_MOVE_ANALYSIS_H

#include "../game_state.h"
#include "../types.h"

namespace solitaire::util {

// Return true if applying move leaves the game state unchanged.
// This is intentionally conservative and only prunes exact no-ops.
bool is_no_op_move(const GameState& state, const Move& move);

}  // namespace solitaire::util

#endif  // SOLITAIRE_UTIL_MOVE_ANALYSIS_H
