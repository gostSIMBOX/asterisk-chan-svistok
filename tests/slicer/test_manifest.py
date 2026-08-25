#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]


def load_manifest_module():
    path = PROJECT_ROOT / "tools" / "clang_manifest.py"
    spec = importlib.util.spec_from_file_location("clang_manifest", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ManifestTests(unittest.TestCase):
    def test_checked_manifest_is_complete_and_has_no_unowned_live_entries(self) -> None:
        manifest_path = PROJECT_ROOT / "manifests" / "symbol-ownership.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        records = manifest["files"]

        expected_roots = set(load_manifest_module().MODIFIED_ROOTS)
        expected_headers = set(load_manifest_module().MODIFIED_HEADERS)
        self.assertEqual(28, len(records))
        self.assertEqual(
            expected_roots | expected_headers,
            {record["legacy_file"] for record in records},
        )

        symbols = [symbol for record in records for symbol in record["symbols"]]
        macros = [macro for record in records for macro in record["macros"]]
        types = [type_unit for record in records for type_unit in record.get("types", [])]
        bridges = [bridge for record in records for bridge in record["bridges"]]
        included = [
            included
            for record in records
            for included in record.get("included_sources", [])
        ]
        included_symbols = [
            symbol for record in included for symbol in record["symbols"]
        ]
        declarations = [
            declaration
            for record in records
            for side in record["declarations"].values()
            for declaration in side
        ]

        self.assertEqual(420, len(symbols))
        self.assertEqual(147, len(macros))
        self.assertEqual(44, len(types))
        self.assertEqual(
            {"equivalent": 30, "modified": 13, "new": 1},
            {
                relation: sum(unit["body_relation"] == relation for unit in types)
                for relation in ("equivalent", "modified", "new")
            },
        )
        self.assertEqual(81, len(bridges))
        self.assertEqual(139, len(included_symbols))
        self.assertEqual(222, len(declarations))
        self.assertEqual(
            {
                "dserial.c",
                "limits.c",
                "select.c",
                "simnode/adiscovery_core.c",
                "simnode/adiscovery_svistok.c",
                "programmator/crc.c",
                "programmator/ttyprog_core.c",
                "programmator/ttyprog_svistok.c",
                "share.c",
                "stat.c",
            },
            {record["legacy_file"] for record in included},
        )

        all_entries = symbols + macros + included_symbols
        self.assertTrue(
            all(
                entry["owner"] == "none"
                if entry["body_relation"] == "removed"
                else entry["owner"] in {"upstream", "svistok"}
                for entry in all_entries
            )
        )
        self.assertTrue(
            all(
                declaration["owner"] in {"upstream", "svistok", "none"}
                and declaration["body_relation"]
                in {"equivalent", "modified", "new", "removed"}
                for declaration in declarations
            )
        )
        self.assertTrue(
            all(
                entry["public_symbol"] is not None
                for entry in symbols + included_symbols
                if entry["linkage"] == "external"
                and entry["body_relation"] != "removed"
            )
        )
        self.assertTrue(
            all(
                entry["owner"] == "svistok" and entry["body_relation"] == "new"
                for entry in included_symbols
            )
        )

    def test_at_parse_definitions_receive_exactly_one_owner(self) -> None:
        module = load_manifest_module()
        record = module.build_file_manifest("at_parse.c")
        symbols = record["symbols"]
        names = {entry["symbol"] for entry in symbols}
        self.assertIn("at_parse_cusd", names)
        self.assertIn("at_parse_sysinfo", names)
        self.assertEqual(19, len(symbols))
        by_name = {entry["symbol"]: entry for entry in symbols}
        self.assertEqual("upstream", by_name["at_parse_cusd"]["owner"])
        self.assertEqual("upstream", by_name["at_parse_cmgr"]["owner"])
        self.assertEqual("svistok", by_name["at_parse_cpin"]["owner"])
        self.assertEqual("new", by_name["at_parse_sysinfo"]["body_relation"])
        self.assertEqual("static", by_name["mark_line"]["linkage"])
        self.assertTrue(all(entry["kind"] == "function" for entry in symbols))
        self.assertTrue(
            all(entry["owner"] in {"upstream", "svistok", "none"} for entry in symbols)
        )
        self.assertTrue(
            all(
                entry["owner"] == "none"
                if entry["body_relation"] == "removed"
                else entry["owner"] in {"upstream", "svistok"}
                for entry in symbols
            )
        )
        self.assertTrue(any(entry["owner"] == "upstream" for entry in symbols))
        self.assertTrue(any(entry["owner"] == "svistok" for entry in symbols))
        bridged = {entry["symbol"] for entry in record["bridges"]}
        self.assertIn("mark_line", bridged)
        self.assertEqual(
            "const char *",
            by_name["parse_cmgr_pdu"]["baseline"]["signature"]["return_type"],
        )


if __name__ == "__main__":
    unittest.main()
