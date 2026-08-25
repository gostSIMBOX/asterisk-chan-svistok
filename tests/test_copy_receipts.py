#!/usr/bin/env python3

from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
LEGACY_ROOT = (PROJECT_ROOT / "../../legacy/asterisk-chan-svistok-v2014").resolve()
SRC_ROOT = PROJECT_ROOT / "src"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


class CopyReceiptTests(unittest.TestCase):
    def test_only_approved_files_were_copied_byte_identically(self) -> None:
        inventory = json.loads(
            (PROJECT_ROOT / "manifests" / "module-files.json").read_text(
                encoding="utf-8"
            )
        )
        expected = {
            path: relation
            for relation in ("new", "modified")
            for path in inventory["module"][relation]
        }
        promotion = json.loads(
            (PROJECT_ROOT / "manifests" / "overlay-promotion.json").read_text(
                encoding="utf-8"
            )
        )
        promoted = {entry["file"]: entry for entry in promotion["files"]}
        adaptations_manifest = json.loads(
            (PROJECT_ROOT / "manifests" / "post-copy-adaptations.json").read_text(
                encoding="utf-8"
            )
        )
        adaptations = {
            entry["file"]: entry for entry in adaptations_manifest["files"]
        }
        rows = {}
        receipt_lines = (
            PROJECT_ROOT / "manifests" / "copy-receipts.sha256"
        ).read_text(encoding="utf-8").splitlines()
        for line in receipt_lines:
            if not line or line.startswith("#"):
                continue
            relation, path, legacy_hash, destination_hash = line.split("\t")
            self.assertNotIn(path, rows)
            rows[path] = (relation, legacy_hash, destination_hash)

        actual = {
            path.relative_to(SRC_ROOT).as_posix()
            for path in SRC_ROOT.rglob("*")
            if path.is_file() or path.is_symlink()
        }
        self.assertEqual(40, len(expected))
        self.assertEqual(expected.keys(), rows.keys())
        self.assertTrue(expected.keys() <= actual)
        for path, relation in expected.items():
            legacy = LEGACY_ROOT / path
            destination = SRC_ROOT / path
            legacy_hash = sha256(legacy)
            self.assertEqual(relation, rows[path][0])
            self.assertEqual(legacy_hash, rows[path][1])
            self.assertEqual(legacy_hash, rows[path][2])
            if relation == "new":
                if path in adaptations:
                    self.assertEqual(legacy_hash, adaptations[path]["before_sha256"])
                    self.assertEqual(
                        sha256(destination), adaptations[path]["after_sha256"]
                    )
                    self.assertEqual("include-only", adaptations[path]["scope"])
                else:
                    self.assertEqual(legacy_hash, sha256(destination))
            else:
                self.assertIn(path, promoted)
                backup = Path(promotion["backup"]) / path
                self.assertEqual(sha256(backup), promoted[path]["before_sha256"])
                self.assertEqual(sha256(destination), promoted[path]["after_sha256"])

        self.assertEqual(set(inventory["module"]["modified"]), promoted.keys())
        self.assertTrue(adaptations.keys() <= set(inventory["module"]["new"]))

        forbidden = set(inventory["module"]["identical"])
        for entries in inventory["excluded_do_not_copy"].values():
            for entry in entries:
                forbidden.add(entry["path"] if isinstance(entry, dict) else entry)
        self.assertEqual(121, len(forbidden))
        self.assertTrue(
            all(not os.path.lexists(SRC_ROOT / path) for path in forbidden)
        )


if __name__ == "__main__":
    unittest.main()
