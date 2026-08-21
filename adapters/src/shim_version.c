/*
 * Asterisk compatibility shim for chan_simbox
 * shim_version.c - Asterisk version reporting
 */
#include <asterisk/ast_version.h>

static const char *simbox_asterisk_version = "11.0.0-simbox";

const char *ast_get_version(void)
{
    return simbox_asterisk_version;
}
