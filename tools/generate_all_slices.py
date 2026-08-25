#!/usr/bin/env python3
"""Generate all build-only upstream/Svistok slices and bridges."""

from __future__ import annotations

import argparse
import importlib.util
import json
from pathlib import Path
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BASELINE_ROOT = PROJECT_ROOT / "asterisk-chan-dongle"
SRC_ROOT = PROJECT_ROOT / "src"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def generate(ownership: dict, output_root: Path) -> dict:
    clang_manifest = load_tool("clang_manifest")
    materializer = load_tool("materialize_build_manifest")
    slicer = load_tool("slice_translation_unit")
    bridges = load_tool("generate_bridges")
    verifier = load_tool("verify_source_ownership")
    materialized = materializer.materialize(ownership)
    output_root.mkdir(parents=True, exist_ok=True)
    (output_root / "materialized-manifest.json").write_text(
        json.dumps(materialized, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )

    generated: list[dict[str, str]] = []
    by_file = {record["legacy_file"]: record for record in materialized["files"]}
    for relative in clang_manifest.MODIFIED_ROOTS:
        record = by_file[relative]
        included = [
            item["legacy_file"] for item in record.get("included_sources", [])
        ]
        unit_root = output_root / Path(relative).stem
        unit_root.mkdir(parents=True, exist_ok=True)
        for side, source in (
            ("upstream", BASELINE_ROOT / relative),
            ("svistok", SRC_ROOT / relative),
        ):
            raw = slicer.emit_slice(
                source.read_bytes(),
                record["symbols"],
                side=side,
                display_path=f"{side}/{relative}",
                included_sources=included,
                declarations=record.get("declarations"),
                bridges=record.get("bridges"),
            )
            errors = verifier.verify_slice(
                raw, source.read_bytes(), record["symbols"], side=side
            )
            if errors:
                raise RuntimeError(f'{relative}/{side}: {"; ".join(errors)}')
            prefix, suffix = bridges.generate_for_side(
                record["symbols"], record["bridges"], side
            )
            output = unit_root / f"{Path(relative).stem}-{side}.c"
            output.write_bytes(
                b'#include "svistok_abi.h"\n'
                + prefix.encode("utf-8")
                + raw
                + suffix.encode("utf-8")
            )
            generated.append(
                {"file": relative, "side": side, "source": str(output)}
            )
    summary = {
        "schema_version": 1,
        "translation_units": len(clang_manifest.MODIFIED_ROOTS),
        "generated_slices": generated,
        "bridge_count": sum(
            len(record["bridges"])
            for record in materialized["files"]
            if record["legacy_file"] in clang_manifest.MODIFIED_ROOTS
        ),
    }
    (output_root / "generation-summary.json").write_text(
        json.dumps(summary, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return summary


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--ownership", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    arguments = parser.parse_args()
    try:
        ownership = json.loads(arguments.ownership.read_text(encoding="utf-8"))
        summary = generate(ownership, arguments.output_root)
    except (OSError, RuntimeError, KeyError) as error:
        print(f"slice generation failed: {error}", file=sys.stderr)
        return 1
    print(
        f'generated {len(summary["generated_slices"])} slices with '
        f'{summary["bridge_count"]} bridges'
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
