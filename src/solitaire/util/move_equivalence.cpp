#include "solitaire/util/move_equivalence.h"

namespace solitaire::util {

bool moves_equivalent(const GameState& state, const Move& m1, const Move& m2) {
    // Apply both moves to copies of the state and compare the resulting positions.
    // If either move is illegal, they're not equivalent.
    if (!state.is_legal(m1) || !state.is_legal(m2)) {
        return false;
    }
    
    GameState s1 = state.apply_move(m1);
    GameState s2 = state.apply_move(m2);
    
    // Compare the full resulting game state.
    return s1 == s2;
}

std::vector<std::size_t> find_move_equivalence_classes(const GameState& state,
                                                        const MoveList& moves) {
    std::vector<std::size_t> canonical_moves;
    std::vector<bool> visited(moves.size(), false);

    // For each move, if not yet visited, mark it and all equivalent moves as visited
    for (std::size_t i = 0; i < moves.size(); ++i) {
        if (visited[i]) {
            continue;
        }

        // This is a new equivalence class; i is the canonical representative
        canonical_moves.push_back(i);
        visited[i] = true;

        // Mark all moves equivalent to moves[i] as visited
        for (std::size_t j = i + 1; j < moves.size(); ++j) {
            if (!visited[j] && moves_equivalent(state, moves[i], moves[j])) {
                visited[j] = true;
            }
        }
    }

    return canonical_moves;
}

}  // namespace solitaire::util
