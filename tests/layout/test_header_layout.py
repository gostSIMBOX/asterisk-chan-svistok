#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
import re
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]


class HeaderLayoutTests(unittest.TestCase):
    def test_external_new_declarations_live_in_svistok_headers(self) -> None:
        layout = json.loads(
            (PROJECT_ROOT / "manifests/function-layout.json").read_text()
        )
        entries = [
            entry for entry in layout["functions"]
            if entry["layout_owner"] == "svistok-new"
            and entry["linkage"] == "external"
        ]
        headers = set()
        for entry in entries:
            stem = Path(entry["source_file"]).stem
            header = PROJECT_ROOT / "src/svistok" / f"{stem}.h"
            compatibility = PROJECT_ROOT / "src" / f"{stem}.h"
            self.assertTrue(header.is_file(), header)
            text = header.read_text(encoding="latin-1")
            self.assertRegex(text, rf"\b{re.escape(entry['symbol'])}\s*\(")
            if compatibility.is_file():
                integration = compatibility
                expected_include = f'#include "svistok/{stem}.h"'
            else:
                integration = PROJECT_ROOT / "src/svistok" / entry["source_file"]
                expected_include = f'#include "{stem}.h"'
            self.assertIn(
                expected_include,
                integration.read_text(encoding="latin-1"),
            )
            headers.add(header)
        self.assertEqual(23, len(entries))
        self.assertEqual(6, len(headers))


if __name__ == "__main__":
    unittest.main()
