"""High-level Python interface for the solitaire solver.

This package prefers the compiled extension installed next to it, but it also
falls back to the local build output when running from the repository checkout.
"""

from __future__ import annotations

import importlib.util
import sys
from pathlib import Path
from typing import Iterable, Optional

try:
    from ._solitaire_binding import *  # type: ignore  # noqa: F401,F403
except ModuleNotFoundError:  # pragma: no cover - local dev fallback
    _binding_dir = Path(__file__).resolve().parent.parent / "build" / "python" / "solitaire"
    _binding_file = None
    if _binding_dir.exists():
        candidates = sorted(_binding_dir.glob("_solitaire_binding*.so"))
        if not candidates:
            candidates = sorted(_binding_dir.glob("_solitaire_binding*.pyd"))
        if candidates:
            _binding_file = candidates[0]

    if _binding_file is None:
        raise ModuleNotFoundError("Could not locate the compiled solitaire binding")

    _spec = importlib.util.spec_from_file_location("solitaire._solitaire_binding", _binding_file)
    if _spec is None or _spec.loader is None:
        raise ModuleNotFoundError("Could not load the compiled solitaire binding")
    _module = importlib.util.module_from_spec(_spec)
    sys.modules[_spec.name] = _module
    _spec.loader.exec_module(_module)
    globals().update({name: getattr(_module, name) for name in dir(_module) if not name.startswith("_")})


def new_game(seed: int = 0, cfg: Optional[GameConfig] = None) -> GameState:
    """Create a new shuffled game state."""
    return deal_game(seed, cfg or GameConfig())


def solve_game(state: GameState, cfg: Optional[SolverConfig] = None) -> SolverResult:
    """Run the complete DFS solver on a state."""
    return CompleteDfsSolver().solve(state, cfg or SolverConfig())


def run_greedy(
    state: GameState,
    cfg: Optional[SolverConfig] = None,
) -> PolicyResult:
    """Run the built-in greedy policy with no backtracking."""
    policy = GreedyPolicy()
    return HeuristicRunner().run(state, policy, cfg or SolverConfig())


def run_random(
    state: GameState,
    seed: int = 0,
    cfg: Optional[SolverConfig] = None,
) -> PolicyResult:
    """Run the built-in random policy with no backtracking."""
    policy = RandomPolicy(seed)
    return HeuristicRunner().run(state, policy, cfg or SolverConfig())


def format_moves(moves: Iterable[Move]) -> str:
    """Return a numbered, human-readable list of moves."""
    lines = []
    for index, move in enumerate(moves, start=1):
        lines.append(f"{index:>2}. {move.to_string()}")
    return "\n".join(lines)


def move_or_none(notation: str) -> Optional[Move]:
    """Parse a move notation string into a Move, or None if invalid."""
    return move_from_notation(notation)


def apply_notation(state: GameState, notation: str) -> GameState:
    """Parse a notation string and apply it if it is legal."""
    move = move_from_notation(notation)
    if move is None:
        raise ValueError(f"Invalid move notation: {notation!r}")
    if not state.is_legal(move):
        raise ValueError(f"Illegal move in this state: {notation!r}")
    return state.apply_move(move)


__all__ = [
    "apply_notation",
    "Card",
    "Color",
    "CompleteDfsSolver",
    "Deck",
    "GameConfig",
    "GameState",
    "GreedyPolicy",
    "HeuristicRunner",
    "IPolicy",
    "Move",
    "MoveKind",
    "PileId",
    "PileKind",
    "PolicyResult",
    "Rank",
    "RandomPolicy",
    "SearchStats",
    "SolverConfig",
    "SolverResult",
    "Suit",
    "format_moves",
    "move_from_notation",
    "move_or_none",
    "move_to_notation",
    "new_game",
    "run_greedy",
    "run_random",
    "solve_game",
    "deal_game",
    "shuffle_deck",
]