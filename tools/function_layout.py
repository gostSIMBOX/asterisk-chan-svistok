#!/usr/bin/env python3
"""Build and validate the approved 45/6/14/87 function-layout manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[1]
OWNERSHIP = PROJECT_ROOT / "manifests" / "symbol-ownership.json"
OUTPUT = PROJECT_ROOT / "manifests" / "function-layout.json"
BASELINE_OUTPUT = PROJECT_ROOT / "manifests" / "function-layout-baseline.json"
SRC_ROOT = PROJECT_ROOT / "src"

DIRECT_BASELINE = {
    "chan_dongle.c": {"closetty", "lock_try"},
    "channel.c": {"iov_write"},
    "cpvt.c": {"init_pipe"},
    "pdiscovery.c": {"pdiscovery_do_cmd", "pdiscovery_list_begin"},
}

PROXIES = {
    "at_parse.c": {
        "at_parse_cpin": {"order": "before", "hook": "log_cpin"},
    },
    "at_queue.c": {
        "at_write": {"order": "before", "hook": "log_at_write"},
    },
    "at_response.c": {
        "at_response_cgmi": {"order": "after", "hook": "persist_manufacturer"},
        "at_response_cgmr": {"order": "after", "hook": "persist_firmware"},
        "at_response_cgsn": {"order": "after", "hook": "persist_imei"},
        "at_response_cimi": {"order": "after", "hook": "load_imsi_state"},
        "at_response_cops": {"order": "after", "hook": "persist_provider"},
        "at_response_csq": {"order": "after-success", "hook": "persist_csq"},
        "at_response_mode": {"order": "after", "hook": "persist_mode"},
        "at_response_rssi": {"order": "after-success", "hook": "persist_rssi"},
    },
    "chan_dongle.c": {
        "load_module": {"order": "before", "hook": "initialize_svistok"},
    },
    "cli.c": {
        "getACD": {"order": "result", "hook": "normalize_acd"},
    },
    "pdiscovery.c": {
        "info_copy": {"order": "after", "hook": "copy_serial"},
        "info_free": {"order": "after", "hook": "free_serial"},
    },
}


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def build(ownership: dict[str, Any]) -> dict[str, Any]:
    entries: list[dict[str, Any]] = []
    for record in ownership["files"]:
        relative = record["legacy_file"]
        for unit in record.get("symbols", []):
            if unit.get("kind") != "function":
                continue
            relation = unit.get("body_relation")
            if relation not in {"new", "modified"}:
                continue
            symbol = unit["symbol"]
            if relation == "new":
                layout_owner = "svistok-new"
                destination = f"src/svistok/{relative}"
                composition = None
            elif symbol in DIRECT_BASELINE.get(relative, set()):
                layout_owner = "baseline"
                destination = f"asterisk-chan-dongle/{relative}"
                composition = None
            elif symbol in PROXIES.get(relative, {}):
                layout_owner = "dongle-proxy"
                destination = f"src/dongle/{relative}"
                composition = {
                    **PROXIES[relative][symbol],
                    "baseline_entry": f"svistok_dongle_impl_{symbol}",
                    "hook_file": f"src/svistok/hooks/{relative}",
                }
            else:
                layout_owner = "svistok-inseparable"
                destination = f"src/{relative}"
                composition = None
            entries.append({
                "source_file": relative,
                "symbol": symbol,
                "linkage": unit.get("linkage"),
                "original_relation": relation,
                "layout_owner": layout_owner,
                "destination": destination,
                "composition": composition,
                "legacy_sha256": (
                    unit.get("legacy") or {}
                ).get("source_sha256"),
                "baseline_sha256": (
                    unit.get("baseline") or {}
                ).get("source_sha256"),
            })
    counts = {
        owner: sum(entry["layout_owner"] == owner for entry in entries)
        for owner in (
            "svistok-new", "baseline", "dongle-proxy", "svistok-inseparable"
        )
    }
    expected = {
        "svistok-new": 45,
        "baseline": 6,
        "dongle-proxy": 14,
        "svistok-inseparable": 87,
    }
    if counts != expected:
        raise RuntimeError(f"function layout drift: {counts} != {expected}")
    return {
        "schema_version": 1,
        "counts": counts,
        "proxy_files": sorted({
            entry["destination"]
            for entry in entries
            if entry["layout_owner"] == "dongle-proxy"
        }),
        "new_files": sorted({
            entry["destination"]
            for entry in entries
            if entry["layout_owner"] == "svistok-new"
        }),
        "functions": sorted(
            entries, key=lambda entry: (entry["source_file"], entry["symbol"])
        ),
    }


def baseline_snapshot(layout: dict[str, Any]) -> dict[str, Any]:
    files = [
        {
            "file": path.relative_to(PROJECT_ROOT).as_posix(),
            "sha256": sha256(path),
        }
        for path in sorted(SRC_ROOT.rglob("*"))
        if path.is_file()
    ]
    return {
        "schema_version": 1,
        "layout_counts": layout["counts"],
        "source_files": files,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--baseline", action="store_true")
    arguments = parser.parse_args()
    ownership = json.loads(OWNERSHIP.read_text(encoding="utf-8"))
    layout = build(ownership)
    if arguments.write:
        OUTPUT.write_text(
            json.dumps(layout, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
    if arguments.baseline:
        BASELINE_OUTPUT.write_text(
            json.dumps(baseline_snapshot(layout), indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    print(json.dumps(layout["counts"], sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
