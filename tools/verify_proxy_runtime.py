#!/usr/bin/env python3
"""Compile and execute the real proxy fragments with instrumented entries."""

from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SRC_ROOT = PROJECT_ROOT / "src"


def render(src_root: Path) -> str:
    includes = "\n".join(
        f'#include "{(src_root / "dongle" / name).resolve().as_posix()}"'
        for name in (
            "at_parse.c", "at_queue.c", "at_response.c",
            "chan_dongle.c", "cli.c", "pdiscovery.c",
        )
    )
    return f'''#include <stdint.h>
#include <stddef.h>
#include <string.h>
#define EXPORT_DEF
struct pvt {{ int unused; }};
struct pdiscovery_result {{ int unused; }};
static char trace_log[32];
static int trace_at;
static int baseline_return;
static void trace(char value) {{ trace_log[trace_at++]=value; trace_log[trace_at]=0; }}
static void reset_trace(void) {{ trace_at=0; trace_log[0]=0; }}
static int same(const char *value) {{ return strcmp(trace_log,value)==0; }}

static void svistok_hook_log_cpin(char *s, size_t n) {{ (void)s; (void)n; trace('H'); }}
static void svistok_hook_log_at_write(struct pvt *p, const char *s, size_t n) {{ (void)p; (void)s; (void)n; trace('H'); }}
static void svistok_hook_persist_manufacturer(struct pvt *p) {{ (void)p; trace('H'); }}
static void svistok_hook_persist_firmware(struct pvt *p) {{ (void)p; trace('H'); }}
static void svistok_hook_persist_imei(struct pvt *p) {{ (void)p; trace('H'); }}
static void svistok_hook_load_imsi_state(struct pvt *p) {{ (void)p; trace('H'); }}
static void svistok_hook_persist_provider(struct pvt *p) {{ (void)p; trace('H'); }}
static void svistok_hook_persist_csq(struct pvt *p) {{ (void)p; trace('H'); }}
static void svistok_hook_persist_mode(struct pvt *p, int rv) {{ (void)p; (void)rv; trace('H'); }}
static void svistok_hook_persist_rssi(struct pvt *p) {{ (void)p; trace('H'); }}
static void svistok_hook_initialize_svistok(void) {{ trace('H'); }}
static int32_t svistok_hook_normalize_acd(int32_t value) {{ trace('H'); return value == -1 ? 0 : value; }}
static void svistok_hook_copy_serial(struct pdiscovery_result *d, const struct pdiscovery_result *s) {{ (void)d; (void)s; trace('H'); }}
static void svistok_hook_free_serial(struct pdiscovery_result *r) {{ (void)r; trace('H'); }}

int svistok_dongle_impl_at_parse_cpin(char *s, size_t n) {{ (void)s; (void)n; trace('B'); return baseline_return; }}
int svistok_dongle_impl_at_write(struct pvt *p, const char *s, size_t n) {{ (void)p; (void)s; (void)n; trace('B'); return baseline_return; }}
#define RESPONSE_IMPL(name, type) int svistok_dongle_impl_##name(struct pvt *p, type s) {{ (void)p; (void)s; trace('B'); return baseline_return; }}
RESPONSE_IMPL(at_response_cgmi, const char *)
RESPONSE_IMPL(at_response_cgmr, const char *)
RESPONSE_IMPL(at_response_cgsn, const char *)
RESPONSE_IMPL(at_response_cimi, const char *)
RESPONSE_IMPL(at_response_cops, char *)
RESPONSE_IMPL(at_response_csq, const char *)
RESPONSE_IMPL(at_response_rssi, const char *)
int svistok_dongle_impl_at_response_mode(struct pvt *p, char *s, size_t n) {{ (void)p; (void)s; (void)n; trace('B'); return baseline_return; }}
int svistok_dongle_impl_load_module(void) {{ trace('B'); return baseline_return; }}
int32_t svistok_dongle_impl_getACD(uint32_t c, uint32_t d) {{ (void)c; (void)d; trace('B'); return baseline_return; }}
void svistok_dongle_impl_info_copy(struct pdiscovery_result *d, const struct pdiscovery_result *s) {{ (void)d; (void)s; trace('B'); }}
void svistok_dongle_impl_info_free(struct pdiscovery_result *r) {{ (void)r; trace('B'); }}

{includes}

#define CHECK(call, expected_trace, expected_return) do {{ reset_trace(); if ((call)!=(expected_return) || !same(expected_trace)) return __LINE__; }} while(0)
#define CHECK_VOID(call, expected_trace) do {{ reset_trace(); call; if (!same(expected_trace)) return __LINE__; }} while(0)
int main(void)
{{
    struct pvt p; struct pdiscovery_result d, s; char text[]="x";
    baseline_return=0;
    CHECK(at_parse_cpin(text,1),"HB",0);
    CHECK(at_write(&p,text,1),"HB",0);
    CHECK(at_response_cgmi(&p,text),"BH",0);
    CHECK(at_response_cgmr(&p,text),"BH",0);
    CHECK(at_response_cgsn(&p,text),"BH",0);
    CHECK(at_response_cimi(&p,text),"BH",0);
    CHECK(at_response_cops(&p,text),"BH",0);
    CHECK(at_response_csq(&p,text),"BH",0);
    CHECK(at_response_mode(&p,text,1),"BH",0);
    CHECK(at_response_rssi(&p,text),"BH",0);
    CHECK(load_module(),"HB",0);
    CHECK(getACD(1,1),"BH",0);
    CHECK_VOID(info_copy(&d,&s),"BH");
    CHECK_VOID(info_free(&d),"BH");
    baseline_return=-7;
    CHECK(at_parse_cpin(text,1),"HB",-7);
    CHECK(at_write(&p,text,1),"HB",-7);
    CHECK(at_response_cgmi(&p,text),"BH",-7);
    CHECK(at_response_cgmr(&p,text),"BH",-7);
    CHECK(at_response_cgsn(&p,text),"BH",-7);
    CHECK(at_response_cimi(&p,text),"BH",-7);
    CHECK(at_response_cops(&p,text),"BH",-7);
    CHECK(at_response_csq(&p,text),"B",-7);
    CHECK(at_response_mode(&p,text,1),"BH",-7);
    CHECK(at_response_rssi(&p,text),"B",-7);
    CHECK(load_module(),"HB",-7);
    CHECK(getACD(1,1),"BH",-7);
    baseline_return=-1;
    CHECK(getACD(1,1),"BH",0);
    return 0;
}}
'''


