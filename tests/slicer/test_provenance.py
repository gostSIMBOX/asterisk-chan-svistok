#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
FIXTURES = Path(__file__).resolve().parent / "fixtures"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ProvenanceTests(unittest.TestCase):
    def test_exact_owner_bodies_pass_while_edited_or_swapped_bodies_fail(self) -> None:
        manifest_tool = load_tool("clang_manifest")
        slicer = load_tool("slice_translation_unit")
        verifier = load_tool("verify_source_ownership")
        baseline = (FIXTURES / "baseline.c").read_bytes()
        legacy = (FIXTURES / "legacy.c").read_bytes()
        record = manifest_tool.compare_definitions(
            "synthetic.c",
            manifest_tool.definitions_from_path(FIXTURES / "legacy.c"),
            manifest_tool.definitions_from_path(FIXTURES / "baseline.c"),
        )
        upstream = slicer.emit_slice(
            baseline, record["symbols"], side="upstream", display_path="baseline.c"
        )
        svistok = slicer.emit_slice(
            legacy, record["symbols"], side="svistok", display_path="legacy.c"
        )
        self.assertEqual(
            [], verifier.verify_slice(upstream, baseline, record["symbols"], side="upstream")
        )
        self.assertEqual(
            [], verifier.verify_slice(svistok, legacy, record["symbols"], side="svistok")
        )

        edited = svistok.replace(b"return value + 20", b"return value + 21")
        edited_errors = verifier.verify_slice(
            edited, legacy, record["symbols"], side="svistok"
        )
        self.assertTrue(any("changed" in error and "legacy" in error for error in edited_errors))

        swapped_errors = verifier.verify_slice(
            svistok, baseline, record["symbols"], side="upstream"
        )
        self.assertTrue(any("unchanged" in error for error in swapped_errors))


if __name__ == "__main__":
    unittest.main()
