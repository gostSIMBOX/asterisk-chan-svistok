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


class ProxyContractTests(unittest.TestCase):
    def test_every_proxy_calls_one_pristine_baseline_and_its_hook(self) -> None:
        layout = json.loads(
            (PROJECT_ROOT / "manifests/function-layout.json").read_text()
        )
        ownership = json.loads(
            (PROJECT_ROOT / "manifests/symbol-ownership.json").read_text()
        )
        report = load_tool("verify_proxy_contract").verify(layout, ownership)
        self.assertTrue(report["ok"], json.dumps(report["errors"], indent=2))
        self.assertEqual(14, report["checked"])
        self.assertEqual(6, report["proxy_files"])


if __name__ == "__main__":
    unittest.main()
