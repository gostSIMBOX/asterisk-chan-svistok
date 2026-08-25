#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]


def load_inventory_module():
    path = PROJECT_ROOT / "tools" / "audit_module_inventory.py"
    specification = importlib.util.spec_from_file_location(
        "audit_module_inventory", path
    )
    if specification is None or specification.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


class InventoryTests(unittest.TestCase):
    def test_inventory_matches_approved_manifest(self) -> None:
        expected = json.loads(
            (PROJECT_ROOT / "manifests" / "module-files.json").read_text(
                encoding="utf-8"
            )
        )
        actual = load_inventory_module().build_inventory()
        self.assertEqual(expected, actual)
        self.assertEqual(19, len(actual["root_translation_units"]))
        self.assertEqual(52, actual["counts"]["module_total"])
        self.assertEqual(12, actual["counts"]["module_new"])
        self.assertEqual(28, actual["counts"]["module_modified"])
        self.assertEqual(12, actual["counts"]["module_identical"])
        self.assertEqual(109, actual["counts"]["excluded_total"])


if __name__ == "__main__":
    unittest.main()
