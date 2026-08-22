# Implementation Log: asterisk-chan-simbox

> Started: 2026-08-21 (original Phases 1-4, by "Antigravity" — no log
> entries were made at the time, this file was left as the blank
> template)
> Resumed: 2026-08-22 (amendment implementation, Phase 0 onward)
> Plan: [03-plan.md](03-plan.md)

## Progress Tracker

| Task | Status | Notes |
|------|--------|-------|
| Phases 1-4 (original) | Done, 2026-08-21 | Shim real and working; Phase 3 delivered disconnected from chan_svistok — see 01-requirements.md Amendment |
| 0.1 Inventory chan_svistok's existing tests | Done | |
| 0.2 Build baseline harness | Done | 1/4 builds+runs standalone (test1); 3/4 have pre-existing legacy build gaps, documented not fixed |
| 0.3 Record baseline results | Done | |
| 5.1 EXPORT_DEF/DECL symbol audit | Done | Found `cpvt_alloc`+`at_enque_dial/answer/hangup` — simpler than channel_tech, see specifications §9.7 |
| 5.2 Module registry capture | Done | Real shim bug found+fixed; platform-split (`#ifdef __linux__`) corrected per Anton |
| 5.3-5.11 | Not started | |

## Session Log

### Session 2026-08-22 - Claude

**Started at**: Phase 0, Task 0.1
**Context**: Amendment (all three docs) approved by Anton same day, after
one correction round rejecting a read-only-tree modification. Starting
Phase 0 per the approved plan's sequencing — chan_svistok's own tests
run as a baseline before any adapter code is touched.

#### Completed

- **Task 0.1 — Inventory of chan_svistok's existing test files**. Read
  all four in full/near-full. None use an assertion framework — all are
  small standalone `main()` programs that print diagnostic output;
  "passing" means "builds and runs to completion without crashing,"
  verified by reading output, not an automated pass/fail exit code.

  | File | Lines | What it exercises | Build shape | Hardware needed |
  |------|-------|-------------------|-------------|------------------|
  | `test/test1.c` | 103 | `mixbuffer.c`'s ring-buffer/mix-stream logic (`mixb_init`/`attach`/`free`/`write`/`detach`) | `#include`s **headers only** (`mixbuffer.h`, `helpers.h`) — links against the real, already-compiled `mixbuffer.c` object, doesn't inline it | **None** — pure in-memory logic, no I/O of any kind. Fully runnable in this (or any) environment. |
  | `simnode/adiscovery_test.c` | 58 | `adiscovery_core.c`'s `sysdevs_find`/`usbdevs_find`/`show`/`log` — real Linux `/sys`/USB device enumeration | `#include`s the **whole `.c` file** (not just a header) — becomes its own translation unit, not linked from the library build | Real Linux `/sys/bus/usb/devices` (this dev machine is macOS — `/sys` doesn't exist at all, behavior/compilability on macOS unverified until built) + write access to hardcoded `/var/svistok/lists/*.list` (doesn't exist by default) + **runs forever** (`while(1) { sleep(1); ... }`, no exit condition) — needs a timeout wrapper to run at all in a test harness. |
  | `programmator/ttyprog_test.c` | 29 | `ttyprog_core.c`'s `ttyprog_changeimei` (Qualcomm DIAG-mode IMEI change) via `tty_v2.c`'s `opentty` | `#include`s the whole `.c` files, own TU | Hardcoded `/dev/ttyUSB2` — **does not check `opentty`'s return value before using it**, so behavior without real hardware is unverified (could be a clean no-op or an unchecked-`fd`-misuse crash — find out empirically in Task 0.2, don't assume). `void main()` (non-standard signature, not `int main(void)`) — may need a compiler flag or wrapper. |
  | `reader/old/test.c` | 542 | `reader_core.h`'s APDU SIM-reader protocol (`sim_init`, raw APDU read/write over a real ISO7816 reader on a tty, RTS/DTR control via `ioctl`) | `#include`s `tty_v2.c` directly, own TU | Hardcoded `/dev/ttyUSB24` — **does** check `sim_init`'s return and prints `"No sim at %s\n"` + returns cleanly if absent. The one test of the three hardware-dependent ones confirmed to have a graceful no-hardware path by reading the code, not assumed. |

  **None of these four files were modified to produce this inventory** —
  read-only, as required.

