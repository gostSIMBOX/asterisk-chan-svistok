#!/usr/bin/env python3
"""Reject upstream-owned definitions physically present in the Svistok overlay."""

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
OWNERSHIP = PROJECT_ROOT / "manifests" / "symbol-ownership.json"
BASELINE_ROOT = PROJECT_ROOT / "asterisk-chan-dongle"
HEADER_MARKER = re.compile(
    rb"/\* SVISTOK_BASELINE_UNIT ([A-Za-z_]+) ([A-Za-z0-9_]+) \*/\r?\n"
)


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def ownership_errors(
    relative: str,
    manifest_symbols: list[dict[str, Any]],
    local_definitions: dict[tuple[str, str], dict[str, Any]],
    proxy_symbols: set[str] | None = None,
) -> list[dict[str, Any]]:
    """Compare physically local definitions with their declared owner."""
    errors: list[dict[str, Any]] = []
    expected = {
        (entry["kind"], entry["symbol"]): entry for entry in manifest_symbols
    }
    current = {
        (record["kind"], record["symbol"]): record
        for record in local_definitions.values()
    }
    for key, record in sorted(current.items()):
        entry = expected.get(key)
        if entry is None:
            errors.append({
                "file": relative,
                "kind": record["kind"],
                "symbol": record["symbol"],
                "reason": "unclassified local definition",
                "range": record["definition_range"],
            })
        elif entry["owner"] == "upstream":
            errors.append({
                "file": relative,
                "kind": record["kind"],
                "symbol": record["symbol"],
                "reason": "upstream-equivalent definition is physically present in src",
                "range": record["definition_range"],
                "baseline_sha256": entry["baseline"]["source_sha256"],
            })
        elif (
            entry["owner"] == "svistok"
            and entry.get("legacy") is not None
            and record["symbol"] not in (proxy_symbols or set())
        ):
            expected_hash = entry["legacy"]["source_sha256"]
            if record.get("source_sha256") != expected_hash:
                errors.append({
                    "file": relative,
                    "kind": record["kind"],
                    "symbol": record["symbol"],
                    "reason": "Svistok-owned body differs from legacy provenance",
                    "range": record["definition_range"],
                    "legacy_sha256": expected_hash,
                    "actual_sha256": record.get("source_sha256"),
                })
    for key, entry in sorted(expected.items()):
        if entry["owner"] != "svistok" or entry.get("legacy") is None:
            continue
        if key not in current:
            errors.append({
                "file": relative,
                "kind": entry["kind"],
                "symbol": entry["symbol"],
                "reason": "Svistok-owned definition is missing from src",
            })
    return errors


