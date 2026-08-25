#!/usr/bin/env python3
"""Mechanically copy only the inventory-approved NEW/MODIFIED module paths."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import shutil
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
INVENTORY_PATH = PROJECT_ROOT / "manifests" / "module-files.json"
LEGACY_ROOT = (PROJECT_ROOT / "../../legacy/asterisk-chan-svistok-v2014").resolve()
DESTINATION_ROOT = PROJECT_ROOT / "src"
RECEIPT_PATH = PROJECT_ROOT / "manifests" / "copy-receipts.sha256"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def approved_paths(inventory: dict) -> list[tuple[str, str]]:
    return [
        (relation, path)
        for relation in ("new", "modified")
        for path in inventory["module"][relation]
    ]


def main() -> int:
    inventory = json.loads(INVENTORY_PATH.read_text(encoding="utf-8"))
    approved = approved_paths(inventory)
    allowed = {path for _, path in approved}
    existing = {
        path.relative_to(DESTINATION_ROOT).as_posix()
        for path in DESTINATION_ROOT.rglob("*")
        if path.is_file() or path.is_symlink()
    }
    unexpected = sorted(existing - allowed)
    if unexpected:
        print(f"copy refused: unexpected src paths: {unexpected}", file=sys.stderr)
        return 1

    receipts: list[str] = ["# relation\tpath\tlegacy_sha256\tdestination_sha256"]
    for relation, relative in approved:
        source = LEGACY_ROOT / relative
        destination = DESTINATION_ROOT / relative
        if not source.is_file() or source.is_symlink():
            print(f"copy refused: source is not a regular file: {relative}", file=sys.stderr)
            return 1
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, destination)
        source_hash = sha256(source)
        destination_hash = sha256(destination)
        if source_hash != destination_hash:
            print(f"copy verification failed: {relative}", file=sys.stderr)
            return 1
        receipts.append(
            f"{relation}\t{relative}\t{source_hash}\t{destination_hash}"
        )

    RECEIPT_PATH.write_text("\n".join(receipts) + "\n", encoding="utf-8")
    print(f"copied {len(approved)} approved paths with byte-identical receipts")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
