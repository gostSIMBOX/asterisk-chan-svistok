#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class FunctionPathTests(unittest.TestCase):
    def test_every_classified_function_has_its_approved_physical_owner(self) -> None:
        layout = json.loads(
            (PROJECT_ROOT / "manifests" / "function-layout.json").read_text()
        )
        ownership = json.loads(
            (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text()
        )
        report = load_tool("verify_function_layout").verify(layout, ownership)
        self.assertTrue(report["ok"], json.dumps(report["errors"][:20], indent=2))


if __name__ == "__main__":
    unittest.main()
