/*
 * Asterisk compatibility shim for chan_simbox
 * shim_pbx.c - PBX dialplan execution and channel variables
 */
#include <asterisk/pbx.h>
#include <asterisk/channel.h>
#include <asterisk/logger.h>
#include <stdlib.h>
#include <string.h>

enum ast_pbx_result ast_pbx_start(struct ast_channel *c)
{
    if (!c) return AST_PBX_FAILED;
    ast_verb(2, "PBX start on channel %s (exten: %s, context: %s)\n",
             c->name, c->exten, c->context);
    return AST_PBX_SUCCESS;
}

void pbx_builtin_setvar_helper(struct ast_channel *chan, const char *name, const char *value)
{
    if (!chan || !name) return;

    ast_channel_lock(chan);
    struct ast_var_t *var;
    AST_LIST_TRAVERSE(&chan->varshead, var, entries) {
        if (strcmp(var->name, name) == 0) {
            free(var->value);
            var->value = value ? strdup(value) : strdup("");
            ast_channel_unlock(chan);
            return;
        }
    }

    var = (struct ast_var_t *)calloc(1, sizeof(struct ast_var_t));
    if (var) {
        var->name = strdup(name);
        var->value = value ? strdup(value) : strdup("");
        AST_LIST_INSERT_TAIL(&chan->varshead, var, entries);
    }
    ast_channel_unlock(chan);
}

const char *pbx_builtin_getvar_helper(struct ast_channel *chan, const char *name)
{
    if (!chan || !name) return NULL;

    ast_channel_lock(chan);
    struct ast_var_t *var;
    AST_LIST_TRAVERSE(&chan->varshead, var, entries) {
        if (strcmp(var->name, name) == 0) {
            const char *val = var->value;
            ast_channel_unlock(chan);
            return val;
        }
    }
    ast_channel_unlock(chan);
    return NULL;
}
