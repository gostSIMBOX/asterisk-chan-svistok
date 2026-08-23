/*
 * Adapter-side override for shim_config.c's config-file path
 * resolution. Makes simbox_config_t.config_dir (see src/simbox_types.h),
 * previously dead - simbox_init() copied it into the instance struct
 * and never read it back out - actually do something.
 */
#ifndef SIMBOX_CONFIG_BRIDGE_H
#define SIMBOX_CONFIG_BRIDGE_H

/* Tells shim_config.c's ast_config_load2() to check "<dir>/<filename>"
 * before its existing CWD-relative/"/etc/asterisk/<filename>"
 * resolution. Copies `dir` into an internal static buffer immediately -
 * safe to free/reuse the caller's string right after this call
 * returns (no ownership transfer, unlike simbox_event_cb). Pass NULL
 * to clear the override and restore the original two-path resolution.
 *
 * Cross-platform on purpose (not gated behind __linux__, unlike most
 * of this SDK's chan_svistok-driving code): src/simbox_api.c only
 * calls this from within its own #ifdef __linux__ block (config-driven
 * device population is Linux-only), but shim_config.c itself compiles
 * everywhere, and tests/test_simbox.c exercises this function directly
 * on any platform - it's pure path-resolution logic, no chan_dongle
 * device population involved.
 *
 * Deep chan_svistok research (sdd-simbox-app-real-driver) confirmed a
 * static, set-once-before-first-load value is sufficient: every
 * dongle.conf reload path (CLI "dongle reload", AMI, Asterisk's own
 * reload_module() hook) reloads the same file from the same
 * directory, never a different one. */
void simbox_config_bridge_set_dir(const char *dir);

#endif /* SIMBOX_CONFIG_BRIDGE_H */
