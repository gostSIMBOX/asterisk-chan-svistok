#!/usr/bin/env python3
"""Promote one fully verified 28-file overlay set with a scoped backup."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"


def load_manifest_tool():
    path = PROJECT_ROOT / "tools" / "clang_manifest.py"
    spec = importlib.util.spec_from_file_location("clang_manifest", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def promote(staging: Path, backup: Path) -> dict:
    manifest = load_manifest_tool()
    paths = [*manifest.MODIFIED_ROOTS, *manifest.MODIFIED_HEADERS]
    if len(paths) != 28 or len(set(paths)) != 28:
        raise RuntimeError("modified source set is not exactly 28 unique paths")
    if backup.exists():
        raise RuntimeError(f"backup already exists: {backup}")
    missing = [path for path in paths if not (staging / path).is_file()]
    if missing:
        raise RuntimeError(f'missing staged paths: {", ".join(missing)}')
    records = []
    for relative in paths:
        current = SRC_ROOT / relative
        candidate = staging / relative
        destination_backup = backup / relative
        destination_backup.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(current, destination_backup)
        records.append({
            "file": relative,
            "before_sha256": digest(current),
            "staged_sha256": digest(candidate),
        })
    for relative in paths:
        candidate = staging / relative
        destination = SRC_ROOT / relative
        shutil.copy2(candidate, destination)
    for record in records:
        after = digest(SRC_ROOT / record["file"])
        if after != record["staged_sha256"]:
            raise RuntimeError(f'promotion hash mismatch: {record["file"]}')
        record["after_sha256"] = after
    return {
        "schema_version": 1,
        "promoted_files": len(records),
        "staging": str(staging),
        "backup": str(backup),
        "files": records,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--staging", type=Path, required=True)
    parser.add_argument("--backup", type=Path, required=True)
    parser.add_argument("--report", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        report = promote(arguments.staging.resolve(), arguments.backup.resolve())
        arguments.report.parent.mkdir(parents=True, exist_ok=True)
        arguments.report.write_text(
            json.dumps(report, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    except (OSError, RuntimeError) as error:
        print(f"overlay promotion failed: {error}", file=sys.stderr)
        return 1
    print(f'promoted {report["promoted_files"]} verified overlay files')
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
