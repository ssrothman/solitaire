#ifndef SOLITAIRE_UTIL_MOVE_ANALYSIS_H
#define SOLITAIRE_UTIL_MOVE_ANALYSIS_H

#include "../game_state.h"
#include "../types.h"

namespace solitaire::util {

// ============================================================================
// Move Analysis: No-Op Move Detection
// ============================================================================
//
// A move is considered a "no-op" (no operation) if applying it leaves the
// game state semantically unchanged in a way that prevents progress. This is
// used in search pruning to avoid exploring equivalent game states.
//
// ============================================================================
// Algorithm Overview
// ============================================================================
//
// The is_no_op_move() function implements a sophisticated analysis that handles
// different move types with specialized logic:
//
// 1. STOCK DRAW and STOCK RECYCLE moves:
//    - Use existing logic: is_stock_cycle_no_op()
//    - Detects cycles where stock operations loop without enabling moves
//    - Example: drawing from stock returns to the original position without
//      enabling any new waste-origin moves
//
// 2. WASTE-TO-FOUNDATION, TABLEAU-TO-FOUNDATION, WASTE-TO-TABLEAU moves:
//    - Immediately return false (never no-ops)
//    - These moves always make progress toward the win condition
//
// 3. TABLEAU-TO-TABLEAU moves (the complex case):
//    - First check: Does the move reduce face-down count (already revealed cards)?
//      If yes → NOT a no-op (makes progress)
//    - Second check: Does the move expose NEW face-down cards?
//      This is detected by: card_count == number of face-up cards on source
//      If yes → NOT a no-op (makes progress, reveals hidden cards)
//      If no  → Continue to DFS analysis
//    - DFS analysis: Perform a depth-first search from the resulting board state
//      exploring all possible chains of tableau-to-tableau moves
//    - Goal: Detect if this chain eventually leads to a NEW T-to-F move
//      (one not available in the original state)
//    - If new T-to-F found → NOT a no-op (enables progress)
//    - If all chains exhausted → IS a no-op (doesn't enable progress)
//
// ============================================================================
// DFS-Based Analysis for Tableau-to-Tableau Moves
// ============================================================================
//
// Background Problem:
// ------------------
// Simple heuristics that only check immediate move availability miss complex
// scenarios where a tableau-to-tableau move creates a *chain* of moves that
// eventually unlock new foundation-bound moves:
//
//   Initial state: T-to-F moves available: {A♠→F}
//   Apply T-to-T move: (move cards in tableau)
//   New state: More T-to-T moves available, but no NEW T-to-F yet
//   Apply chain of T-to-T moves: (further rearrangements)
//   Final state: NEW T-to-F move available: {2♠→F}
//
// Result: The initial T-to-T move should NOT have been classified as a no-op,
// because following its consequences reveals new moves.
//
// Critical Constraint:
// --------------------
// We must only explore T-to-T moves that haven't been seen anywhere in the
// move chain so far (from start to current point). This prevents false 
// positives where a move is incorrectly flagged as useful just because other
// moves could lead to a foundation move.
//
// The set of "accumulated moves" grows as we traverse deeper:
//   Level 0: Acc = original T-to-T moves
//   Level 1: Acc = original moves ∪ new moves from level 0
//   Level 2: Acc = original moves ∪ new from level 0 ∪ new from level 1
//   etc.
//
// Example of why this matters:
//   Initial state: T-to-T moves available: {Move A, Move B}
//   Apply Move A: New state has T-to-T moves: {Move B, Move C}
//   We should only explore Move C (not seen before)
//   Not Move B (seen at start)
//   
//   Note: Moves that expose face-down cards (detected by card_count == face_up_count)
//   skip the DFS entirely and return immediately as NOT no-ops.
//
// Algorithm:
// ----------
// For each T-to-T move M from state S:
//
//   1. Apply M to get state S'
//   2. Check if M reduces face-down count → return NOT a no-op
//   3. Check if M exposes new face-down cards:
//       - Test: move.card_count == face_up_count(source)
//       - If true → return NOT a no-op (exposes hidden cards)
//   4. Otherwise, continue to DFS:
//       a. Collect T-to-F moves available in S (call set A)
//       b. Collect T-to-T moves available in S (call set Acc for "accumulated")
//       c. Initialize visited set V
//       d. Call dfs(S', A, Acc, V):
//          - If current state's hash not in V:
//              • Add state's hash to V
//              • If any new T-to-F move found (not in A) → return true
//              • Get current state's T-to-T moves (call set C_t)
//              • For each move in (C_t \ Acc): // Only newly-seen moves
//                  - Apply move, continue to next state
//                  - Build new_accumulated = Acc ∪ C_t
//                  - Call dfs(next, A, new_accumulated, V)
//                  - If recursive call returns true → return true
//              • If all moves exhausted → return false
//   5. If DFS returns true (new T-to-F found) → M is NOT a no-op
//      If DFS returns false (no new T-to-F) → M is a no-op
//
// Key point: accumulated_moves grows as we traverse deeper into the chain,
// ensuring we only explore truly new moves at each level.
//
// ============================================================================
// Caching Strategy
// ============================================================================
//
// Per-DFS Visited Set:
// - Local to each DFS traversal, prevents infinite loops
// - Tracks which board states have been explored within this DFS
// - Reset for each is_no_op_move() call
// - Uses simple board-state hashing to detect state revisits
//
// No Persistent Cache:
// - Removed due to context-dependency: the set of "new" moves depends on
//   which moves have been seen so far in the chain
// - Per-DFS visited set is sufficient to prevent infinite loops
// - Simplifies correctness guarantees
//
// ============================================================================
// Performance Characteristics
// ============================================================================
//
// Best case (cache hit):      O(1)     - board state found in visited set (immediate
//                                        revisit to same state)
// Typical case:               O(d)     - DFS depth d before finding new T-to-F
//                                        or exhausting nearby states
// Worst case (no progress):   O(2^d)   - full DFS tree exploration (rare)
//                                        bounded by game tree size
//
// In practice, typical game trees have shallow local T-to-T chains (d ~ 2-5),
// so DFS is efficient. Per-DFS visited set prevents infinite loops.
//
// Note: No persistent cache means each is_no_op_move() call is independent,
// which simplifies correctness but may result in re-exploration of similar
// scenarios. This trade-off is acceptable given the typical small DFS depths.
//
// ============================================================================
// Testing and Validation
// ============================================================================
//
// Covered scenarios:
// - T-to-T moves revealing face-down cards
// - W-to-F, T-to-F, W-to-T moves (always not no-ops)
// - Stock draw/recycle cycles (original logic preserved)
// - Deterministic results across repeated calls
// - Accumulated move filtering (only explores newly-seen moves)
// - T-to-T moves without progress (DFS-based evaluation)
//
// ============================================================================

// Return a list of all NON-no-op moves (moves that have potential consequences).
// This is used in solver pruning to identify moves worth exploring.
//
// A move is a "no-op" if applying it leaves the game state in a way that prevents
// progress (doesn't enable new foundation moves or useful empty tableaus). This function
// returns all moves that are NOT no-ops, using:
//
// - Early returns for obviously productive moves (face-down exposure, waste→*, T→F, etc.)
// - Stock cycle analysis for draw/recycle moves
// - Global DFS with parent-child tracking for T-to-T moves, reusing work across moves
//
// Args:
//   state: Current game state
//
// Returns:
//   MoveList - All legal moves that are NOT no-ops (have productive consequences)
//
// Note: Moves explicitly filtered out include:
// - All no-op stock draws/recycles
// - Empty T-to-T moves that don't lead to productivity
// - All other moves identified as non-productive by static analysis
MoveList all_non_no_op_moves(const GameState& state);

}  // namespace solitaire::util

#endif  // SOLITAIRE_UTIL_MOVE_ANALYSIS_H
