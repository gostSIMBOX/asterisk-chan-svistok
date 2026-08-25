#!/usr/bin/env python3
"""Shared source-range helpers for physical and composed Svistok headers."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Any


DEFINE = re.compile(rb"^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z0-9_]*)", re.MULTILINE)


def line_range(source: bytes, begin: int, end: int) -> tuple[int, int]:
    start = source.rfind(b"\n", 0, begin) + 1
    finish = source.find(b"\n", end)
    return start, len(source) if finish < 0 else finish + 1


def macro_ranges(source: bytes) -> dict[str, tuple[int, int]]:
    result: dict[str, tuple[int, int]] = {}
    for match in DEFINE.finditer(source):
        begin = source.rfind(b"\n", 0, match.start()) + 1
        end = source.find(b"\n", match.end())
        end = len(source) if end < 0 else end + 1
        while source[begin:end].rstrip(b"\r\n").endswith(b"\\") and end < len(source):
            next_end = source.find(b"\n", end)
            end = len(source) if next_end < 0 else next_end + 1
        result[match.group(1).decode("ascii")] = (begin, end)
    return result


def guard_macro(record: dict[str, Any]) -> str | None:
    candidates = [
        macro["symbol"]
        for macro in record.get("macros", [])
        if macro["owner"] == "upstream"
        and (
            macro["symbol"].endswith("_INCLUDED")
            or macro["symbol"].startswith("____")
        )
    ]
    return candidates[0] if candidates else None


def current_header_record(
    original: dict[str, Any],
    relative: str,
    src_root: Path,
    clang_manifest: Any,
) -> dict[str, Any]:
    definitions = clang_manifest.definitions_for(src_root, relative)
    definition_by_key = {
        (record["kind"], record["symbol"]): record
        for record in definitions.values()
    }
    symbols: list[dict[str, Any]] = []
    for symbol in original["symbols"]:
        copied = dict(symbol)
        copied["legacy"] = definition_by_key.get((symbol["kind"], symbol["symbol"]))
        symbols.append(copied)

    declaration_owners = {
        item["symbol"]: item
        for item in original.get("declarations", {}).get("legacy", [])
    }
    declarations: list[dict[str, Any]] = []
    for current in clang_manifest.declarations_for(src_root, relative):
        copied = dict(current)
        known = declaration_owners.get(current["symbol"])
        copied.update(
            {
                "owner": known["owner"] if known else "svistok",
                "body_relation": known["body_relation"] if known else "adapter",
            }
        )
        declarations.append(copied)

    current_types = clang_manifest.type_units_for(src_root, relative)
    type_by_key = {
        (record["ast_kind"], record["symbol"]): record
        for record in current_types.values()
    }
    types: list[dict[str, Any]] = []
    for unit in original.get("types", []):
        copied = dict(unit)
        copied["legacy"] = type_by_key.get((unit["ast_kind"], unit["symbol"]))
        types.append(copied)

    result = dict(original)
    result["symbols"] = symbols
    result["types"] = types
    result["declarations"] = {
        "legacy": declarations,
        "baseline": original.get("declarations", {}).get("baseline", []),
    }
    return result


def owned_ranges(record: dict[str, Any], source: bytes) -> list[dict[str, Any]]:
    ranges: list[dict[str, Any]] = []
    for collection, range_key in (("symbols", "definition_range"), ("types", "definition_range")):
        for unit in record.get(collection, []):
            current = unit.get("legacy")
            if unit["owner"] != "upstream" or current is None:
                continue
            source_range = current[range_key]
            begin, end = line_range(source, int(source_range["begin"]), int(source_range["end"]))
            ranges.append({
                "begin": begin,
                "end": end,
                "kind": unit["kind"],
                "symbol": unit["symbol"],
                "preprocessor": False,
            })
    for unit in record.get("declarations", {}).get("legacy", []):
        if unit.get("owner") != "upstream":
            continue
        source_range = unit["declaration_range"]
        begin, end = line_range(source, int(source_range["begin"]), int(source_range["end"]))
        ranges.append({
            "begin": begin,
            "end": end,
            "kind": "declaration",
            "symbol": unit["symbol"],
            "preprocessor": False,
        })
    macros = macro_ranges(source)
    guard = guard_macro(record)
    for unit in record.get("macros", []):
        if unit["owner"] != "upstream" or unit["symbol"] == guard:
            continue
        source_range = macros.get(unit["symbol"])
        if source_range is None:
            continue
        ranges.append({
            "begin": source_range[0],
            "end": source_range[1],
            "kind": "macro",
            "symbol": unit["symbol"],
            "preprocessor": True,
        })
    return ranges


def non_overlapping(ranges: list[dict[str, Any]]) -> list[dict[str, Any]]:
    """Drop ranges wholly covered by a larger owned atomic source unit."""
    ordered = sorted(ranges, key=lambda item: (item["begin"], -item["end"]))
    result: list[dict[str, Any]] = []
    for item in ordered:
        if any(
            existing["begin"] <= item["begin"] and item["end"] <= existing["end"]
            for existing in result
        ):
            continue
        result.append(item)
    return result