def verify(src_root: Path) -> None:
    compiler = shutil.which("clang") or shutil.which("cc")
    if compiler is None:
        raise RuntimeError("C compiler not found")
    build = PROJECT_ROOT / "build" / "proxy-runtime"
    build.mkdir(parents=True, exist_ok=True)
    source = build / "proxy-runtime.c"
    executable = build / "proxy-runtime"
    source.write_text(render(src_root), encoding="utf-8")
    completed = subprocess.run(
        [compiler, "-std=gnu89", str(source), "-o", str(executable)],
        cwd=PROJECT_ROOT, text=True, capture_output=True, check=False,
    )
    if completed.returncode:
        raise RuntimeError("proxy harness compile failed:\n" + completed.stderr)
    completed = subprocess.run(
        [str(executable)], cwd=PROJECT_ROOT, text=True, capture_output=True, check=False
    )
    if completed.returncode:
        raise RuntimeError(
            f"proxy harness failed at assertion line {completed.returncode}: "
            + completed.stderr
        )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--src-root", type=Path, default=SRC_ROOT)
    arguments = parser.parse_args()
    try:
        verify(arguments.src_root.resolve())
    except (OSError, RuntimeError) as error:
        print(f"proxy runtime verification failed: {error}", file=sys.stderr)
        return 1
    print("proxy runtime passed: 14 wrappers, success/error ordering")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
