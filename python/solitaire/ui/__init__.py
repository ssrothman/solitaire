"""UI subpackage for the `solitaire` Python package.

This module provides the terminal UI modules under `solitaire.ui`.
"""

from . import cli  # re-export for tests and callers
from . import main  # ensure dispatcher is available

__all__ = ["cli", "main"]
