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


class OverlayPurityTests(unittest.TestCase):
    def test_unit_comparison_rejects_equivalent_and_unclassified_definitions(self) -> None:
        verifier = load_tool("verify_overlay_purity")
        manifest = [
            {
                "kind": "function",
                "symbol": "unchanged",
                "owner": "upstream",
                "baseline": {"source_sha256": "baseline"},
                "legacy": {"source_sha256": "baseline"},
            },
            {
                "kind": "function",
                "symbol": "changed",
                "owner": "svistok",
                "baseline": {"source_sha256": "old"},
                "legacy": {"source_sha256": "new"},
            },
        ]
        local = {
            ("FunctionDecl", "unchanged"): {
                "kind": "function",
                "symbol": "unchanged",
                "definition_range": {"begin": 0, "end": 10},
            },
            ("FunctionDecl", "changed"): {
                "kind": "function",
                "symbol": "changed",
                "definition_range": {"begin": 11, "end": 20},
                "source_sha256": "new",
            },
            ("FunctionDecl", "mystery"): {
                "kind": "function",
                "symbol": "mystery",
                "definition_range": {"begin": 21, "end": 30},
            },
        }
        errors = verifier.ownership_errors("fixture.c", manifest, local)
        self.assertEqual(
            {"unchanged", "mystery"},
            {error["symbol"] for error in errors},
        )

    def test_real_src_contains_only_svistok_owned_definitions(self) -> None:
        verifier = load_tool("verify_overlay_purity")
        ownership = json.loads(
            (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text(
                encoding="utf-8"
            )
        )
        report = verifier.verify_tree(ownership)
        self.assertTrue(report["ok"], json.dumps(report["errors"][:20], indent=2))


if __name__ == "__main__":
    unittest.main()
