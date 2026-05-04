from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from typing import Optional

from solitaire import (
    CompleteDfsSolver,
    GameConfig,
    GameState,
    GreedyPolicy,
    HeuristicRunner,
    Move,
    PolicyResult,
    RandomPolicy,
    SolverConfig,
    format_moves,
    move_from_notation,
    new_game,
)


ANSI_RED = "\033[31m"
ANSI_BLUE = "\033[34m"
ANSI_RESET = "\033[0m"
CARD_TOKEN_RE = re.compile(r"(10|[2-9AJQK])([♥♦♣♠])")
TABLEAU_LINE_RE = re.compile(r"^(\s*T\d+:\s*)(.*?)(\s*\[\d+ hidden\])?$")


def _colorize_cards_in_text(text: str) -> str:
    def repl(match: re.Match[str]) -> str:
        rank = match.group(1)
        suit = match.group(2)
        color = ANSI_RED if suit in {"♥", "♦"} else ANSI_BLUE
        return f"{color}{rank}{suit}{ANSI_RESET}"

    return CARD_TOKEN_RE.sub(repl, text)


@dataclass
class Session:
    state: GameState
    seed: int
    cfg: GameConfig
    history: list[str]
    use_color: bool = True


def _default_solver_cfg(state: GameState) -> SolverConfig:
    cfg = SolverConfig()
    cfg.max_depth = max(cfg.max_depth, 256)
    cfg.max_nodes = max(cfg.max_nodes, 50000)
    return cfg


def _render_cards(text: str, use_color: bool) -> str:
    return _colorize_cards_in_text(text) if use_color else text


def _reverse_tableau_face_up_order(state_text: str) -> str:
    lines = state_text.splitlines()
    in_tableau = False
    out: list[str] = []

    for line in lines:
        if line.startswith("Tableau:"):
            in_tableau = True
            out.append(line)
            continue
        if line.startswith("Foundation:"):
            in_tableau = False
            out.append(line)
            continue

        if in_tableau:
            match = TABLEAU_LINE_RE.match(line)
            if match:
                prefix, cards_part, hidden_part = match.groups()
                cards = [m.group(0) for m in CARD_TOKEN_RE.finditer(cards_part)]
                if len(cards) > 1:
                    rebuilt = prefix + " ".join(reversed(cards))
                    if hidden_part:
                        rebuilt += hidden_part
                    out.append(rebuilt)
                    continue

        out.append(line)

    return "\n".join(out)


def _render_state_text(state: GameState, use_color: bool) -> str:
    reordered = _reverse_tableau_face_up_order(state.to_string().rstrip())
    return _render_cards(reordered, use_color)


def _describe_state(state: GameState, use_color: bool = True) -> str:
    lines = [
        f"turns={state.turn_count}, stock_cycle={state.stock_cycle_position}",
        f"stock={state.stock_size()} waste={state.waste_size()} won={state.is_won()} lost={state.is_lost()}",
        "",
        _render_state_text(state, use_color),
    ]
    return "\n".join(lines)


def _describe_board(state: GameState, use_color: bool = True) -> str:
    lines = ["Board:"]
    foundation = []
    for suit_index in range(4):
        top = state.foundation_top(suit_index).to_string() if state.foundation_size(suit_index) > 0 else "--"
        foundation.append(f"F{suit_index}:{_render_cards(top, use_color)}")
    lines.append("  " + "  ".join(foundation))
    lines.append(f"  Stock: {state.stock_size()} cards")
    lines.append(f"  Waste: {state.waste_top().to_string() if state.has_waste() else 'empty'}")
    lines.append("  Tableau:")
    for pile in range(7):
        top = state.tableau_top(pile)
        hidden = state.tableau_face_down_count(pile)
        face_up = state.tableau_face_up_count(pile)
        card_text = top.to_string() if face_up else "empty"
        lines.append(f"    T{pile}: {_render_cards(card_text, use_color)} ({face_up} up, {hidden} down)")
    return "\n".join(lines)


def _history_lines(session: Session) -> list[str]:
    if not session.history:
        return ["No moves yet."]
    return [f"{index:>2}. {move}" for index, move in enumerate(session.history, start=1)]


def _print_moves(state: GameState) -> None:
    moves = state.legal_moves()
    if not moves:
        print("No legal moves.")
        return
    print(format_moves(moves))


def _check_game_over(state: GameState) -> str:
    """Check if the game is over and return status."""
    won = state.is_won()
    lost = state.is_lost()
    
    if won:
        return "Game won! ✓"
    elif lost:
        return "Game lost! ✗"
    else:
        return "Game in progress."


def _infer_foundation_move(state: GameState, notation: str) -> Optional[str]:
    """
    If notation is a move to bare "F" (e.g., "T0->F" or "W->F"), 
    infer the correct foundation index based on the card being moved.
    Returns the inferred notation or None if inference fails.
    """
    # Check if notation contains "->" or "→" and ends with just "F"
    for arrow in ["->", "→"]:
        if arrow not in notation:
            continue
        parts = notation.split(arrow)
        if len(parts) != 2:
            continue
        source_str, target_str = parts
        
        # Extract any parenthetical card count from target
        target_base = target_str.split("(")[0] if "(" in target_str else target_str
        
        if target_base != "F":
            continue
        
        # Try to get the source card
        if source_str == "W":
            if not state.has_waste():
                return None
            card = state.waste_top()
        elif source_str.startswith("T"):
            try:
                tableau_idx = int(source_str[1:])
                face_up = state.tableau_face_up_count(tableau_idx)
                if face_up == 0:
                    return None
                card = state.tableau_top(tableau_idx)
            except (ValueError, IndexError):
                return None
        else:
            return None
        
        # Infer foundation pile based on suit
        suit_index = card.suit.value  # Suit is an enum, so value gives us 0-3
        
        # Reconstruct notation with inferred foundation
        card_count_part = target_str[1:] if len(target_str) > 1 else ""
        inferred_notation = f"{source_str}{arrow}F{suit_index}{card_count_part}"
        return inferred_notation
    
    return None


