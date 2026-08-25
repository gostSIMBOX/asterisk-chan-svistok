#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


PDU_DRIVER = r'''
#include <stdio.h>
#include <string.h>
#include "pdu.h"

static void build_case(const char *name, const char *sca, const char *dst,
    const char *message, unsigned validity, int report)
{
    char output[1024] = {0};
    int length = pdu_build(output, sizeof(output), sca, dst, message, validity, report);
    printf("%s|%d|%s\n", name, length, length >= 0 ? output : "<error>");
}

int main(void)
{
    printf("digits|%d|%d|%d|%d|%d\n",
        pdu_digit2code('0'), pdu_digit2code('9'), pdu_digit2code('*'),
        pdu_digit2code('#'), pdu_digit2code('A'));
    build_case("ascii", "+1234567890", "+15551234567", "hello", 60, 1);
    build_case("ucs2", "+1234567890", "+66812345678", "Привет", 1440, 0);
    build_case("empty-sca", "", "12345", "A", 0, 0);
    build_case("small-output", "", "12345", "0123456789", 5, 1);
    return 0;
}
'''


class PduOracleTests(unittest.TestCase):
    def test_pdu_build_vectors_match_frozen_legacy_output(self) -> None:
        output = compile_and_run(
            "pdu-build",
            PDU_DRIVER,
            ("pdu.c", "char_conv.c"),
            compiler_defines=("-DCHAN_DONGLE_HELPERS_H_INCLUDED",),
            compiler_flags=(
                "-include",
                "stdio.h",
                "-include",
                "stddef.h",
                "-include",
                "string.h",
            ),
            linker_flags=("-liconv",),
        )
        expected = (
            CHARACTERIZATION_ROOT / "fixtures" / "pdu" / "build-vectors.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