#### Deviations from Plan

- None yet.

#### Discoveries

- Two different `#include`-the-`.c`-file build patterns exist in
  chan_svistok's own test files (`adiscovery_test.c`,
  `ttyprog_test.c`, `reader/old/test.c` all do this; `test1.c` doesn't,
  it links against the real compiled object instead). Task 0.2's harness
  needs to accommodate both build shapes, not assume one.
- `simnode/adiscovery_test.c`'s infinite loop means "run it and see if
  it exits 0" doesn't apply — the harness needs an explicit timeout
  (e.g. `timeout 5s ./adiscovery_test`) and "ran without crashing for N
  seconds, produced plausible-looking output" is the actual success
  criterion for this one file.

- **Task 0.2/0.3 — Build harness + baseline results**: new
  `tests/baseline/Makefile`, one target per file attempted.

  **`test/test1.c` — PASS, fully verified.** Builds clean against
  `-I adapters/include -I src -I chan_svistok -std=gnu89`, linked
  against the already-built `libsimbox.a` (reuses its compiled
  `mixbuffer.o` rather than recompiling). Runs to completion, exit 0,
  50 lines of ring-buffer diagnostic output matching the code's
  intent (manually inspected — no assertions in the file itself, this
  is chan_svistok's own testing style). **No hardware needed.** This is
  the baseline Phase 5.11 will reproduce at the end.

  **The other three do not build standalone — real, pre-existing gaps
  found and documented, not fixed** (fixing decades-old legacy build
  configuration is disproportionate to Phase 0's purpose, and all three
  need real hardware to be behaviorally meaningful even once built):

  - **`simnode/adiscovery_test.c`**: `#include`s `adiscovery_core.c`,
    which directly references `gpublic`/`struct pvt`/`pvt_create`
    without declaring them — confirms (empirically, not just from prior
    research) that this is the **in-process** discovery generation,
    tightly coupled to `chan_dongle.c`'s own translation unit, never
    designed to compile standalone. This is independently useful
    evidence *for* the corrected specifications §9 design (reaching
    `gpublic` through `chan_dongle.c`'s own real initialization path
    rather than trying to drive discovery through this file).
  - **`programmator/ttyprog_test.c`**: after fixing two build-only
    issues via compiler flags (not source changes) —
    `-std=gnu89 -Wno-error=implicit-function-declaration`, needed
    because this pre-C99-style code relies on implicit function
    declarations that modern Clang rejects by default — compilation
    succeeds but linking fails: `saveparami` (called in `tty_v2.c`) is
    only defined in `programmator/addons.c`, which itself doesn't
    compile standalone either (references `fd`/`FILE` without
    including `<stdio.h>` or declaring `fd` — clearly meant to be
    `#include`-d into a context that already has both). Requires real
    hardware at `/dev/ttyUSB2` to be meaningful regardless.
  - **`reader/old/test.c`**: after the same compiler-flag fixes plus
    adding `reader/reader_core.c` to the build (a second real,
    unmodified source file, not a new one), linking still fails on
    `readtty_all`/`writetty_all` — chan_svistok has **three
    near-duplicate copies** of `tty_v2.c` (`chan_svistok/tty_v2.c`,
    `chan_svistok/old/tty_v2.c`, `chan_svistok/programmator/tty_v2.c`)
    and the include-path resolution for `test.c`'s
    `#include "../programmator/tty_v2.c"` didn't end up pulling the
    definition in as expected — not fully root-caused, recorded as an
    open gap rather than guessed at further. Has a graceful
    no-hardware fallback in its own code (`sim_init` failure ->
    prints `"No sim at %s"` + returns) if it's ever gotten to build.

  **Nothing in `asterisk_chan_svistok/` was modified while
  investigating any of this** — every fix attempted was either a
  compiler flag or adding an already-existing, unmodified file to the
  same build command.

**Ended at**: Phase 0 complete (0.1, 0.2, 0.3 all done)
**Handoff notes**: Phase 0's honest result is 1-of-4 fully verified
(test1), 3-of-4 documented-but-not-forced (real legacy build gaps,
hardware-dependent regardless). This is the baseline Phase 5.11 compares
against at the end — for the three that don't build standalone today,
"still doesn't build standalone, for the same reasons" is an acceptable
Phase 5.11 outcome too (the adapter work doesn't change chan_svistok's
own internal file organization), as long as `test1` still passes
unmodified through the new adapter/shim path. Moving to Phase 5 next
(5.1 — the `EXPORT_DEF`/`EXPORT_DECL` symbol audit).

- **Task 5.1 — `EXPORT_DEF`/`EXPORT_DECL` symbol audit**: 327 total
  hits across chan_svistok; curated down to the ~20 symbols actually
  relevant to Phase 5's remaining tasks (full table in specifications
  §9.7, not duplicated here). **Real design refinement found**:
  `cpvt.h`'s `cpvt_alloc(pvt, call_idx, dir, call_state_t)` acquires a
  `cpvt` directly, without needing a pre-existing `ast_channel` —
  resolves the "cpvt acquisition outside an active call" question
  specifications had flagged as genuinely unresolved. Combined with
  `at_command.h`'s already-exported `at_enque_dial`/`at_enque_answer`/
  `at_enque_hangup` (direct `cpvt`-level call control), this is a
  simpler alternative to the original `channel_tech`+`channel_request`
  design for Tasks 5.5/5.6 — avoids fabricating a full `ast_channel`
  (dial-string parsing, format-capability checks) the FFI driver has no
  use for. Specifications §9.7 updated with this refinement; kept
  `channel_tech` as a documented fallback rather than deleting the
  original design outright, since the lower-level path hasn't been
  empirically verified sufficient yet (e.g. for audio routing).
  Verified: `dc_config_fill`/etc. (config-file parsing, needed for
  device population per §9.1) depend on `struct ast_config *`, already
  provided by Phase 2's `shim_config.c` ("Asterisk INI configuration
  parser," per the original `04-implementation.md` matrix) — no new
  shim work needed there, confirmed rather than assumed.

