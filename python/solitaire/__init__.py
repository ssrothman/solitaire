"""High-level Python interface for the solitaire solver."""

from __future__ import annotations

from typing import Iterable, Optional

from ._solitaire_binding import *  # noqa: F401,F403


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