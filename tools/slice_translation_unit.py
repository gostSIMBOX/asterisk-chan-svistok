#!/usr/bin/env python3
"""Emit a C translation-unit slice using only Clang-provided source ranges."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
from typing import Any


def expand_to_line_start(source: bytes, offset: int) -> int:
    line_start = source.rfind(b"\n", 0, offset) + 1
    prefix = source[line_start:offset]
    if prefix.strip() and not all(
        character == 95 or 65 <= character <= 90 or 97 <= character <= 122
        or 48 <= character <= 57 or character in b" \t"
        for character in prefix
    ):
        return offset
    return line_start


def blank_range(buffer: bytearray, begin: int, end: int) -> None:
    for index in range(begin, end):
        if buffer[index] not in (10, 13):
            buffer[index] = 32


def blank_definition_range(
    buffer: bytearray, source: bytes, begin: int, end: int
) -> None:
    cursor = begin
    while cursor < end:
        line_start = source.rfind(b"\n", 0, cursor) + 1
        line_end = source.find(b"\n", cursor, end)
        if line_end < 0:
            line_end = end
        else:
            line_end += 1
        if not source[line_start:line_end].lstrip().startswith(b"#"):
            blank_range(buffer, cursor, min(line_end, end))
        cursor = line_end


def blank_direct_c_includes(
    buffer: bytearray, source: bytes, included_sources: set[str]
) -> None:
    offset = 0
    for line in source.splitlines(keepends=True):
        stripped = line.strip()
        matched = next(
            (
                path
                for path in included_sources
                if stripped == f'#include "{path}"'.encode("utf-8")
            ),
            None,
        )
        if matched is not None:
            blank_range(buffer, offset, offset + len(line))
        offset += len(line)


def data_declaration(name: str, data_type: str) -> str:
    array = re.fullmatch(r"(.+?)(\[[^]]+\](?:\[[^]]+\])*)", data_type)
    if array:
        return f"{array.group(1).rstrip()} {name}{array.group(2)}"
    return f"{data_type} {name}"


def function_declaration(name: str, signature: dict[str, Any]) -> str:
    parameters = [
        f'{parameter["type"]} {parameter["name"]}'
        for parameter in signature["parameters"]
    ]
    if signature.get("variadic"):
        parameters.append("...")
    if not parameters:
        parameters.append("void")
    return f'{signature["return_type"]} {name}({", ".join(parameters)})'


def data_bridge_declaration(
    source: bytes,
    consumer_record: dict[str, Any],
    bridge_symbol: str,
    owner_record: dict[str, Any],
) -> bytes:
    source_range = consumer_record["definition_range"]
    begin = int(source_range["begin"])
    name_offset = consumer_record.get("name_offset")
    if name_offset is not None:
        prefix = source[begin:int(name_offset)]
        if b"{" in prefix:
            prefix = re.sub(rb"\bstatic\b", b"extern", prefix, count=1)
            array = re.fullmatch(
                r"(.+?)(\[[^]]+\](?:\[[^]]+\])*)",
                owner_record["signature"]["type"],
            )
            suffix = array.group(2) if array else ""
            return prefix + f"{bridge_symbol}{suffix};\n".encode("utf-8")
    rendered = data_declaration(bridge_symbol, owner_record["signature"]["type"])
    return f"extern {rendered};\n".encode("utf-8")


def emit_slice(
    source: bytes,
    symbols: list[dict[str, Any]],
    *,
    side: str,
    display_path: str,
    included_sources: list[str] | None = None,
    declarations: dict[str, list[dict[str, Any]]] | None = None,
    bridges: list[dict[str, Any]] | None = None,
) -> bytes:
    if side not in {"upstream", "svistok"}:
        raise ValueError(f"unknown slice side: {side}")
    source_key = "baseline" if side == "upstream" else "legacy"
    output = bytearray(source)
    if side == "upstream" and included_sources:
        blank_direct_c_includes(output, source, set(included_sources))
    ranges: list[tuple[int, int, str]] = []
    for symbol in symbols:
        record = symbol.get(source_key)
        if record is None:
            continue
        if symbol["owner"] == side:
            continue
        source_range = record["definition_range"]
        begin = expand_to_line_start(source, int(source_range["begin"]))
        end = int(source_range["end"])
        ranges.append((begin, end, symbol["symbol"]))
    ranges.sort()
    for previous, current in zip(ranges, ranges[1:]):
        if previous[1] > current[0]:
            raise RuntimeError(
                f"overlapping AST ranges: {previous[2]} and {current[2]}"
            )
    for begin, end, _ in ranges:
        blank_definition_range(output, source, begin, end)
    bridge_injections: list[tuple[int, int, bytes]] = []
    if bridges:
        bridge_by_symbol = {
            bridge["symbol"]: bridge
            for bridge in bridges
            if side in bridge.get("consumers", [])
        }
        for symbol in symbols:
            bridge = bridge_by_symbol.get(symbol["symbol"])
            if bridge is None:
                continue
            consumer_record = symbol.get(source_key)
            if consumer_record is None or symbol["owner"] == side:
                continue
            owner_key = "baseline" if bridge["owner"] == "upstream" else "legacy"
            owner_record = symbol[owner_key]
            bridge_symbol = f'svistok_bridge_{bridge["owner"]}_{symbol["symbol"]}'
            matching_declarations = (
                [
                    item
                    for item in (declarations or {}).get(source_key, [])
                    if item["symbol"] == symbol["symbol"]
                ]
                if symbol["kind"] == "function"
                else []
            )
            if matching_declarations:
                continue
            if symbol["kind"] == "function":
                rendered = function_declaration(bridge_symbol, owner_record["signature"])
                declaration = f"extern {rendered};\n".encode("utf-8")
            else:
                declaration = data_bridge_declaration(
                    source, consumer_record, bridge_symbol, owner_record
                )
            source_range = consumer_record["definition_range"]
            begin = expand_to_line_start(source, int(source_range["begin"]))
            end = int(source_range["end"])
            bridge_injections.append((begin, end, declaration))
    if declarations and bridges:
        bridged = {
            bridge["symbol"]
            for bridge in bridges
            if side in bridge.get("consumers", [])
        }
        source_key = "baseline" if side == "upstream" else "legacy"
        by_symbol = {symbol["symbol"]: symbol for symbol in symbols}
        bridge_by_symbol = {bridge["symbol"]: bridge for bridge in bridges}
        for source_declaration in declarations.get(source_key, []):
            if source_declaration["symbol"] not in bridged:
                continue
            source_range = source_declaration["declaration_range"]
            begin = expand_to_line_start(source, int(source_range["begin"]))
            end = int(source_range["end"])
            blank_definition_range(
                output,
                source,
                begin,
                end,
            )
            bridge = bridge_by_symbol[source_declaration["symbol"]]
            symbol = by_symbol[source_declaration["symbol"]]
            owner_key = "baseline" if bridge["owner"] == "upstream" else "legacy"
            owner_record = symbol[owner_key]
            bridge_symbol = (
                f'svistok_bridge_{bridge["owner"]}_{source_declaration["symbol"]}'
            )
            rendered = function_declaration(bridge_symbol, owner_record["signature"])
            bridge_injections.append((begin, end, f"extern {rendered};\n".encode("utf-8")))
    for begin, end, declaration in sorted(bridge_injections, reverse=True):
        if len(declaration) < end - begin:
            declaration += bytes(output[begin + len(declaration):end])
        output[begin:end] = declaration
    directive = f'#line 1 "{display_path}"\n'.encode("utf-8")
    return directive + bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--file", required=True)
    parser.add_argument("--side", choices=("upstream", "svistok"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        manifest = json.loads(arguments.manifest.read_text(encoding="utf-8"))
        record = next(
            item for item in manifest["files"] if item["legacy_file"] == arguments.file
        )
        rendered = emit_slice(
            arguments.source.read_bytes(),
            record["symbols"],
            side=arguments.side,
            display_path=arguments.file,
            included_sources=[
                item["legacy_file"] for item in record.get("included_sources", [])
            ],
            declarations=record.get("declarations"),
            bridges=record.get("bridges"),
        )
        arguments.output.write_bytes(rendered)
    except (OSError, ValueError, RuntimeError, StopIteration, KeyError) as error:
        print(f"slicing failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
