# Requirements: asterisk-chan-simbox

> Version: 1.2
> Status: REOPENED — Amendment below approved scope, see "Amendment 2026-08-22"
> Last Updated: 2026-08-22

## Amendment 2026-08-22 — reopened after a real gap found downstream

`sdd-flutter_gsm-ffi` (the consumer flow, binding `flutter_gsm`'s Linux
platform to `libsimbox` via `dart:ffi`) found, while implementing SMS/
USSD wiring, that **the delivered implementation never actually drives
`chan_svistok`'s real logic**. Confirmed by direct grep: `src/simbox_modem.c`,
`simbox_discovery.c`, `simbox_api.c` contain **zero references** to any
chan_svistok symbol (`pvt`, `cpvt`, `at_enque_*`, `channel_request`,
`find_device_by_resource`, etc.). `simbox_call_originate`/`simbox_sms_send`/
`simbox_ussd_send`/`simbox_at_command` all just flip an in-memory
`struct simbox_device_internal` or return a canned string — no real
serial I/O, no AT commands sent, ever. `chan_svistok`'s actual channel/
AT-command engine (20 files, confirmed compiling into `libsimbox.a` per
`04-implementation.md`'s own matrix) is linked but never called.

This directly contradicts this document's own original Must-Have #3
("adapters/`src/` link/compile `asterisk_chan_svistok`'s unmodified
source **against the shim**... to produce a standalone binary/library")
and the approved plan's own Task 3.3, which explicitly declared
`simbox_modem.c` **dependent on** Phase 2's shim (`shim_channel.c`,
`shim_pbx.c` — the pieces that wrap chan_svistok's real channel-tech
callbacks). The dependency was declared but never actually exercised in
the delivered code, and neither `04-implementation-log.md` (still the
blank template — never filled in) nor `04-implementation.md` documents
this as a deliberate deviation. It appears to have been silently
dropped, not decided.

**What was real and should be kept**: the Asterisk-compatibility shim
itself (`adapters/` — 22 headers + 12 C shim files) — chan_svistok's 20
source files genuinely compile against it (verified, not just claimed:
see this flow's own `04-implementation.md` §3, and re-confirmed by
`sdd-flutter_gsm-ffi` independently rebuilding it clean). That part of
the Strangler Fig worked. **What didn't**: the public API layer (`src/`)
was supposed to be a thin adapter *driving* that shimmed chan_svistok
through its real entry points, and instead became a parallel,
from-scratch simulation that happens to expose the same function
signatures. New Must-Have #8 below makes this failure mode mechanically
checkable going forward, not just describable in prose.

### Correction 2026-08-22 (same day, Anton's direct guidance)

The first draft of this amendment's fix (specifications §9 v1, plan
Phase 5 v1) proposed adding `EXPORT_DEF` to five `static` functions
*inside* `asterisk_chan_svistok/` as a "visibility-only, not a logic
change" exception to the read-only rule. **Anton rejected this
explicitly**: the read-only constraint has no exceptions, not even
visibility-only ones. Corrected instruction, restated precisely:

- `asterisk_chan_svistok/` and `asterisk_chan_dongle/` (both forks) stay
  **100% byte-identical, zero diffs, no exceptions** — this was already
  the rule; the correction is that it really does mean *zero*, including
  changes as small as adding a linkage keyword.
- All "прокидка" (forwarding/proxying) of chan_svistok's real functions
  happens **from the adapter side only** — either by calling symbols
  chan_svistok *already* exports with external linkage (its own
  `EXPORT_DEF`/`EXPORT_DECL` convention — re-audit needed, see Must-Have
  #9 below, since several relevant symbols turn out to already be
  exported: `channel_tech` — the `ast_channel_tech` struct holding
  `.call`/`.hangup`/`.answer`/`.requester` function pointers —
  `at_enque_sms`/`at_enque_ussd`/`at_enque_cmd_proc`, `find_device_ex`/
  `find_device_ext`, and the global device list `gpublic` itself are
  *all* already `EXPORT_DEF`/`EXPORT_DECL`), or by writing new adapter-
  side code that captures an already-exposed *indirect* hook — concretely,
  the shim's own `adapters/include/asterisk/module.h` (part of the
  adapter, not the read-only tree) defines the `AST_MODULE_INFO` macro
  that chan_dongle.c's unmodified `AST_MODULE_INFO(...)` invocation
  expands against; changing *that macro's definition* (adapter-side) to
  register the resulting `ast_module_info->load`/`.unload`/`.reload`
  function pointers somewhere externally reachable gives the adapter a
  legitimate way to trigger `load_module()` (and therefore real device
  population) without chan_svistok's source changing by one byte.
