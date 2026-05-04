# Pruning Strategies

The complete DFS solver supports a small pruning pipeline that removes redundant move choices before search expands them.

## Available Strategies

### Move-Equivalence Pruning

Two moves are considered equivalent when applying either move to the same state produces the same resulting `GameState`.

This is implemented by:

- grouping legal moves into equivalence classes,
- keeping the lowest-indexed move in each class, and
- discarding the rest before DFS explores child states.

This strategy is enabled by default through `SolverConfig.enable_move_equivalence_pruning`.

### No-Op Pruning

No-op pruning removes moves that do not make genuine progress and do not expose any new useful move choices.

The implementation is intentionally conservative:

- it first checks that the move is legal,
- it applies the move to a copy of the state,
- it keeps the move if it reveals a hidden tableau card, moves cards to foundation, or opens a useful waste move,
- it treats stock milling as a no-op only when repeated draw/recycle cycles return to an already seen gameplay position without uncovering new useful moves.

This is controlled by `SolverConfig.enable_no_op_pruning`.

The helper also uses `GameState::same_position()` for gameplay-state comparisons that ignore cycle metadata.

## Composition

The solver builds a single pruning pipeline per solve and runs strategies in order.

Current order:

1. Move-equivalence pruning
2. No-op pruning

That means a move can be filtered out by the first strategy before the second strategy sees it.

## Correctness Notes

- Pruning is optional and configurable.
- The strategies are conservative by design.
- The codebase keeps the pruning surface isolated so future strategies can be added without rewriting DFS.