#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
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


class ProxyRuntimeTests(unittest.TestCase):
    def test_real_wrappers_call_baseline_once_in_the_approved_order(self) -> None:
        load_tool("verify_proxy_runtime").verify(PROJECT_ROOT / "src")


if __name__ == "__main__":
    unittest.main()
