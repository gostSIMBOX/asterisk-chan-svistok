#!/usr/bin/env python3
"""Physically extract Svistok-owned definitions into a staged source tree."""

from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import sys
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"
OWNERSHIP = PROJECT_ROOT / "manifests" / "symbol-ownership.json"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def owned_ranges(record: dict[str, Any]) -> list[tuple[int, int, str, str]]:
    ranges: list[tuple[int, int, str, str]] = []
    for symbol in record["symbols"]:
        current = symbol.get("legacy")
        if current is None or symbol["owner"] != "upstream":
            continue
        source_range = current["definition_range"]
        ranges.append((
            int(source_range["begin"]),
            int(source_range["end"]),
            symbol["kind"],
            symbol["symbol"],
        ))
    for declaration in record.get("declarations", {}).get("legacy", []):
        if declaration.get("owner") != "upstream":
            continue
        source_range = declaration["declaration_range"]
        ranges.append((
            int(source_range["begin"]),
            int(source_range["end"]),
            "declaration",
            declaration["symbol"],
        ))
    return ranges


def blank_owned_ranges(
    source: bytes,
    ranges: list[tuple[int, int, str, str]],
) -> tuple[bytes, list[dict[str, Any]]]:
    slicer = load_tool("slice_translation_unit")
    output = bytearray(source)
    normalized: list[tuple[int, int, str, str]] = []
    for begin, end, kind, symbol in ranges:
        normalized.append((slicer.expand_to_line_start(source, begin), end, kind, symbol))
    normalized.sort()
    for previous, current in zip(normalized, normalized[1:]):
        if previous[1] > current[0]:
            raise RuntimeError(
                f"overlapping extraction ranges: {previous[2]} {previous[3]} "
                f"and {current[2]} {current[3]}"
            )
    removed: list[dict[str, Any]] = []
    for begin, end, kind, symbol in normalized:
        slicer.blank_definition_range(output, source, begin, end)
        removed.append({
            "kind": kind,
            "symbol": symbol,
            "range": {"begin": begin, "end": end},
        })
    return bytes(output), removed


def extract_root_source(
    source: bytes,
    record: dict[str, Any],
    relative: str,
) -> tuple[bytes, list[dict[str, Any]]]:
    slicer = load_tool("slice_translation_unit")
    rendered = slicer.emit_slice(
        source,
        record["symbols"],
        side="svistok",
        display_path=relative,
        included_sources=[
            item["legacy_file"] for item in record.get("included_sources", [])
        ],
        declarations=record.get("declarations"),
        bridges=record.get("bridges"),
    )
    rendered = rendered.split(b"\n", 1)[1]
    declaration_ranges = []
    for declaration in record.get("declarations", {}).get("legacy", []):
        if declaration.get("owner") != "upstream":
            continue
        source_range = declaration["declaration_range"]
        declaration_ranges.append((
            int(source_range["begin"]),
            int(source_range["end"]),
            "declaration",
            declaration["symbol"],
        ))
    rendered, removed_declarations = blank_owned_ranges(
        rendered, declaration_ranges
    )
    removed_definitions = [
        {
            "kind": symbol["kind"],
            "symbol": symbol["symbol"],
            "range": symbol["legacy"]["definition_range"],
        }
        for symbol in record["symbols"]
        if symbol["owner"] == "upstream" and symbol.get("legacy") is not None
    ]
    return rendered, removed_definitions + removed_declarations


def inject_composed_header_wrapper(
    source: bytes,
    relative: str,
    guard: str,
) -> bytes:
    macro = (
        "SVISTOK_COMPOSED_"
        + relative.upper().replace(".", "_").replace("/", "_")
        + "_HEADER"
    )
    defines = load_tool("header_units").macro_ranges(source)
    guard_range = defines.get(guard)
    if guard_range is None:
        raise RuntimeError(f"{relative}: cannot locate include guard {guard}")
    prefix = (
        f"#ifdef {macro}\n#include {macro}\n#else\n"
    ).encode("ascii")
    source = source[:guard_range[1]] + prefix + source[guard_range[1]:]
    marker = source.rfind(b"#endif")
    if marker < 0:
        raise RuntimeError(f"{relative}: header has no final #endif")
    suffix = f"#endif /* {macro} */\n".encode("ascii")
    return source[:marker] + suffix + source[marker:]


