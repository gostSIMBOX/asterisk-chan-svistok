#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
from pathlib import Path
import shutil
import subprocess
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[2]
FIXTURES = Path(__file__).resolve().parent / "fixtures"
BUILD_ROOT = PROJECT_ROOT / "build" / "slicer" / "bridges"


def load_tool(name: str):
    path = PROJECT_ROOT / "tools" / f"{name}.py"
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class BridgeTests(unittest.TestCase):
    def test_static_calls_in_both_directions_share_one_mutable_datum(self) -> None:
        manifest_tool = load_tool("clang_manifest")
        slicer = load_tool("slice_translation_unit")
        bridge_tool = load_tool("generate_bridges")
        baseline_path = FIXTURES / "bridge-baseline.c"
        legacy_path = FIXTURES / "bridge-legacy.c"
        record = manifest_tool.compare_definitions(
            "bridge.c",
            manifest_tool.definitions_from_path(legacy_path),
            manifest_tool.definitions_from_path(baseline_path),
        )
        record["bridges"] = [
            {"symbol": "upstream_helper", "owner": "upstream", "consumers": ["svistok"]},
            {"symbol": "overlay_helper", "owner": "svistok", "consumers": ["upstream"]},
            {"symbol": "shared_counter", "owner": "svistok", "consumers": ["upstream"]},
            {"symbol": "shared_values", "owner": "upstream", "consumers": ["svistok"]},
            {"symbol": "variadic_sum", "owner": "upstream", "consumers": ["svistok"]},
        ]

        if BUILD_ROOT.exists():
            shutil.rmtree(BUILD_ROOT)
        BUILD_ROOT.mkdir(parents=True)
        generated = []
        for side, source_path in (("upstream", baseline_path), ("svistok", legacy_path)):
            prefix, suffix = bridge_tool.generate_for_side(
                record["symbols"], record["bridges"], side
            )
            sliced = slicer.emit_slice(
                source_path.read_bytes(),
                record["symbols"],
                side=side,
                display_path=source_path.name,
                bridges=record["bridges"],
            ).decode("utf-8")
            output = BUILD_ROOT / f"{side}.c"
            output.write_text(prefix + sliced + suffix, encoding="utf-8")
            generated.append(output)
        driver = BUILD_ROOT / "driver.c"
        driver.write_text(
            "#include <stdio.h>\n"
            "int upstream_call(int); int svistok_call(int); int read_counter(void); "
            "int overlay_variadic_call(void);\n"
            "int main(void) { int a = upstream_call(2); int b = svistok_call(2); "
            "int c = read_counter(); int d = overlay_variadic_call(); "
            "printf(\"%d|%d|%d|%d\\n\", a, b, c, d); return 0; }\n",
            encoding="utf-8",
        )
        compiler = shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler)
        executable = BUILD_ROOT / "bridge-test"
        completed = subprocess.run(
            [str(compiler), *(str(path) for path in generated), str(driver), "-o", str(executable)],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        self.assertEqual(0, completed.returncode, completed.stderr)
        result = subprocess.run(
            [str(executable)], check=True, stdout=subprocess.PIPE, text=True
        )
        self.assertEqual("22|9|11|9\n", result.stdout)


if __name__ == "__main__":
    unittest.main()
