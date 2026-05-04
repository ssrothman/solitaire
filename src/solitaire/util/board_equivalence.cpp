#include "solitaire/util/board_equivalence.h"

#include <deque>
#include <string>
#include <unordered_set>

namespace {

std::string state_key(const solitaire::GameState& state) {
    return state.to_string();
}

bool is_board_edge(const solitaire::GameState& state, const solitaire::Move& move) {
    if (move.kind != solitaire::MoveKind::TableauToTableau) {
        return false;
    }

    if (!state.is_legal(move)) {
        return false;
    }

    const int src = move.source.index();
    return move.card_count < state.tableau_face_up_count(src);
}

}  // namespace

namespace solitaire::util {

GameState canonicalize_board_state(const GameState& state) {
    std::deque<GameState> pending;
    std::unordered_set<std::string> seen;

    pending.push_back(state);
    seen.insert(state_key(state));

    GameState canonical = state;
    std::string canonical_key = state_key(state);

    while (!pending.empty()) {
        GameState current = pending.front();
        pending.pop_front();

        const std::string current_key = state_key(current);
        if (current_key < canonical_key) {
            canonical = current;
            canonical_key = current_key;
        }

        for (const auto& move : current.legal_moves()) {
            if (!is_board_edge(current, move)) {
                continue;
            }

            GameState next = current.apply_move(move);
            const std::string next_key = state_key(next);
            if (seen.insert(next_key).second) {
                pending.push_back(next);
            }
        }
    }

    return canonical;
}

bool board_states_equivalent(const GameState& a, const GameState& b) {
    return canonicalize_board_state(a) == canonicalize_board_state(b);
}

}  // namespace solitaire::util