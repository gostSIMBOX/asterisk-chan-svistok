#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]


class AppUpstreamReuseTests(unittest.TestCase):
    def test_app_register_is_equivalent_and_absent_from_overlay_source(self) -> None:
        ownership = json.loads(
            (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text(
                encoding="utf-8"
            )
        )
        app = next(record for record in ownership["files"] if record["legacy_file"] == "app.c")
        symbol = next(entry for entry in app["symbols"] if entry["symbol"] == "app_register")
        self.assertEqual("equivalent", symbol["body_relation"])
        self.assertEqual("upstream", symbol["owner"])
        source = (PROJECT_ROOT / "src" / "app.c").read_text(encoding="latin-1")
        definition = re.compile(r"\bapp_register\s*\([^;{}]*\)\s*\{")
        self.assertIsNone(definition.search(source))


if __name__ == "__main__":
    unittest.main()
