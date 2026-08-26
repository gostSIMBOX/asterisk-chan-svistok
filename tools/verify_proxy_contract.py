#!/usr/bin/env python3
"""Verify approved proxy wrappers and their pristine dongle entry points."""

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
    generated = PROJECT_ROOT / "build" / "proxy-contract-verify"
    summary = generator.generate(ownership, generated, src_root=src_root)
    expected = {
        entry["symbol"]: entry
        for entry in layout["functions"]
        if entry["layout_owner"] == "dongle-proxy"
    }
    ownership_by_file = {
        record["legacy_file"]: record for record in ownership["files"]
    }
    overlay_definitions: dict[str, tuple[dict[str, Any], Path]] = {}
    baseline_definitions: dict[str, dict[str, Any]] = {}
    actual_proxy_symbols: set[str] = set()
    for item in summary["generated_slices"]:
        source = Path(item["source"])
        ast = clang_manifest.dump_ast(
            source,
            (
                "-I", str(src_root),
                "-I", str(PROJECT_ROOT / "asterisk-chan-dongle"),
            ),
        )
        if item["kind"] == "overlay-composition":
            grouped = clang_manifest.definitions_by_provenance(ast, source, src_root)
            relative = f'dongle/{item["file"]}'
            for record in grouped.get(relative, {}).values():
                if record["kind"] != "function":
                    continue
                actual_proxy_symbols.add(record["symbol"])
                overlay_definitions[record["symbol"]] = (
                    record,
                    src_root / relative,
                )
        elif item["kind"] == "baseline-slice":
            grouped = clang_manifest.definitions_by_provenance(
                ast, source, generated
            )
            for definitions in grouped.values():
                for record in definitions.values():
                    baseline_definitions[record["symbol"]] = record

    errors: list[dict[str, Any]] = []
    if actual_proxy_symbols != set(expected):
        errors.append({
            "contract": "proxy-definition-set",
            "expected": sorted(expected),
            "actual": sorted(actual_proxy_symbols),
        })
    for symbol, entry in sorted(expected.items()):
        located = overlay_definitions.get(symbol)
        if located is None:
            continue
        record, source = located
        source_bytes = source.read_bytes()
        body_range = record["body_range"]
        body = source_bytes[int(body_range["begin"]):int(body_range["end"])].decode(
            "latin-1"
        )
        baseline_entry = entry["composition"]["baseline_entry"]
        hook_entry = "svistok_hook_" + entry["composition"]["hook"]
        baseline_calls = len(re.findall(rf"\b{re.escape(baseline_entry)}\s*\(", body))
        self_calls = len(re.findall(rf"\b{re.escape(symbol)}\s*\(", body))
        hook_calls = len(re.findall(rf"\b{re.escape(hook_entry)}\s*\(", body))
        if baseline_calls != 1 or self_calls != 0 or hook_calls < 1:
            errors.append({
                "contract": "proxy-call-shape",
                "symbol": symbol,
                "baseline_calls": baseline_calls,
                "self_calls": self_calls,
                "hook_calls": hook_calls,
            })
        hidden = baseline_definitions.get(baseline_entry)
        owner_unit = next(
            unit
            for unit in ownership_by_file[entry["source_file"]]["symbols"]
            if unit["symbol"] == symbol
        )
        expected_hash = owner_unit["baseline"]["source_sha256"]
        if hidden is None or hidden["source_sha256"] != expected_hash:
            errors.append({
                "contract": "pristine-baseline-body",
                "symbol": symbol,
                "expected": expected_hash,
                "actual": None if hidden is None else hidden["source_sha256"],
            })
    return {
        "schema_version": 1,
        "checked": len(expected),
        "proxy_files": len({entry["destination"] for entry in expected.values()}),
        "errors": errors,
        "ok": not errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--src-root", type=Path, default=SRC_ROOT)
    parser.add_argument("--json", action="store_true")
    arguments = parser.parse_args()
    layout = json.loads((PROJECT_ROOT / "manifests/function-layout.json").read_text())
    ownership = json.loads((PROJECT_ROOT / "manifests/symbol-ownership.json").read_text())
    try:
        report = verify(
            layout, ownership, src_root=arguments.src_root.resolve()
        )
    except (OSError, RuntimeError, KeyError, StopIteration) as error:
        print(f"proxy verification failed: {error}", file=sys.stderr)
        return 1
    if arguments.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    elif report["ok"]:
        print(
            f'proxy contract passed: {report["checked"]} functions in '
            f'{report["proxy_files"]} files'
        )
    else:
        print(json.dumps(report["errors"], indent=2), file=sys.stderr)
    return 0 if report["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
