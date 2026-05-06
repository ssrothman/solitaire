#ifndef SOLITAIRE_UTIL_MOVE_ANALYSIS_H
#define SOLITAIRE_UTIL_MOVE_ANALYSIS_H

#include "../game_state.h"
#include "../types.h"

namespace solitaire::util {

    MoveList all_non_no_op_moves(const GameState& state);

}  // namespace solitaire::util

#endif  // SOLITAIRE_UTIL_MOVE_ANALYSIS_H
