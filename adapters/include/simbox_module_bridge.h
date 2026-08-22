/*
 * Adapter-side bridge to chan_dongle.c's module lifecycle
 * (load_module/unload_module/reload_module), captured via the
 * AST_MODULE_INFO macro (see adapters/include/asterisk/module.h).
 * src/simbox_api.c uses this to trigger chan_dongle's real,
 * unmodified, config-file-driven device population — see
 * flows/sdd-asterisk-chan-simbox/02-specifications.md §9.1.
 */
#ifndef SIMBOX_MODULE_BRIDGE_H
#define SIMBOX_MODULE_BRIDGE_H

/* Calls chan_dongle.c's real load_module() via the captured
 * ast_module_info->load pointer. Returns AST_MODULE_LOAD_SUCCESS (0)
 * on success, chan_dongle's own load_module() return value otherwise.
 * Returns -1 if ast_module_info/.load isn't available at all (macro
 * expansion didn't run — chan_dongle.c wasn't compiled into this
 * binary, which should never happen given the root Makefile always
 * includes it, but checked rather than assumed). */
int simbox_module_bridge_load(void);

/* Calls chan_dongle.c's real unload_module(), if available. No-op if
 * not. */
void simbox_module_bridge_unload(void);

#endif /* SIMBOX_MODULE_BRIDGE_H */
