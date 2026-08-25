#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
LEGACY_ROOT = (PROJECT_ROOT / "../../legacy/asterisk-chan-svistok-v2014").resolve()
BASELINE_ROOT = (PROJECT_ROOT / "asterisk-chan-dongle").resolve()
STUB_ROOT = PROJECT_ROOT / "tests" / "characterization" / "stubs"
BUILD_ROOT = PROJECT_ROOT / "build" / "generated" / "at_parse"
sys.path.insert(0, str(PROJECT_ROOT / "tests" / "characterization"))

from test_at_parse_oracle import AT_PARSE_DRIVER  # noqa: E402


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class AtParsePilotTests(unittest.TestCase):
    def test_composed_real_slices_match_the_pre_copy_oracle(self) -> None:
        manifest_tool = load_tool("clang_manifest")
        slicer = load_tool("slice_translation_unit")
        bridge_tool = load_tool("generate_bridges")
        verifier = load_tool("verify_source_ownership")
        guard = load_tool("check_source_guards")
        self.assertTrue(guard.check()["ok"])

        record = manifest_tool.build_file_manifest("at_parse.c")
        if BUILD_ROOT.exists():
            shutil.rmtree(BUILD_ROOT)
        BUILD_ROOT.mkdir(parents=True)

        generated_sources: list[Path] = []
        for side, source_path in (
            ("upstream", BASELINE_ROOT / "at_parse.c"),
            ("svistok", LEGACY_ROOT / "at_parse.c"),
        ):
            raw_slice = slicer.emit_slice(
                source_path.read_bytes(),
                record["symbols"],
                side=side,
                display_path=f"{side}/at_parse.c",
                declarations=record.get("declarations"),
                bridges=record.get("bridges"),
            )
            provenance_errors = verifier.verify_slice(
                raw_slice, source_path.read_bytes(), record["symbols"], side=side
            )
            self.assertEqual([], provenance_errors)
            prefix, suffix = bridge_tool.generate_for_side(
                record["symbols"], record["bridges"], side
            )
            output = BUILD_ROOT / f"at_parse-{side}.c"
            output.write_bytes(
                b'#include "at_parse.h"\n'
                + prefix.encode("utf-8")
                + raw_slice
                + suffix.encode("utf-8")
            )
            generated_sources.append(output)

        driver = BUILD_ROOT / "driver.c"
        driver.write_text(AT_PARSE_DRIVER, encoding="utf-8")
        compiler = shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler)
        common = [
            str(compiler),
            "-std=gnu89",
            "-Wno-invalid-pp-token",
            "-Wno-deprecated-non-prototype",
            "-DICONV_CONST=",
            "-DICONV_T=iconv_t",
            "-DCHAN_DONGLE_H_INCLUDED",
            "-DCHAN_DONGLE_HELPERS_H_INCLUDED",
            "-I",
            str(STUB_ROOT),
            "-I",
            str(LEGACY_ROOT),
            "-I",
            str(BASELINE_ROOT),
            "-include",
            str(STUB_ROOT / "asterisk_compat.h"),
            "-include",
            "stdio.h",
            "-include",
            "stddef.h",
            "-include",
            "string.h",
        ]
        compile_sources = [
            *generated_sources,
            LEGACY_ROOT / "pdu.c",
            LEGACY_ROOT / "char_conv.c",
            LEGACY_ROOT / "memmem.c",
            driver,
        ]
        objects: list[Path] = []
        for index, source in enumerate(compile_sources):
            output = BUILD_ROOT / f"object-{index}.o"
            completed = subprocess.run(
                [*common, "-c", str(source), "-o", str(output)],
                check=False,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
            )
            self.assertEqual(0, completed.returncode, completed.stderr)
            objects.append(output)
        executable = BUILD_ROOT / "at-parse-pilot"
        linked = subprocess.run(
            [str(compiler), *(str(path) for path in objects), "-liconv", "-o", str(executable)],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.assertEqual(0, linked.returncode, linked.stderr)
        result = subprocess.run(
            [str(executable)],
            check=True,
            stdout=subprocess.PIPE,
            text=True,
        )
        expected = (
            PROJECT_ROOT / "tests" / "characterization" / "fixtures" / "at_parse" / "vectors.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, result.stdout)
        self.assertTrue(guard.check()["ok"])


if __name__ == "__main__":
    unittest.main()
