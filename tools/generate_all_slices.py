#!/usr/bin/env python3
"""Generate baseline slices and non-filtering Svistok compositions."""

from __future__ import annotations

import argparse
import copy
import importlib.util
import json
from pathlib import Path
import re
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
BASELINE_ROOT = PROJECT_ROOT / "asterisk-chan-dongle"
SRC_ROOT = PROJECT_ROOT / "src"
FUNCTION_LAYOUT = PROJECT_ROOT / "manifests" / "function-layout.json"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def effective_ownership(ownership: dict, layout: dict | None) -> dict:
    result = copy.deepcopy(ownership)
    if layout is None:
        return result
    direct = {
        (entry["source_file"], entry["symbol"])
        for entry in layout["functions"]
        if entry["layout_owner"] == "baseline"
    }
    for record in result["files"]:
        relative = record["legacy_file"]
        for symbol in record["symbols"]:
            if (relative, symbol["symbol"]) in direct:
                symbol["owner"] = "upstream"
                symbol["body_relation"] = "equivalent"
        by_name = {entry["symbol"]: entry for entry in record["symbols"]}
        consumers: dict[str, set[str]] = {}
        for entry in record["symbols"]:
            owner = entry["owner"]
            if owner not in {"upstream", "svistok"}:
                continue
            definition = entry["baseline" if owner == "upstream" else "legacy"]
            if definition is None:
                continue
            for dependency in definition.get("dependencies", []):
                target = by_name.get(dependency)
                if target is None or target["linkage"] != "static":
                    continue
                if target["owner"] in {"upstream", "svistok"} and target["owner"] != owner:
                    consumers.setdefault(dependency, set()).add(owner)
        proxy_symbols = {
            entry["symbol"]
            for entry in layout["functions"]
            if entry["source_file"] == relative
            and entry["layout_owner"] == "dongle-proxy"
        }
        # A proxy's pristine implementation is emitted into the upstream slice.
        # Account for its baseline dependencies as upstream consumers too.
        for symbol in proxy_symbols:
            unit = by_name[symbol]
            for dependency in unit["baseline"].get("dependencies", []):
                target = by_name.get(dependency)
                if target is None or target["linkage"] != "static":
                    continue
                if target["owner"] == "svistok":
                    consumers.setdefault(dependency, set()).add("upstream")
        record["bridges"] = [
            {
                "symbol": symbol,
                "owner": by_name[symbol]["owner"],
                "consumers": sorted(sides),
            }
            for symbol, sides in sorted(consumers.items())
        ]
        for side in record.get("declarations", {}).values():
            for declaration in side:
                unit = by_name.get(declaration["symbol"])
                if unit is not None:
                    declaration["owner"] = unit["owner"]
                    declaration["body_relation"] = unit["body_relation"]
    return result


def proxy_baseline_entries(record: dict, source: bytes, layout: dict | None) -> bytes:
    if layout is None:
        return b""
    approved = {
        entry["symbol"]: entry
        for entry in layout["functions"]
        if entry["source_file"] == record["legacy_file"]
        and entry["layout_owner"] == "dongle-proxy"
    }
    if not approved:
        return b""
    chunks = [b"\n/* Hidden pristine dongle implementations for proxy composition. */\n"]
    for unit in record["symbols"]:
        entry = approved.get(unit["symbol"])
        if entry is None:
            continue
        baseline = unit["baseline"]
        source_range = baseline["definition_range"]
        begin, end = int(source_range["begin"]), int(source_range["end"])
        snippet = source[begin:end]
        name_at = int(baseline["name_offset"]) - begin
        prefix = snippet[:name_at]
        prefix = prefix.replace(b"static ", b"", 1)
        name = unit["symbol"].encode("ascii")
        renamed = entry["composition"]["baseline_entry"].encode("ascii")
        snippet = prefix + renamed + snippet[name_at + len(name):]
        chunks.append(b'__attribute__((visibility("hidden"))) ' + snippet + b"\n")
    return b"".join(chunks)


def layout_forward_declarations(
    record: dict, layout: dict | None, bridges
) -> str:
    """Declare extracted functions before their fragments are included."""
    if layout is None:
        return ""
    moved = {
        entry["symbol"]
        for entry in layout["functions"]
        if entry["source_file"] == record["legacy_file"]
        and entry["layout_owner"] in {"svistok-new", "dongle-proxy"}
    }
    declarations: list[str] = []
    for unit in record["symbols"]:
        if unit["symbol"] not in moved:
            continue
        definition = unit["legacy"]
        declarations.append(
            ("static " if unit["linkage"] == "static" else "extern ")
            + bridges.function_declaration(unit["symbol"], definition["signature"])
            + ";"
        )
    return ("\n".join(declarations) + "\n") if declarations else ""