- **New rule for `src/`**: everything in `libsCpp/asterisk_chan_simbox/src/`
  must be **minimal** — thin functions that marshal `simbox_api.h`'s
  primitive-typed parameters into calls against symbols the adapter
  layer exposes, and marshal the results back. No independent logic,
  no reimplementation, no parallel state machines. If a `src/` function
  needs more than a few lines to do its job, that's a signal the real
  logic belongs in `adapters/` (wrapping/exposing more of chan_svistok),
  not in `src/`.
- **New process requirement**: test coverage comes *first*, not last.
  Before any adapter/API code is written, establish a verified baseline
  of chan_svistok's own existing test files (`chan_svistok/test/test1.c`,
  `chan_svistok/simnode/adiscovery_test.c`,
  `chan_svistok/programmator/ttyprog_test.c`,
  `chan_svistok/reader/old/test.c` — all already exist, not written by
  this flow) — see new Must-Have #10. Then, as the **final** step once
  all `adapters/`/`src/` code is written, those exact same test files
  are **copied verbatim (not rewritten, not adapted)** into
  `libsCpp/asterisk_chan_simbox/tests/` and run *through* the new
  adapter/shim build path — if they still pass unmodified, that's direct
  proof the forwarding preserves chan_svistok's real, verified behavior
  rather than reimplementing it. This is the strongest possible
  regression guard against the exact failure this amendment exists to
  fix.

**Cross-flow note**: `sdd-flutter_gsm-ffi` already built real, tested
Dart FFI bindings against the *current* `simbox_api.h` signatures
(including a new `simbox_device_register()` this amendment's rework will
likely need to reshape internally — see Open Questions). Per this
document's own Must-Have #4, `simbox_api.h`'s public C signatures are
the FFI seam and should not need to change for this rework to land —
only what happens *inside* those functions changes. If a signature
change turns out to be unavoidable, flag it back to
`sdd-flutter_gsm-ffi` explicitly rather than silently breaking its
bindings.

### Correction 2026-08-22 (later same day) — platform split clarified

While verifying Task 5.2, `chan_dongle.c`'s real `load_module()` was
found to unconditionally start a background discovery thread that calls
into real Linux `/sys/bus/usb/devices` scanning (`pdiscovery.c`) — which
crashed on the macOS dev machine used for this session, since `/sys`
doesn't exist there in the Linux sense. Anton clarified the intended
architecture directly, resolving this cleanly: **`chan_svistok` is
Linux-only by design — that's expected, not a bug to route around.**
`chan_simbox` (this flow's `src/`/`adapters/` layer) is the
cross-platform piece, and its job is: **on Linux, call chan_svistok's
real, unmodified code as designed; on macOS/Windows/other platforms,
provide separate implementations inside `#ifdef`-guarded branches keyed
on OS**, not attempt to force chan_svistok's Linux-specific internals to
run (or be tolerated as flaky) on platforms they were never designed
for.

This means: the real chan_svistok-driving code this amendment specifies
(§9 in specifications) is the **Linux branch** of `src/simbox_*.c`.
Non-Linux branches don't call `load_module()`/discovery/any Linux-only
chan_svistok internals at all — they're a separate, explicitly-scoped
implementation path (stub-first, per the existing Should-Have default
for Windows/macOS, until/unless a real non-Linux driver is ever wanted).
This isn't new scope — it's the correct realization of the *existing*
"OS-specific code via `#ifdef`/`OS_type` convention inside
`adapters/`/`src/`, never inside the read-only trees" secondary user
story (already in this document, see below) — flagging that Task 5.2's
first design draft didn't yet apply it consistently, now corrected.

## Problem Statement

`flutter_gsm` (see `libsFlutter/flutter_gsm/flows/sdd-flutter_gsm/`) needs
a real, cross-platform, Asterisk-free ttyUSB/AT-command GSM modem driver
on its native/desktop side. The only stable, verified, edge-case-hardened
implementation of that logic is `chan_svistok` — but it's an Asterisk
channel driver, tightly coupled to Asterisk's C API (115 distinct `ast_*`
functions, ~15 struct types, confirmed by grep, not estimated).

