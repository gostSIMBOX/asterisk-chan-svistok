#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"
STUB_ROOT = PROJECT_ROOT / "tests" / "characterization" / "stubs"
BUILD_ROOT = PROJECT_ROOT / "build" / "include-ownership"
INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.MULTILINE)


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class IncludeOwnershipTests(unittest.TestCase):
    def test_local_and_upstream_headers_have_explicit_owners(self) -> None:
        inventory = json.loads(
            (PROJECT_ROOT / "manifests" / "module-files.json").read_text(
                encoding="utf-8"
            )
        )
        local = set(inventory["module"]["new"] + inventory["module"]["modified"])
        local.update({"svistok_abi.h", "svistok_state.c"})
        layout_promotion = json.loads(
            (PROJECT_ROOT / "manifests/function-layout-promotion.json").read_text()
        )
        local.update(layout_promotion["after"])
        upstream = set(inventory["module"]["identical"])
        for source in SRC_ROOT.rglob("*"):
            if not source.is_file():
                continue
            relative_source = source.relative_to(SRC_ROOT)
            text = source.read_text(encoding="latin-1")
            for delimiter, include in INCLUDE_PATTERN.findall(text):
                if delimiter == '"':
                    sibling = SRC_ROOT / relative_source.parent / include
                    resolved = (
                        sibling.resolve().relative_to(SRC_ROOT.resolve()).as_posix()
                        if sibling.is_file()
                        else Path(include).as_posix()
                    )
                    self.assertIn(resolved, local, f"implicit/unowned include in {relative_source}: {include}")
                elif include.startswith("asterisk-chan-dongle/"):
                    resolved = include.removeprefix("asterisk-chan-dongle/")
                    self.assertIn(resolved, upstream, f"non-identical upstream include in {relative_source}: {include}")
                    self.assertTrue((PROJECT_ROOT / include).is_file())

    def test_compiler_dependencies_resolve_all_modified_roots(self) -> None:
        if BUILD_ROOT.exists():
            shutil.rmtree(BUILD_ROOT)
        BUILD_ROOT.mkdir(parents=True)
        config = BUILD_ROOT / "svistok_config.h"
        config.write_text(load_tool("generate_config").render(), encoding="utf-8")
        manifest_tool = load_tool("clang_manifest")
        compiler = shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler)
        for relative in manifest_tool.MODIFIED_ROOTS:
            source = SRC_ROOT / relative
            if not source.is_file():
                source = SRC_ROOT / "svistok" / relative
            if not source.is_file():
                continue
            completed = subprocess.run(
                [
                    str(compiler),
                    "-std=gnu89",
                    "-DHAVE_CONFIG_H=1",
                    "-I",
                    str(BUILD_ROOT),
                    "-I",
                    str(STUB_ROOT),
                    "-I",
                    str(SRC_ROOT),
                    "-I",
                    str(PROJECT_ROOT),
                    "-MM",
                    "-MG",
                    str(source),
                ],
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            self.assertEqual(0, completed.returncode, f"{relative}: {completed.stderr}")
            self.assertIn("svistok_config.h", completed.stdout)


if __name__ == "__main__":
    unittest.main()
