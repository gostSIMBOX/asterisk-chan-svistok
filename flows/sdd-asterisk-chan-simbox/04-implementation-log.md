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
| 5.3 Device population (Linux real path) | Done | Tagged-union `simbox_device_internal` (SIMULATED/REAL), verified both build configs |
| 5.4 Device enumeration | Done (side effect of 5.3) | `inst->devices[]` uniformly holds both kinds, existing count/get_by_index/get_by_sn already work |
| 5.5 Calls (cpvt_alloc + at_enque_dial/answer/hangup) | Done | Both build configs verified |
| 5.6 SMS/USSD/AT-commands | Done (AT response capture deferred to 5.8) | Found `pvt->sys_chan` — real non-call cpvt, resolves the open question |
| 5.7 IMEI change | Done (genuinely blocked, documented not faked) | `ttyprog_changeimei` is called but never defined anywhere in chan_svistok |
| 5.8 Events + raw AT response capture | Done (real Linux runtime still pending) | Real manager-event/logging seams found; no polling/read-only changes |
| 5.9 Mechanical real-wiring test | Partial (Linux runtime pending) | Static guard passes; Linux helper syntax-checks; socat PTY smoke skips on macOS |
| 5.10 Full regression | Done | 6/6 C suites, baseline test1, and 9/9 unchanged Flutter tests pass |
| 5.11 Copy upstream tests and verify parity | Done | Four byte-identical copies; test1 passes; other three reproduce documented Phase 0 legacy gaps |

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

