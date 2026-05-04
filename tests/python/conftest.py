from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
BUILD_PYTHON = ROOT / "build" / "python"

if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

if BUILD_PYTHON.exists() and str(BUILD_PYTHON) not in sys.path:
    sys.path.insert(0, str(BUILD_PYTHON))