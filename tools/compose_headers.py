#!/usr/bin/env python3
"""Compose build-only headers from baseline-owned and overlay-owned units."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import sys
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"
BASELINE_ROOT = PROJECT_ROOT / "asterisk-chan-dongle"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def baseline_ranges(record: dict[str, Any], source: bytes) -> dict[tuple[str, str], tuple[int, int]]:
    header_units = load_tool("header_units")
    result: dict[tuple[str, str], tuple[int, int]] = {}
    for collection, range_key in (("symbols", "definition_range"), ("types", "definition_range")):
        for unit in record.get(collection, []):
            baseline = unit.get("baseline")
            if unit["owner"] != "upstream" or baseline is None:
                continue
            source_range = baseline[range_key]
            result[(unit["kind"], unit["symbol"])] = header_units.line_range(
                source, int(source_range["begin"]), int(source_range["end"])
            )
    for unit in record.get("declarations", {}).get("baseline", []):
        if unit.get("owner") != "upstream":
            continue
        source_range = unit["declaration_range"]
        result[("declaration", unit["symbol"])] = header_units.line_range(
            source, int(source_range["begin"]), int(source_range["end"])
        )
    macros = header_units.macro_ranges(source)
    guard = header_units.guard_macro(record)
    for unit in record.get("macros", []):
        if unit["owner"] != "upstream" or unit["symbol"] == guard:
            continue
        if unit["symbol"] in macros:
            result[("macro", unit["symbol"])] = macros[unit["symbol"]]
    return result


MARKER = re.compile(rb"/\* SVISTOK_BASELINE_UNIT ([A-Za-z_]+) ([A-Za-z0-9_]+) \*/\r?\n")


def compose_one(
    original: dict[str, Any],
    relative: str,
    src_root: Path,
    clang_manifest: Any,
) -> tuple[bytes, list[dict[str, Any]]]:
    header_units = load_tool("header_units")
    overlay_source = (src_root / relative).read_bytes()
    baseline_source = (BASELINE_ROOT / relative).read_bytes()
    baseline = baseline_ranges(original, baseline_source)
    replacements: list[tuple[int, int, bytes, dict[str, Any]]] = []
    for match in MARKER.finditer(overlay_source):
        kind = match.group(1).decode("ascii")
        symbol = match.group(2).decode("ascii")
        key = (kind, symbol)
        source_range = baseline.get(key)
        if source_range is None:
            raise RuntimeError(f"{relative}: no baseline range for {key}")
        replacement = baseline_source[source_range[0]:source_range[1]]
        replacements.append((
            match.start(),
            match.end(),
            replacement,
            {
                "kind": kind,
                "symbol": symbol,
                "baseline_range": {"begin": source_range[0], "end": source_range[1]},
                "baseline_sha256": hashlib.sha256(replacement).hexdigest(),
            },
        ))
    output = bytearray(overlay_source)
    for begin, end, replacement, _ in sorted(replacements, reverse=True):
        output[begin:end] = replacement
    composed_macro = (
        "SVISTOK_COMPOSED_"
        + relative.upper().replace(".", "_").replace("/", "_")
        + "_HEADER"
    )
    wrapper_prefix = (
        f"#ifdef {composed_macro}\n#include {composed_macro}\n#else\n"
    ).encode("ascii")
    wrapper_suffix = f"#endif /* {composed_macro} */\n".encode("ascii")
    output = bytearray(bytes(output).replace(wrapper_prefix, b"", 1))
    output = bytearray(bytes(output).replace(wrapper_suffix, b"", 1))
    guard = header_units.guard_macro(original)
    if guard:
        composed_guard = (
            "SVISTOK_COMPOSED_CONTENT_"
            + relative.upper().replace(".", "_").replace("/", "_")
            + "_INCLUDED"
        )
        output = bytearray(bytes(output).replace(guard.encode("ascii"), composed_guard.encode("ascii")))
    return bytes(output), [item[3] for item in replacements]


def compose(
    ownership: dict[str, Any],
    output_root: Path,
    *,
    src_root: Path = SRC_ROOT,
) -> dict[str, Any]:
    clang_manifest = load_tool("clang_manifest")
    records = {record["legacy_file"]: record for record in ownership["files"]}
    files: list[dict[str, Any]] = []
    output_root.mkdir(parents=True, exist_ok=True)
    for relative in clang_manifest.MODIFIED_HEADERS:
        rendered, replacements = compose_one(
            records[relative], relative, src_root, clang_manifest
        )
        output = output_root / relative
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_bytes(rendered)
        files.append({
            "file": relative,
            "output": str(output),
            "baseline_units": replacements,
        })
    return {
        "schema_version": 1,
        "files": files,
        "baseline_unit_count": sum(len(item["baseline_units"]) for item in files),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ownership", type=Path, required=True)
    parser.add_argument("--src-root", type=Path, default=SRC_ROOT)
    parser.add_argument("--output-root", type=Path, required=True)
    parser.add_argument("--report", type=Path)
    arguments = parser.parse_args()
    try:
        ownership = json.loads(arguments.ownership.read_text(encoding="utf-8"))
        report = compose(
            ownership,
            arguments.output_root.resolve(),
            src_root=arguments.src_root.resolve(),
        )
        if arguments.report:
            arguments.report.parent.mkdir(parents=True, exist_ok=True)
            arguments.report.write_text(
                json.dumps(report, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
    except (OSError, RuntimeError, KeyError, json.JSONDecodeError) as error:
        print(f"header composition failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
