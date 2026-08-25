#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]


class StateRelocationTests(unittest.TestCase):
    def test_header_state_has_one_explicit_storage_owner(self) -> None:
        manifest = json.loads(
            (PROJECT_ROOT / "manifests" / "state-relocations.json").read_text(
                encoding="utf-8"
            )
        )
        header = (PROJECT_ROOT / manifest["source_header"]).read_text(
            encoding="latin-1"
        )
        owner = (PROJECT_ROOT / manifest["storage_owner"]).read_text(
            encoding="utf-8"
        )
        self.assertEqual(4, len(manifest["symbols"]))
        for entry in manifest["symbols"]:
            name = entry["name"]
            self.assertRegex(header, rf"\bextern\b[^;]*\b{name}\b")
            definitions = re.findall(rf"^(?!extern\b)[^/\n;]*\b{name}\b[^;]*;", owner, re.MULTILINE)
            self.assertEqual(1, len(definitions), name)


if __name__ == "__main__":
    unittest.main()