This flow owns building the bridge: a new package,
`libsCpp/asterisk_chan_simbox`, containing (a) read-only reference copies
of the proven codebases, and (b) a new adapter layer that lets
`flutter_gsm` drive `chan_svistok`'s real, unmodified logic without a
running Asterisk instance — via an Asterisk-API-compatible shim, using a
**Strangler Fig** approach: the new adapter incrementally takes over
functionality while the old (chan_svistok) code remains completely
unmodified and serves as the stable foundation underneath.

This flow was carved out of `sdd-flutter_gsm` (2026-08-21) once its scope
grew large enough to be its own unit of work — see that flow's
requirements history for the earlier, now-superseded framing ("core/glue
separation") that this flow's Strangler Fig approach replaces.

## Repository Layout (already scaffolded by Anton, outside this flow)

```
libsCpp/asterisk_chan_simbox/
├── asterisk_chan_svistok/      # READ-ONLY. The proven, stable driver.
│   └── chan_svistok/           #   (moved from libsCpp/chan_svistok)
├── asterisk_chan_dongle/       # READ-ONLY. Upstream reference forks.
│   ├── asterisk-chan-dongle-by-wdoekes/    # more functionality
│   └── asterisk-chan-dongle-by-pulpoff/    # Asterisk 20+ support
├── adapters/                   # NEW WORK GOES HERE (Strangler Fig)
└── src/                        # NEW WORK GOES HERE (Strangler Fig)
```

**Hard rule, stated by Anton and non-negotiable**: `asterisk_chan_svistok/`
and `asterisk_chan_dongle/` are **read-only, permanently**. Every change,
addition, modification, replacement, and refactor happens only in
`adapters/` and `src/`, built *around* the stable code, never *inside* it.

## Reference Codebases — what each is for, precisely

- **`asterisk_chan_svistok/`** — the only codebase the actual
  simbox bridge/adapter may be built on. Stable, edge-case-hardened,
  currently the live Asterisk module. Built on an **outdated**
  `asterisk_chan_dongle` base (predates both forks below) — its own
  AT-command/PDU/discovery/tty logic is trusted; its Asterisk-version
  targeting is old (pre-opaque-`ast_channel`-era, roughly Asterisk 1.8).
- **`asterisk_chan_dongle/asterisk-chan-dongle-by-wdoekes/`** — reference
  only, **not a base for the simbox bridge**. More functionality than
  chan_svistok's base: `smsdb.c/h` (SMS persistence database — chan_svistok
  has no equivalent), `gsm7_luts.h` (GSM 7-bit lookup tables), `error.c/h`
  (structured error handling), and notably **`ast_compat.h`/
  `ast_config.h`** — this fork already built its own
  Asterisk-version-compatibility shim concept. Worth studying as prior
  art for *this flow's* shim design, even though we can't use its code
  directly in the bridge.
- **`asterisk_chan_dongle/asterisk-chan-dongle-by-pulpoff/`** — reference
  only, **not a base for the simbox bridge**. Its README documents the
  exact Asterisk 1.8→20 API migration needed: opaque `ast_channel`
  (accessor functions instead of direct field access), new format-
  capabilities API (`ast_format_cap`/`ast_format_slin`), changed
  `channel_request` callback signature, changed `ast_channel_alloc`
  (assignedids/requestor params), `ast_bridged_channel` →
  `ast_channel_bridge_peer`, changed module registration
  (`AST_MODULE_INFO` load_pri/support_level), removed
  `ASTERISK_FILE_VERSION` macro. **This is a ready-made checklist of
  exactly which `ast_*` symbols changed shape between Asterisk versions**
  — directly useful for scoping the shim's API surface and knowing which
  calls need version-aware handling.

