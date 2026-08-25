#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


PROGRAMMER_DRIVER = r'''
#include <stdio.h>
#include <string.h>

unsigned short pppfcs16(unsigned short fcs, void *data, int length);
void crcdat(char *data, unsigned short length, char *result);

static void vector(const char *name, char *data, unsigned short length)
{
    unsigned char result[2] = {0};
    unsigned short raw = pppfcs16(0xffff, data, length);
    crcdat(data, length, (char *)result);
    printf("%s|%04X|%02X%02X\n", name, raw, result[0], result[1]);
}

int main(void)
{
    char empty[] = "";
    char ascii[] = "123456789";
    char diag[] = { 0x01, 0x73, 0x00, 0x00 };
    vector("empty", empty, 0);
    vector("ascii", ascii, (unsigned short)strlen(ascii));
    vector("diag", diag, sizeof(diag));
    return 0;
}
'''


class ProgrammerOracleTests(unittest.TestCase):
    def test_crc_vectors_match_frozen_legacy_output(self) -> None:
        output = compile_and_run(
            "programmer-crc",
            PROGRAMMER_DRIVER,
            ("programmator/crc.c",),
        )
        expected = (
            CHARACTERIZATION_ROOT / "fixtures" / "programmer" / "crc.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
