/*
 * Asterisk compatibility shim for chan_simbox
 * shim_app.c - Application registration stubs
 */
#include <asterisk/app.h>
#include <asterisk/logger.h>

int ast_register_application2(const char *app, int (*execute)(struct ast_channel *, const char *),
                              const char *synopsis, const char *description, void *mod)
{
    ast_verb(2, "Registered application: %s\n", app ? app : "null");
    return 0;
}

int ast_unregister_application(const char *app)
{
    return 0;
}
