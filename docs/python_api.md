# Python API and CLI Guide

This repository ships both a Python package and an interactive terminal UI.

## Package Overview

The `solitaire` package wraps the C++ library and exposes:

- Core types: `Card`, `Move`, `GameState`, `GameConfig`, `SolverConfig`
- Solver APIs: `CompleteDfsSolver`, `HeuristicRunner`, `GreedyPolicy`, `RandomPolicy`
- Utility helpers: `new_game()`, `solve_game()`, `run_greedy()`, `run_random()`, `format_moves()`

Notes:

- `new_game(seed, cfg)` is the most convenient way to create a playable state.
- `GameState.from_deck(deck, cfg)` expects a deck of `Card` objects, not integer placeholders.
- The package also exposes `deal_game(seed, cfg)` and `shuffle_deck(seed)` for lower-level workflows.

Example:

```python
import solitaire

state = solitaire.new_game(seed=42)
result = solitaire.solve_game(state)
print(result.solvable)
print(result.stats.nodes_explored)
```

## Interactive CLI

Launch the text UI with:

```bash
solitaire-ui --seed 42
```

Useful commands:

- `state` - Print a summary of the current state.
- `board` - Render tableau, stock, waste, and foundation in a compact board view.
- `moves` - List legal moves.
- `history` - Show the sequence of moves applied so far.
- `move <n|notation>` - Apply a move by number or notation.
- `auto greedy` - Play using the greedy policy.
- `auto random [seed]` - Play using the random policy.
- `solve` - Run the complete DFS solver from the current position.
- `reset [seed]` - Start a new shuffled game.

## Notes

- The package supports editable installs from a checkout and an installed `solitaire-ui` entry point.
- The CLI prints canonical move notation, so move history is stable across input forms.
- The `board` command is intentionally compact and text-only; it is meant for quick inspection rather than a full graphical render.