#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent


class GoldenManifestTests(unittest.TestCase):
    def test_manifest_covers_every_fixture_with_matching_hash(self) -> None:
        manifest = json.loads(
            (CHARACTERIZATION_ROOT / "golden-manifest.json").read_text(
                encoding="utf-8"
            )
        )
        entries = {entry["path"]: entry for entry in manifest["fixtures"]}
        actual_paths = {
            path.relative_to(CHARACTERIZATION_ROOT).as_posix()
            for path in (CHARACTERIZATION_ROOT / "fixtures").rglob("*")
            if path.is_file()
        }
        self.assertEqual(actual_paths, set(entries))
        for relative_path, entry in entries.items():
            fixture = CHARACTERIZATION_ROOT / relative_path
            digest = hashlib.sha256(fixture.read_bytes()).hexdigest()
            self.assertEqual(entry["sha256"], digest, relative_path)
            self.assertTrue(entry["sources"], relative_path)
            self.assertTrue(entry["covered_symbols"], relative_path)


if __name__ == "__main__":
    unittest.main()
