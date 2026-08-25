#!/usr/bin/env python3
"""Generate build-time bridges for cross-slice static functions and data."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import sys
from typing import Any


PREFIX = "svistok_bridge_"


def bridge_name(owner: str, symbol: str) -> str:
    return f"{PREFIX}{owner}_{symbol}"


def function_declaration(name: str, signature: dict[str, Any]) -> str:
    parameters = signature["parameters"]
    declarations = [f'{item["type"]} {item["name"]}' for item in parameters]
    if signature.get("variadic"):
        declarations.append("...")
    if not declarations:
        declarations.append("void")
    return f'{signature["return_type"]} {name}({", ".join(declarations)})'


def function_wrapper(symbol: str, name: str, signature: dict[str, Any]) -> str:
    if signature.get("variadic"):
        return (
            "#if defined(__APPLE__)\n"
            f'static __typeof__(&{symbol}) const {name}_keep '
            f'__attribute__((used)) = &{symbol};\n'
            f'__asm__(".globl _{name}\\n.private_extern _{name}\\n_{name} = _{symbol}");\n'
            "#else\n"
            f'extern __typeof__({symbol}) {name} '
            f'__attribute__((alias("{symbol}"), visibility("hidden")));\n'
            "#endif\n"
        )
    declaration = function_declaration(name, signature)
    arguments = ", ".join(item["name"] for item in signature["parameters"])
    call = f"{symbol}({arguments})"
    statement = f"{call};" if signature["return_type"] == "void" else f"return {call};"
    return (
        f'__attribute__((visibility("hidden"))) {declaration}\n'
        f"{{\n    {statement}\n}}\n"
    )


def data_declaration(name: str, data_type: str) -> str:
    array = re.fullmatch(r"(.+?)(\[[^]]+\](?:\[[^]]+\])*)", data_type)
    if array:
        return f"{array.group(1).rstrip()} {name}{array.group(2)}"
    return f"{data_type} {name}"


def symbol_alias(symbol: str, name: str) -> str:
    return (
        "#if defined(__APPLE__)\n"
        f'static __typeof__(&{symbol}) const {name}_keep '
        f'__attribute__((used)) = &{symbol};\n'
        f'__asm__(".globl _{name}\\n.private_extern _{name}\\n_{name} = _{symbol}");\n'
        "#else\n"
        f'extern __typeof__({symbol}) {name} '
        f'__attribute__((alias("{symbol}"), visibility("hidden")));\n'
        "#endif\n"
    )


def generate_for_side(
    symbols: list[dict[str, Any]], bridges: list[dict[str, Any]], side: str
) -> tuple[str, str]:
    """Return (consumer prefix, producer suffix) for one slice side."""
    prefix: list[str] = []
    suffix: list[str] = []
    by_name = {entry["symbol"]: entry for entry in symbols}
    for bridge in bridges:
        symbol = bridge["symbol"]
        owner = bridge["owner"]
        consumers = bridge["consumers"]
        entry = by_name[symbol]
        record = entry["baseline" if owner == "upstream" else "legacy"]
        if record is None:
            raise RuntimeError(f"bridge owner has no definition: {symbol}")
        name = bridge_name(owner, symbol)
        if entry["kind"] == "function":
            declaration = function_declaration(name, record["signature"])
            if side in consumers:
                prefix.append(f"#define {symbol} {name}")
            if side == owner:
                suffix.append(function_wrapper(symbol, name, record["signature"]))
        else:
            data_type = record["signature"]["type"]
            declaration = data_declaration(name, data_type)
            if side in consumers:
                prefix.append(f"#define {symbol} {name}")
            if side == owner:
                suffix.append(symbol_alias(symbol, name))
    prefix_text = "\n".join(prefix)
    suffix_text = "\n".join(suffix)
    return (
        (prefix_text + "\n") if prefix_text else "",
        ("\n" + suffix_text) if suffix_text else "",
    )


def generate_composition_for_side(
    symbols: list[dict[str, Any]], bridges: list[dict[str, Any]], side: str
) -> tuple[str, str]:
    """Return declarations/remaps and producer wrappers for an unsliced overlay."""
    prefix: list[str] = []
    by_name = {entry["symbol"]: entry for entry in symbols}
    for bridge in bridges:
        if side not in bridge["consumers"]:
            continue
        entry = by_name[bridge["symbol"]]
        owner_key = "baseline" if bridge["owner"] == "upstream" else "legacy"
        record = entry[owner_key]
        if record is None:
            raise RuntimeError(f'bridge owner has no definition: {bridge["symbol"]}')
        name = bridge_name(bridge["owner"], bridge["symbol"])
        if entry["kind"] == "function":
            declaration = function_declaration(name, record["signature"])
        else:
            declaration = data_declaration(name, record["signature"]["type"])
        prefix.append(f"extern {declaration};")
        prefix.append(f'#define {bridge["symbol"]} {name}')
    _, suffix = generate_for_side(symbols, bridges, side)
    return (("\n".join(prefix) + "\n") if prefix else "", suffix)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--file", required=True)
    parser.add_argument("--side", choices=("upstream", "svistok"), required=True)
    parser.add_argument("--prefix-output", type=Path, required=True)
    parser.add_argument("--suffix-output", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        manifest = json.loads(arguments.manifest.read_text(encoding="utf-8"))
        record = next(
            item for item in manifest["files"] if item["legacy_file"] == arguments.file
        )
        prefix, suffix = generate_for_side(
            record["symbols"], record.get("bridges", []), arguments.side
        )
        arguments.prefix_output.write_text(prefix, encoding="utf-8")
        arguments.suffix_output.write_text(suffix, encoding="utf-8")
    except (OSError, ValueError, RuntimeError, StopIteration, KeyError) as error:
        print(f"bridge generation failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