**Constraint restated for clarity**: reference the wdoekes/pulpoff forks
for *patterns and migration knowledge only*. The actual bridge to simbox
is built exclusively against `asterisk_chan_svistok`'s behavior/API shape.
Never copy chan_dongle-fork code into the bridge; never modify any of the
three reference trees.

## User Stories

### Primary

**As a** `flutter_gsm` developer
**I want** to call chan_svistok's real, unmodified AT-command/PDU/
discovery/call-state logic from outside Asterisk, via FFI
**So that** `flutter_gsm`'s desktop platform implementations get a
proven, edge-case-hardened driver instead of a from-scratch reimplementation.

**As a** maintainer of `asterisk_chan_svistok`
**I want** the module's source to remain completely unmodified and still
compile/run against a real Asterisk installation
**So that** nothing about this effort risks or requires abandoning the
existing, working Asterisk deployment path — the two consumers (real
Asterisk, and `flutter_gsm` via the shim) are strictly additive.

### Secondary

**As a** developer debugging the adapter layer
**I want** the Asterisk-compatibility shim to be a real seam for standard
C unit tests (mock the `ast_*` surface, assert chan_svistok's channel/
call-state logic behaves correctly)
**So that** this codebase gets test coverage it structurally cannot have
today (Asterisk-only builds have no practical unit-test path).

**As a** developer porting to Windows/macOS/Android/OpenWRT
**I want** OS-specific code added via preprocessor macros
(`#ifdef`-style, e.g. an `OS_type` convention) inside the new
`adapters/`/`src/` layer — never inside the read-only reference trees
**So that** Linux behavior needs minimal changes (it already works) while
other platforms get their own code paths without touching proven logic.

## Acceptance Criteria

### Must Have

1. **Given** the read-only constraint on `asterisk_chan_svistok/` and
   `asterisk_chan_dongle/`
   **When** any code is written in this flow
   **Then** zero diffs land in those three trees — enforce via a
   specifications/plan-phase decision on how to check this mechanically
   (e.g. a pre-commit check, CI diff guard, or simply keeping them out of
   the new package's own git tracking entirely and treating them as
   vendored/pinned externals).

