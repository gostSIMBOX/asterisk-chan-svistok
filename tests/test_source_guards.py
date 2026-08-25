#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]


def load_guard_module():
    path = PROJECT_ROOT / "tools" / "check_source_guards.py"
    specification = importlib.util.spec_from_file_location("check_source_guards", path)
    if specification is None or specification.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


class SourceGuardTests(unittest.TestCase):
    def test_read_only_sources_match_approved_baselines(self) -> None:
        result = load_guard_module().check()
        self.assertTrue(result["ok"], result["errors"])
        self.assertEqual("chan_svistok.so", result["target_module"])


if __name__ == "__main__":
    unittest.main()
