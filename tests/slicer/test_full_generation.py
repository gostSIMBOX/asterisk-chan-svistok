#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import shutil
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
BUILD_ROOT = PROJECT_ROOT / "build" / "generated" / "all"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class FullGenerationTests(unittest.TestCase):
    def test_all_modified_roots_generate_once_per_owner(self) -> None:
        if BUILD_ROOT.exists():
            shutil.rmtree(BUILD_ROOT)
        ownership = json.loads(
            (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text(
                encoding="utf-8"
            )
        )
        summary = load_tool("generate_all_slices").generate(ownership, BUILD_ROOT)
        self.assertEqual(16, summary["translation_units"])
        self.assertEqual(32, len(summary["generated_slices"]))
        self.assertEqual(81, summary["bridge_count"])
        self.assertTrue(
            all(Path(item["source"]).is_file() for item in summary["generated_slices"])
        )
        upstream_at_response = (
            BUILD_ROOT / "at_response" / "at_response-upstream.c"
        ).read_text(encoding="latin-1")
        svistok_at_response = (
            BUILD_ROOT / "at_response" / "at_response-svistok.c"
        ).read_text(encoding="latin-1")
        self.assertNotIn('#include "dserial.c"', upstream_at_response)
        self.assertNotIn('#include "limits.c"', upstream_at_response)
        self.assertIn('#include "dserial.c"', svistok_at_response)
        self.assertIn('#include "limits.c"', svistok_at_response)


if __name__ == "__main__":
    unittest.main()