def verify_tree(
    ownership: dict[str, Any],
    *,
    src_root: Path = SRC_ROOT,
) -> dict[str, Any]:
    clang_manifest = load_tool("clang_manifest")
    generated_root = PROJECT_ROOT / "build" / "overlay-purity-generated"
    summary = load_tool("generate_all_slices").generate(
        ownership, generated_root, src_root=src_root
    )
    compositions = {
        item["file"]: Path(item["source"])
        for item in summary["generated_slices"]
        if item.get("kind") == "overlay-composition"
    }
    materialized = json.loads(
        (generated_root / "materialized-manifest.json").read_text(encoding="utf-8")
    )
    records = {record["legacy_file"]: record for record in materialized["files"]}
    layout = json.loads(
        (PROJECT_ROOT / "manifests/function-layout.json").read_text(encoding="utf-8")
    )
    proxy_by_file: dict[str, set[str]] = {}
    for entry in layout["functions"]:
        if entry["layout_owner"] == "dongle-proxy":
            proxy_by_file.setdefault(entry["source_file"], set()).add(entry["symbol"])
    errors: list[dict[str, Any]] = []
    checked = 0
    for relative in clang_manifest.MODIFIED_ROOTS:
        source = compositions.get(relative)
        if source is None:
            local = {}
            errors.extend(
                ownership_errors(
                    relative,
                    records[relative]["symbols"],
                    local,
                    proxy_by_file.get(relative, set()),
                )
            )
            continue
        ast = clang_manifest.dump_ast(
            source,
            ("-I", str(src_root)),
        )
        grouped = clang_manifest.definitions_by_provenance(ast, source, src_root)
        local = {}
        for physical in (
            relative,
            f"svistok/{relative}",
            f"dongle/{relative}",
        ):
            local.update(grouped.get(physical, {}))
        checked += len(local)
        errors.extend(
            ownership_errors(
                relative,
                records[relative]["symbols"],
                local,
                proxy_by_file.get(relative, set()),
            )
        )

    checked_header_markers = 0
    compose_headers = load_tool("compose_headers")
    extraction = json.loads(
        (PROJECT_ROOT / "manifests" / "overlay-extraction.json").read_text(
            encoding="utf-8"
        )
    )
    extracted_by_file = {
        entry["file"]: {
            (unit["kind"], unit["symbol"]) for unit in entry["removed"]
        }
        for entry in extraction["files"]
    }
    for relative in clang_manifest.MODIFIED_HEADERS:
        record = records[relative]
        baseline_source = (BASELINE_ROOT / relative).read_bytes()
        overlay_source = (src_root / relative).read_bytes()
        baseline_ranges = compose_headers.baseline_ranges(record, baseline_source)
        expected = extracted_by_file.get(relative, set())
        actual_entries = [
            (match.group(1).decode("ascii"), match.group(2).decode("ascii"))
            for match in HEADER_MARKER.finditer(overlay_source)
        ]
        actual = set(actual_entries)
        checked_header_markers += len(actual_entries)
        if len(actual_entries) != len(actual):
            errors.append({
                "file": relative,
                "kind": "header-marker",
                "symbol": "*",
                "reason": "duplicate baseline-unit marker in src header",
            })
        for kind, symbol in sorted(expected - actual):
            errors.append({
                "file": relative,
                "kind": kind,
                "symbol": symbol,
                "reason": "baseline-owned header unit marker is missing from src",
            })
        for kind, symbol in sorted(actual - expected):
            errors.append({
                "file": relative,
                "kind": kind,
                "symbol": symbol,
                "reason": "unexpected baseline-unit marker in src header",
            })
        for (kind, symbol), (begin, end) in sorted(baseline_ranges.items()):
            snippet = baseline_source[begin:end]
            if snippet.strip() and snippet in overlay_source:
                errors.append({
                    "file": relative,
                    "kind": kind,
                    "symbol": symbol,
                    "reason": "baseline-owned header unit text is physically present in src",
                })
    return {
        "schema_version": 1,
        "checked_local_definitions": checked,
        "checked_header_baseline_markers": checked_header_markers,
        "errors": errors,
        "ok": not errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ownership", type=Path, default=OWNERSHIP)
    parser.add_argument("--src-root", type=Path, default=SRC_ROOT)
    parser.add_argument("--json", action="store_true")
    arguments = parser.parse_args()
    try:
        ownership = json.loads(arguments.ownership.read_text(encoding="utf-8"))
        report = verify_tree(ownership, src_root=arguments.src_root.resolve())
    except (OSError, RuntimeError, KeyError, json.JSONDecodeError) as error:
        print(f"overlay purity verification failed: {error}", file=sys.stderr)
        return 2
    if arguments.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        for error in report["errors"]:
            location = error["file"]
            if "range" in error:
                location += f':{error["range"]["begin"]}'
            print(
                f'{location}: {error["kind"]} {error["symbol"]}: '
                f'{error["reason"]}',
                file=sys.stderr,
            )
        if report["ok"]:
            print(
                f'overlay purity passed: {report["checked_local_definitions"]} '
                "local definitions and "
                f'{report["checked_header_baseline_markers"]} baseline header markers'
            )
    return 0 if report["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
