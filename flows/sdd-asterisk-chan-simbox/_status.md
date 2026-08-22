# Status: sdd-asterisk-chan-simbox

## Current Phase

IMPLEMENTATION (Phase 0 complete; Phase 5: 5.1-5.8 and 5.10-5.11 done;
5.9 Linux runtime check pending)

## Phase Status

PAUSED — Task 5.9's committed static guard passes, but its socat PTY
smoke requires a real Linux host

## Last Updated

2026-08-22 by Claude

## Blockers

- Two things flagged for a **real Linux environment** when one becomes
  available (not blocking further work on this macOS machine, but not
  fully verified either): `load_module()`'s real path has only been
  exercised via one-off smoke tests, not the committed suite; and
  `simbox_shutdown()` deliberately doesn't call
  `simbox_module_bridge_unload()` yet (re-entrancy unverified).
- **Task 5.9 runtime blocker**: this host is macOS and has no `socat`.
  The committed `tests/test_real_wiring.sh` therefore passes its static
  guard and deliberately skips the Linux-only PTY wire smoke. Run
  `make test-real-wiring` on Linux with `socat` and `timeout` available.
- **Session-wide note**: Anton is handling all git commits himself this
  session — do not run `git add`/`commit`/`push` without explicit
  request.

## Progress

- [x] Original Phases 1-4, Amendment (v1.2, corrected twice per Anton's
      direct guidance — see `04-implementation-log.md` for full history)
- [x] **Phase 0 complete**: chan_svistok's own `test1.c` verified
      baseline; other 3 test files have real pre-existing legacy build
      gaps, documented not fixed.
- [x] **Task 5.1** — `EXPORT_DEF`/`EXPORT_DECL` audit. Found
      `cpvt_alloc`+`at_enque_dial`/`answer`/`hangup` (simpler than
      `channel_tech`) and `pvt->sys_chan` (real non-call cpvt).
