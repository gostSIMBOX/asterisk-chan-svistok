#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
CHARACTERIZATION = PROJECT_ROOT / "tests" / "characterization"
STUB_ROOT = CHARACTERIZATION / "stubs"
BUILD_ROOT = PROJECT_ROOT / "build" / "migrated-golden"
sys.path.insert(0, str(CHARACTERIZATION))

from test_at_read_oracle import AT_READ_DRIVER  # noqa: E402
from test_buffers_oracle import BUFFER_DRIVER  # noqa: E402
from test_pdu_oracle import PDU_DRIVER  # noqa: E402


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def generated_root() -> Path:
    root = BUILD_ROOT / "generated"
    ownership = json.loads(
        (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text(
            encoding="utf-8"
        )
    )
    load_tool("generate_all_slices").generate(ownership, root)
    (root / "svistok_config.h").write_text(
        load_tool("generate_config").render(), encoding="utf-8"
    )
    return root


def compile_and_run(case: str, driver: str, sources: list[Path]) -> str:
    guard = load_tool("check_source_guards")
    if not guard.check()["ok"]:
        raise RuntimeError("source guard failed before migrated replay")
    case_root = BUILD_ROOT / case
    if case_root.exists():
        shutil.rmtree(case_root)
    case_root.mkdir(parents=True)
    driver_path = case_root / "driver.c"
    driver_path.write_text(driver, encoding="utf-8")
    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise RuntimeError("C compiler not found")
    config = BUILD_ROOT / "generated" / "svistok_config.h"
    common = [
        compiler,
        "-std=gnu89",
        "-fno-common",
        "-Wno-invalid-pp-token",
        "-Wno-deprecated-non-prototype",
        "-Wno-error=implicit-function-declaration",
        "-Wno-return-mismatch",
        "-Wno-int-conversion",
        "-DICONV_CONST=",
        "-DICONV_T=iconv_t",
        "-include",
        str(config),
        "-include",
        str(BUILD_ROOT / "generated" / "composed-header-defines.h"),
        "-I",
        str(STUB_ROOT),
        "-I",
        str(PROJECT_ROOT / "src"),
        "-I",
        str(PROJECT_ROOT),
        "-I",
        str(PROJECT_ROOT / "asterisk-chan-dongle"),
    ]
    objects = []
    for index, source in enumerate([*sources, driver_path]):
        output = case_root / f"{index:02d}-{source.stem}.o"
        completed = subprocess.run(
            [*common, "-c", str(source), "-o", str(output)],
            cwd=PROJECT_ROOT,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        if completed.returncode:
            raise RuntimeError(f"compile failed for {source}:\n{completed.stderr}")
        objects.append(output)
    executable = case_root / case
    link = [compiler, *(str(path) for path in objects)]
    if platform.system() == "Darwin" and case == "pdu":
        link.append("-liconv")
    completed = subprocess.run(
        [*link, "-o", str(executable)],
        cwd=PROJECT_ROOT,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode:
        raise RuntimeError(f"link failed:\n{completed.stderr}")
    result = subprocess.run(
        [str(executable)], check=True, stdout=subprocess.PIPE, text=True
    ).stdout
    if not guard.check()["ok"]:
        raise RuntimeError("source guard failed after migrated replay")
    return result


class MigratedGoldenReplayTests(unittest.TestCase):
    def test_pdu_composition_matches_frozen_legacy_output(self) -> None:
        root = generated_root()
        output = compile_and_run(
            "pdu",
            PDU_DRIVER,
            [
                root / "pdu" / "pdu-upstream.c",
                root / "pdu" / "pdu-overlay.c",
                PROJECT_ROOT / "asterisk-chan-dongle" / "char_conv.c",
            ],
        )
        expected = (CHARACTERIZATION / "fixtures" / "pdu" / "build-vectors.txt").read_text(encoding="utf-8")
        self.assertEqual(expected, output)

    def test_buffer_composition_matches_frozen_legacy_output(self) -> None:
        root = generated_root()
        output = compile_and_run(
            "buffers",
            BUFFER_DRIVER,
            [
                root / "ringbuffer" / "ringbuffer-upstream.c",
                root / "ringbuffer" / "ringbuffer-overlay.c",
                PROJECT_ROOT / "asterisk-chan-dongle" / "memmem.c",
                PROJECT_ROOT / "asterisk-chan-dongle" / "mixbuffer.c",
            ],
        )
        expected = (CHARACTERIZATION / "fixtures" / "buffers" / "ringbuffer.txt").read_text(encoding="utf-8")
        self.assertEqual(expected, output)

    def test_at_read_composition_matches_frozen_legacy_output(self) -> None:
        root = generated_root()
        output = compile_and_run(
            "at-read",
            AT_READ_DRIVER,
            [
                root / "at_read" / "at_read-upstream.c",
                root / "at_read" / "at_read-overlay.c",
                root / "ringbuffer" / "ringbuffer-upstream.c",
                root / "ringbuffer" / "ringbuffer-overlay.c",
                PROJECT_ROOT / "asterisk-chan-dongle" / "memmem.c",
            ],
        )
        expected = (CHARACTERIZATION / "fixtures" / "buffers" / "at-read.txt").read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
