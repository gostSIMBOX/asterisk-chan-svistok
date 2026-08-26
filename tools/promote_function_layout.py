#!/usr/bin/env python3
"""Promote the fully verified function-layout staging tree as one set."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"
STAGE_ROOT = PROJECT_ROOT / "build/staging-function-layout/src"
BACKUP_ROOT = PROJECT_ROOT / "build/function-layout-backup/src"
REPORT = PROJECT_ROOT / "manifests/function-layout-promotion.json"


def inventory(root: Path) -> dict[str, str]:
    return {
        path.relative_to(root).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
        for path in sorted(root.rglob("*"))
        if path.is_file()
    }


def promote() -> dict:
    staged = inventory(STAGE_ROOT)
    current = inventory(SRC_ROOT)
    if BACKUP_ROOT.parent.exists():
        if not REPORT.is_file():
            raise RuntimeError(f"existing backup has no promotion report: {BACKUP_ROOT.parent}")
        previous = json.loads(REPORT.read_text(encoding="utf-8"))
        before = inventory(BACKUP_ROOT)
        if before != previous["before"]:
            raise RuntimeError("existing backup differs from the recorded starting state")
    else:
        before = current
        BACKUP_ROOT.parent.mkdir(parents=True)
        shutil.copytree(SRC_ROOT, BACKUP_ROOT)
    added = sorted(set(staged) - set(before))
    removed = sorted(set(before) - set(staged))
    modified = sorted(
        path for path in set(before) & set(staged) if before[path] != staged[path]
    )
    try:
        for relative in sorted(set(current) - set(staged)):
            (SRC_ROOT / relative).unlink()
        for relative in sorted(
            path for path in staged if current.get(path) != staged[path]
        ):
            destination = SRC_ROOT / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(STAGE_ROOT / relative, destination)
    except Exception:
        for path in sorted(SRC_ROOT.rglob("*"), reverse=True):
            if path.is_file():
                path.unlink()
            elif path.is_dir():
                try:
                    path.rmdir()
                except OSError:
                    pass
        for source in BACKUP_ROOT.rglob("*"):
            if source.is_file():
                destination = SRC_ROOT / source.relative_to(BACKUP_ROOT)
                destination.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(source, destination)
        raise
    after = inventory(SRC_ROOT)
    if after != staged:
        raise RuntimeError("post-promotion source hashes do not equal staging")
    if inventory(BACKUP_ROOT) != before:
        raise RuntimeError("backup hashes do not equal pre-promotion source")
    return {
        "schema_version": 1,
        "stage": str(STAGE_ROOT),
        "backup": str(BACKUP_ROOT),
        "added": added,
        "modified": modified,
        "removed": removed,
        "before": before,
        "after": after,
    }


def main() -> int:
    try:
        report = promote()
        REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    except (OSError, RuntimeError) as error:
        print(f"function-layout promotion failed: {error}", file=sys.stderr)
        return 1
    print(
        f'promoted function layout: {len(report["added"])} added, '
        f'{len(report["modified"])} modified, {len(report["removed"])} removed'
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