- [x] **Task 5.2** — module registry capture (`AST_MODULE_INFO` macro,
      adapter-side only). Found+fixed a real shim bug
      (`shim_config.c`'s sentinel-pointer crash). Platform split
      (`#ifdef __linux__`) established as load-bearing after finding
      chan_svistok's background discovery thread is genuinely
      Linux-only (confirmed via a real macOS crash, resolved by design
      per Anton's guidance, not by tolerating instability).
- [x] **Task 5.3/5.4** — device population + enumeration. Tagged-union
      `simbox_device_internal` (SIMULATED/REAL). `simbox_device_register()`
      is **not retired** — permanent non-Linux/test path, resolving the
      `sdd-flutter_gsm-ffi` cross-flow concern without needing
      coordination after all.
- [x] **Task 5.5** — calls, via `cpvt_alloc`+`at_enque_dial`/`answer`/
      `hangup` (the simpler path Task 5.1 found, not `channel_tech`).
- [x] **Task 5.6** — SMS/USSD/AT-commands, via `pvt->sys_chan`. SMS/USSD
      complete. `simbox_at_command`'s response-text capture explicitly
      deferred to Task 5.8 (same underlying response-arrival mechanism).
- [x] **Task 5.7** — IMEI: genuinely blocked, documented not faked.
      `ttyprog_changeimei` is called in 3 places across chan_svistok but
      defined nowhere in the checked-in tree — a real, pre-existing
      chan_svistok gap, not an adapter problem. Also confirmed
      `simbox_prog_change_imei` is an independent, disconnected
      simulation (out of this amendment's scope, flagged for later).
- [x] **Task 5.8 — Events + raw AT response capture**: incoming calls
      use `ast_pbx_start`; call-state/SMS/USSD use chan_svistok's real
      `manager_event_*` helpers terminating at adapter-owned
      `manager_event()`; synchronous raw AT accumulates the real
      `CMD_USER` response lines emitted through `ast_log`. Also fixed
      cpvt ownership for queued hangup and incoming-call answer.
- [~] **Task 5.9** — committed mechanical wiring guard passes; Linux
      helper syntax-checks; dynamic `socat` PTY wire smoke is pending a
      real Linux host.
- [x] **Task 5.10** — full available regression passes: all 6 C
      integration suites, Phase 0 `test1`, and all 9 unchanged Flutter
      repository tests. Forced Linux syntax-check also passes.
- [x] **Task 5.11** — all four chan_svistok tests copied byte-for-byte
      and guarded by `cmp`; copied `test1` builds/runs successfully.
      The other three reproduce Phase 0's pre-existing legacy build
      gaps (standalone coupling/duplicate included-C symbols and the
      still-missing `ttyprog_changeimei`), so parity is preserved rather
      than misreported as a new adapter regression.

## Context Notes

Key decisions and context for resuming:

- **The original shim (Phases 1-2, `adapters/`) is real and confirmed
  working** — chan_svistok's 20 files genuinely compile against it.
- **`BUILD_SINGLE` discovery**: chan_svistok's own `export.h` has a
  build-mode switch (`EXPORT_DEF`/`EXPORT_DECL` → `static` under
  `-DBUILD_SINGLE`, else `extern`/empty). This build never defines
  `BUILD_SINGLE` — the whole "reach chan_svistok via its own
  EXPORT_DECL convention" design uses chan_svistok's own *intended*
  multi-file build mode, not a workaround.
- **Verification pattern established and reused every task**: macOS
  real build (`make` — proves zero regression, and via `nm`, that
  `#ifdef __linux__` code truly doesn't compile in on non-Linux) +
  `gcc -fsyntax-only -D__linux__` forced check (proves the Linux branch
  at least parses correctly, since no real Linux environment is
  available this session).
- **`g_active_linux_instance`** (Task 5.8): a new process-global in
  `simbox_api.c` tracking whichever `simbox_instance` is "the" active
  Linux one, since `shim_pbx.c`'s hooks have no access to the private
  instance struct otherwise. Reasoned safe because `gpublic`/
  `load_module()` are already process-global by chan_dongle.c's own
  design — there's only ever meaningfully one real Linux instance.
- **Task 5.8 dispatch discovery**: `BUILD_MANAGER` is chan_svistok's
  intended switch. With it enabled from adapter `config.h`, unchanged
  `at_response.c`/`channel.c` call the real exported manager-event
  helpers, which terminate at shim-owned `manager_event()`. No polling
  and no read-only changes are needed.
- **Raw AT response seam**: unchanged `at_response.c` logs every
  `CMD_USER` response line before dispatch. `shim_logging.c` recognizes
  only that exact notice, correlates it by device serial, and wakes the
  synchronous API on terminal `OK`/error.
- **No Linux environment available in this session** — every claim
  about the Linux branch beyond syntax-checking is provisional.
- **Task 5.9 harness**: `tests/test_real_wiring.sh` guards the concrete
  real entry points mechanically and, on Linux, builds
  `tests/test_at_wire.c` against `libsimbox.a`; a socat PTY peer then
  verifies that public `simbox_at_command()` emits exact `AT+CSQ\r`
  bytes. The direct `cpvt` design intentionally replaces the plan's
  earlier `channel_tech` draft, per specifications §9.7.
- **Task 5.11 parity**: the copies are intentionally untouched source,
  not cleaned-up ports. This keeps the baseline meaningful: 1/4 works
  without hardware and 3/4 retain their documented upstream build
  limitations.

## Next Actions

1. On a real Linux host with `socat` and GNU `timeout`, run
   `make test-real-wiring`; the static guard and PTY byte assertion must
   both report PASS.
2. On that host, repeat the full C regression and exercise real module
   load/unload before enabling `simbox_module_bridge_unload()`.
3. Record those Linux results; if clean, close Task 5.9 and complete
   the flow. IMEI remains an explicitly documented upstream source gap.
