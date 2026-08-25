#!/usr/bin/env python3

from __future__ import annotations

from pathlib import Path
import sys
import unittest


CHARACTERIZATION_ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(CHARACTERIZATION_ROOT))

from harness import compile_and_run  # noqa: E402


STATE_DRIVER = r'''
#include "stat_compat.h"
#include <stdio.h>
#include <string.h>

static time_t fake_time;
static int integer_writes;
static int long_writes;
static int temp_calls;
static int final_calls;
static int final_duration;
char IAXME1[256] = "normalized-server";

time_t time(time_t *result)
{
    if (result) *result = fake_time;
    return fake_time;
}

void putfilei(const char *a, const char *b, const char *c, long value)
{
    (void)a; (void)b; (void)c; (void)value;
    integer_writes++;
}

void putfilel(const char *a, const char *b, const char *c, long value)
{
    (void)a; (void)b; (void)c; (void)value;
    long_writes++;
}

void putfiles(const char *a, const char *b, const char *c, const char *value)
{
    (void)a; (void)b; (void)c; (void)value;
    integer_writes++;
}

void getfilel_def(const char *a, const char *b, const char *c,
    long *value, long fallback)
{
    (void)a; (void)b; (void)c; (void)fallback;
    *value = 40;
}

void getfiles_def(const char *a, const char *b, const char *c,
    char *value, const char *fallback)
{
    (void)a; (void)b; (void)c;
    strcpy(value, fallback);
}

void limits_temp(struct pvt *pvt) { (void)pvt; temp_calls++; }
void limits_final(struct pvt *pvt, int duration)
{
    (void)pvt;
    final_calls++;
    final_duration = duration;
}
void timenow(char *value) { strcpy(value, "normalized-time"); }
void datenow(char *value) { strcpy(value, "normalized-date"); }
void ast_verb(int level, const char *format, ...)
{
    (void)level; (void)format;
}

static void print_state(const char *label, const struct pvt *pvt)
{
    printf("%s|sf=%ld|start=%ld|response=%ld|connected=%ld|fas=%ld|pddc=%ld|saved=%ld|end=%ld|writes=%d,%d|limits=%d,%d,%d\n",
        label,
        pvt->stat.stat_call_sf,
        pvt->stat.stat_call_start,
        pvt->stat.stat_call_response,
        pvt->stat.stat_call_connected,
        pvt->stat.stat_call_fas,
        pvt->stat.stat_call_pddc,
        pvt->stat.stat_call_saved,
        pvt->stat.stat_call_end,
        integer_writes, long_writes,
        temp_calls, final_calls, final_duration);
}

int main(void)
{
    struct pvt pvt;
    memset(&pvt, 0, sizeof(pvt));
    pvt.id = "oracle";
    strcpy(pvt.imsi, "00101");
    pvt.stat.billing_pay = 1;
    strcpy(pvt.stat.billing_direction, "AB");

    fake_time = 1000;
    v_stat_call_start(&pvt);
    print_state("start", &pvt);

    fake_time = 1010;
    v_stat_call_response(&pvt);
    fake_time = 1011;
    v_stat_call_response(&pvt);
    v_stat_call_pddc(&pvt);
    v_stat_call_fas(&pvt);
    print_state("response", &pvt);

    fake_time = 1020;
    v_stat_call_connected(&pvt);
    print_state("connected", &pvt);

    fake_time = 1027;
    v_stat_call_process(&pvt);
    print_state("process", &pvt);

    fake_time = 1030;
    v_stat_call_end(&pvt, 10);
    print_state("end", &pvt);
    return 0;
}
'''


class StateOracleTests(unittest.TestCase):
    def test_call_state_transitions_match_frozen_legacy_output(self) -> None:
        output = compile_and_run(
            "stat-state",
            STATE_DRIVER,
            ("stat.c",),
            compiler_flags=(
                "-include",
                str(CHARACTERIZATION_ROOT / "stubs" / "stat_compat.h"),
            ),
        )
        expected = (
            CHARACTERIZATION_ROOT / "fixtures" / "state" / "stat.txt"
        ).read_text(encoding="utf-8")
        self.assertEqual(expected, output)


if __name__ == "__main__":
    unittest.main()
