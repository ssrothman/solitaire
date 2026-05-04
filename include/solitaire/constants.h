#ifndef SOLITAIRE_CONSTANTS_H
#define SOLITAIRE_CONSTANTS_H

// Human-reviewed: do not edit further with AI assistance unless explicitly confirmed.

namespace solitaire {

// Game dimensions
inline constexpr int NUM_TABLEAU_PILES = 7;
inline constexpr int NUM_FOUNDATIONS = 4;
inline constexpr int NUM_SUITS = 4;
inline constexpr int NUM_RANKS = 13;
inline constexpr int DECK_SIZE = NUM_SUITS * NUM_RANKS;

// Stock/waste configuration
inline constexpr int DEFAULT_CARDS_PER_DRAW = 3;
inline constexpr bool DEFAULT_UNLIMITED_RECYCLE = true;

// Solver configuration
inline constexpr int DEFAULT_SOLVER_MAX_DEPTH = 100000;
inline constexpr int DEFAULT_SOLVER_MAX_NODES = 10000000;
inline constexpr bool DEFAULT_SOLVER_ENABLE_PRUNING = false;
inline constexpr int DEFAULT_SOLVER_SEED = 0;
inline constexpr bool DEFAULT_SOLVER_ENABLE_MOVE_EQUIVALENCE_PRUNING = true;
inline constexpr bool DEFAULT_SOLVER_ENABLE_NO_OP_PRUNING = false;

}  // namespace solitaire

#endif  // SOLITAIRE_CONSTANTS_H
