#!/usr/bin/env python3
"""Verify physical paths of classified function definitions under src."""

from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import re
import sys
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def verify(
    layout: dict[str, Any], ownership: dict[str, Any], *, src_root: Path = SRC_ROOT
) -> dict[str, Any]:
    generator = load_tool("generate_all_slices")
    clang_manifest = load_tool("clang_manifest")
    generated = PROJECT_ROOT / "build" / "function-layout-verify"
    summary = generator.generate(ownership, generated, src_root=src_root)
    wanted = {entry["symbol"]: entry for entry in layout["functions"]}
    if len(wanted) != len(layout["functions"]):
        raise RuntimeError("layout function symbols are not globally unique")
    physical: dict[str, set[str]] = {symbol: set() for symbol in wanted}
    physical_hashes: dict[str, set[str]] = {symbol: set() for symbol in wanted}
    for item in summary["generated_slices"]:
        if item.get("kind") != "overlay-composition":
            continue
        source = Path(item["source"])
        ast = clang_manifest.dump_ast(source, ("-I", str(src_root)))
        grouped = clang_manifest.definitions_by_provenance(ast, source, src_root)
        for relative, definitions in grouped.items():
            for record in definitions.values():
                symbol = record["symbol"]
                if symbol in physical:
                    physical[symbol].add(f"src/{relative}")
                    physical_hashes[symbol].add(record["source_sha256"])
    for entry in layout["functions"]:
        if not entry["source_file"].endswith(".h"):
            continue
        relative = entry["destination"].removeprefix("src/")
        path = src_root / relative
        if not path.is_file():
            continue
        source_text = path.read_text(encoding="utf-8", errors="surrogateescape")
        pattern = rf"\b{re.escape(entry['symbol'])}\s*\([^;{{}}]*\)\s*\{{"
        if re.search(pattern, source_text, re.DOTALL):
            physical[entry["symbol"]].add(entry["destination"])
    errors: list[dict[str, Any]] = []
    for symbol, entry in sorted(wanted.items()):
        paths = sorted(physical[symbol])
        if entry["layout_owner"] == "baseline":
            if paths:
                errors.append({
                    "symbol": symbol,
                    "expected": "absent from src",
                    "actual": paths,
                })
        elif paths != [entry["destination"]]:
            errors.append({
                "symbol": symbol,
                "expected": entry["destination"],
                "actual": paths,
            })
        elif (
            not entry["source_file"].endswith(".h")
            and entry["layout_owner"]
            in {"svistok-new", "svistok-inseparable"}
            and physical_hashes[symbol] != {entry["legacy_sha256"]}
        ):
            errors.append({
                "symbol": symbol,
                "expected_hash": entry["legacy_sha256"],
                "actual_hashes": sorted(physical_hashes[symbol]),
            })
    return {
        "schema_version": 1,
        "checked": len(wanted),
        "errors": errors,
        "ok": not errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--src-root", type=Path, default=SRC_ROOT)
    arguments = parser.parse_args()
    layout = json.loads(
        (PROJECT_ROOT / "manifests" / "function-layout.json").read_text()
    )
    ownership = json.loads(
        (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text()
    )
    report = verify(layout, ownership, src_root=arguments.src_root.resolve())
    if arguments.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    elif report["ok"]:
        print(f'function layout passed: {report["checked"]} functions')
    else:
        for error in report["errors"]:
            print(
                f'{error["symbol"]}: expected {error["expected"]}, '
                f'actual {error["actual"]}', file=sys.stderr
            )
    return 0 if report["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
