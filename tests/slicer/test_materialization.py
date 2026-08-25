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


class OverlayMaterializationTests(unittest.TestCase):
    def test_build_composition_preserves_delta_and_reuses_baseline_units(self) -> None:
        ownership = json.loads(
            (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text(
                encoding="utf-8"
            )
        )
        purity = load_tool("verify_overlay_purity").verify_tree(ownership)
        self.assertTrue(purity["ok"], purity["errors"][:20])
        self.assertEqual(166, purity["checked_header_baseline_markers"])

        generated = PROJECT_ROOT / "build" / "materialization-test"
        summary = load_tool("generate_all_slices").generate(ownership, generated)
        self.assertEqual(16, summary["baseline_slices"])
        self.assertEqual(16, summary["overlay_compositions"])
        self.assertEqual(166, summary["composed_header_baseline_units"])


if __name__ == "__main__":
    unittest.main()
