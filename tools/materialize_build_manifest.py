#!/usr/bin/env python3
"""Refresh Svistok AST ranges from adapted copies without changing ownership."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
from pathlib import Path
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"


def load_clang_manifest():
    path = PROJECT_ROOT / "tools" / "clang_manifest.py"
    spec = importlib.util.spec_from_file_location("clang_manifest", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def refresh_record(record: dict, definitions: dict) -> dict:
    by_name = {entry["symbol"]: entry for entry in definitions.values()}
    refreshed = copy.deepcopy(record)
    for symbol in refreshed["symbols"]:
        original = symbol.get("legacy")
        if original is None:
            continue
        current = by_name.get(symbol["symbol"])
        if current is None:
            raise RuntimeError(
                f'{record["legacy_file"]}: copied definition disappeared: {symbol["symbol"]}'
            )
        if current["source_sha256"] != original["source_sha256"]:
            raise RuntimeError(
                f'{record["legacy_file"]}: copied body changed: {symbol["symbol"]}'
            )
        symbol["legacy"] = current
    return refreshed


def materialize(ownership: dict) -> dict:
    clang_manifest = load_clang_manifest()
    output = copy.deepcopy(ownership)
    for index, record in enumerate(output["files"]):
        relative = record["legacy_file"]
        if relative not in clang_manifest.MODIFIED_ROOTS:
            continue
        source = SRC_ROOT / relative
        ast = clang_manifest.dump_ast(
            source,
            (
                "-include",
                str(SRC_ROOT / "svistok_abi.h"),
            ),
        )
        grouped = clang_manifest.definitions_by_provenance(ast, source, SRC_ROOT)
        refreshed = refresh_record(record, grouped.get(relative, {}))
        refreshed["declarations"] = clang_manifest.annotate_declarations(
            {
                "legacy": clang_manifest.declarations_by_provenance(
                    ast, source, SRC_ROOT
                ).get(relative, []),
                "baseline": refreshed.get("declarations", {}).get("baseline", []),
            },
            refreshed["symbols"],
        )
        output["files"][index] = refreshed
    output["generator"] = "clang-json-ast/materialized-src-ranges"
    return output


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ownership", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        ownership = json.loads(arguments.ownership.read_text(encoding="utf-8"))
        rendered = json.dumps(materialize(ownership), indent=2, sort_keys=True) + "\n"
        arguments.output.parent.mkdir(parents=True, exist_ok=True)
        arguments.output.write_text(rendered, encoding="utf-8")
    except (OSError, RuntimeError, KeyError) as error:
        print(f"build manifest materialization failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
