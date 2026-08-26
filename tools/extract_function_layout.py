#!/usr/bin/env python3
"""Generate the approved function/file layout in an isolated staging tree."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
from typing import Any


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"
LEGACY_ROOT = (PROJECT_ROOT / "../../legacy/asterisk-chan-svistok-v2014").resolve()
OWNERSHIP = PROJECT_ROOT / "manifests" / "symbol-ownership.json"
LAYOUT = PROJECT_ROOT / "manifests" / "function-layout.json"
DEFAULT_STAGE = PROJECT_ROOT / "build" / "staging-function-layout" / "src"
REPORT = PROJECT_ROOT / "manifests" / "function-layout-extraction.json"
HOOK_REPORT = PROJECT_ROOT / "manifests" / "hook-proxy-provenance.json"
BACKUP_SOURCE = PROJECT_ROOT / "build" / "function-layout-backup" / "src"
COLLAPSED_ROOTS = {"at_parse.c", "at_queue.c", "ringbuffer.c"}
REMOVED_ROOTS = COLLAPSED_ROOTS | {"cpvt.c"}


def hook_and_proxy(relative: str) -> tuple[str, str]:
    if relative == "at_parse.c":
        return (
            '''static void svistok_hook_log_cpin(char *str, size_t len)\n{\n\t(void)len;\n\tast_verb(3,"ATCPIN: %s",str);\n}\n''',
            '''extern int svistok_dongle_impl_at_parse_cpin(char *str, size_t len);\nEXPORT_DEF int at_parse_cpin(char *str, size_t len)\n{\n\tsvistok_hook_log_cpin(str, len);\n\treturn svistok_dongle_impl_at_parse_cpin(str, len);\n}\n''',
        )
    if relative == "at_queue.c":
        return (
            '''static void svistok_hook_log_at_write(struct pvt *pvt, const char *buf, size_t count)\n{\n\tchar dn[256];\n\ttimenow(dn);\n\tat_log(pvt,dn,strlen(dn));\n\tat_log(pvt," >> ",4);\n\tat_log(pvt,buf,count);\n}\n''',
            '''extern int svistok_dongle_impl_at_write(struct pvt *pvt, const char *buf, size_t count);\nEXPORT_DEF int at_write(struct pvt *pvt, const char *buf, size_t count)\n{\n\tsvistok_hook_log_at_write(pvt, buf, count);\n\treturn svistok_dongle_impl_at_write(pvt, buf, count);\n}\n''',
        )
    if relative == "at_response.c":
        hooks = '''
static void svistok_hook_persist_manufacturer(struct pvt *pvt) { putfiles("dongles/state",PVT_ID(pvt),"manufacturer",pvt->manufacturer); }
static void svistok_hook_persist_firmware(struct pvt *pvt) { putfiles("dongles/state",PVT_ID(pvt),"firmware",pvt->firmware); }
static void svistok_hook_persist_imei(struct pvt *pvt) { putfiles("dongles/state",PVT_ID(pvt),"imei",pvt->imei); }
static void svistok_hook_load_imsi_state(struct pvt *pvt) { readpvtinfo(pvt); writepvtinfo(pvt); readpvtlimits(pvt); writepvtlimits(pvt); }
static void svistok_hook_persist_provider(struct pvt *pvt) { putfiles("sim/state",pvt->imsi,"provider_name",pvt->provider_name); putfiles("dongles/state",PVT_ID(pvt),"operator",pvt->provider_name); }
static void svistok_hook_persist_csq(struct pvt *pvt) { putfilei("dongles/state",PVT_ID(pvt),"rssi",pvt->rssi); }
static void svistok_hook_persist_mode(struct pvt *pvt, int rv) { if(!rv) { putfilei("dongles/state",PVT_ID(pvt),"mode",pvt->linkmode); putfilei("dongles/state",PVT_ID(pvt),"submode",pvt->linksubmode); } at_enque_sysinfo(&pvt->sys_chan); }
static void svistok_hook_persist_rssi(struct pvt *pvt) { putfilei("dongles/state",PVT_ID(pvt),"rssi",pvt->rssi); }
'''
        proxy = '''
extern int svistok_dongle_impl_at_response_cgmi(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cgmr(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cgsn(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cimi(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_cops(struct pvt *, char *);
extern int svistok_dongle_impl_at_response_csq(struct pvt *, const char *);
extern int svistok_dongle_impl_at_response_mode(struct pvt *, char *, size_t);
extern int svistok_dongle_impl_at_response_rssi(struct pvt *, const char *);
static int at_response_cgmi(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cgmi(pvt,str); svistok_hook_persist_manufacturer(pvt); return rv; }
static int at_response_cgmr(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cgmr(pvt,str); svistok_hook_persist_firmware(pvt); return rv; }
static int at_response_cgsn(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cgsn(pvt,str); svistok_hook_persist_imei(pvt); return rv; }
static int at_response_cimi(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_cimi(pvt,str); svistok_hook_load_imsi_state(pvt); return rv; }
static int at_response_cops(struct pvt *pvt, char *str) { int rv=svistok_dongle_impl_at_response_cops(pvt,str); svistok_hook_persist_provider(pvt); return rv; }
static int at_response_csq(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_csq(pvt,str); if(!rv) svistok_hook_persist_csq(pvt); return rv; }
static int at_response_mode(struct pvt *pvt, char *str, size_t len) { int rv=svistok_dongle_impl_at_response_mode(pvt,str,len); svistok_hook_persist_mode(pvt,rv); return rv; }
static int at_response_rssi(struct pvt *pvt, const char *str) { int rv=svistok_dongle_impl_at_response_rssi(pvt,str); if(!rv) svistok_hook_persist_rssi(pvt); return rv; }
'''
        return hooks, proxy
    if relative == "chan_dongle.c":
        return (
            '''static void svistok_hook_initialize_svistok(void)\n{\n\tdserial_init();\n\tclear_state();\n\tputfiles("","svistok","version",svistok_version);\n\tIAXME_get();\n}\n''',
            '''extern int svistok_dongle_impl_load_module(void);\nstatic int load_module(void)\n{\n\tsvistok_hook_initialize_svistok();\n\treturn svistok_dongle_impl_load_module();\n}\n''',
        )
    if relative == "cli.c":
        return (
            '''static int32_t svistok_hook_normalize_acd(int32_t value)\n{\n\treturn value == -1 ? 0 : value;\n}\n''',
            '''extern int32_t svistok_dongle_impl_getACD(uint32_t calls, uint32_t duration);\nstatic int32_t getACD(uint32_t calls, uint32_t duration)\n{\n\treturn svistok_hook_normalize_acd(svistok_dongle_impl_getACD(calls, duration));\n}\n''',
        )
    if relative == "pdiscovery.c":
        return (
            '''static void svistok_hook_copy_serial(struct pdiscovery_result *dst, const struct pdiscovery_result *src)\n{\n\tif(src->serial) dst->serial=ast_strdup(src->serial);\n}\nstatic void svistok_hook_free_serial(struct pdiscovery_result *res)\n{\n\tif(res->serial) { ast_free(res->serial); res->serial=NULL; }\n}\n''',
            '''extern void svistok_dongle_impl_info_copy(struct pdiscovery_result *, const struct pdiscovery_result *);\nextern void svistok_dongle_impl_info_free(struct pdiscovery_result *);\nstatic void info_copy(struct pdiscovery_result *dst, const struct pdiscovery_result *src)\n{\n\tsvistok_dongle_impl_info_copy(dst,src);\n\tsvistok_hook_copy_serial(dst,src);\n}\nstatic void info_free(struct pdiscovery_result *res)\n{\n\tsvistok_dongle_impl_info_free(res);\n\tsvistok_hook_free_serial(res);\n}\n''',
        )
    raise KeyError(relative)


def declaration_text(symbol: str, signature: dict[str, Any]) -> bytes:
    parameters = [
        f'{item["type"]} {item["name"]}' for item in signature["parameters"]
    ]
    if signature.get("variadic"):
        parameters.append("...")
    if not parameters:
        parameters.append("void")
    return (
        f'EXPORT_DECL {signature["return_type"]} {symbol}'
        f'({", ".join(parameters)});'
    ).encode("latin-1")


def split_new_declarations(
    stage: Path, ownership: dict[str, Any], layout: dict[str, Any]
) -> dict[str, list[str]]:
    records = {record["legacy_file"]: record for record in ownership["files"]}
    new_external = [
        entry
        for entry in layout["functions"]
        if entry["layout_owner"] == "svistok-new"
        and entry["linkage"] == "external"
    ]
    module_by_symbol = {
        entry["symbol"]: entry["source_file"] for entry in new_external
    }
    declarations: dict[str, bytes] = {}
    header_declarations: dict[str, bytes] = {}
    declaration_locations: dict[str, str] = {}
    for record in ownership["files"]:
        legacy_bytes = (LEGACY_ROOT / record["legacy_file"]).read_bytes()
        for declaration in record.get("declarations", {}).get("legacy", []):
            if declaration["symbol"] not in {entry["symbol"] for entry in new_external}:
                continue
            source_range = declaration["declaration_range"]
            begin = int(source_range["begin"])
            end = int(source_range["end"])
            snippet = legacy_bytes[begin:end]
            if legacy_bytes[end:end + 1] == b";":
                snippet += b";"
            declarations[declaration["symbol"]] = snippet
            line_begin = legacy_bytes.rfind(b"\n", 0, begin) + 1
            prefix = legacy_bytes[line_begin:begin]
            header_declarations[declaration["symbol"]] = (
                b"EXPORT_DECL " + snippet
                if prefix.strip() in {b"EXPORT_DEF", b"EXPORT_DECL"}
                else snippet
            )
            declaration_locations[declaration["symbol"]] = record["legacy_file"]
    by_module: dict[str, list[tuple[str, bytes]]] = {}
    for entry in new_external:
        unit = next(
            unit
            for unit in records[entry["source_file"]]["symbols"]
            if unit["symbol"] == entry["symbol"]
        )
        text = header_declarations.get(entry["symbol"])
        if text is None:
            text = declaration_text(entry["symbol"], unit["legacy"]["signature"])
        by_module.setdefault(entry["source_file"], []).append((entry["symbol"], text))
    for symbol, relative in declaration_locations.items():
        candidates = [stage / relative, stage / "svistok" / relative]
        source_path = next((path for path in candidates if path.is_file()), None)
        if source_path is None:
            raise RuntimeError(f"{relative}:{symbol}: declaration owner is missing")
        source = source_path.read_bytes()
        snippet = declarations[symbol]
        begin = source.find(snippet)
        if begin < 0 or source.find(snippet, begin + 1) >= 0:
            raise RuntimeError(f"{relative}:{symbol}: non-unique declaration")
        end = begin + len(snippet)
        stem = Path(module_by_symbol[symbol]).stem
        line_begin = source.rfind(b"\n", 0, begin) + 1
        if source[line_begin:begin].strip() in {b"EXPORT_DEF", b"EXPORT_DECL"}:
            begin = line_begin
        replacement = b"\n"
        if not (stage / f"{stem}.h").is_file():
            replacement = f'#include "{stem}.h"\n'.encode("ascii")
        source_path.write_bytes(
            source[:begin] + replacement + source[end:]
        )
    report: dict[str, list[str]] = {}
    for relative, entries in sorted(by_module.items()):
        stem = Path(relative).stem
        header = stage / "svistok" / f"{stem}.h"
        guard = f"SVISTOK_{stem.upper()}_H_INCLUDED"
        header.write_bytes(
            f"#ifndef {guard}\n#define {guard}\n\n".encode("ascii")
            + b"\n".join(text for _, text in entries)
            + f"\n\n#endif /* {guard} */\n".encode("ascii")
        )
        compatibility = stage / f"{stem}.h"
        include = f'#include "svistok/{stem}.h"\n'.encode("ascii")
        if compatibility.is_file():
            source = compatibility.read_bytes()
            if include not in source:
                insertion = source.rfind(b"#endif")
                if insertion < 0:
                    raise RuntimeError(f"{relative}: compatibility header has no #endif")
                compatibility.write_bytes(source[:insertion] + include + source[insertion:])
        report[header.relative_to(stage).as_posix()] = [
            symbol for symbol, _ in entries
        ]
    # Collapsed C fragments moved one directory deeper. Keep their original
    # quoted includes routed to compatibility headers instead of accidentally
    # shadowing them with the new Svistok-only headers.
    for source_path in (stage / "svistok").glob("*.c"):
        source = source_path.read_bytes()
        updated = source
        for relative in by_module:
            stem = Path(relative).stem
            if not (stage / f"{stem}.h").is_file():
                continue
            updated = updated.replace(
                f'#include "{stem}.h"'.encode("ascii"),
                f'#include "../{stem}.h"'.encode("ascii"),
            )
        if updated != source:
            source_path.write_bytes(updated)
    return report


def extract(stage: Path, *, source_root: Path) -> dict[str, Any]:
    ownership = json.loads(OWNERSHIP.read_text())
    layout = json.loads(LAYOUT.read_text())
    records = {record["legacy_file"]: record for record in ownership["files"]}
    if stage.exists():
        shutil.rmtree(stage)
    shutil.copytree(source_root, stage)
    by_file: dict[str, list[dict[str, Any]]] = {}
    for entry in layout["functions"]:
        if entry["source_file"].endswith(".c"):
            by_file.setdefault(entry["source_file"], []).append(entry)
    report_files = []
    for relative, entries in sorted(by_file.items()):
        source_path = stage / relative
        source = source_path.read_bytes()
        record = records[relative]
        symbols = {unit["symbol"]: unit for unit in record["symbols"]}
        removals = []
        new_snippets = []
        for entry in entries:
            if entry["layout_owner"] == "svistok-inseparable":
                continue
            unit = symbols[entry["symbol"]]
            legacy_range = unit["legacy"]["definition_range"]
            snippet = (LEGACY_ROOT / relative).read_bytes()[
                legacy_range["begin"]:legacy_range["end"]
            ]
            begin = source.find(snippet)
            if begin < 0 or source.find(snippet, begin + 1) >= 0:
                raise RuntimeError(f"{relative}:{entry['symbol']}: non-unique source body")
            end = begin + len(snippet)
            line_begin = source.rfind(b"\n", 0, begin) + 1
            declaration_prefix = source[line_begin:begin]
            if declaration_prefix.strip() == b"EXPORT_DEF":
                begin = line_begin
            else:
                declaration_prefix = b""
            removals.append((begin, end, entry["symbol"]))
            if entry["layout_owner"] == "svistok-new":
                new_snippets.append((entry["symbol"], declaration_prefix + snippet))
        output = bytearray(source)
        for begin, end, _ in sorted(removals, reverse=True):
            output[begin:end] = b"\n"
        if removals:
            output = bytearray(bytes(output).rstrip(b" \t\r\n") + b"\n")
        source_path.write_bytes(bytes(output))
        if new_snippets:
            destination = stage / "svistok" / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(
                b"/* Svistok-only composition fragment. */\n\n"
                + b"\n\n".join(snippet for _, snippet in new_snippets)
                + b"\n"
            )
        if relative in {"at_parse.c", "at_queue.c", "at_response.c", "chan_dongle.c", "cli.c", "pdiscovery.c"}:
            hook, proxy = hook_and_proxy(relative)
            hook_path = stage / "svistok" / "hooks" / relative
            proxy_path = stage / "dongle" / relative
            hook_path.parent.mkdir(parents=True, exist_ok=True)
            proxy_path.parent.mkdir(parents=True, exist_ok=True)
            hook_path.write_text("/* Svistok hook composition fragment. */\n" + hook, encoding="latin-1")
            proxy_path.write_text("/* Dongle proxy-only composition fragment. */\n" + proxy, encoding="latin-1")
        if relative in COLLAPSED_ROOTS:
            destination = stage / "svistok" / relative
            destination.write_bytes(bytes(output) + b"\n" + destination.read_bytes())
            source_path.unlink()
        elif relative == "cpvt.c":
            source_path.unlink()
        report_files.append({
            "file": relative,
            "removed": [symbol for _, _, symbol in removals],
            "new": [symbol for symbol, _ in new_snippets],
            "root_sha256": hashlib.sha256(bytes(output)).hexdigest(),
            "root_removed": relative in REMOVED_ROOTS,
        })
    header_layout = split_new_declarations(stage, ownership, layout)
    hook_files = sorted((stage / "svistok/hooks").glob("*.c"))
    proxy_files = sorted((stage / "dongle").glob("*.c"))
    return {
        "schema_version": 1,
        "source_root": str(source_root),
        "stage": str(stage),
        "files": report_files,
        "hook_proxy": {
            "hooks": {
                path.relative_to(stage).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in hook_files
            },
            "proxies": {
                path.relative_to(stage).as_posix(): hashlib.sha256(path.read_bytes()).hexdigest()
                for path in proxy_files
            },
        },
        "new_declaration_headers": header_layout,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage", type=Path, default=DEFAULT_STAGE)
    parser.add_argument("--report", type=Path, default=REPORT)
    parser.add_argument(
        "--source-root",
        type=Path,
        default=BACKUP_SOURCE if BACKUP_SOURCE.is_dir() else SRC_ROOT,
    )
    arguments = parser.parse_args()
    report = extract(
        arguments.stage.resolve(), source_root=arguments.source_root.resolve()
    )
    arguments.report.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    HOOK_REPORT.write_text(
        json.dumps(
            {"schema_version": 1, **report["hook_proxy"]},
            indent=2,
            sort_keys=True,
        ) + "\n"
    )
    print(f"staged {len(report['files'])} root modules at {report['stage']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
