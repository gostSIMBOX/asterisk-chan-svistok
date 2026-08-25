#!/usr/bin/env python3
"""Verify that emitted definition bytes originate from their declared owner."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
from typing import Any


def definition_fragment(source: bytes, record: dict[str, Any]) -> bytes:
    source_range = record["definition_range"]
    return source[int(source_range["begin"]):int(source_range["end"])]


def verify_slice(
    generated: bytes,
    owner_source: bytes,
    symbols: list[dict[str, Any]],
    *,
    side: str,
) -> list[str]:
    if side not in {"upstream", "svistok"}:
        raise ValueError(f"unknown side: {side}")
    source_key = "baseline" if side == "upstream" else "legacy"
    errors: list[str] = []
    for symbol in symbols:
        record = symbol.get(source_key)
        if record is None:
            continue
        fragment = definition_fragment(owner_source, record)
        occurrences = generated.count(fragment)
        should_exist = symbol["owner"] == side
        if should_exist and occurrences != 1:
            errors.append(
                f'{symbol["symbol"]}: expected exactly one {source_key} definition, found {occurrences}'
            )
        if not should_exist and occurrences:
            errors.append(
                f'{symbol["symbol"]}: unowned {source_key} definition is present'
            )
    return errors


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--file", required=True)
    parser.add_argument("--side", choices=("upstream", "svistok"), required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--generated", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        manifest = json.loads(arguments.manifest.read_text(encoding="utf-8"))
        record = next(
            item for item in manifest["files"] if item["legacy_file"] == arguments.file
        )
        errors = verify_slice(
            arguments.generated.read_bytes(),
            arguments.source.read_bytes(),
            record["symbols"],
            side=arguments.side,
        )
    except (OSError, ValueError, StopIteration, KeyError) as error:
        print(f"provenance verification failed: {error}", file=sys.stderr)
        return 1
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
