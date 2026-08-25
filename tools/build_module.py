#!/usr/bin/env python3
"""Generate, compile, link, and audit chan_svistok.so."""

from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BASELINE_ROOT = PROJECT_ROOT / "asterisk-chan-dongle"
SRC_ROOT = PROJECT_ROOT / "src"
OWNERSHIP = PROJECT_ROOT / "manifests" / "symbol-ownership.json"
IDENTICAL_SOURCES = ("char_conv.c", "memmem.c", "mixbuffer.c")


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def run(command: list[str]) -> subprocess.CompletedProcess[str]:
    completed = subprocess.run(
        command,
        cwd=PROJECT_ROOT,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode:
        raise RuntimeError(
            f'command failed ({completed.returncode}): {" ".join(command)}\n'
            f"{completed.stderr}"
        )
    return completed


def defined_symbols(path: Path) -> set[str]:
    output = run(["nm", "-a", str(path)]).stdout
    symbols: set[str] = set()
    for line in output.splitlines():
        fields = line.split()
        if len(fields) < 2 or fields[-2].upper() == "U":
            continue
        symbol = fields[-1]
        if platform.system() == "Darwin" and symbol.startswith("_"):
            symbol = symbol[1:]
        symbols.add(symbol)
    return symbols


def build(build_root: Path, asterisk_include: Path, compiler: str) -> dict:
    guard = load_tool("check_source_guards")
    if not guard.check()["ok"]:
        raise RuntimeError("source guard failed before build")
    if not (asterisk_include / "asterisk.h").is_file():
        raise RuntimeError(f"Asterisk include root has no asterisk.h: {asterisk_include}")

    ownership = json.loads(OWNERSHIP.read_text(encoding="utf-8"))
    generated_root = build_root / "generated"
    summary = load_tool("generate_all_slices").generate(ownership, generated_root)
    config = generated_root / "svistok_config.h"
    config.write_text(load_tool("generate_config").render(), encoding="utf-8")
    objects_root = build_root / "objects"
    objects_root.mkdir(parents=True, exist_ok=True)

    sources = [Path(item["source"]) for item in summary["generated_slices"]]
    sources.extend(BASELINE_ROOT / item for item in IDENTICAL_SOURCES)
    sources.append(SRC_ROOT / "svistok_state.c")
    common = [
        compiler,
        "-std=gnu89",
        "-fno-common",
        "-fPIC",
        "-Wno-invalid-pp-token",
        "-Wno-deprecated-non-prototype",
        "-Wno-error=implicit-function-declaration",
        "-Wno-return-mismatch",
        "-Wno-int-conversion",
        "-Wno-incompatible-function-pointer-types",
        "-ferror-limit=0",
        "-DICONV_CONST=",
        "-DICONV_T=iconv_t",
        "-DBUILD_APPLICATIONS=1",
        "-DBUILD_MANAGER=1",
        "-DASTERISK_VERSION_NUM=110000",
        "-include",
        str(config),
        "-I",
        str(asterisk_include),
        "-I",
        str(SRC_ROOT),
        "-I",
        str(PROJECT_ROOT),
        "-I",
        str(BASELINE_ROOT),
    ]
    objects: list[Path] = []
    for index, source in enumerate(sources):
        output = objects_root / f"{index:02d}-{source.stem}.o"
        run([*common, "-c", str(source), "-o", str(output)])
        objects.append(output)

    object_by_pair = {
        (item["file"], item["side"]): objects[index]
        for index, item in enumerate(summary["generated_slices"])
    }
    materialized = json.loads(
        (generated_root / "materialized-manifest.json").read_text(encoding="utf-8")
    )
    ownership_errors: list[str] = []
    checked_public_symbols = 0
    for record in materialized["files"]:
        relative = record["legacy_file"]
        if (relative, "upstream") not in object_by_pair:
            continue
        side_symbols = {
            side: defined_symbols(object_by_pair[(relative, side)])
            for side in ("upstream", "svistok")
        }
        entries = list(record["symbols"])
        entries.extend(
            symbol
            for included in record.get("included_sources", [])
            for symbol in included["symbols"]
        )
        expected = {
            entry["symbol"]: entry
            for entry in entries
            if entry["linkage"] == "external"
            and entry["owner"] in {"upstream", "svistok"}
        }
        for symbol, entry in expected.items():
            checked_public_symbols += 1
            owner = entry["owner"]
            other = "svistok" if owner == "upstream" else "upstream"
            if symbol not in side_symbols[owner]:
                ownership_errors.append(f"{relative}: {symbol} missing from {owner}")
            if symbol in side_symbols[other]:
                ownership_errors.append(f"{relative}: {symbol} duplicated in {other}")
    if ownership_errors:
        raise RuntimeError("object ownership audit failed: " + "; ".join(ownership_errors))

    module = build_root / "chan_svistok.so"
    link = [compiler]
    if platform.system() == "Darwin":
        link.extend(("-bundle", "-undefined", "dynamic_lookup"))
    else:
        link.append("-shared")
    link.extend(("-o", str(module), *(str(path) for path in objects)))
    if platform.system() == "Darwin":
        link.append("-liconv")
    run(link)

    undefined = run(["nm", "-u", str(module)]).stdout
    if "svistok_bridge_" in undefined:
        raise RuntimeError("linked module has unresolved Svistok bridges")
    if platform.system() == "Darwin":
        public = run(["nm", "-gU", str(module)]).stdout
    else:
        public = run(["nm", "-D", "--defined-only", str(module)]).stdout
    if "svistok_bridge_" in public:
        raise RuntimeError("Svistok bridges leaked into the public module API")
    final_symbols = defined_symbols(module)
    relocated_state = {
        "nosim2offline",
        "total_stat_acdl",
        "total_stat_pddl",
        "total_stat_datt",
    }
    if not relocated_state <= final_symbols:
        raise RuntimeError("relocated header state is missing from linked module")
    if not guard.check()["ok"]:
        raise RuntimeError("source guard failed after build")

    report = {
        "schema_version": 1,
        "module": str(module),
        "module_name": module.name,
        "compiler": compiler,
        "asterisk_include": str(asterisk_include.resolve()),
        "compatibility_stubs": asterisk_include.resolve()
        == (PROJECT_ROOT / "tests" / "characterization" / "stubs").resolve(),
        "objects": len(objects),
        "slices": len(summary["generated_slices"]),
        "bridges": summary["bridge_count"],
        "unresolved_bridges": 0,
        "public_bridges": 0,
        "public_symbols_checked": checked_public_symbols,
        "object_ownership_errors": 0,
        "single_owner_state": sorted(relocated_state),
    }
    (build_root / "build-report.json").write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return report


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, default=PROJECT_ROOT / "build" / "module")
    parser.add_argument("--asterisk-include", type=Path, required=True)
    parser.add_argument("--compiler", default=shutil.which("clang") or shutil.which("cc"))
    arguments = parser.parse_args()
    try:
        if not arguments.compiler:
            raise RuntimeError("C compiler not found")
        report = build(arguments.build_dir.resolve(), arguments.asterisk_include.resolve(), arguments.compiler)
    except (OSError, RuntimeError, KeyError) as error:
        print(f"module build failed: {error}", file=sys.stderr)
        return 1
    print(
        f'built {report["module_name"]}: {report["objects"]} objects, '
        f'{report["bridges"]} hidden bridges'
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