def current_header_record(
    original: dict[str, Any],
    relative: str,
    clang_manifest: Any,
) -> dict[str, Any]:
    definitions = clang_manifest.definitions_for(SRC_ROOT, relative)
    by_key = {
        (record["kind"], record["symbol"]): record
        for record in definitions.values()
    }
    symbols: list[dict[str, Any]] = []
    for symbol in original["symbols"]:
        copied = dict(symbol)
        current = by_key.get((symbol["kind"], symbol["symbol"]))
        if symbol.get("legacy") is not None and current is None:
            # Header tentative definitions may have been deliberately relocated
            # to the single target-owned storage translation unit. Their local
            # declaration is collected below and the original header definition
            # must not be recreated during extraction.
            copied["legacy"] = None
            symbols.append(copied)
            continue
        copied["legacy"] = current
        symbols.append(copied)
    original_declarations = {
        item["symbol"]: item
        for item in original.get("declarations", {}).get("legacy", [])
    }
    declarations: list[dict[str, Any]] = []
    for current in clang_manifest.declarations_for(SRC_ROOT, relative):
        copied = dict(current)
        known = original_declarations.get(current["symbol"])
        if known is None:
            copied.update({"owner": "svistok", "body_relation": "adapter"})
        else:
            copied.update({
                "owner": known["owner"],
                "body_relation": known["body_relation"],
            })
        declarations.append(copied)
    copied_record = dict(original)
    copied_record["symbols"] = symbols
    copied_record["declarations"] = {
        "legacy": declarations,
        "baseline": original.get("declarations", {}).get("baseline", []),
    }
    return copied_record


def extract(
    ownership: dict[str, Any],
    output_root: Path,
) -> dict[str, Any]:
    clang_manifest = load_tool("clang_manifest")
    materializer = load_tool("materialize_build_manifest")
    materialized = materializer.materialize(ownership)
    records = {record["legacy_file"]: record for record in materialized["files"]}
    original_records = {
        record["legacy_file"]: record for record in ownership["files"]
    }
    output_root.mkdir(parents=True, exist_ok=True)
    modified = set(clang_manifest.MODIFIED_ROOTS) | set(clang_manifest.MODIFIED_HEADERS)
    for source_path in SRC_ROOT.rglob("*"):
        if not source_path.is_file():
            continue
        relative_path = source_path.relative_to(SRC_ROOT)
        if relative_path.as_posix() in modified:
            continue
        destination = output_root / relative_path
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_path, destination)
    files: list[dict[str, Any]] = []
    for relative in (*clang_manifest.MODIFIED_ROOTS, *clang_manifest.MODIFIED_HEADERS):
        source_path = SRC_ROOT / relative
        record = records[relative]
        if relative in clang_manifest.MODIFIED_HEADERS:
            header_units = load_tool("header_units")
            record = header_units.current_header_record(
                original_records[relative], relative, SRC_ROOT, clang_manifest
            )
            source = source_path.read_bytes()
            ranges = header_units.non_overlapping(
                header_units.owned_ranges(record, source)
            )
            output = bytearray(source)
            for item in sorted(ranges, key=lambda value: value["begin"], reverse=True):
                marker = (
                    f'/* SVISTOK_BASELINE_UNIT {item["kind"]} {item["symbol"]} */\n'
                ).encode("ascii")
                output[item["begin"]:item["end"]] = marker
            rendered = bytes(output)
            guard = header_units.guard_macro(record)
            if guard is None:
                raise RuntimeError(f"{relative}: no include guard ownership")
            rendered = inject_composed_header_wrapper(
                rendered, relative, guard
            )
            removed = [
                {
                    "kind": item["kind"],
                    "symbol": item["symbol"],
                    "range": {"begin": item["begin"], "end": item["end"]},
                }
                for item in ranges
            ]
        else:
            rendered, removed = extract_root_source(
                source_path.read_bytes(), record, relative
            )
        output = output_root / relative
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(rendered)
        files.append({
            "file": relative,
            "removed": removed,
            "removed_count": len(removed),
        })
    return {
        "schema_version": 1,
        "source_root": str(SRC_ROOT),
        "output_root": str(output_root),
        "files": files,
        "removed_units": sum(item["removed_count"] for item in files),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ownership", type=Path, default=OWNERSHIP)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    arguments = parser.parse_args()
    try:
        ownership = json.loads(arguments.ownership.read_text(encoding="utf-8"))
        report = extract(ownership, arguments.output_root.resolve())
        rendered = json.dumps(report, indent=2, sort_keys=True) + "\n"
        if arguments.report:
            arguments.report.parent.mkdir(parents=True, exist_ok=True)
            arguments.report.write_text(rendered, encoding="utf-8")
        print(
            f'extracted {len(report["files"])} files; '
            f'removed {report["removed_units"]} upstream-owned units'
        )
    except (OSError, RuntimeError, KeyError, json.JSONDecodeError) as error:
        print(f"overlay extraction failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
