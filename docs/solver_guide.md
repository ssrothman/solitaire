# Solver Guide

This project provides two solver styles:

- `CompleteDfsSolver` for exhaustive search
- `HeuristicRunner` for non-backtracking play with a policy

## Complete DFS Solver

Use this when you want to answer whether a position is solvable.

```python
import solitaire

state = solitaire.new_game(seed=42)
cfg = solitaire.SolverConfig()
cfg.max_nodes = 1_000_000
cfg.enable_move_equivalence_pruning = True
cfg.enable_no_op_pruning = False

result = solitaire.solve_game(state, cfg)
print(result.solvable)
print(result.stats.nodes_explored)
print(result.stats.moves_pruned)
```

### Practical Tips

- Use a node limit when exploring harder positions.
- Keep move-equivalence pruning enabled unless you are debugging raw search order.
- Leave no-op pruning disabled unless you are specifically experimenting with it.

## Heuristic Runner

Use this for greedy or random autoplay without backtracking.

```python
import solitaire

state = solitaire.new_game(seed=42)
print(solitaire.run_greedy(state))
print(solitaire.run_random(state, seed=7))
```

## Benchmarking

The repository includes an editable-install friendly workflow and a simple benchmark script for pruning experiments.

Recommended sequence:

1. Build or install the package in the `dev` environment.
2. Run a small benchmark on a handful of seeds.
3. Compare node counts with pruning on and off.

The current implementation keeps pruning conservative so correctness remains easy to reason about.