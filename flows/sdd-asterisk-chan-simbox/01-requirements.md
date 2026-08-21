# Requirements: asterisk-chan-simbox

> Version: 1.0
> Status: DRAFT
> Last Updated: 2026-08-21

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

- [ ] **Sequencing vs. `sdd-flutter_gsm`**: does `flutter_gsm`'s Linux
      implementation wait for this flow's shim, or does `flutter_gsm`
      ship stubbed (per its own decided default) while this flow proceeds
      in parallel, with integration as a later, explicit step? Recommend
      parallel-with-explicit-integration-step, matching how
      `sdd-flutter_gsmsip-interface`/`vdd-simbox-app-uiux` were
      sequenced (build against a fake/stub, integrate later) — confirm.
- [ ] **Discovery generation choice** (Must Have #5) — confirm the
      standalone-daemon generation (`adiscovery_core_new.c`/
      `adiscovery_simnode.c`) over the in-process one, or provide a
      reason to prefer the in-process path instead.
- [ ] **Enforcement mechanism for the read-only constraint** (Must Have
      #1) — pre-commit hook, CI check, or structural (don't `git add` the
      reference trees at all, treat them as pinned external checkouts)?
      Needs a decision before implementation, not just a policy.
- [ ] **OpenWRT toolchain specifics** — cross-compilation via OpenWRT's
      SDK/buildroot, or a more generic "just needs to compile for
      MIPS/ARM Linux with musl libc" framing? Affects how much OpenWRT-
      specific work this flow needs to do vs. inherits for free from
      general Linux portability.
- [ ] Should `libsCpp/asterisk_chan_simbox` get its own git repo (like
      `flutter_gsm`/`flutter_gsmsip`/`chan_svistok` did), and if so,
      should the read-only reference trees be git submodules pointing at
      their real upstreams (wdoekes/pulpoff's actual GitHub repos,
      chan_svistok's real repo) rather than plain copied directories?
      Submodules would make the "read-only" constraint self-enforcing at
      the tooling level — worth considering before this flow generates
      much history.

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

- [ ] Reviewed by: Anton Dodonov
- [ ] Approved on:
- [ ] Notes:
