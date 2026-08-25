#!/usr/bin/env python3
"""Build syntax-aware symbol ownership records from Clang JSON AST."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import shlex
import subprocess
import sys
from typing import Any, Iterable


PROJECT_ROOT = Path(__file__).resolve().parents[1]
LEGACY_ROOT = (PROJECT_ROOT / "../../legacy/asterisk-chan-svistok-v2014").resolve()
BASELINE_ROOT = (PROJECT_ROOT / "asterisk-chan-dongle").resolve()
STUB_ROOT = PROJECT_ROOT / "tests" / "characterization" / "stubs"

COMMON_ARGS = (
    "-std=gnu89",
    "-Wno-invalid-pp-token",
    "-Wno-deprecated-non-prototype",
    "-Wno-error=implicit-function-declaration",
    "-Wno-return-mismatch",
    "-Wno-int-conversion",
    "-ferror-limit=0",
    "-DICONV_CONST=",
    "-DICONV_T=iconv_t",
    "-DBUILD_APPLICATIONS=1",
    "-DBUILD_MANAGER=1",
    "-DASTERISK_VERSION_NUM=110000",
    "-include",
    "stdio.h",
    "-include",
    "stdlib.h",
    "-include",
    "string.h",
    "-include",
    "strings.h",
    "-include",
    "stdarg.h",
    "-include",
    "limits.h",
)

FILE_ARGS: dict[str, tuple[str, ...]] = {
    "at_parse.c": (
        "-DCHAN_DONGLE_H_INCLUDED",
        "-DCHAN_DONGLE_HELPERS_H_INCLUDED",
        "-include",
        str(STUB_ROOT / "asterisk_compat.h"),
        "-include",
        "stdio.h",
        "-include",
        "stddef.h",
        "-include",
        "string.h",
    ),
    "at_queue.c": (
        "-include",
        str(STUB_ROOT / "queue_compat.h"),
    ),
    "at_read.c": (
        "-DCHAN_DONGLE_H_INCLUDED",
        "-include",
        str(STUB_ROOT / "asterisk_compat.h"),
        "-include",
        "string.h",
    ),
    "pdu.c": (
        "-DCHAN_DONGLE_HELPERS_H_INCLUDED",
        "-include",
        "stdio.h",
        "-include",
        "stddef.h",
        "-include",
        "string.h",
    ),
    "ringbuffer.c": (),
}

MODIFIED_ROOTS = (
    "app.c", "at_command.c", "at_parse.c", "at_queue.c", "at_read.c",
    "at_response.c", "chan_dongle.c", "channel.c", "cli.c", "helpers.c",
    "manager.c", "ringbuffer.c", "cpvt.c", "dc_config.c", "pdu.c",
    "pdiscovery.c",
)
MODIFIED_HEADERS = (
    "app.h", "at_command.h", "at_parse.h", "at_response.h", "chan_dongle.h",
    "channel.h", "cli.h", "cpvt.h", "dc_config.h", "pdiscovery.h", "pdu.h",
    "ringbuffer.h",
)


def clang_path() -> str:
    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise RuntimeError("Clang-compatible C compiler not found")
    return compiler


def dump_ast(source: Path, extra_args: Iterable[str]) -> dict[str, Any]:
    command = [
        clang_path(),
        *COMMON_ARGS,
        "-I",
        str(STUB_ROOT),
        "-I",
        str(source.parent),
        "-I",
        str(PROJECT_ROOT),
        *extra_args,
        "-Xclang",
        "-ast-dump=json",
        "-fsyntax-only",
        str(source),
    ]
    completed = subprocess.run(
        command,
        cwd=PROJECT_ROOT,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"Clang AST failed for {source}:\n{completed.stderr}"
        )
    try:
        return json.loads(completed.stdout)
    except json.JSONDecodeError as error:
        raise RuntimeError(f"Clang emitted invalid JSON for {source}") from error


def preprocess_macros(source: Path, extra_args: Iterable[str]) -> dict[str, str]:
    command = [
        clang_path(),
        *COMMON_ARGS,
        "-I",
        str(STUB_ROOT),
        "-I",
        str(source.parent),
        "-I",
        str(PROJECT_ROOT),
        *extra_args,
        "-E",
        "-dD",
        str(source),
    ]
    completed = subprocess.run(
        command,
        cwd=PROJECT_ROOT,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    if completed.returncode != 0:
        raise RuntimeError(
            f"Clang preprocessing failed for {source}:\n{completed.stderr}"
        )
    current: Path | None = None
    macros: dict[str, str] = {}
    canonical_source = source.resolve()
    for line in completed.stdout.splitlines():
        if line.startswith("# "):
            try:
                fields = shlex.split(line)
            except ValueError:
                current = None
                continue
            if len(fields) >= 3 and not fields[2].startswith("<"):
                current = Path(fields[2]).resolve()
            else:
                current = None
            continue
        if current != canonical_source or not line.startswith("#define "):
            continue
        definition = line[len("#define "):].strip()
        head = definition.split(None, 1)[0] if definition else ""
        name = head.split("(", 1)[0]
        if name:
            macros[name] = definition
    return dict(sorted(macros.items()))


def location_offset(location: dict[str, Any]) -> int | None:
    if "offset" in location:
        return int(location["offset"])
    for key in ("expansionLoc", "spellingLoc"):
        nested = location.get(key)
        if isinstance(nested, dict):
            offset = location_offset(nested)
            if offset is not None:
                return offset
    return None


def location_token_length(location: dict[str, Any]) -> int:
    if "tokLen" in location:
        return int(location["tokLen"])
    for key in ("expansionLoc", "spellingLoc"):
        nested = location.get(key)
        if isinstance(nested, dict):
            length = location_token_length(nested)
            if length:
                return length
    return 0


def node_range(node: dict[str, Any]) -> tuple[int, int]:
    source_range = node.get("range", {})
    begin = location_offset(source_range.get("begin", {}))
    end_location = source_range.get("end", {})
    end = location_offset(end_location)
    if begin is None or end is None:
        raise RuntimeError(
            f"AST node {node.get('kind')} {node.get('name')} has no byte range"
        )
    return begin, end + location_token_length(end_location)


def definition_body(node: dict[str, Any]) -> dict[str, Any] | None:
    if node.get("kind") != "FunctionDecl":
        return None
    for child in node.get("inner", []):
        if child.get("kind") == "CompoundStmt":
            return child
    return None


IGNORED_AST_KEYS = {
    "id",
    "loc",
    "range",
    "mangledName",
    "isUsed",
    "isReferenced",
    "valueCategory",
    "previousDecl",
    "referencedMemberDecl",
    "typeAliasDeclId",
    "parentDeclContextId",
}


def normalized_ast(value: Any, *, ignore_types: bool) -> Any:
    if isinstance(value, list):
        return [normalized_ast(item, ignore_types=ignore_types) for item in value]
    if not isinstance(value, dict):
        return value
    result: dict[str, Any] = {}
    for key, item in sorted(value.items()):
        if key in IGNORED_AST_KEYS or (ignore_types and key == "type"):
            continue
        if key == "referencedDecl" and isinstance(item, dict):
            result[key] = {
                field: normalized_ast(item[field], ignore_types=ignore_types)
                for field in ("kind", "name")
                if field in item
            }
            continue
        result[key] = normalized_ast(item, ignore_types=ignore_types)
    return result


def ast_hash(node: dict[str, Any], *, ignore_types: bool) -> str:
    normalized = json.dumps(
        normalized_ast(node, ignore_types=ignore_types),
        sort_keys=True,
        separators=(",", ":"),
    ).encode("utf-8")
    return hashlib.sha256(normalized).hexdigest()


def source_hash(source_bytes: bytes, node: dict[str, Any]) -> str:
    begin, end = node_range(node)
    return hashlib.sha256(source_bytes[begin:end]).hexdigest()


def referenced_symbols(node: Any) -> list[str]:
    found: set[str] = set()
    if isinstance(node, list):
        for item in node:
            found.update(referenced_symbols(item))
    elif isinstance(node, dict):
        reference = node.get("referencedDecl")
        if isinstance(reference, dict) and reference.get("kind") in {
            "FunctionDecl",
            "VarDecl",
        }:
            name = reference.get("name")
            if name:
                found.add(name)
        for key, item in node.items():
            if key != "referencedDecl":
                found.update(referenced_symbols(item))
    return sorted(found)


def definition_record(
    node: dict[str, Any], source_bytes: bytes
) -> tuple[tuple[str, str], dict[str, Any]] | None:
        kind = node.get("kind")
        name = node.get("name")
        if not name or kind not in {"FunctionDecl", "VarDecl"}:
            return None
        body = definition_body(node)
        if kind == "FunctionDecl" and body is None:
            return None
        if kind == "VarDecl" and node.get("storageClass") == "extern":
            return None
        payload = body if body is not None else node
        begin, end = node_range(node)
        if body is not None:
            function_type = node.get("type", {}).get("qualType", "")
            return_type = function_type.split("(", 1)[0].rstrip()
            parameters = [
                {
                    "name": child.get("name") or f"argument_{index}",
                    "type": child.get("type", {}).get("qualType", "int"),
                }
                for index, child in enumerate(node.get("inner", []))
                if child.get("kind") == "ParmVarDecl"
            ]
            signature = {
                "return_type": return_type,
                "parameters": parameters,
                "variadic": function_type.endswith(", ...)") or function_type.endswith("(... )"),
            }
        else:
            signature = {"type": node.get("type", {}).get("qualType", "int")}
        return (kind, name), {
            "symbol": name,
            "kind": "function" if kind == "FunctionDecl" else "data",
            "linkage": "static" if node.get("storageClass") == "static" else "external",
            "definition_range": {"begin": begin, "end": end},
            "name_offset": location_offset(node.get("loc", {})),
            "body_range": (
                {"begin": node_range(body)[0], "end": node_range(body)[1]}
                if body is not None
                else None
            ),
            "signature": signature,
            "dependencies": referenced_symbols(payload),
            "ast_sha256": ast_hash(payload, ignore_types=body is not None),
            "source_sha256": source_hash(source_bytes, payload),
        }


def declaration_record(
    node: dict[str, Any], source_bytes: bytes
) -> dict[str, Any] | None:
    if node.get("kind") != "FunctionDecl" or definition_body(node) is not None:
        return None
    name = node.get("name")
    if not name or node.get("isImplicit"):
        return None
    begin, end = node_range(node)
    return {
        "symbol": name,
        "linkage": "static" if node.get("storageClass") == "static" else "external",
        "declaration_range": {"begin": begin, "end": end},
        "source_sha256": hashlib.sha256(source_bytes[begin:end]).hexdigest(),
    }


def definitions_by_provenance(
    ast: dict[str, Any], source_path: Path, source_root: Path
) -> dict[str, dict[tuple[str, str], dict[str, Any]]]:
    current_file = source_path.resolve()
    grouped: dict[str, dict[tuple[str, str], dict[str, Any]]] = {}
    source_cache: dict[Path, bytes] = {}
    for node in ast.get("inner", []):
        explicit_file = node.get("loc", {}).get("file")
        if explicit_file:
            current_file = Path(explicit_file).resolve()
        try:
            relative = current_file.relative_to(source_root.resolve()).as_posix()
        except ValueError:
            continue
        if current_file not in source_cache:
            if not current_file.is_file():
                continue
            source_cache[current_file] = current_file.read_bytes()
        record = definition_record(node, source_cache[current_file])
        if record is not None:
            key, value = record
            grouped.setdefault(relative, {})[key] = value
    return grouped


def declarations_by_provenance(
    ast: dict[str, Any], source_path: Path, source_root: Path
) -> dict[str, list[dict[str, Any]]]:
    current_file = source_path.resolve()
    grouped: dict[str, list[dict[str, Any]]] = {}
    source_cache: dict[Path, bytes] = {}
    for node in ast.get("inner", []):
        explicit_file = node.get("loc", {}).get("file")
        if explicit_file:
            current_file = Path(explicit_file).resolve()
        try:
            relative = current_file.relative_to(source_root.resolve()).as_posix()
        except ValueError:
            continue
        if current_file not in source_cache:
            if not current_file.is_file():
                continue
            source_cache[current_file] = current_file.read_bytes()
        record = declaration_record(node, source_cache[current_file])
        if record is not None:
            grouped.setdefault(relative, []).append(record)
    return grouped


def declarations_for(root: Path, relative: str) -> list[dict[str, Any]]:
    source = root / relative
    ast = dump_ast(source, FILE_ARGS.get(relative, ()))
    return declarations_by_provenance(ast, source, root).get(relative, [])


def type_units_for(root: Path, relative: str) -> dict[tuple[str, str], dict[str, Any]]:
    """Return outermost file-local typedef/record/enum source units."""
    source = root / relative
    ast = dump_ast(source, FILE_ARGS.get(relative, ()))
    source_bytes = source.read_bytes()
    current_file = source.resolve()
    candidates: list[tuple[dict[str, Any], int, int]] = []
    for node in ast.get("inner", []):
        explicit_file = node.get("loc", {}).get("file")
        if explicit_file:
            current_file = Path(explicit_file).resolve()
        if current_file != source.resolve() or node.get("isImplicit"):
            continue
        if node.get("kind") not in {"TypedefDecl", "RecordDecl", "EnumDecl"}:
            continue
        try:
            begin, end = node_range(node)
        except RuntimeError:
            continue
        candidates.append((node, begin, end))
    typedef_ranges = [
        (begin, end)
        for node, begin, end in candidates
        if node.get("kind") == "TypedefDecl"
    ]
    records: dict[tuple[str, str], dict[str, Any]] = {}
    anonymous_index = 0
    for node, begin, end in candidates:
        kind = node["kind"]
        if kind != "TypedefDecl" and any(
            outer_begin <= begin and end <= outer_end
            for outer_begin, outer_end in typedef_ranges
        ):
            continue
        name = node.get("name")
        if not name:
            anonymous_index += 1
            name = f"anonymous_{kind}_{anonymous_index}"
        key = (kind, name)
        semantic_nodes = [node]
        if kind == "TypedefDecl":
            semantic_nodes.extend(
                child
                for child, child_begin, child_end in candidates
                if child is not node
                and begin <= child_begin
                and child_end <= end
                and child.get("kind") in {"RecordDecl", "EnumDecl"}
            )
        records[key] = {
            "symbol": name,
            "kind": {
                "TypedefDecl": "typedef",
                "RecordDecl": "record",
                "EnumDecl": "enum",
            }[kind],
            "ast_kind": kind,
            "definition_range": {"begin": begin, "end": end},
            "ast_sha256": ast_hash(
                {"kind": "TypeUnit", "inner": semantic_nodes},
                ignore_types=False,
            ),
            "source_sha256": hashlib.sha256(source_bytes[begin:end]).hexdigest(),
        }
    return records


def compare_type_units(
    legacy: dict[tuple[str, str], dict[str, Any]],
    baseline: dict[tuple[str, str], dict[str, Any]],
) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for key in sorted(set(legacy) | set(baseline)):
        legacy_entry = legacy.get(key)
        baseline_entry = baseline.get(key)
        if legacy_entry is None:
            relation, owner = "removed", "none"
        elif baseline_entry is None:
            relation, owner = "new", "svistok"
        elif legacy_entry["ast_sha256"] == baseline_entry["ast_sha256"]:
            relation, owner = "equivalent", "upstream"
        else:
            relation, owner = "modified", "svistok"
        exemplar = legacy_entry or baseline_entry
        assert exemplar is not None
        records.append({
            "symbol": exemplar["symbol"],
            "kind": exemplar["kind"],
            "ast_kind": exemplar["ast_kind"],
            "body_relation": relation,
            "owner": owner,
            "legacy": legacy_entry,
            "baseline": baseline_entry,
        })
    return records


def annotate_declarations(
    declarations: dict[str, list[dict[str, Any]]], symbols: list[dict[str, Any]]
) -> dict[str, list[dict[str, Any]]]:
    definitions = {entry["symbol"]: entry for entry in symbols}
    for side, entries in declarations.items():
        other_side = "baseline" if side == "legacy" else "legacy"
        other = declarations.get(other_side, [])
        for entry in entries:
            definition = definitions.get(entry["symbol"])
            if definition is not None:
                entry["body_relation"] = definition["body_relation"]
                entry["owner"] = definition["owner"]
                continue
            counterparts = [item for item in other if item["symbol"] == entry["symbol"]]
            if any(item["source_sha256"] == entry["source_sha256"] for item in counterparts):
                relation, owner = "equivalent", "upstream"
            elif side == "legacy" and counterparts:
                relation, owner = "modified", "svistok"
            elif side == "legacy":
                relation, owner = "new", "svistok"
            else:
                relation, owner = "removed", "none"
            entry["body_relation"] = relation
            entry["owner"] = owner
    return declarations


def main_file_definitions(
    ast: dict[str, Any], source_path: Path, source_bytes: bytes
) -> dict[tuple[str, str], dict[str, Any]]:
    grouped = definitions_by_provenance(ast, source_path, source_path.parent)
    return grouped.get(source_path.name, {})


def definitions_for(root: Path, relative: str) -> dict[tuple[str, str], dict[str, Any]]:
    source = root / relative
    ast = dump_ast(source, FILE_ARGS.get(relative, ()))
    return main_file_definitions(ast, source, source.read_bytes())


def included_definitions_for(
    root: Path, relative: str
) -> dict[str, dict[tuple[str, str], dict[str, Any]]]:
    source = root / relative
    ast = dump_ast(source, FILE_ARGS.get(relative, ()))
    grouped = definitions_by_provenance(ast, source, root)
    return {
        path: definitions
        for path, definitions in grouped.items()
        if path != relative and path.endswith(".c")
    }


def compare_definitions(
    relative: str,
    legacy: dict[tuple[str, str], dict[str, Any]],
    baseline: dict[tuple[str, str], dict[str, Any]],
    *,
    baseline_exists: bool = True,
) -> dict[str, Any]:
    """Assign one final owner to every definition in a source pair."""
    symbols: list[dict[str, Any]] = []
    for key in sorted(set(legacy) | set(baseline)):
        legacy_entry = legacy.get(key)
        baseline_entry = baseline.get(key)
        if legacy_entry is None:
            relation = "removed"
            owner = "none"
        elif baseline_entry is None:
            relation = "new"
            owner = "svistok"
        elif legacy_entry["ast_sha256"] == baseline_entry["ast_sha256"]:
            relation = "equivalent"
            owner = "upstream"
        else:
            relation = "modified"
            owner = "svistok"
        exemplar = legacy_entry or baseline_entry
        assert exemplar is not None
        symbols.append({
            "symbol": exemplar["symbol"],
            "kind": exemplar["kind"],
            "linkage": exemplar["linkage"],
            "body_relation": relation,
            "owner": owner,
            "public_symbol": (
                exemplar["symbol"] if exemplar["linkage"] == "external" else None
            ),
            "legacy": legacy_entry,
            "baseline": baseline_entry,
        })
    by_name = {entry["symbol"]: entry for entry in symbols}
    bridge_consumers: dict[str, set[str]] = {}
    for entry in symbols:
        consumer_owner = entry["owner"]
        if consumer_owner not in {"upstream", "svistok"}:
            continue
        consumer_record = entry[
            "baseline" if consumer_owner == "upstream" else "legacy"
        ]
        if consumer_record is None:
            continue
        for dependency in consumer_record.get("dependencies", []):
            target = by_name.get(dependency)
            if target is None or target["linkage"] != "static":
                continue
            target_owner = target["owner"]
            if target_owner in {"upstream", "svistok"} and target_owner != consumer_owner:
                bridge_consumers.setdefault(dependency, set()).add(consumer_owner)
    bridges = [
        {
            "symbol": symbol,
            "owner": by_name[symbol]["owner"],
            "consumers": sorted(consumers),
        }
        for symbol, consumers in sorted(bridge_consumers.items())
    ]
    return {
        "legacy_file": relative,
        "baseline_file": relative if baseline_exists else None,
        "symbols": symbols,
        "bridges": bridges,
    }


def compare_macros(legacy: dict[str, str], baseline: dict[str, str]) -> list[dict[str, Any]]:
    records: list[dict[str, Any]] = []
    for name in sorted(set(legacy) | set(baseline)):
        legacy_definition = legacy.get(name)
        baseline_definition = baseline.get(name)
        if legacy_definition is None:
            relation, owner = "removed", "none"
        elif baseline_definition is None:
            relation, owner = "new", "svistok"
        elif legacy_definition == baseline_definition:
            relation, owner = "equivalent", "upstream"
        else:
            relation, owner = "modified", "svistok"
        records.append({
            "symbol": name,
            "kind": "macro",
            "body_relation": relation,
            "owner": owner,
            "legacy_definition": legacy_definition,
            "baseline_definition": baseline_definition,
        })
    return records


def definitions_from_path(source: Path, extra_args: Iterable[str] = ()) -> dict[tuple[str, str], dict[str, Any]]:
    ast = dump_ast(source, extra_args)
    return main_file_definitions(ast, source, source.read_bytes())


def build_file_manifest(relative: str) -> dict[str, Any]:
    legacy = definitions_for(LEGACY_ROOT, relative)
    baseline_path = BASELINE_ROOT / relative
    baseline = definitions_for(BASELINE_ROOT, relative) if baseline_path.is_file() else {}
    record = compare_definitions(
        relative,
        legacy,
        baseline,
        baseline_exists=baseline_path.is_file(),
    )
    legacy_macros = preprocess_macros(
        LEGACY_ROOT / relative, FILE_ARGS.get(relative, ())
    )
    baseline_macros = (
        preprocess_macros(baseline_path, FILE_ARGS.get(relative, ()))
        if baseline_path.is_file()
        else {}
    )
    record["macros"] = compare_macros(legacy_macros, baseline_macros)
    record["types"] = compare_type_units(
        type_units_for(LEGACY_ROOT, relative),
        type_units_for(BASELINE_ROOT, relative) if baseline_path.is_file() else {},
    )
    record["declarations"] = annotate_declarations({
        "legacy": declarations_for(LEGACY_ROOT, relative),
        "baseline": declarations_for(BASELINE_ROOT, relative)
        if baseline_path.is_file()
        else [],
    }, record["symbols"])
    if relative in MODIFIED_ROOTS:
        legacy_included = included_definitions_for(LEGACY_ROOT, relative)
        baseline_included = (
            included_definitions_for(BASELINE_ROOT, relative)
            if baseline_path.is_file()
            else {}
        )
        record["included_sources"] = [
            compare_definitions(
                path,
                legacy_included.get(path, {}),
                baseline_included.get(path, {}),
                baseline_exists=(BASELINE_ROOT / path).is_file(),
            )
            for path in sorted(set(legacy_included) | set(baseline_included))
        ]
    return record


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="*")
    parser.add_argument("--all-modified", action="store_true")
    parser.add_argument("--output", type=Path)
    arguments = parser.parse_args()
    try:
        files = list(arguments.files)
        if arguments.all_modified:
            files = [*MODIFIED_ROOTS, *MODIFIED_HEADERS]
        if not files:
            raise RuntimeError("no files selected")
        manifest = {
            "schema_version": 1,
            "generator": "clang-json-ast",
            "files": [build_file_manifest(path) for path in files],
        }
    except (OSError, RuntimeError) as error:
        print(f"manifest failed: {error}", file=sys.stderr)
        return 1
    rendered = json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    if arguments.output:
        arguments.output.write_text(rendered, encoding="utf-8")
    else:
        print(rendered, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
