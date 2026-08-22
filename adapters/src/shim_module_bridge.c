/*
 * Adapter-side bridge to chan_dongle.c's module lifecycle — see
 * ../include/simbox_module_bridge.h.
 */
#include "simbox_module_bridge.h"
#include <asterisk/module.h>
#include <stddef.h>

int simbox_module_bridge_load(void)
{
    if (!ast_module_info || !ast_module_info->load) {
        return -1;
    }
    return ast_module_info->load();
}

void simbox_module_bridge_unload(void)
{
    if (ast_module_info && ast_module_info->unload) {
        ast_module_info->unload();
    }
}
