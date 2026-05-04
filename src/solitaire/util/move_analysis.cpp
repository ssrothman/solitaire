#include "solitaire/util/move_analysis.h"

namespace solitaire::util {

bool is_no_op_move(const GameState& state, const Move& move) {
    if (!state.is_legal(move)) {
        return false;
    }

    GameState next = state.apply_move(move);
    return next == state;
}

}  // namespace solitaire::util
