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


class SlicingTests(unittest.TestCase):
    def test_dual_slice_partitions_equivalent_changed_new_and_removed(self) -> None:
        manifest_tool = load_tool("clang_manifest")
        slicer = load_tool("slice_translation_unit")
        baseline_path = FIXTURES / "baseline.c"
        legacy_path = FIXTURES / "legacy.c"
        record = manifest_tool.compare_definitions(
            "synthetic.c",
            manifest_tool.definitions_from_path(legacy_path),
            manifest_tool.definitions_from_path(baseline_path),
        )
        upstream = slicer.emit_slice(
            baseline_path.read_bytes(),
            record["symbols"],
            side="upstream",
            display_path="baseline.c",
        ).decode("utf-8")
        svistok = slicer.emit_slice(
            legacy_path.read_bytes(),
            record["symbols"],
            side="svistok",
            display_path="legacy.c",
        ).decode("utf-8")

        self.assertIn("int unchanged", upstream)
        self.assertIn("private_value = 7", upstream)
        self.assertNotIn("int changed", upstream)
        self.assertNotIn("int removed", upstream)
        self.assertNotIn("int added", upstream)
        self.assertIn("int changed", svistok)
        self.assertIn("return value + 20", svistok)
        self.assertIn("int added", svistok)
        self.assertNotIn("int unchanged", svistok)
        self.assertNotIn("private_value = 7", svistok)

    def test_recursive_c_include_keeps_its_own_source_provenance(self) -> None:
        manifest_tool = load_tool("clang_manifest")
        slicer = load_tool("slice_translation_unit")
        baseline_root = FIXTURES / "baseline-tree"
        legacy_root = FIXTURES / "legacy-tree"

        baseline = manifest_tool.definitions_by_provenance(
            manifest_tool.dump_ast(baseline_root / "root.c", ()),
            baseline_root / "root.c",
            baseline_root,
        )
        legacy = manifest_tool.definitions_by_provenance(
            manifest_tool.dump_ast(legacy_root / "root.c", ()),
            legacy_root / "root.c",
            legacy_root,
        )

        self.assertEqual({"root.c", "nested.c"}, set(baseline))
        self.assertEqual({"root.c", "nested.c"}, set(legacy))
        nested = manifest_tool.compare_definitions(
            "nested.c", legacy["nested.c"], baseline["nested.c"]
        )
        self.assertEqual(1, len(nested["symbols"]))
        self.assertEqual("nested_value", nested["symbols"][0]["symbol"])
        self.assertEqual("modified", nested["symbols"][0]["body_relation"])
        self.assertEqual("svistok", nested["symbols"][0]["owner"])

        root_record = manifest_tool.compare_definitions(
            "root.c", legacy["root.c"], baseline["root.c"]
        )
        upstream_slice = slicer.emit_slice(
            (baseline_root / "root.c").read_bytes(),
            root_record["symbols"],
            side="upstream",
            display_path="root.c",
            included_sources=["nested.c"],
        )
        svistok_slice = slicer.emit_slice(
            (legacy_root / "root.c").read_bytes(),
            root_record["symbols"],
            side="svistok",
            display_path="root.c",
            included_sources=["nested.c"],
        )
        self.assertNotIn(b'#include "nested.c"', upstream_slice)
        self.assertIn(b'#include "nested.c"', svistok_slice)

    def test_macros_respect_relations_and_active_conditional_branch(self) -> None:
        manifest_tool = load_tool("clang_manifest")
        baseline_path = FIXTURES / "baseline-tree" / "conditional.h"
        legacy_path = FIXTURES / "legacy-tree" / "conditional.h"

        enabled = manifest_tool.compare_macros(
            manifest_tool.preprocess_macros(legacy_path, ("-DFEATURE_MODE=1",)),
            manifest_tool.preprocess_macros(baseline_path, ("-DFEATURE_MODE=1",)),
        )
        enabled_by_name = {entry["symbol"]: entry for entry in enabled}
        self.assertEqual("equivalent", enabled_by_name["SAME_MACRO"]["body_relation"])
        self.assertEqual("modified", enabled_by_name["CHANGED_MACRO"]["body_relation"])
        self.assertEqual("new", enabled_by_name["ADDED_MACRO"]["body_relation"])
        self.assertEqual("removed", enabled_by_name["REMOVED_MACRO"]["body_relation"])
        self.assertEqual(
            "equivalent", enabled_by_name["CONDITIONAL_MACRO"]["body_relation"]
        )

        disabled = manifest_tool.compare_macros(
            manifest_tool.preprocess_macros(legacy_path, ("-DFEATURE_MODE=0",)),
            manifest_tool.preprocess_macros(baseline_path, ("-DFEATURE_MODE=0",)),
        )
        disabled_by_name = {entry["symbol"]: entry for entry in disabled}
        self.assertEqual(
            "modified", disabled_by_name["CONDITIONAL_MACRO"]["body_relation"]
        )
        self.assertEqual("svistok", disabled_by_name["CONDITIONAL_MACRO"]["owner"])


if __name__ == "__main__":
    unittest.main()
