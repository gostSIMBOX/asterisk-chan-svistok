#!/usr/bin/env python3

from __future__ import annotations

import json
from pathlib import Path
import shutil
import subprocess
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BUILD_ROOT = PROJECT_ROOT / "build" / "module-test"


class ModuleBuildTests(unittest.TestCase):
    def test_compatibility_build_links_chan_svistok_with_hidden_bridges(self) -> None:
        if BUILD_ROOT.exists():
            shutil.rmtree(BUILD_ROOT)
        completed = subprocess.run(
            [
                "python3",
                str(PROJECT_ROOT / "tools" / "build_module.py"),
                "--build-dir",
                str(BUILD_ROOT),
                "--asterisk-include",
                str(PROJECT_ROOT / "tests" / "characterization" / "stubs"),
            ],
            cwd=PROJECT_ROOT,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)
        report = json.loads(
            (BUILD_ROOT / "build-report.json").read_text(encoding="utf-8")
        )
        self.assertEqual("chan_svistok.so", report["module_name"])
        self.assertEqual(36, report["objects"])
        self.assertEqual(32, report["slices"])
        self.assertEqual(16, report["baseline_slices"])
        self.assertEqual(16, report["overlay_compositions"])
        self.assertEqual(81, report["bridges"])
        self.assertEqual(0, report["unresolved_bridges"])
        self.assertEqual(0, report["public_bridges"])
        self.assertGreater(report["public_symbols_checked"], 100)
        self.assertEqual(0, report["object_ownership_errors"])
        self.assertEqual(4, len(report["single_owner_state"]))
        self.assertTrue(report["compatibility_stubs"])
        self.assertTrue((BUILD_ROOT / "chan_svistok.so").is_file())


if __name__ == "__main__":
    unittest.main()
