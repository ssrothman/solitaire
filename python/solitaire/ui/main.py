from __future__ import annotations

import argparse
import sys
from typing import Optional

from . import cli as cli_mod


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="solitaire", description="Solitaire tools")
    subparsers = parser.add_subparsers(dest="command")

    # UI subcommand (default)
    p_ui = subparsers.add_parser("ui", help="Launch interactive text UI")
    p_ui.add_argument("args", nargs=argparse.REMAINDER)

    # Expose raw access to the existing CLI commands for future subcommands
    return parser


def main(argv: Optional[list[str]] = None) -> int:
    if argv is None:
        argv = sys.argv[1:]

    parser = build_arg_parser()
    parsed = parser.parse_args(argv)

    # Default to UI when no subcommand provided
    if parsed.command is None or parsed.command == "ui":
        # Pass through any remaining args to the existing UI main
        remaining = getattr(parsed, "args", None)
        return cli_mod.main(remaining if remaining is not None and len(remaining) > 0 else None)

    # Unknown command (placeholder)
    parser.print_help()
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
