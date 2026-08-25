#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


AT_PARSE_DRIVER = r'''
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "at_parse.h"

void ast_verb(int level, const char *format, ...)
{
    (void)level;
    (void)format;
}

int main(void)
{
    char cnum[] = "+CNUM: \"Subscriber Number\",\"+66812345678\",145";
    char cops[] = "+COPS: 0,0,\"AIS\",0";
    char spn[] = "^SPN:1,0,SIM-1";
    char cusd[] = "+CUSD: 0,\"100,00 THB\",15";
    char csca_line[] = "+CSCA: \"+66810000000\",145";
    char clcc[] = "+CLCC: 2,1,4,0,0,\"+66812345678\",145";
    char sysinfo[] = "^SYSINFO:1,2,3,4,5";
    char mode[] = "^MODE:5,7";
    char *text;
    unsigned call_idx, dir, state, call_mode, mpty, toa, call_class;
    int type, dcs, rssi, link_mode, submode;
    int srvst, srvd, roamst, sysmode, simst;

    printf("cnum|%s\n", at_parse_cnum(cnum));
    printf("cops|%s\n", at_parse_cops(cops));
    printf("spn|%s\n", at_parse_spn(spn));
    printf("cmti|%d|%d\n", at_parse_cmti("+CMTI: \"SM\",17"),
        at_parse_cmti("invalid"));
    printf("cusd|%d|", at_parse_cusd(cusd, &type, &text, &dcs));
    printf("%d|%s|%d\n", type, text, dcs);
    printf("csq|%d|", at_parse_csq("+CSQ:23,99", &rssi));
    printf("%d\n", rssi);
    printf("rssi|%d|%d\n", at_parse_rssi("^RSSI:19"), at_parse_rssi("bad"));
    printf("mode|%d|", at_parse_mode(mode, &link_mode, &submode));
    printf("%d|%d\n", link_mode, submode);
    printf("sysinfo|%d|", at_parse_sysinfo(sysinfo, &srvst, &srvd, &roamst, &sysmode, &simst));
    printf("%d|%d|%d|%d|%d\n", srvst, srvd, roamst, sysmode, simst);
    printf("csca|%d|", at_parse_csca(csca_line, &text));
    printf("%s\n", text);
    printf("clcc|%d|", at_parse_clcc(clcc, &call_idx, &dir, &state,
        &call_mode, &mpty, &text, &toa));
    printf("%u|%u|%u|%u|%u|%s|%u\n", call_idx, dir, state,
        call_mode, mpty, text, toa);
    printf("ccwa|%d|", at_parse_ccwa("+CCWA: \"+6681\",145,1", &call_class));
    printf("%u\n", call_class);
    return 0;
}
'''


class AtParseOracleTests(unittest.TestCase):
    def test_at_parse_vectors_match_frozen_legacy_output(self) -> None:
        output = compile_and_run(
            "at-parse",
            AT_PARSE_DRIVER,
            ("at_parse.c", "pdu.c", "char_conv.c", "memmem.c"),
            compiler_defines=(
                "-DCHAN_DONGLE_H_INCLUDED",
                "-DCHAN_DONGLE_HELPERS_H_INCLUDED",
            ),
            compiler_flags=(
                "-include",
                "asterisk_compat.h",
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
            CHARACTERIZATION_ROOT / "fixtures" / "at_parse" / "vectors.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
