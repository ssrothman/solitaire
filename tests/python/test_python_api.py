from __future__ import annotations

import pytest

import solitaire
from python_ui import cli


def test_package_helpers_round_trip_move_notation() -> None:
    state = solitaire.new_game(123)
    moves = state.legal_moves()

    assert state.stock_size() == 24
    assert moves

    move = moves[0]
    notation = solitaire.move_to_notation(move)
    parsed = solitaire.move_from_notation(notation)

    assert parsed is not None
    assert parsed == move
    assert solitaire.move_or_none(notation) == move


def test_apply_notation_requires_legal_move() -> None:
    state = solitaire.new_game(123)
    move = state.legal_moves()[0]
    notation = solitaire.move_to_notation(move)

    next_state = solitaire.apply_notation(state, notation)

    assert next_state != state
    assert next_state.stock_size() == 21
    assert next_state.waste_size() == 3


def test_solve_and_auto_helpers_on_default_state() -> None:
    state = solitaire.GameState()

    solved = solitaire.solve_game(state)
    greedy = solitaire.run_greedy(state)
    random_result = solitaire.run_random(state, seed=7)

    assert solved.solvable is False
    assert solved.stats.nodes_explored == 1
    assert greedy.won is False
    assert greedy.moves == []
    assert random_result.won is False
    assert random_result.moves == []


def test_ui_helpers_describe_and_apply_choice() -> None:
    session = cli.Session(state=solitaire.new_game(123), seed=123, cfg=solitaire.GameConfig(), history=[])

    description = cli._describe_state(session.state)
    assert "stock=" in description
    assert "waste=" in description

    board = cli._describe_board(session.state)
    assert "Board:" in board
    assert "Tableau:" in board

    legal_moves = session.state.legal_moves()
    assert legal_moves

    moved = cli._apply_choice(session.state, "1")
    assert moved != session.state

    notation = solitaire.move_to_notation(legal_moves[0])
    moved_by_notation = cli._apply_choice(session.state, notation)
    assert moved_by_notation == moved


def test_ui_history_records_canonical_moves() -> None:
    session = cli.Session(state=solitaire.new_game(123), seed=123, cfg=solitaire.GameConfig(), history=[])

    next_state = cli._apply_choice_and_record(session, "1")

    assert next_state != session.state
    assert len(session.history) == 1
    assert session.history[0] == solitaire.move_to_notation(session.state.legal_moves()[0])

    session.state = next_state
    assert cli._history_lines(session)[0] == " 1. " + session.history[0]


def test_ui_history_empty_message() -> None:
    session = cli.Session(state=solitaire.new_game(123), seed=123, cfg=solitaire.GameConfig(), history=[])
    assert cli._history_lines(session) == ["No moves yet."]


def test_cli_parser_defaults() -> None:
    parser = cli.build_arg_parser()
    args = parser.parse_args([])

    assert args.seed == 0
    assert args.draw == 3
    assert args.no_recycle is False
    assert args.no_color is False


def test_ui_no_color_rendering_has_no_ansi_sequences() -> None:
    state = solitaire.new_game(123)

    state_text = cli._describe_state(state, use_color=False)
    board_text = cli._describe_board(state, use_color=False)

    assert "\033[" not in state_text
    assert "\033[" not in board_text


def test_ui_reverses_tableau_face_up_listing_order() -> None:
    text = "\n".join(
        [
            "=== GameState ===",
            "Tableau:",
            "  T0: A♥ K♣ 10♦ [2 hidden]",
            "  T1: 9♠ ",
            "Foundation: -- -- -- -- ",
        ]
    )

    rendered = cli._reverse_tableau_face_up_order(text)

    assert "  T0: 10♦ K♣ A♥ [2 hidden]" in rendered
    assert "  T1: 9♠ " in rendered