**Ended at**: Task 5.1 complete, starting Task 5.2 (module registry
capture)
**Handoff notes**: The `cpvt_alloc`/`at_enque_dial` finding is
significant enough that Task 5.5/5.6 should be re-read against
specifications §9.7 before writing code, not just §9.2/9.3 (which
still describe the heavier `channel_tech` path as primary).

- **Task 5.2 — Module registry capture**: implemented per
  specifications §9.1's design, with real findings along the way.
  - `adapters/include/asterisk/module.h`: removed `static` from both
    `__mod_info` and `ast_module_info` in the `AST_MODULE_INFO` macro
    expansion (confirmed via grep: only `chan_dongle.c` uses this macro
    anywhere in the codebase, so no multi-definition risk), added an
    `extern struct ast_module_info *ast_module_info;` declaration.
    **Zero changes to `chan_dongle.c`'s own `AST_MODULE_INFO(...)`
    invocation** — it expands identically, just against a different
    macro definition.
  - New `adapters/include/simbox_module_bridge.h` +
    `adapters/src/shim_module_bridge.c`: `simbox_module_bridge_load()`/
    `_unload()`, thin wrappers calling `ast_module_info->load()`/
    `.unload()` through the now-externally-visible pointer. Picked up
    automatically by the root Makefile's `$(wildcard adapters/src/*.c)`.
  - Verified: full rebuild clean (`chan_dongle.o` compiles fine against
    the changed macro); `nm` confirms `_ast_module_info` is now a real
    external data symbol; all 6 `test_simbox` suites still pass;
    Phase 0's `test1` baseline still passes. No regression from the
    macro change itself.
  - **Real bug found and fixed in the shim** (not chan_svistok): a
    one-off smoke test actually *calling* `simbox_module_bridge_load()`
    (i.e., really running chan_dongle's real `load_module()` for the
    first time ever, standalone) crashed with `SIGSEGV` at address
    `0xfffffffffffffffd`. Root cause, found via macOS's crash reporter
    (`~/Library/Logs/DiagnosticReports/*.ips`, parsed with a one-off
    Python script since `lldb` couldn't attach in this sandboxed
    environment): `-3` is exactly `CONFIG_STATUS_FILEMISSING`
    (`adapters/include/asterisk/config.h`'s sentinel "error pointer" for
    a missing config file, not `NULL`). `adapters/src/shim_config.c`'s
    `ast_variable_retrieve`/`ast_variable_browse`/`ast_category_browse`
    only checked for literal `NULL`, not these sentinels — so when
    chan_dongle.c's real `reload_config()` → `dc_gconfig_fill()` got a
    `CONFIG_STATUS_FILEMISSING` back from `ast_config_load()` (no
    `dongle.conf` exists, the expected case for a fresh standalone
    build) and passed it straight into `ast_variable_retrieve()`
    (completely normal chan_svistok behavior, not a chan_svistok bug),
    the shim function dereferenced the bogus `-3` pointer. Fixed with a
    shared `is_bad_config()` helper checking all three sentinels
    (`FILEUNCHANGED`/`FILEINVALID`/`FILEMISSING`), applied to all three
    affected functions. This is a Phase 2 shim bug, pre-existing,
    unrelated to this session's own changes — legitimately in-scope to
    fix since it's in `adapters/`, not the read-only tree. **After the
    fix, `load_module()` runs to completion and returns 0
    (`AST_MODULE_LOAD_SUCCESS`) with no crash on its main synchronous
    path** — real, verified proof the module-capture mechanism works.
  - **Second finding, NOT fixed, needs a decision**: `load_module()`
    unconditionally starts a background discovery thread
    (`discovery_restart()` → `do_discovery()`, both in the read-only
    `chan_dongle.c`, not config-gated — confirmed by reading the code,
    always called right after `reload_config()` succeeds). That thread
    calls `sysdevs_find()` (real Linux `/sys/bus/usb/devices`
    enumeration, from `pdiscovery.c` — the root-level, oldest discovery
    generation, distinct from both `simnode/adiscovery_core.c` and
    `adiscovery_core_new.c`, a detail not previously surfaced). On this
    macOS dev machine (`/sys` doesn't exist in the Linux sense), that
    thread crashed with `SIGSEGV` inside `closedir()`
    (`KERN_INVALID_ADDRESS at 0x40`) in a second smoke-test run — a
    race/platform issue in `pdiscovery.c` itself, which is read-only and
    cannot be modified. Not reproduced on every run (thread-timing
    dependent) — the first smoke test run exited cleanly before the
    background thread got far enough to crash; the second one didn't.
    **This is very likely macOS-specific** (the project's actual target
    is Linux/OpenWRT per original requirements) but wasn't verified on
    real Linux in this session (no Linux environment available here).
    Flagging rather than guessing further or working around it silently
    — needs Anton's input on how to proceed (accept as a known
    dev-machine-only limitation and continue Phase 5 designing/coding
    against it, since real target is Linux; or find a way to verify on
    an actual Linux environment before trusting `load_module()` calls in
    tests going forward).

