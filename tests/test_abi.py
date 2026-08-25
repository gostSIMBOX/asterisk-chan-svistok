#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import shutil
import subprocess
import unittest


PROJECT_ROOT = Path(__file__).resolve().parents[1]
STUB_ROOT = PROJECT_ROOT / "tests" / "characterization" / "stubs"
SOURCE = PROJECT_ROOT / "tests" / "test_abi.c"
BUILD_ROOT = PROJECT_ROOT / "build" / "abi"


class AbiTests(unittest.TestCase):
    def compile(self, context: str, *extra: str) -> subprocess.CompletedProcess[str]:
        BUILD_ROOT.mkdir(parents=True, exist_ok=True)
        compiler = shutil.which("clang") or shutil.which("cc")
        self.assertIsNotNone(compiler)
        return subprocess.run(
            [
                str(compiler),
                "-std=gnu89",
                "-Wno-invalid-pp-token",
                f"-DSVISTOK_ABI_CONTEXT_{context}=1",
                *extra,
                "-I",
                str(STUB_ROOT),
                "-I",
                str(PROJECT_ROOT / "src"),
                "-I",
                str(PROJECT_ROOT),
                "-c",
                str(SOURCE),
                "-o",
                str(BUILD_ROOT / f"{context.lower()}.o"),
            ],
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

    def test_canonical_abi_compiles_in_upstream_slice_context(self) -> None:
        completed = self.compile("UPSTREAM")
        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_canonical_abi_compiles_in_svistok_slice_context(self) -> None:
        completed = self.compile("SVISTOK")
        self.assertEqual(0, completed.returncode, completed.stderr)

    def test_deliberately_altered_enum_contract_fails(self) -> None:
        completed = self.compile("ALTERED", "-DSVISTOK_ABI_EXPECT_CALL_STATE_WAITING=6")
        self.assertNotEqual(0, completed.returncode)
        self.assertIn("svistok_abi_assert_call_state_waiting", completed.stderr)


if __name__ == "__main__":
    unittest.main()