2. **Given** chan_svistok calls ~115 `ast_*` functions and uses ~15
   `struct ast_*` types (counted via grep in `sdd-flutter_gsm`'s
   research, to be re-verified against the moved path)
   **When** the Asterisk-compatibility shim is specified
   **Then** it implements exactly the subset chan_svistok actually calls
   (not all of Asterisk), scoped from a real inventory (a Should-Have
   deliverable below), with each symbol's shim behavior documented
   (no-op, trivial pass-through, or real logic needed to satisfy
   chan_svistok's expectations).

3. **Given** the Strangler Fig framing
   **When** the adapter is built
   **Then** `adapters/`/`src/` link/compile `asterisk_chan_svistok`'s
   unmodified source against the shim (not real Asterisk headers) to
   produce a standalone binary/library, while a parallel build path
   (unchanged) still links it against real Asterisk — both must build
   from the same unmodified source tree.

4. **Given** `flutter_gsm`'s need to FFI-bind this from Dart
   **When** the adapter's public surface is specified
   **Then** it exposes a clean C API (not Asterisk types) that
   `flutter_gsm`'s Linux `dartPluginClass` implementation can call via
   `dart:ffi` — this is the actual seam `sdd-flutter_gsm` deferred to
   this flow.

5. **Given** the three discovery generations found in `sdd-flutter_gsm`'s
   research (root `pdiscovery.c` — oldest; `simnode/adiscovery_core.c` —
   in-process, used by the live module; `simnode/adiscovery_core_new.c`+
   `adiscovery_simnode.c` — standalone daemon, already macro-gates its
   Asterisk include via `#ifdef IN_SIMBOX`)
   **When** specifications pick the discovery target
   **Then** the standalone-daemon generation is the default target
   (already closest to decoupled) **unless** specifications find a
   concrete reason to prefer the in-process one — document the choice
   either way, don't silently default without recording why.

6. **Given** `programmator/` (Huawei 1550/173/171 DIAG-mode firmware
   flashing + bricked-modem recovery) and `reader/` (ttyUSB SIM readers,
   APDU, no radio module) both exist in `asterisk_chan_svistok`
   **When** the adapter's scope is specified
   **Then** both get their own adapter surface (not folded into the
   modem/call surface) — `reader/` in particular needs a distinct
   `SimReaderDevice`-shaped API since it has no calls/SMS/registration.

7. **Given** OpenWRT as an explicit target (added to `sdd-flutter_gsm`'s
   requirements per Anton)
   **When** cross-platform scope is specified here
   **Then** OpenWRT is understood as a build/cross-compile target for
   this native core (embedded Linux, likely headless/server-side
   consumption, not a Flutter UI target) — confirm this framing
   explicitly in specifications, don't assume.

8. **(New, 2026-08-22 amendment, corrected same day)** **Given** this
   flow's whole purpose is letting `flutter_gsm` drive chan_svistok's
   *real* logic, not reimplement it, **and** given the read-only
   constraint permits zero modification of `asterisk_chan_svistok/` —
   not even visibility-only ones (see Correction above)
   **When** `src/simbox_*.c`'s public API functions are implemented
   **Then** each one that has a real chan_svistok equivalent must
   actually call it, reached *only* through symbols chan_svistok already
   exports (`EXPORT_DEF`/`EXPORT_DECL`) or through adapter-side capture
   of an indirect hook (e.g. the `AST_MODULE_INFO` macro) — never via a
   source change to the read-only tree:
   - `simbox_call_originate`/`hangup`/`answer` must reach
     `channel_tech.call()`/`channel_tech.hangup()`/`channel_tech.answer()`/
     `channel_tech.requester()` — the already-`EXPORT_DEF`'d
     `struct ast_channel_tech channel_tech` in `channel.c` holds these as
     function pointers, reachable without `channel_request`/`channel_call`/
     etc. themselves needing to be non-`static`.
   - `simbox_sms_send`/`simbox_ussd_send`/`simbox_at_command` must reach
     `at_enque_sms()`/`at_enque_ussd()`/`at_enque_cmd_proc()` — already
     `EXPORT_DEF` in `at_command.c`, no capture needed.
   - Device population must come from the real, already-`EXPORT_DECL`'d
     global `gpublic` (`public_state_t *`, `chan_dongle.h`) — specifically
     its `devices` field, a real `AST_RWLIST_HEAD` of `struct pvt`,
     populated by `load_module()` → `public_state_init(gpublic)` →
     `pvt_create()` internally. Since `load_module` is `static`, the
     adapter must trigger it via the `AST_MODULE_INFO` macro-capture
     mechanism (adapter-side change to `adapters/include/asterisk/module.h`),
     not a direct call — not a hand-rolled parallel struct either way.
   - **Verification**: `grep` for these symbol names (`channel_tech`,
     `at_enque_sms`, `at_enque_ussd`, `at_enque_cmd_proc`, `gpublic`)
     across `src/*.c` must find them present, not absent — add this as a
     literal shell check in the test suite (fails loudly if a future
     change silently reintroduces a simulation). Additionally, a
     real-or-emulated-serial-port smoke test (e.g. a `socat` PTY pair
     standing in for a ttyUSB device) must show actual AT command bytes
     crossing the wire for at least one operation (`simbox_at_command`
     is the simplest to verify this way) — "the function returns 0"
     alone is not sufficient evidence, per this amendment's whole reason
     for existing.

9. **(New, 2026-08-22)** **Given** chan_svistok's own `EXPORT_DEF`/
   `EXPORT_DECL` convention already marks a meaningful subset of its
   functions/globals with external linkage (confirmed: `channel_tech`,
   `at_enque_sms`/`ussd`/`cmd_proc`, `find_device_ex`/`find_device_ext`,
   `gpublic`, `pvt_try_restate`, `pvt_str_state`, and others)
   **When** specifications design the adapter's calls into chan_svistok
   **Then** a full audit of `EXPORT_DEF`/`EXPORT_DECL` symbols happens
   first (a Should-Have deliverable, mirroring the original `ast_*`
   inventory's method) so the adapter is built against a complete,
   accurate picture of what's *already* legitimately reachable, rather
   than rediscovering reachable symbols one at a time while writing
   adapter code.

10. **(New, 2026-08-22)** **Given** chan_svistok already has its own
    test files (`chan_svistok/test/test1.c`,
    `chan_svistok/simnode/adiscovery_test.c`,
    `chan_svistok/programmator/ttyprog_test.c`,
    `chan_svistok/reader/old/test.c`) that this flow did not write and
    must not modify
    **When** the plan is sequenced
    **Then** (a) establishing a verified baseline of these tests passing
    against chan_svistok in isolation is the **first** implementation
    task, before any adapter/API code is written, and (b) copying these
    same files **verbatim** into `libsCpp/asterisk_chan_simbox/tests/`
    and confirming they still pass when built through the new adapter/
    shim path is the **last** implementation task — direct, mechanical
    proof that forwarding preserves real behavior rather than
    reimplementing it.

### Should Have

- A concrete inventory (table) of the ~115 `ast_*` functions and ~15
  `struct ast_*` types, each annotated with: where chan_svistok calls it,
  what it needs to do, and whether the wdoekes/pulpoff forks' handling of
  the same symbol (if any) is instructive. This is the shim's actual
  design input — do this before writing shim code, not alongside it.
- A short comparison note on `asterisk-chan-dongle-by-wdoekes`'s
  `ast_compat.h`/`ast_config.h` approach — it solved a *related* problem
  (compatibility across real Asterisk versions) that overlaps
  conceptually with *this* problem (compatibility with no Asterisk at
  all). Worth 20 minutes of reading before designing the shim from
  scratch.
- Windows/macOS: mirror `sdd-flutter_gsm`'s decided default (stub-first,
  Windows discovery explicitly allowed to stay stubbed longest) unless
  this flow's specifications find a reason to diverge.

### Won't Have (This Iteration)

- Any modification to `asterisk_chan_svistok/` or either
  `asterisk_chan_dongle/` fork — permanent constraint, not just this
  iteration's.
- Porting wdoekes/pulpoff's specific features (smsdb, GSM7 LUTs,
  Asterisk-20 compatibility) into chan_svistok or the adapter — they're
  read-only reference, not a merge source, unless a future flow
  explicitly decides otherwise.