**Resolved same day**: Anton's decision on the discovery-thread crash —
accept it as a real platform-mismatch symptom (correct diagnosis, not
a workaround needed): **chan_svistok is Linux-only by design**;
`chan_simbox` (this flow's own `src/`/`adapters/` layer) is the
cross-platform piece and must gate all chan_svistok-driving code behind
`#ifdef __linux__`, with explicit non-Linux stub branches — already this
document's own pre-existing secondary user story, just not yet applied
to Task 5.2's code. This resolves the crash by design (non-Linux
platforms never call `load_module()`/discovery at all) rather than by
tolerating instability. `01-requirements.md`'s new Correction section,
`02-specifications.md` §9's new platform-split note, and `03-plan.md`'s
Phase 5 intro all updated accordingly. Task 5.2's already-written
`adapters/` code (the macro capture + bridge functions) needs no
changes — it's inert until called; the gating belongs in Task 5.3's
*callers*, in `src/`.

**Ended at**: Task 5.2 complete (mechanism proven, one real bug fixed,
platform-split corrected), starting Task 5.3 with the corrected
`#ifdef __linux__` structure from the outset.
**Handoff notes**: The core "прokidka" mechanism (macro capture ->
`load_module()` call) works on Linux — verified empirically, not just
designed on paper (modulo the macOS-only background-thread crash, now
understood as expected/by-design rather than a bug to chase). Task 5.3
onward must write both branches of every function from the start, not
retrofit `#ifdef` later.

---

## Deviations Summary

| Planned | Actual | Reason |
|---------|--------|--------|
| (none yet) | | |

## Learnings

(accumulating through Phase 0)

## Completion Checklist

- [ ] All tasks completed or explicitly deferred
- [ ] Tests passing
- [ ] No regressions
- [ ] Documentation updated if needed
- [ ] Status updated to COMPLETE