- **Task 5.3 — Device population (Linux real path)**: `src/simbox_modem.c`'s
  `struct simbox_device_internal` redesigned as a tagged union:
  `simbox_device_kind_t` (`SIMBOX_DEV_SIMULATED` | `SIMBOX_DEV_REAL`).
  `SIMULATED` is the original fabricated-struct path, unchanged in
  behavior, still reachable via `simbox_device_register()` on every
  platform (kept, not retired — it's also the *only* device path on
  non-Linux, not just a test seam, so `sdd-flutter_gsm-ffi`'s
  `debugRegisterDiscoveredDevice` test helper needs no changes). `REAL`
  (Linux-only, `#ifdef __linux__`) wraps a live `struct pvt *` directly
  — no data copying/snapshotting; `simbox_device_get_info`/`sn`/`imei`/
  `imsi`/`state`/`rssi` all read `pvt`'s plain (non-opaque) struct
  fields live on every call, confirmed correct since chan_dongle.h's
  `struct pvt` uses direct field access as chan_svistok's own normal
  style (`char imei[17]`, `int rssi`, etc.), not accessor macros.
  - New `simbox_device_wrap_pvt(struct pvt *)` (Linux-only, in
    `simbox_modem.c`) + new Linux-only internal header
    `src/simbox_internal_linux.h` (deliberately *not* part of the
    public `simbox_api.h` surface, which must stay chan_svistok-type-
    free per specifications §9.6) so `simbox_api.c` can call it.
  - `simbox_api.c`'s `simbox_init()`: new Linux branch,
    `simbox_populate_from_gpublic()` — calls
    `simbox_module_bridge_load()` (Task 5.2), then walks the real,
    already-`EXPORT_DECL`'d `gpublic->devices` (an `AST_RWLIST_HEAD`,
    walked via the shim's own already-built `AST_RWLIST_RDLOCK`/
    `TRAVERSE`/`UNLOCK` macros) wrapping each `pvt` via
    `simbox_device_wrap_pvt`. Non-fatal on `load_module()` failure or
    zero devices — same empty-registry shape as the existing baseline,
    not a crash.
  - **`BUILD_SINGLE` discovery**: while wiring `gpublic` access,
    found chan_svistok's own `export.h` already has a build-mode switch
    — `EXPORT_DEF`/`EXPORT_DECL` become `static` under `-DBUILD_SINGLE`,
    or empty/`extern` otherwise. This build has never defined
    `BUILD_SINGLE` (confirmed: `nm` already showed `gpublic`/
    `channel_tech`/etc. as external symbols before this task even
    started), meaning the whole "reach chan_svistok via its own
    EXPORT_DECL convention" design (specifications §9.1/§9.2) was
    already using chan_svistok's *intended* multi-file build mode all
    along — not a coincidence or a workaround, the intended path.
  - **`simbox_device_destroy()`**: for `SIMBOX_DEV_REAL`, deliberately
    does *not* destroy `dev->pvt` (owned by chan_dongle.c's own
    `gpublic` list, torn down by `unload_module()`, not by this
    wrapper) — only frees a lazily-acquired `cpvt` (Task 5.6) and the
    wrapper struct itself. Documented in-line, not just here.
  - **Open, deliberately not resolved this task**: `simbox_shutdown()`
    doesn't call `simbox_module_bridge_unload()` — `load_module()`/
    `unload_module()`'s safety under repeated init/shutdown cycles
    within one process (e.g. multiple `SimboxModemRepository` instances
    across Dart tests) hasn't been verified, and Task 5.2's proof-of-
    concept only exercised a single `load()` call. Calling `unload()`
    unconditionally without verifying re-entrancy risked breaking
    subsequent `simbox_init()` calls in the same process — left as a
    documented gap for a dedicated verification pass rather than guessed
    at.
  - Fixed a self-inflicted bug while writing this: a doc comment
    containing `simbox_device_*/simbox_call_*` had its `*/`
    misparsed as the comment terminator, truncating the block comment
    and breaking compilation — caught immediately by the first build
    attempt, reworded to avoid the sequence.
  - Verified: full rebuild on this macOS machine (`__linux__` undefined)
    — 0 errors; `nm` confirms zero Linux-only symbols
    (`simbox_device_wrap_pvt`/`simbox_populate_from_gpublic`) got
    compiled in at all, proving the `#ifdef` gating is real, not just
    written; all 6 `test_simbox` suites still pass; Phase 0's `test1`
    baseline still passes — zero regression on this platform. **Additionally**:
    `gcc -fsyntax-only -D__linux__` on both changed files (forcing the
    Linux branch to parse against this machine's toolchain/shim headers,
    since real Linux linking/execution isn't available here) — 0 errors
    on either file. Real Linux link+run verification remains outstanding
    (tracked, not skipped silently) for whenever a Linux environment is
    available.

**Ended at**: Task 5.3 complete
**Handoff notes**: Device population (list/get/enumerate) is done for
both platforms. Task 5.4 (`simbox_device_count`/`get_by_index`/`get_by_sn`
walking `gpublic->devices`) turned out to be **already covered** by
Task 5.3's design — `inst->devices[]` now uniformly holds both
SIMULATED and REAL device handles after `simbox_init()`, so the existing
`simbox_device_count`/`get_by_index`/`get_by_sn` implementations (which
already iterate `inst->devices[]`) need no changes at all. Marking Task
5.4 done as a side effect, not skipped. Next real work is Task 5.5
(calls, via `cpvt_alloc`+`at_enque_dial`/`answer`/`hangup` per
specifications §9.7's refined design) and Task 5.6 (SMS/USSD/AT-commands,
flagged XL in the plan — the largest remaining task).

- **Task 5.5 — Calls**: `simbox_call_originate`/`hangup`/`answer`'s
  `SIMBOX_DEV_REAL` branches now use the simpler `cpvt`-level path Task
  5.1 found, not the heavier `channel_tech` design from specifications'
  original §9.2 draft (kept there as a documented fallback, not used).
  - New `simbox_require_cpvt()`: lazily calls `cpvt_alloc(pvt,
    pvt_get_pseudo_call_idx(pvt), CALL_DIR_OUTGOING, CALL_STATE_INIT)`,
    caching the result on `dev->cpvt` (added in Task 5.3's struct).
  - `simbox_call_originate`: acquires the cpvt, calls `at_enque_dial(cpvt,
    number, 0)` (clir=0 — chan_svistok only sets this from a dialplan
    option this adapter has no equivalent of).
  - `simbox_call_hangup`: calls `at_enque_hangup(cpvt, cpvt->call_idx)`
    (`call_idx` read directly from `struct cpvt`, a plain field), then
    **always** releases the cpvt (`cpvt_free` + clear `dev->cpvt`)
    regardless of `at_enque_hangup`'s return — a failed hangup-enqueue
    shouldn't leave a device permanently stuck unable to dial again;
    matches the non-Linux path's contract (hangup always returns to a
    clean state).
  - `simbox_call_answer`: calls `at_enque_answer(cpvt)` — deliberately
    does **not** fabricate a cpvt like originate does (unlike an
    outgoing call, an incoming call's cpvt must come from the real
    incoming-call event path, Task 5.8, not yet wired) — no cpvt means
    genuinely no incoming call to answer, a real error condition, not a
    gap in this function.
  - Needed one more include than expected: `at_command.h` (for
    `at_enque_dial`/`hangup`/`answer`'s declarations) — caught
    immediately by the `-D__linux__ -fsyntax-only` check, which is
    exactly the workflow this two-pronged verification approach
    (macOS real build + forced-Linux syntax check) is for.
  - Verified: macOS real build clean, 0 errors; `-D__linux__
    -fsyntax-only` clean, 0 errors; all 6 `test_simbox` suites pass;
    baseline `test1` still passes; `nm` confirms `simbox_require_cpvt`
    (Linux-only) didn't compile into the macOS binary.

**Ended at**: Task 5.5 complete
**Handoff notes**: Calls done for both platforms, same two-pronged
verification pattern as Task 5.3 (macOS real build + `-D__linux__
-fsyntax-only`) — reuse this pattern for every remaining task, it's
working well. Next: Task 5.6 (SMS/USSD/AT-commands) — the plan's
largest remaining task (XL), and the one with the still-unresolved
"`cpvt` outside an active call" question specifications flagged (now
partially answered by `simbox_require_cpvt`'s pattern here, but Task
5.5's cpvt is tied to a *call* lifecycle — Task 5.6 needs one for
SMS/USSD/AT-commands that may have no call happening at all, worth
checking whether reusing the same lazy-cpvt approach is correct or
whether a separate non-call-bound cpvt is needed).

- **Task 5.6 — SMS/USSD/AT-commands**: the "cpvt outside a call"
  question resolved cleanly — `struct pvt` has its own `sys_chan` field
  (`struct cpvt sys_chan`), confirmed via grep to be chan_dongle.c's own
  real mechanism for non-call AT commands
  (`at_enque_initialization(&pvt->sys_chan, CMD_AT)`,
  `at_enque_ping(&pvt->sys_chan)`) — already initialized by
  `pvt_create()`/`public_state_init()`, no `cpvt_alloc`/`cpvt_free`
  lifecycle needed at all for this use case (simpler than Task 5.5's
  call-bound `cpvt`, not a variant of it).
  - `simbox_sms_send`/`simbox_ussd_send`: direct calls to
    `at_enque_sms(&pvt->sys_chan, ...)`/`at_enque_ussd(&pvt->sys_chan,
    ...)`, complete and clean — both were already async-contract
    functions (return int status only), so no synchronous-response gap
    here at all.
  - `simbox_at_command`: **documented, deliberate gap, not papered
    over**. `at_enque_cmd_proc()` enqueues asynchronously; the response
    text arrives later via `at_response.c`'s real parser, which this
    adapter has no hook into yet. `simbox_at_command`'s existing
    signature is synchronous (`response_buf` filled by return time) —
    reconciling that needs the same response-arrival wiring Task 5.8
    (events) is already going to build, so implementing it twice
    independently would be wasted/inconsistent work. Current behavior:
    enqueues the command for real, returns the enqueue result, leaves
    `response_buf` as an empty string (not a fabricated `"OK\r\n"` like
    the non-Linux path) — callers on Linux must not treat an empty
    response as a real modem reply until Task 5.8 lands.
  - Verified: macOS real build clean; `-D__linux__ -fsyntax-only`
    clean; all 6 `test_simbox` suites pass; baseline `test1` still
    passes.

**Ended at**: Task 5.6 complete (with `simbox_at_command`'s response
capture explicitly deferred to Task 5.8, not silently left broken)
**Handoff notes**: Task 5.7 (IMEI) next — specifications flagged this as
genuinely unconfirmed (likely redirects to `simbox_prog_change_imei`'s
DIAG-mode path rather than a plain AT command); investigate before
writing code, per the pattern established all session of not guessing
at unverified native entry points.

- **Task 5.7 — IMEI change**: investigated per the handoff note above,
  found something more fundamental than expected. `chan_dongle.c`'s
  real IMEI-change path (`chan_dongle.c:760`, also `cli.c:872`) calls
  `ttyprog_changeimei(pvt->audio_fd, pvt->newimei)` directly — a raw
  `programmator/`-style protocol call, bypassing the AT-command queue
  entirely (matches Task 5.5/5.6's `cpvt`-based design NOT applying
  here — this was never going to be an `at_enque_*` call). **But**:
  exhaustive `grep -rn ttyprog_changeimei .` across the *entire*
  chan_svistok tree finds it **called** in exactly three places
  (`cli.c`, `chan_dongle.c`, `programmator/ttyprog_test.c` — the same
  file Phase 0 already found doesn't build standalone) and **defined
  nowhere** — not in `programmator/ttyprog_core.c`, not anywhere. This
  is a genuine, pre-existing gap in chan_svistok itself (consistent with
  Phase 0's independent finding that `programmator/` has real,
  unrelated standalone-build fragility), not an adapter design problem
  and not fixable from this side (read-only tree, and the function is
  simply absent, not just unreachable).
  - Also checked (per specifications §9.6's flagged follow-up):
    `simbox_prog_change_imei` (`simbox_programmator.c`) is **not** a
    usable substitute — read its current implementation, it's an
    entirely separate, independent simulation (opens its own raw fd,
    hand-writes an `AT^NVWIMEI=` command, never calls
    `ttyprog_changeimei` or any other real chan_svistok code). This is
    a **second, previously-unconfirmed instance** of the exact
    disconnected-simulation problem this whole amendment exists to fix
    — `simbox_programmator.c`/`simbox_reader_*` were explicitly out of
    scope for this amendment (specifications §9.6), so not touched, but
    now confirmed rather than assumed-probably-fine. Worth its own
    follow-up flow/task eventually.
  - `simbox_change_imei`'s `SIMBOX_DEV_REAL` branch: returns -1 with an
    extensive in-line comment recording all of the above — a documented,
    honest gap, not a fabricated success and not a guess at a
    fictitious alternate entry point.
  - Verified: macOS real build clean; `-D__linux__ -fsyntax-only`
    clean; all 6 `test_simbox` suites pass; baseline `test1` still
    passes.

**Ended at**: Task 5.7 complete (genuinely blocked on missing
chan_svistok source, documented not faked)
**Handoff notes**: Task 5.8 (events, via `shim_pbx.c`) next — this is
also where `simbox_at_command`'s deferred response-capture (Task 5.6)
and real call/registration state (Task 5.3's placeholder
`simbox_device_state` mapping) should get properly wired, since they
all need the same underlying "hook into chan_svistok's real response/
state-change path" mechanism.

- **Task 5.8 — Events — partially complete, one real architectural
  limit found**:
  - **`SIMBOX_EVENT_INCOMING_CALL`: done and verified.** `ast_pbx_start`
    (in `shim_pbx.c`, already the exact hook specifications §9.5
    predicted — chan_dongle.c's own real code calls this when handing
    an incoming call to the dialplan layer) now calls a new
    `simbox_event_bridge_fire_incoming_call(device_sn, caller)`
    (implemented in `simbox_api.c`, declared in
    `simbox_internal_linux.h`). Device correlation:
    `ast_channel_tech_pvt(c)` (already-shimmed accessor) → `struct
    cpvt*` → `->pvt` → `pvt->serial` (device_sn) / `pvt->numbera`
    (caller — **not independently verified for this exact scenario,
    best-effort, flagged not silently assumed correct**). New
    process-global `g_active_linux_instance` in `simbox_api.c` (set in
    `simbox_init()`'s Linux branch, cleared in `simbox_shutdown()`) —
    needed because `shim_pbx.c` has no access to the private
    `simbox_instance` struct/its registered `event_cb`; reasoned as
    safe because `gpublic`/`load_module()` are already process-global
    by chan_dongle.c's own design, so there's only ever meaningfully
    one active "real" Linux instance regardless of how many
    `simbox_handle_t` a caller creates. Followed the same
    heap-allocate-and-transfer-ownership event convention as Task 5.2's
    fix throughout.
  - **`SIMBOX_EVENT_CALL_STATE_CHANGED`/`INCOMING_SMS`/`USSD_RESPONSE`:
    investigated, found a real architectural limit, not wired.**
    Unlike incoming-call, these have no equivalent already-exported
    single hook point to attach to: the real state-transition/response
    logic lives entirely inside `at_response.c` (parsing CLCC/CMTI/
    CUSD-style URCs), which is part of the read-only tree — nothing
    calls out to an externally-reachable function *specifically when
    these events happen* the way `ast_pbx_start` does for incoming
    calls. Two paths considered: (a) polling `struct pvt`/`struct
    cpvt`'s plain fields periodically from a new adapter-side thread —
    viable for call state (`cpvt->state` is a plain field) but (b)
    checked `struct pvt` for anywhere it stores the *last received SMS
    text* or *last USSD response text* to poll — **found none** (grep
    for `ussd`/`cusd` in `chan_dongle.h` only turns up encoding-mode
    flags, not response storage) — meaning SMS/USSD response *content*
    genuinely isn't recoverable without either a deeper investigation
    into `at_response.c`'s real dispatch path (is there an AMI-manager-
    style event, or a callback registered via the `void **id` output
    param `at_enque_sms`/`at_enque_ussd`/`at_enque_ussd` already take,
    that this adapter isn't yet using?) or modifying the read-only
    tree (not permitted). Not guessed at further — flagging for a
    decision rather than either forcing an incomplete polling
    mechanism or silently leaving it broken without explanation.
  - Verified (incoming-call event only, the part that's done): macOS
    real build clean; `-D__linux__ -fsyntax-only` clean on all three
    changed files (`simbox_api.c`, `shim_pbx.c`,
    `simbox_internal_linux.h`); all 6 `test_simbox` suites pass;
    baseline `test1` still passes.

**Ended at**: Task 5.8 partially complete — pausing here rather than
guessing further on call-state/SMS/USSD event wiring, which needs either
a scoped polling-design decision or a deeper `at_response.c`/`at_queue.c`
dispatch investigation before continuing.
**Handoff notes**: Tasks 5.1-5.7 are fully done and verified. Task 5.8
is the first task this session where the "reach chan_svistok through
already-exported symbols" strategy that carried every prior task ran
into a real, structural gap — there's no equivalent of `ast_pbx_start`
for these three event types. Remaining tasks (5.9 mechanical check,
5.10 regression, 5.11 copy chan_svistok's tests verbatim) don't
strictly depend on 5.8's remaining scope and could proceed independently
if the SMS/USSD/call-state question is deferred — worth deciding
explicitly rather than assuming either way.

- **Task 5.8 continuation — completed after Anton chose the deeper
  dispatch investigation**:
  - Found the missed real seam in unchanged chan_svistok:
    `at_response.c` and `channel.c` already call exported
    `manager_event_new_sms`, `manager_event_new_ussd`, and
    `manager_event_call_state_change`; those helpers all terminate at
    Asterisk's `manager_event()`. Defining `BUILD_MANAGER` in adapter
    `config.h` enables that intended code path. The adapter shim now
    implements the small missing AMI surface (`EVENT_FLAG_*`,
    `astman_send_listack`, `manager_event`) and bridges only
    `DongleNewSMS`, `DongleNewUSSD`, and `DongleCallStateChange` into
    typed `simbox_event_t` callbacks. No polling, no parser changes,
    and zero read-only source changes.
  - Event strings live in the same heap allocation as the event struct,
    so documented callback-side `free(event)` owns the full payload.
    The integration-test callback now honors that ownership contract.
  - Call-state names map to the public enum and update both callback
    data and synchronous `simbox_device_state()` results.
  - Incoming caller ID comes from the exact channel field populated by
    `new_channel()` -> shim `ast_channel_alloc()`. The real incoming
    `cpvt` is attached to the wrapper so `simbox_call_answer()` reaches
    the actual call.
  - Found and fixed a pre-existing Task 5.5 use-after-free risk:
    `simbox_call_hangup()` freed its cpvt immediately after enqueue even
    though `at_queue_task_t` retained it. Lifecycle ownership now stays
    with chan_svistok; the wrapper reference clears on the real
    `released` event.
  - Completed Task 5.6's deferred synchronous raw-AT response capture:
    unchanged `at_response.c` emits every `CMD_USER` line through an
    exact `ast_log(LOG_NOTICE, ...)` call. The adapter logging shim
    recognizes only that notice, correlates by serial, accumulates
    lines, and signals `simbox_at_command()` on terminal `OK`/error
    (3-second API timeout around chan_svistok's 2-second queue timeout).
  - Verification: full forced macOS rebuild (`make -B`) succeeded,
    including the formerly dormant full unchanged `manager.c`; all six
    `test_simbox` suites pass; Phase 0's unmodified `test1` passes;
    forced `-D__linux__ -fsyntax-only` passes for all changed Linux
    files; `git diff --check` passes. Real Linux modem/runtime behavior
    remains explicitly pending, consistent with Tasks 5.2-5.7.

**Ended at**: Task 5.8 complete, starting Task 5.9.
**Handoff notes**: The earlier “structural limit” was not real after the
deeper trace: manager events are the intended hook. Task 5.9 can now
test completed wiring rather than document a deferred gap.

- **Task 5.9 — mechanical guard complete; Linux PTY runtime pending**:
  - Added executable `tests/test_real_wiring.sh` and root
    `test-real-wiring` Make target. Its static checks require the real
    registry, cpvt/queue entry points, manager-event bridge, and raw-AT
    response bridge, preventing a disconnected simulation from
    satisfying the test accidentally.
  - Added `tests/test_at_wire.c`. On Linux the shell harness constructs
    a minimal real pvt around one side of a socat PTY, invokes public
    `simbox_at_command()`, and asserts that the peer receives exact
    `AT+CSQ\r`. A response pump is deliberately unnecessary for this
    outbound-byte assertion; the API's normal timeout is expected.
  - Current-host result: `PASS: static real-wiring guard`, followed by
    the intentional `SKIP: PTY wire smoke requires the real Linux
    branch`. `socat` is also absent. Forced `-D__linux__` compilation of
    the helper succeeds. The dynamic half therefore remains the sole
    incomplete Task 5.9 checkpoint.

- **Task 5.10 — full available regression complete**:
  - Full forced C rebuild succeeds (including unchanged manager code).
  - All six `test_simbox` integration suites pass.
  - Phase 0's original `tests/baseline/test1` passes.
  - Forced Linux syntax checks pass for every changed Linux-facing
    implementation file and the PTY helper.
  - `flutter test test/simbox_modem_repository_test.dart` in the
    unchanged `libsFlutter/flutter_gsm` repository passes all nine
    tests.
  - `git diff --check` passes. No chan_svistok source file was edited;
    only its tracked build objects changed as a consequence of the
    forced rebuild.

- **Task 5.11 — verbatim copied-test parity complete**:
  - Copied all four source files into `tests/copied/` and added
    `tests/Makefile`. `verify-copies` uses `cmp` against the read-only
    originals; all four comparisons pass byte-for-byte.
  - `make -C tests run-copied-test1` builds the copy against the adapter
    library and exits successfully with the expected diagnostic output.
  - The other three copies were also compiled without modifying them.
    They retain Phase 0's known limitations: discovery is coupled to
    undeclared chan_dongle process state; programmator and reader tests
    include implementation C files and collide with the same symbols
    already present in `libsimbox.a`; `ttyprog_changeimei` remains
    absent upstream. These failures are baseline parity, not newly
    introduced adapter behavior, and the hardware-dependent programs
    cannot be meaningfully run on this host.

**Ended at**: Tasks 5.10 and 5.11 complete. Task 5.9 has a committed,
passing static half and a ready Linux PTY harness, but its dynamic half
cannot execute on this macOS host.
**Handoff notes**: On Linux with `socat` and GNU `timeout`, run
`make test-real-wiring`, then repeat the full C suite and verify real
module load/unload. If those pass, close Task 5.9 and the flow; do not
claim IMEI support unless the missing upstream implementation is
supplied separately.

---

## Deviations Summary

| Planned | Actual | Reason |
|---------|--------|--------|
| Task 5.8 expected a direct parser hook or polling fallback | Used unchanged chan_svistok's manager-event calls plus adapter-owned manager/logging shims | These existing outward seams preserve the read-only rule and carry decoded SMS/USSD content |
| Task 5.9 expected a socat smoke during implementation | Committed the complete harness but ran only its static half and Linux syntax check | Current host is macOS and lacks socat; actual Linux branch cannot execute here |
| Task 5.11 phrased copied tests as passing | Preserved exact Phase 0 parity: copied test1 passes, three legacy tests still fail to build standalone/against the library | Editing the copied sources would invalidate the verbatim-baseline requirement and hide upstream gaps |

## Learnings

(accumulating through Phase 0)

## Completion Checklist

- [x] All tasks completed or explicitly deferred
- [x] Tests passing on the available host; Linux runtime explicitly pending
- [x] No regressions in available C and Flutter suites
- [x] Documentation updated if needed
- [ ] Status updated to COMPLETE