- A real Windows/macOS driver (stub/interface-parity only, per
  `sdd-flutter_gsm`'s established default).
- Publishing `asterisk_chan_simbox` anywhere, or setting up its own git
  remote (not asked for; address only if raised).
- Changes to `flutter_gsm`'s Dart-side platform-interface contract itself
  — that's `sdd-flutter_gsm`'s scope; this flow provides the native
  library it binds to.

## Constraints

- **Technical**: The shim must be built from a real, counted inventory of
  chan_svistok's actual `ast_*` usage, not a guess at "what Asterisk
  provides" — over-scoping the shim wastes effort, under-scoping breaks
  the build.
- **Technical**: Both build paths (real Asterisk, and the new shim) must
  compile from the *same* unmodified `asterisk_chan_svistok` source —
  divergence here would silently reintroduce the "two versions of the
  truth" problem this flow exists to avoid.
- **Process**: Read-only trees must not appear as diffs in this flow's
  commits — resolve the mechanical enforcement question in specifications
  (Must Have #1) rather than relying on discipline alone.
- **Dependencies**: `flutter_gsm`'s Linux implementation is the consumer
  of this flow's output; sequencing between the two flows (which lands
  first, or whether they proceed in parallel with a stub interface until
  this flow's shim is ready) should be decided explicitly, not assumed —
  see Open Questions.

## Open Questions

- [x] **Sequencing vs. `sdd-flutter_gsm`**: **Параллельно.** `flutter_gsm`
      идёт параллельно со стабами, интеграция — отдельный шаг позднее.
      *(Решено Антоном 2026-08-21)*
- [x] **Discovery generation choice** (Must Have #5): **Оставить как есть,
      ничего не трогать.** Не менять текущую архитектуру discovery на данном
      этапе. *(Решено Антоном 2026-08-21)*
- [x] **Enforcement mechanism for the read-only constraint** (Must Have
      #1): **Ручная проверка.** Антон смотрит вручную, автоматизация
      (pre-commit hook, CI) не требуется. *(Решено Антоном 2026-08-21)*
- [x] **OpenWRT toolchain specifics**: **Достаточно generic Linux.**
      На данном этапе достаточно "просто компилируется для MIPS/ARM Linux
      с musl libc", без специфичной интеграции с OpenWRT SDK/buildroot.
      *(Решено Антоном 2026-08-21)*
- [x] **Git repo / submodules**: **Git управляется Антоном самостоятельно.**
      Решения по структуре репозитория, submodules и git remote —
      вне скоупа этого флоу. *(Решено Антоном 2026-08-21)*
- [ ] **(New, 2026-08-22)** Does rewiring `src/simbox_*.c` to actually
      drive chan_svistok's real `pvt`/`channel_tech`/`at_enque_*` engine
      require any `simbox_api.h` signature changes, or can it stay a
      pure internal-implementation change? Preliminary read: no
      signature changes needed (the public API was already designed
      around opaque handles + primitive params, which is compatible
      with either a fake struct or a real `pvt*` underneath) — confirm
      during specifications, since `sdd-flutter_gsm-ffi` has live Dart
      FFI bindings against the current header that would break on any
      signature drift.
- [ ] **(New, 2026-08-22)** `simbox_discovery.c`'s real
      `/sys/bus/usb/devices` enumeration is still a stub (`ctx->count = 0`
      unconditionally) — `sdd-flutter_gsm-ffi` already worked around the
      *registry* half of this gap (`simbox_device_register()`, letting a
      discovered device become queryable), but the *discovery* half
      (finding real USB devices at all) remains unimplemented. Is real
      `/sys/bus/usb/devices` enumeration in scope for this amendment, or
      a further follow-up? Given Must-Have #8's device-population
      requirement now points at `pvt_create()`, and `chan_dongle.c`'s own
      `load_module()`/`public_state_init()` already has real discovery
      logic (config-file-driven via `dc_config.c`, per the original
      Asterisk module) — the pragmatic path may be reusing *that*
      existing logic rather than fixing `simbox_discovery.c`'s USB scan
      independently. Flag for specifications to decide, don't assume.

## References

- `libsFlutter/flutter_gsm/flows/sdd-flutter_gsm/01-requirements.md` —
  the flow this one was carved out of; contains the original chan_svistok
  research (discovery generations, `programmator/`/`reader/` discovery,
  the 115-function/~15-type Asterisk API count) in fuller detail.
- `libsCpp/asterisk_chan_simbox/asterisk_chan_svistok/` — the stable base
  (read-only).
- `libsCpp/asterisk_chan_simbox/asterisk_chan_dongle/
  asterisk-chan-dongle-by-wdoekes/` — pattern reference (read-only),
  notably `ast_compat.h`.
- `libsCpp/asterisk_chan_simbox/asterisk_chan_dongle/
  asterisk-chan-dongle-by-pulpoff/` — pattern reference (read-only),
  notably its README's Asterisk 1.8→20 migration checklist.
- `legacy/chan_svistok-v2015` — older, separate read-only historical
  snapshot (predates the `libsCpp` reorganization); not to be confused
  with `asterisk_chan_svistok/`, which is the live reference this flow
  actually builds against.

---

## Approval

- [x] Reviewed by: Anton Dodonov
- [x] Approved on: 2026-08-21 (original scope)
- [x] Reviewed by: Anton Dodonov — corrected the first draft of this
      amendment directly (rejected any read-only-tree modification,
      even visibility-only; redirected to adapter-side-only forwarding
      via already-exported symbols + `AST_MODULE_INFO` capture; added
      the test-first/test-copy-at-end sequencing requirement)
- [x] Approved on: 2026-08-22 (Amendment, corrected version — new
      Must-Haves #8-#10 + two Open Questions)
- [ ] Notes:
