# Status: sdd-asterisk-chan-simbox

## Current Phase

REQUIREMENTS

## Phase Status

REVIEW

## Last Updated

2026-08-21 by Claude

## Blockers

- Awaiting explicit "requirements approved" from Anton, plus answers to
  the 5 Open Questions (sequencing vs. sdd-flutter_gsm, discovery
  generation choice, read-only enforcement mechanism, OpenWRT toolchain
  specifics, git-repo/submodule structure).

## Progress

- [x] Requirements drafted
- [x] README.md created (EN + RU) — documents all major enhancements
      vs. chan_dongle: node-discovery (3 gens), programmator (DIAG mode),
      reader (APDU), IMEI change (single AT cmd), S/N identification,
      modem auto-recovery, Asterisk 20+ compatibility (pulpoff/wdoekes)
- [ ] Requirements approved
- [ ] Specifications drafted
- [ ] Specifications approved
- [ ] Plan drafted
- [ ] Plan approved
- [ ] Implementation started
- [ ] Implementation complete

## Context Notes

Key decisions and context for resuming:

- **Carved out of `sdd-flutter_gsm`** (`libsFlutter/flutter_gsm/flows/
  sdd-flutter_gsm/`) on 2026-08-21, once the chan_svistok/chan_dongle/
  Asterisk-shim scope grew large enough to be its own flow. That flow's
  requirements doc has been trimmed accordingly (see its own status/log)
  — don't duplicate research, cross-reference instead.
- Anton had already physically scaffolded the repository layout before
  this flow started (`libsCpp/asterisk_chan_simbox/{asterisk_chan_svistok,
  asterisk_chan_dongle/{by-wdoekes,by-pulpoff},adapters,src}` all exist
  on disk; `adapters/`/`src/` are empty, ready for this flow's work).
  `asterisk_chan_svistok` is a real git checkout (moved from the old
  `libsCpp/chan_svistok`); the two chan_dongle forks are real git clones
  of the wdoekes/pulpoff repos (66 and 72 top-level files respectively,
  confirmed real source not placeholders). `libsCpp/asterisk_chan_simbox`
  itself has no `.git` yet.
- **Architecture is Strangler Fig, not core/glue separation**: the
  earlier framing (in `sdd-flutter_gsm`, before Anton's correction) was
  to exclude Asterisk-glue files from a build. The corrected, current
  approach: leave `asterisk_chan_svistok` completely unmodified, build an
  external Asterisk-API-compatible shim in `adapters/`/`src/` that lets
  its unmodified source compile/link/run standalone. Preserves real-
  Asterisk compatibility by construction and opens a path to unit tests.
- Read the two reference forks' READMEs: `asterisk-chan-dongle-by-pulpoff`
  documents the exact Asterisk 1.8→20 API migration (opaque
  `ast_channel`, new format-caps API, changed `channel_request`/
  `ast_channel_alloc` signatures, `ast_bridged_channel`→
  `ast_channel_bridge_peer`, module registration changes) — a ready-made
  checklist for scoping the shim. `asterisk-chan-dongle-by-wdoekes` has
  `smsdb.c/h`, `gsm7_luts.h`, `error.c/h`, and its own `ast_compat.h`/
  `ast_config.h` compatibility-shim attempt — worth studying before
  designing this flow's shim from scratch, per requirements' Should-Have.
- Hard, permanent constraint: `asterisk_chan_svistok/` and both
  `asterisk_chan_dongle/` forks are read-only forever. All new code goes
  in `adapters/`/`src/`. Enforcement mechanism (pre-commit hook vs. CI
  check vs. structural/submodule) is an open question, not yet decided.
- **README.md created** (2026-08-21): comprehensive EN + RU README
  documenting all major functional enhancements vs. original chan_dongle,
  production reliability stats (1 dead out of 500, 2 unrecoverable out
  of 10+ bricked), and Asterisk 20+ compatibility from both forks.

## Next Actions

1. Present `01-requirements.md` to Anton for review.
2. On "requirements approved" (and open questions answered), begin
   `02-specifications.md`: the `ast_*` symbol inventory (Should-Have
   deliverable), the discovery-generation decision, the shim's C API
   surface design (the actual FFI seam `flutter_gsm` will bind to), and
   the read-only-enforcement mechanism.
