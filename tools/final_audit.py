#!/usr/bin/env python3
"""Run the complete available audit sequentially and record its evidence."""

from __future__ import annotations

from datetime import date
import json
from pathlib import Path
import platform
import subprocess
import sys
import time


PROJECT_ROOT = Path(__file__).resolve().parents[1]
OUTPUT = PROJECT_ROOT / "manifests" / "final-audit.json"

CHECKS = (
    ("source guards", ["python3", "tests/test_source_guards.py"]),
    ("52/109 inventory", ["python3", "tests/test_inventory.py"]),
    ("copy receipts/exclusions", ["python3", "tests/test_copy_receipts.py"]),
    ("effective config", ["python3", "tests/test_config.py"]),
    ("include ownership", ["python3", "tests/test_include_ownership.py"]),
    ("canonical ABI", ["python3", "tests/test_abi.py"]),
    ("single-owner state", ["python3", "tests/test_state_relocations.py"]),
    ("symbol manifest", ["python3", "tests/slicer/test_manifest.py"]),
    ("syntax-aware slicing", ["python3", "tests/slicer/test_slicing.py"]),
    ("static bridges", ["python3", "tests/slicer/test_bridges.py"]),
    ("body provenance", ["python3", "tests/slicer/test_provenance.py"]),
    ("overlay purity", ["python3", "tests/slicer/test_overlay_purity.py"]),
    ("delta/baseline composition", ["python3", "tests/slicer/test_materialization.py"]),
    ("overlay extraction", ["python3", "tests/slicer/test_overlay_extraction.py"]),
    ("direct app_register baseline reuse", ["python3", "tests/slicer/test_app_upstream_reuse.py"]),
    ("function layout manifest", ["python3", "tests/layout/test_function_layout_manifest.py"]),
    ("function physical paths", ["python3", "tests/layout/test_function_paths.py"]),
    ("new declaration headers", ["python3", "tests/layout/test_header_layout.py"]),
    ("proxy source contract", ["python3", "tests/layout/test_proxy_contract.py"]),
    ("proxy runtime ordering", ["python3", "tests/layout/test_proxy_runtime.py"]),
    ("at_parse pilot", ["python3", "tests/slicer/test_at_parse_pilot.py"]),
    ("full slice generation", ["python3", "tests/slicer/test_full_generation.py"]),
    ("chan_svistok.so compatibility build", ["python3", "tests/test_module_build.py"]),
    ("legacy harness oracle", ["python3", "tests/characterization/test_harness.py"]),
    ("legacy PDU oracle", ["python3", "tests/characterization/test_pdu_oracle.py"]),
    ("legacy AT parser oracle", ["python3", "tests/characterization/test_at_parse_oracle.py"]),
    ("legacy buffer oracle", ["python3", "tests/characterization/test_buffers_oracle.py"]),
    ("legacy AT read oracle", ["python3", "tests/characterization/test_at_read_oracle.py"]),
    ("legacy programmer oracle", ["python3", "tests/characterization/test_programmer_oracle.py"]),
    ("legacy queue oracle", ["python3", "tests/characterization/test_queue_oracle.py"]),
    ("legacy state oracle", ["python3", "tests/characterization/test_state_oracle.py"]),
    ("golden fixture hashes", ["python3", "tests/characterization/test_golden_manifest.py"]),
    (
        "migrated golden replay",
        ["python3", "tests/migrated/test_golden_replay.py"],
    ),
)


def main() -> int:
    results = []
    for name, command in CHECKS:
        started = time.monotonic()
        completed = subprocess.run(
            command,
            cwd=PROJECT_ROOT,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        results.append({
            "name": name,
            "command": command,
            "status": "passed" if completed.returncode == 0 else "failed",
            "returncode": completed.returncode,
            "duration_seconds": round(time.monotonic() - started, 3),
        })
        if completed.returncode:
            print(f"final audit failed: {name}\n{completed.stderr}", file=sys.stderr)
            return 1

    inventory = json.loads(
        (PROJECT_ROOT / "manifests" / "module-files.json").read_text(encoding="utf-8")
    )
    ownership = json.loads(
        (PROJECT_ROOT / "manifests" / "symbol-ownership.json").read_text(encoding="utf-8")
    )
    build_report = json.loads(
        (PROJECT_ROOT / "build" / "module-test" / "build-report.json").read_text(
            encoding="utf-8"
        )
    )
    gaps = json.loads(
        (PROJECT_ROOT / "manifests" / "effective-legacy-config.json").read_text(
            encoding="utf-8"
        )
    )["explicit_gaps"]
    extraction = json.loads(
        (PROJECT_ROOT / "manifests" / "overlay-extraction.json").read_text(
            encoding="utf-8"
        )
    )
    function_layout = json.loads(
        (PROJECT_ROOT / "manifests/function-layout.json").read_text(encoding="utf-8")
    )
    function_promotion = json.loads(
        (PROJECT_ROOT / "manifests/function-layout-promotion.json").read_text(
            encoding="utf-8"
        )
    )
    report = {
        "schema_version": 1,
        "audited_on": date.today().isoformat(),
        "host": {"system": platform.system(), "machine": platform.machine()},
        "status": "available_checks_passed",
        "checks": results,
        "inventory": inventory["counts"],
        "ownership": {
            "files": len(ownership["files"]),
            "root_symbols": sum(len(item["symbols"]) for item in ownership["files"]),
            "macros": sum(len(item["macros"]) for item in ownership["files"]),
            "types": sum(len(item.get("types", [])) for item in ownership["files"]),
            "declarations": sum(
                len(entries)
                for item in ownership["files"]
                for entries in item["declarations"].values()
            ),
            "bridges": sum(len(item["bridges"]) for item in ownership["files"]),
        },
        "overlay": {
            "modified_paths": len(extraction["files"]),
            "removed_upstream_units": sum(
                item["removed_count"] for item in extraction["files"]
            ),
            "removed_c_units": sum(
                item["removed_count"]
                for item in extraction["files"]
                if item["file"].endswith(".c")
            ),
            "removed_header_units": sum(
                item["removed_count"]
                for item in extraction["files"]
                if item["file"].endswith(".h")
            ),
        },
        "build": build_report,
        "function_layout": {
            "counts": function_layout["counts"],
            "new_files": function_layout["new_files"],
            "proxy_files": function_layout["proxy_files"],
            "removed_root_files": function_promotion["removed"],
        },
        "external_gaps": gaps,
    }
    OUTPUT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"final audit passed: {len(results)} sequential checks")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
