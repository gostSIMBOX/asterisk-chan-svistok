#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
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


class OverlayExtractionTests(unittest.TestCase):
    def test_extraction_removes_app_register_but_keeps_changed_app_status(self) -> None:
        app = (PROJECT_ROOT / "src" / "app.c").read_text(encoding="latin-1")
        self.assertNotIn("EXPORT_DEF void app_register()", app)
        self.assertIn("static int app_status_exec", app)
        report = __import__("json").loads(
            (PROJECT_ROOT / "manifests" / "overlay-extraction.json").read_text(
                encoding="utf-8"
            )
        )
        app_report = next(item for item in report["files"] if item["file"] == "app.c")
        self.assertIn(
            "app_register",
            {item["symbol"] for item in app_report["removed"]},
        )


if __name__ == "__main__":
    unittest.main()
