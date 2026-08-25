#!/usr/bin/env python3
"""Compile and execute read-only legacy characterization programs out of tree."""

from __future__ import annotations

import hashlib
import importlib.util
from pathlib import Path
import shutil
import subprocess
from typing import Iterable


PROJECT_ROOT = Path(__file__).resolve().parents[2]
LEGACY_ROOT = (PROJECT_ROOT / "../../legacy/asterisk-chan-svistok-v2014").resolve()
STUB_ROOT = Path(__file__).resolve().parent / "stubs"
BUILD_ROOT = PROJECT_ROOT / "build" / "characterization"


def load_source_guard_module():
    path = PROJECT_ROOT / "tools" / "check_source_guards.py"
    specification = importlib.util.spec_from_file_location("check_source_guards", path)
    if specification is None or specification.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def stable_case_directory(case_name: str, driver_source: str, sources: Iterable[str]) -> Path:
    digest = hashlib.sha256()
    digest.update(driver_source.encode("utf-8"))
    for source in sources:
        digest.update(source.encode("utf-8"))
        digest.update((LEGACY_ROOT / source).read_bytes())
    return BUILD_ROOT / f"{case_name}-{digest.hexdigest()[:12]}"


def run_command(command: list[str], *, cwd: Path) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"command failed ({completed.returncode}): {' '.join(command)}\n"
            f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
        )
    return completed


def compile_and_run(
    case_name: str,
    driver_source: str,
    legacy_sources: Iterable[str],
    *,
    compiler_defines: Iterable[str] = (),
    compiler_flags: Iterable[str] = (),
    linker_flags: Iterable[str] = (),
) -> str:
    source_list = tuple(legacy_sources)
    guard = load_source_guard_module()
    before = guard.check()
    if not before["ok"]:
        raise RuntimeError(f"source guard failed before oracle run: {before['errors']}")

    case_directory = stable_case_directory(case_name, driver_source, source_list)
    if case_directory.exists():
        shutil.rmtree(case_directory)
    case_directory.mkdir(parents=True)

    driver_path = case_directory / "driver.c"
    driver_path.write_text(driver_source, encoding="utf-8")

    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise RuntimeError("no C compiler found")

    common_flags = [
        "-std=gnu89",
        "-Wno-invalid-pp-token",
        "-Wno-deprecated-non-prototype",
        "-DICONV_CONST=",
        "-DICONV_T=iconv_t",
        "-I",
        str(STUB_ROOT),
        "-I",
        str(LEGACY_ROOT),
        *compiler_defines,
        *compiler_flags,
    ]

    objects: list[str] = []
    for index, relative in enumerate(source_list):
        source = LEGACY_ROOT / relative
        if not source.is_file():
            raise RuntimeError(f"legacy oracle source does not exist: {relative}")
        object_path = case_directory / f"legacy-{index}-{source.stem}.o"
        run_command(
            [compiler, *common_flags, "-c", str(source), "-o", str(object_path)],
            cwd=case_directory,
        )
        objects.append(str(object_path))

    driver_object = case_directory / "driver.o"
    run_command(
        [compiler, *common_flags, "-c", str(driver_path), "-o", str(driver_object)],
        cwd=case_directory,
    )
    executable = case_directory / case_name
    run_command(
        [compiler, str(driver_object), *objects, *linker_flags, "-o", str(executable)],
        cwd=case_directory,
    )
    result = run_command([str(executable)], cwd=case_directory)

    after = guard.check()
    if not after["ok"]:
        raise RuntimeError(f"source guard failed after oracle run: {after['errors']}")
    return result.stdout
