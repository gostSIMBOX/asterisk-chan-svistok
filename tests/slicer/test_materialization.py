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


class MaterializationTests(unittest.TestCase):
    def test_adapted_sources_preserve_all_legacy_owned_bodies(self) -> None:
        ownership = json.loads(
            (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text(
                encoding="utf-8"
            )
        )
        materialized = load_tool("materialize_build_manifest").materialize(ownership)
        roots = set(load_tool("clang_manifest").MODIFIED_ROOTS)
        records = [item for item in materialized["files"] if item["legacy_file"] in roots]
        self.assertEqual(16, len(records))
        for record in records:
            for symbol in record["symbols"]:
                if symbol["legacy"] is not None:
                    original_record = next(
                        item for item in ownership["files"]
                        if item["legacy_file"] == record["legacy_file"]
                    )
                    original = next(
                        item for item in original_record["symbols"]
                        if item["symbol"] == symbol["symbol"]
                    )
                    self.assertEqual(
                        original["legacy"]["source_sha256"],
                        symbol["legacy"]["source_sha256"],
                    )


if __name__ == "__main__":
    unittest.main()
