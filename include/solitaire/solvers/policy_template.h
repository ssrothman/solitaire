#ifndef SOLITAIRE_SOLVERS_POLICY_TEMPLATE_H
#define SOLITAIRE_SOLVERS_POLICY_TEMPLATE_H

#include "solver.h"

namespace solitaire::solvers {

// ============================================================================
// Custom Policy Template
// ============================================================================
// Use this as a starting point for implementing your own heuristic strategies.
//
// Example:
//
//   class MyCustomPolicy : public IPolicy {
//   public:
//       Move choose_move(const GameState& state, const MoveList& legal_moves) override {
//           // Implement your strategy here
//           // Examine the state and legal_moves
//           // Return the move you want to play
//           return legal_moves[0];  // Simple: pick first move
//       }
//   };
//
//   MyCustomPolicy my_policy;
//   HeuristicRunner runner;
//   PolicyResult result = runner.run(initial_state, my_policy);
//

// ============================================================================
// Extended Policy Interface (Optional)
// ============================================================================
// For more advanced strategies, you can override evaluate_move to provide
// a scoring function:
//
//   class ScoringPolicy : public IPolicy {
//   public:
//       double evaluate_move(const GameState& state, const Move& move) override {
//           // Return a score for this move (higher = better)
//           // This is called by your choose_move implementation
//           // Example: reward moves that reveal face-down cards
//           return 0.5;
//       }
//   };
//

// ============================================================================
// Tips for Policy Implementation
// ============================================================================
//
// 1. State Inspection:
//    - Use state.tableau_face_up(i) to see face-up cards in pile i
//    - Use state.tableau_face_down_count(i) to count hidden cards
//    - Use state.foundation_top(suit) to check foundation progress
//    - Use state.waste_top() to see the waste pile top card
//
// 2. Move Filtering:
//    - The legal_moves list contains only legal moves
//    - You can filter by move.kind to prioritize certain types
//
// 3. Look-Ahead (Optional):
//    - Create a copy of state: auto next = state.apply_move(move)
//    - Examine the new state to evaluate consequences
//    - This allows limited lookahead without backtracking
//
// 4. Determinism:
//    - To make your policy reproducible, store a seed in the policy
//    - Use std::mt19937 for seeded randomness if needed
//
// Example: Custom Multi-Level Policy
// ============================================================================
//
// class MultiLevelPolicy : public IPolicy {
// public:
//     Move choose_move(const GameState& state, const MoveList& legal_moves) override {
//         // Level 1: Try to build foundations
//         for (const auto& move : legal_moves) {
//             if (move.kind == MoveKind::TableauToFoundation ||
//                 move.kind == MoveKind::WasteToFoundation) {
//                 return move;
//             }
//         }
//
//         // Level 2: Try to reveal face-down cards
//         for (const auto& move : legal_moves) {
//             if (move.kind == MoveKind::TableauToTableau) {
//                 auto next = state.apply_move(move);
//                 // If this reveal more cards, prefer it
//                 return move;
//             }
//         }
//
//         // Level 3: Draw from stock
//         for (const auto& move : legal_moves) {
//             if (move.kind == MoveKind::StockDraw) {
//                 return move;
//             }
//         }
//
//         // Fallback: pick first legal move
//         return legal_moves.empty() ? Move() : legal_moves[0];
//     }
// };

}  // namespace solitaire::solvers

#endif  // SOLITAIRE_SOLVERS_POLICY_TEMPLATE_H
