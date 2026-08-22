# Status: sdd-asterisk-chan-simbox

## Current Phase

IMPLEMENTATION (Phase 0 complete; Phase 5.1 done, 5.2 mechanism proven)

## Phase Status

IN PROGRESS

## Last Updated

2026-08-22 by Claude

## Blockers

- None. Anton decided (2026-08-22): accept the background-discovery-
  thread crash as a macOS-dev-machine-only limitation (real target is
  Linux/OpenWRT). Continue Phase 5, isolating/timeout-wrapping any
  `load_module()` calls in tests on this machine; get a real Linux
  verification pass later before fully trusting this path.
- No read-only-tree exceptions anywhere in the approved plan — the first
  draft's Task 5.1 (adding `EXPORT_DEF` inside `asterisk_chan_svistok/`)
  was rejected by Anton directly and replaced with an adapter-side-only
  design (see Context Notes). **Zero modifications to either read-only
  tree anywhere in this amendment.**
- **Session-wide note**: Anton is handling all git commits himself this
  session — do not run `git add`/`commit`/`push` without explicit
  request.

## Progress

- [x] Original Phases 1-4 (shim + delivered-but-disconnected adapter
      layer) — implemented 2026-08-21, marked COMPLETE by "Antigravity"
- [x] Gap found (2026-08-22, by `sdd-flutter_gsm-ffi` while implementing
      SMS/USSD wiring): `src/simbox_*.c` never actually calls into
      chan_svistok — confirmed via grep, zero references to
      `pvt`/`cpvt`/`at_enque_*`/`channel_request` anywhere in `src/`
- [x] Requirements amended (v1.1): new Must-Have #8 + 2 new Open
      Questions
- [x] Specifications amended (v1.1): new §9 wiring design
- [x] Plan amended (v1.1): new Phase 5
- [x] **Correction round (same day)**: Anton rejected v1.1's read-only-
      tree `EXPORT_DEF` exception directly. All three docs revised to
      v1.2: reach chan_svistok exclusively through already-exported
      symbols (`channel_tech`, `gpublic`, `at_enque_*`, `find_device_ex`/
      `ext`) plus one adapter-side mechanism (`AST_MODULE_INFO` macro
      capture, entirely inside `adapters/`). Added: new Must-Have #9
      (full `EXPORT_DEF`/`EXPORT_DECL` symbol audit as a design
      prerequisite), new Must-Have #10 + new Plan Phase 0 (chan_svistok's
      own existing tests run as a baseline **first**, before any adapter
      code) + new Plan Task 5.11 (those same tests copied verbatim into
      `tests/` and re-run through the adapter as the **final** step,
      proving forwarding preserves real behavior).
- [x] Amendment approved by Anton (final, corrected version, 2026-08-22)
- [x] Phase 0 complete (2026-08-22): `tests/baseline/test1` builds and
      runs chan_svistok's own `test/test1.c` unmodified, exit 0, 50
      lines of expected output — the verified baseline. Other 3 of
      chan_svistok's 4 test files documented as not buildable standalone
      (real, pre-existing legacy gaps, not caused by this flow) — see
      implementation log for exact findings.
- [ ] Phase 5 implementation started
- [ ] Phase 5 implementation complete

## Context Notes

Key decisions and context for resuming:

- **The original shim (Phases 1-2, `adapters/`) is real and confirmed
  working** — untouched by this amendment. chan_svistok's 20 files
  genuinely compile against the shim headers. Only the public API layer
  (`src/`, originally Phase 3) needs rework.
- **Root cause of the gap, as best understood**: the approved plan's
  Task 3.3 correctly specified `simbox_modem.c` as depending on Phase
  2's shim, but the delivered implementation built a parallel
  simulation instead. No implementation log was kept
  (`04-implementation-log.md` is still the blank template) — treat as
  an unintentional deviation, not a documented design choice.
- **The corrected wiring design, reach-without-modifying**: chan_svistok
  already exports (via its own `EXPORT_DEF`/`EXPORT_DECL` convention)
  everything the adapter actually needs:
  - `struct ast_channel_tech channel_tech` (in `channel.c`) — its
    `.requester`/`.call`/`.hangup`/`.answer` fields reach the
    individually-`static` `channel_request`/`channel_call`/
    `channel_hangup`/`channel_answer` functions *through the struct*,
    with zero visibility changes needed.
  - `at_enque_sms`/`at_enque_ussd`/`at_enque_cmd_proc` (in
    `at_command.c`) — already `EXPORT_DEF`.
  - `gpublic` (in `chan_dongle.h`) — the live global `public_state_t*`,
    already `EXPORT_DECL`, whose `.devices` field is a real, walkable
    `AST_RWLIST_HEAD` of `struct pvt` once populated.
  - The only genuinely `static`, unreachable piece is `load_module()`
    itself (which populates `gpublic` by calling `pvt_create()`
    internally, config-file-driven). Reached via an **adapter-side**
    change to `adapters/include/asterisk/module.h`'s `AST_MODULE_INFO`
    macro expansion — chan_dongle.c's unmodified invocation of that
    macro then registers `.load`/`.unload`/`.reload` somewhere the
    adapter can retrieve them. This is the one new mechanism Phase 5
    needs to build, and it lives entirely in `adapters/`.
- **Discovery reframing** (unchanged from v1.1): real USB auto-discovery
  stays a stub; device population reuses chan_dongle.c's own
  config-file-driven path via the mechanism above — closer to how the
  real Asterisk module always worked.
- **Cross-flow dependency, still open**: `sdd-flutter_gsm-ffi`'s test
  helper `debugRegisterDiscoveredDevice` calls `simbox_device_register()`
  directly, which Phase 5.3 here proposes retiring in favor of real
  `gpublic`-backed device population. Coordinate before removing it.
- Two things still explicitly flagged as **not yet traced**: (1) `cpvt`
  acquisition for SMS/USSD/AT-commands outside an active call context;
  (2) IMEI change's real entry point (best guess: redirects to
  `simbox_prog_change_imei`). Both are Phase 5 implementation-time
  investigations, not guessed in the docs.

## Next Actions

1. Get Anton's final approval on the corrected amendment (all three
   docs, v1.2).
2. Once approved, start with **Phase 0** (chan_svistok test baseline —
   run its own existing tests, unmodified, before writing any adapter
   code), then Phase 5 in dependency order (5.1 audit → 5.2 module-
   registry capture → 5.3 device population → 5.4 enumeration → 5.5
   calls → 5.6 SMS/USSD/AT → 5.7 IMEI → 5.8 events → 5.9 mechanical
   check → 5.10 regression → **5.11 copy chan_svistok's tests verbatim,
   verify parity**). Same checkpoint discipline as every other flow this
   session — verify after each task, never batch.
3. Keep `sdd-flutter_gsm-ffi` informed of anything here that affects its
   Dart-side code or tests, especially Task 5.3's
   `simbox_device_register()` retirement question.
