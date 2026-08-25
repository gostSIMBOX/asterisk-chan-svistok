#!/usr/bin/env python3
"""Reproduce the approved chan_svistok source closure and exclusion manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import sys
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[1]
LEGACY_RELATIVE_ROOT = "../../legacy/asterisk-chan-svistok-v2014"
BASELINE_RELATIVE_ROOT = "asterisk-chan-dongle"
LEGACY_ROOT = (PROJECT_ROOT / LEGACY_RELATIVE_ROOT).resolve()
BASELINE_ROOT = (PROJECT_ROOT / BASELINE_RELATIVE_ROOT).resolve()
DEFAULT_OUTPUT = PROJECT_ROOT / "manifests" / "module-files.json"

ROOT_TRANSLATION_UNITS = (
    "app.c",
    "at_command.c",
    "at_parse.c",
    "at_queue.c",
    "at_read.c",
    "at_response.c",
    "chan_dongle.c",
    "channel.c",
    "char_conv.c",
    "cli.c",
    "helpers.c",
    "manager.c",
    "memmem.c",
    "ringbuffer.c",
    "cpvt.c",
    "dc_config.c",
    "pdu.c",
    "mixbuffer.c",
    "pdiscovery.c",
)

QUOTED_INCLUDE = re.compile(
    rb'^\s*#\s*include\s*"([^"]+)"', re.MULTILINE
)


def tree_entries(root: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for directory, directories, filenames in os.walk(root, followlinks=False):
        directories[:] = [name for name in directories if name != ".git"]
        for name in tuple(directories):
            path = Path(directory) / name
            if path.is_symlink():
                relative = path.relative_to(root).as_posix()
                entries[relative] = "symlink"
                directories.remove(name)
        for name in filenames:
            path = Path(directory) / name
            relative = path.relative_to(root).as_posix()
            entries[relative] = "symlink" if path.is_symlink() else "file"
    return dict(sorted(entries.items()))


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def local_includes(relative: str) -> list[str]:
    source = LEGACY_ROOT / relative
    matches = QUOTED_INCLUDE.findall(source.read_bytes())
    resolved: list[str] = []
    for raw_include in matches:
        include = raw_include.decode("latin-1")
        candidates = (source.parent / include, LEGACY_ROOT / include)
        for candidate in candidates:
            if candidate.is_file():
                canonical = candidate.resolve()
                try:
                    resolved.append(canonical.relative_to(LEGACY_ROOT).as_posix())
                except ValueError as error:
                    raise RuntimeError(
                        f"local include escapes legacy tree: {relative} -> {include}"
                    ) from error
                break
    return resolved


def module_closure() -> list[str]:
    pending = list(ROOT_TRANSLATION_UNITS)
    seen: set[str] = set()
    while pending:
        relative = PurePosixPath(pending.pop()).as_posix()
        if relative in seen:
            continue
        source = LEGACY_ROOT / relative
        if not source.is_file():
            raise RuntimeError(f"module source/include does not exist: {relative}")
        seen.add(relative)
        pending.extend(local_includes(relative))
    return sorted(seen)


def relation(relative: str) -> str:
    legacy_path = LEGACY_ROOT / relative
    baseline_path = BASELINE_ROOT / relative
    if not baseline_path.exists():
        return "new"
    if not legacy_path.is_file() or not baseline_path.is_file():
        return "modified"
    return "identical" if sha256(legacy_path) == sha256(baseline_path) else "modified"


def grouped_relations(paths: list[str]) -> dict[str, list[str]]:
    groups = {"new": [], "modified": [], "identical": []}
    for relative in paths:
        groups[relation(relative)].append(relative)
    return groups


def build_inventory() -> dict[str, Any]:
    legacy_entries = tree_entries(LEGACY_ROOT)
    baseline_entries = tree_entries(BASELINE_ROOT)
    closure = module_closure()
    closure_set = set(closure)

    excluded_regular = sorted(
        path
        for path, kind in legacy_entries.items()
        if kind == "file" and path not in closure_set
    )
    excluded_symlinks = [
        {
            "path": path,
            "target": os.readlink(LEGACY_ROOT / path),
        }
        for path, kind in legacy_entries.items()
        if kind == "symlink" and path not in closure_set
    ]
    module_groups = grouped_relations(closure)
    excluded_groups = grouped_relations(excluded_regular)
    baseline_only = sorted(set(baseline_entries) - set(legacy_entries))

    return {
        "schema_version": 1,
        "roots": {
            "legacy": LEGACY_RELATIVE_ROOT,
            "baseline": BASELINE_RELATIVE_ROOT,
        },
        "root_translation_units": list(ROOT_TRANSLATION_UNITS),
        "counts": {
            "legacy_entries": len(legacy_entries),
            "baseline_entries": len(baseline_entries),
            "module_total": len(closure),
            "module_new": len(module_groups["new"]),
            "module_modified": len(module_groups["modified"]),
            "module_identical": len(module_groups["identical"]),
            "excluded_total": len(excluded_regular) + len(excluded_symlinks),
            "excluded_regular": len(excluded_regular),
            "excluded_symlinks": len(excluded_symlinks),
        },
        "module": module_groups,
        "excluded_do_not_copy": {
            **excluded_groups,
            "symlinks": excluded_symlinks,
        },
        "baseline_only": baseline_only,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()

    try:
        inventory = build_inventory()
    except (OSError, RuntimeError) as error:
        print(f"inventory failed: {error}", file=sys.stderr)
        return 1

    rendered = json.dumps(inventory, indent=2, sort_keys=True) + "\n"
    if arguments.output:
        arguments.output.resolve().write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
