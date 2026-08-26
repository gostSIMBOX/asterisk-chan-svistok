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


class FunctionLayoutManifestTests(unittest.TestCase):
    def test_approved_partition_is_complete_and_stable(self) -> None:
        ownership = json.loads(
            (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text()
        )
        actual = load_tool("function_layout").build(ownership)
        checked = json.loads(
            (PROJECT_ROOT / "manifests" / "function-layout.json").read_text()
        )
        self.assertEqual(checked, actual)
        self.assertEqual(
            {
                "svistok-new": 45,
                "baseline": 6,
                "dongle-proxy": 14,
                "svistok-inseparable": 87,
            },
            actual["counts"],
        )
        self.assertEqual(9, len(actual["new_files"]))
        self.assertEqual(6, len(actual["proxy_files"]))


if __name__ == "__main__":
    unittest.main()
