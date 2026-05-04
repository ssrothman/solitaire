# Solitaire Solver

A fast C++23 solitaire solver library with extensible heuristic framework and Python bindings.

## Build

### Prerequisites
- C++23 compiler (GCC 13+, Clang 17+, MSVC 2022+)
- CMake 3.24+
- Python 3.9+ (for bindings)
- pybind11

### Build C++ Library

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Run C++ Tests

```bash
cd build
ctest --verbose
```

### Build Python Bindings (Editable Install)

```bash
pip install -e .
```

## Quick Start

### C++ Usage

```cpp
#include "solitaire/game_state.h"
#include "solitaire/shuffle.h"
#include "solitaire/solvers/complete_dfs.h"

auto deck = solitaire::shuffle_deck(12345);  // seeded
auto state = solitaire::GameState::from_deck(deck, solitaire::GameConfig());
auto result = solitaire::CompleteDfsSolver().solve(state);
if (result.solvable) {
    std::cout << "Found solution: " << result.solution.size() << " moves\n";
}
```

### Python Usage

```python
import solitaire

cfg = solitaire.GameConfig(cards_per_draw=3, unlimited_recycle=True)
state = solitaire.new_game(seed=42, cfg=cfg)

solver = solitaire.CompleteDfsSolver()
result = solver.solve(state, solitaire.SolverConfig())
print(f"Solvable: {result.solvable}, Nodes: {result.stats.nodes_explored}")
```

### Python Package and CLI

The Python package exposes the full solver API plus convenience helpers such as `new_game()`, `solve_game()`, `run_greedy()`, and `run_random()`.

The interactive terminal UI is available as:

```bash
solitaire-ui --help
```

For a deeper tour of the Python API and the CLI, see [docs/python_api.md](docs/python_api.md).

## Architecture

### Layers

1. **Domain Model** (types.h, constants.h)
   - Card, Suit, Rank, Color, Move, GameConfig

2. **Rules Engine** (game_state.h)
   - GameState with tableau, foundation, stock, waste
   - Legal move generation
   - Move application with invariant maintenance

3. **Solver Framework** (solvers/)
   - ISolver interface (complete search)
   - IPolicy interface (heuristic one-shot play)
   - CompleteDfsSolver (brute-force DFS with state hashing)
   - GreedyPolicy, RandomPolicy
   - Extensible custom policy template

### File Structure

```
include/solitaire/
├── types.h
├── constants.h
├── game_state.h
├── shuffle.h
├── solvers/
│   ├── solver.h
│   ├── complete_dfs.h
│   ├── greedy_policy.h
│   ├── random_policy.h
│   └── policy_template.h
└── util/
    ├── move_notation.h
   ├── move_equivalence.h
   ├── move_analysis.h
   └── pruning.h

src/solitaire/ (implementations)
tests/cpp/    (C++ tests)
bindings/     (pybind11 module)
solitaire/    (Python package)
python_ui/    (terminal UI)
```

## Rules

Solitaire (classic/Klondike):

- **Tableau**: 7 piles, face-up/face-down split. Cards move if one rank lower + opposite color.
- **Foundation**: 4 piles, build by suit from Ace to King.
- **Stock/Waste**: Draw `cards_per_draw` at a time (default 3). Recycle unlimited.
- **Win**: All cards in foundation.
- **Lose**: No legal moves and stock exhausted.

## Development

### Code Style
- Include guards for all headers (no `#pragma once`)
- C++23 standard, all warnings enabled
- Immutable GameState pattern preferred

### Testing Strategy
- Unit tests for move legality and rules
- Golden tests for deterministic deals
- Solver correctness on curated fixtures
- Property tests for state invariants

## Solver Modes

### Complete (DFS)
Exhaustive search with state hashing and pruning. Answers "is this game solvable?"

### Heuristic (Policy-based)
Apply a strategy (greedy, random, or custom) deterministically. No backtracking.

### Custom Heuristics
Extend `IPolicy` interface to add new strategies. See `policy_template.h` for details.

## Documentation

- [Python API and CLI guide](docs/python_api.md)
- [Solver and pruning guide](docs/solver_guide.md)
- [Pruning strategy reference](docs/pruning_strategies.md)

## Future

- Qt/PySide6 GUI for interactive play
- Parallel DFS exploration
- Transposition table caching
- Advanced heuristics (MCTS, neural net evaluation)
- Endgame tablebases