def declaration_prerequisites(source: Path) -> str:
    """Repeat guarded header includes needed by generated early prototypes."""
    includes: list[str] = []
    for line in source.read_text(encoding="utf-8", errors="surrogateescape").splitlines():
        match = re.match(r"\s*#\s*include\s+([<\"][^>\"]+[>\"])", line)
        if match is None:
            continue
        target = match.group(1)
        if target.strip('<>"').endswith(("config.h", "svistok_config.h")):
            continue
        if target.endswith('.h>') or target.endswith('.h"'):
            if target.startswith('"'):
                local = source.parent / target.strip('"')
                directive = (
                    f'#include "{local.resolve().as_posix()}"'
                    if local.is_file()
                    else f"#include {target}"
                )
            else:
                directive = f"#include {target}"
            if directive not in includes:
                includes.append(directive)
    return ("\n".join(includes) + "\n") if includes else ""


def generate(ownership: dict, output_root: Path, *, src_root: Path = SRC_ROOT) -> dict:
    clang_manifest = load_tool("clang_manifest")
    slicer = load_tool("slice_translation_unit")
    bridges = load_tool("generate_bridges")
    verifier = load_tool("verify_source_ownership")
    layout = (
        json.loads(FUNCTION_LAYOUT.read_text(encoding="utf-8"))
        if FUNCTION_LAYOUT.is_file()
        else None
    )
    materialized = effective_ownership(ownership, layout)
    output_root.mkdir(parents=True, exist_ok=True)
    composed_root = output_root / "composed-headers"
    composed_report = load_tool("compose_headers").compose(
        ownership, composed_root, src_root=src_root
    )
    composed_defines: list[str] = []
    for item in composed_report["files"]:
        relative = item["file"]
        macro = "SVISTOK_COMPOSED_" + relative.upper().replace(".", "_").replace("/", "_") + "_HEADER"
        composed_defines.append(f'#define {macro} "{Path(item["output"]).resolve().as_posix()}"')
    (output_root / "composed-headers.json").write_text(
        json.dumps(composed_report, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    api_defines_header = output_root / "composed-header-defines.h"
    api_defines_header.write_text(
        "#ifndef SVISTOK_GENERATED_COMPOSED_HEADER_DEFINES_H_INCLUDED\n"
        "#define SVISTOK_GENERATED_COMPOSED_HEADER_DEFINES_H_INCLUDED\n"
        + "\n".join(composed_defines)
        + "\n#endif\n",
        encoding="utf-8",
    )
    api_macro = (
        f'#include "{api_defines_header.resolve().as_posix()}"\n'
    ).encode("utf-8")
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
        source = BASELINE_ROOT / relative
        side = "upstream"
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
            api_macro
            + b'#include "svistok_abi.h"\n'
            + prefix.encode("utf-8")
            + raw
            + suffix.encode("utf-8")
            + proxy_baseline_entries(record, source.read_bytes(), layout)
        )
        generated.append(
            {"file": relative, "side": side, "kind": "baseline-slice", "source": str(output)}
        )

        side = "svistok"
        prefix, suffix = bridges.generate_for_side(
            record["symbols"], record["bridges"], side
        )
        output = unit_root / f"{Path(relative).stem}-overlay.c"
        fragment_paths = [
            src_root / "svistok" / relative,
            src_root / "svistok" / "hooks" / relative,
            src_root / "dongle" / relative,
        ]
        root_source = src_root / relative
        includes = [
            path for path in [root_source, *fragment_paths] if path.is_file()
        ]
        if not includes:
            if suffix:
                raise RuntimeError(
                    f"{relative}: no overlay source for generated producer bridge"
                )
            continue
        source = includes[0]
        output.write_text(
            api_macro.decode("utf-8")
            + '#include "svistok_abi.h"\n'
            + declaration_prerequisites(source)
            + prefix
            + layout_forward_declarations(record, layout, bridges)
            + "".join(
                f'#include "{path.resolve().as_posix().replace(chr(34), chr(92) + chr(34))}"\n'
                for path in includes
            )
            + suffix,
            encoding="utf-8",
        )
        generated.append(
            {"file": relative, "side": side, "kind": "overlay-composition", "source": str(output)}
        )
    summary = {
        "schema_version": 1,
        "translation_units": len(clang_manifest.MODIFIED_ROOTS),
        "generated_slices": generated,
        "baseline_slices": len(clang_manifest.MODIFIED_ROOTS),
        "overlay_compositions": sum(
            item["kind"] == "overlay-composition" for item in generated
        ),
        "composed_header_defines": str(api_defines_header),
        "composed_header_baseline_units": composed_report["baseline_unit_count"],
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
    parser.add_argument("--src-root", type=Path, default=SRC_ROOT)
    arguments = parser.parse_args()
    try:
        ownership = json.loads(arguments.ownership.read_text(encoding="utf-8"))
        summary = generate(
            ownership, arguments.output_root, src_root=arguments.src_root.resolve()
        )
    except (OSError, RuntimeError, KeyError) as error:
        print(f"slice generation failed: {error}", file=sys.stderr)
        return 1
    print(
        f'generated {summary["baseline_slices"]} baseline slices and '
        f'{summary["overlay_compositions"]} overlay compositions with '
        f'{summary["bridge_count"]} bridges'
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
