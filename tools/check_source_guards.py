#!/usr/bin/env python3
"""Fail unless the read-only legacy and upstream trees match the SDD baseline."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = PROJECT_ROOT / "manifests" / "source-baselines.json"


def run_git(root: Path, *arguments: str) -> str:
    completed = subprocess.run(
        ["git", "-C", str(root), *arguments],
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode != 0:
        message = completed.stderr.strip() or completed.stdout.strip()
        raise RuntimeError(f"git {' '.join(arguments)} failed for {root}: {message}")
    return completed.stdout.strip()


def count_tree_entries(root: Path) -> int:
    count = 0
    for directory, directories, filenames in os.walk(root, followlinks=False):
        directories[:] = [name for name in directories if name != ".git"]
        count += len(filenames)
        symlinked_directories = [
            name for name in directories if (Path(directory) / name).is_symlink()
        ]
        count += len(symlinked_directories)
        directories[:] = [name for name in directories if name not in symlinked_directories]
    return count


def inspect_source(name: str, specification: dict[str, Any]) -> dict[str, Any]:
    root = (PROJECT_ROOT / specification["path"]).resolve()
    if not root.is_dir():
        raise RuntimeError(f"{name} source directory does not exist: {root}")

    commit = run_git(root, "rev-parse", "HEAD")
    status = run_git(root, "status", "--porcelain=v1", "--untracked-files=all")
    entry_count = count_tree_entries(root)

    errors: list[str] = []
    if commit != specification["expected_commit"]:
        errors.append(
            f"commit {commit} != expected {specification['expected_commit']}"
        )
    if status:
        errors.append(f"worktree is dirty: {status}")
    if entry_count != specification["entry_count"]:
        errors.append(
            f"entry count {entry_count} != expected {specification['entry_count']}"
        )

    return {
        "name": name,
        "path": str(root),
        "commit": commit,
        "entry_count": entry_count,
        "clean": not status,
        "errors": errors,
    }


def check(manifest_path: Path = DEFAULT_MANIFEST) -> dict[str, Any]:
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    sources = [
        inspect_source(name, specification)
        for name, specification in sorted(manifest["sources"].items())
    ]
    errors = [
        f"{source['name']}: {error}"
        for source in sources
        for error in source["errors"]
    ]
    return {
        "ok": not errors,
        "target_module": manifest["target_module"],
        "sources": sources,
        "errors": errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--json", action="store_true", dest="as_json")
    arguments = parser.parse_args()

    try:
        result = check(arguments.manifest.resolve())
    except (OSError, RuntimeError, KeyError, json.JSONDecodeError) as error:
        print(f"source guard failed: {error}", file=sys.stderr)
        return 1

    if arguments.as_json:
        print(json.dumps(result, indent=2, sort_keys=True))
    elif result["ok"]:
        for source in result["sources"]:
            print(
                f"{source['name']}: clean {source['commit']} "
                f"({source['entry_count']} entries)"
            )
    else:
        for error in result["errors"]:
            print(error, file=sys.stderr)

    return 0 if result["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