def _parse_move_notation(state: GameState, notation: str) -> Optional[Move]:
    """
    Parse move notation, with support for foundation inference.
    Returns the parsed Move or None if parsing fails.
    """
    # Try to infer foundation moves first
    inferred = _infer_foundation_move(state, notation)
    if inferred:
        notation = inferred
    
    return move_from_notation(notation)


def _apply_choice(state: GameState, choice: str) -> GameState:
    choice = choice.strip()
    if not choice:
        raise ValueError("empty move")

    legal = state.legal_moves()
    if choice.isdigit():
        idx = int(choice)
        if idx < 1 or idx > len(legal):
            raise ValueError(f"move number out of range: {choice}")
        return state.apply_move(legal[idx - 1])

    move = _parse_move_notation(state, choice)
    if move is None:
        raise ValueError(f"invalid move notation: {choice}")
    if not state.is_legal(move):
        raise ValueError(f"illegal move in current state: {choice}")
    return state.apply_move(move)


def _apply_choice_and_record(session: Session, choice: str) -> GameState:
    choice = choice.strip()
    if choice.isdigit():
        legal = session.state.legal_moves()
        idx = int(choice)
        if idx < 1 or idx > len(legal):
            raise ValueError(f"move number out of range: {choice}")
        move = legal[idx - 1]
        session.history.append(move.to_string())
        return session.state.apply_move(move)

    move = _parse_move_notation(session.state, choice)
    if move is None:
        raise ValueError(f"invalid move notation: {choice}")
    if not session.state.is_legal(move):
        raise ValueError(f"illegal move in current state: {choice}")
    session.history.append(move.to_string())
    return session.state.apply_move(move)


def _auto_play(state: GameState, mode: str, seed: int = 0) -> PolicyResult:
    runner = HeuristicRunner()
    if mode == "greedy":
        return runner.run(state, GreedyPolicy(), SolverConfig())
    if mode == "random":
        return runner.run(state, RandomPolicy(seed), SolverConfig())
    raise ValueError(f"unknown auto mode: {mode}")


def interactive_loop(session: Session) -> None:
    print("Solitaire UI")
    print("Type 'help' for commands.\n")
    while True:
        prompt = f"solitaire[{session.seed}]> "
        try:
            raw = input(prompt)
        except (EOFError, KeyboardInterrupt):
            print()
            return

        command = raw.strip()
        if not command:
            continue

        head, _, tail = command.partition(" ")
        head = head.lower()
        tail = tail.strip()

        try:
            if head in {"quit", "exit", "q"}:
                return
            if head in {"help", "h", "?"}:
                print(
                    "Commands: state, board, moves, history, move <n|notation>, status, auto greedy, auto random [seed], solve, reset [seed], quit"
                )
            elif head in {"state", "s"}:
                print(_describe_state(session.state, session.use_color))
            elif head == "board":
                print(_describe_board(session.state, session.use_color))
            elif head in {"status", "gameover"}:
                print(_check_game_over(session.state))
            elif head in {"moves", "m"}:
                _print_moves(session.state)
            elif head in {"history", "hist"}:
                print("\n".join(_history_lines(session)))
            elif head == "move":
                session.state = _apply_choice_and_record(session, tail)
                print(_render_state_text(session.state, session.use_color))
            elif head == "auto":
                mode, _, mode_tail = tail.partition(" ")
                if not mode:
                    raise ValueError("usage: auto greedy|random [seed]")
                seed = session.seed
                if mode == "random" and mode_tail:
                    seed = int(mode_tail)
                result = _auto_play(session.state, mode, seed)
                print(f"won={result.won}, turns={result.turns}, moves={len(result.moves)}")
            elif head == "solve":
                cfg = _default_solver_cfg(session.state)
                result = CompleteDfsSolver().solve(session.state, cfg)
                print(
                    f"solvable={result.solvable}, nodes={result.stats.nodes_explored}, depth={result.stats.max_depth}, "
                    f"pruned={result.stats.moves_pruned}, time={result.stats.time_seconds:.4f}s"
                )
                if result.solution:
                    print(format_moves(result.solution))
            elif head == "reset":
                seed = session.seed if not tail else int(tail)
                session.seed = seed
                session.state = new_game(seed, session.cfg)
                session.history.clear()
                print(_render_state_text(session.state, session.use_color))
            else:
                session.state = _apply_choice_and_record(session, command)
                print(_render_state_text(session.state, session.use_color))
        except Exception as exc:  # noqa: BLE001 - surface errors in the UI
            print(f"error: {exc}")


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Interactive solitaire text UI")
    parser.add_argument("--seed", type=int, default=0, help="Shuffle seed")
    parser.add_argument("--draw", type=int, default=3, help="Cards drawn from stock at a time")
    parser.add_argument("--no-recycle", action="store_true", help="Disable unlimited stock recycling")
    parser.add_argument("--no-color", action="store_true", help="Disable ANSI card coloring")
    return parser


def main(argv: Optional[list[str]] = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    cfg = GameConfig(cards_per_draw=args.draw, unlimited_recycle=not args.no_recycle)
    session = Session(
        state=new_game(args.seed, cfg),
        seed=args.seed,
        cfg=cfg,
        history=[],
        use_color=not args.no_color,
    )
    print(_render_state_text(session.state, session.use_color))
    interactive_loop(session)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())