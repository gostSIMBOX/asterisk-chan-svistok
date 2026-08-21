/*
 * Asterisk compatibility shim for chan_simbox
 * pbx.h - Dialplan, channel variables, and PBX invocation
 */
#ifndef ASTERISK_PBX_H
#define ASTERISK_PBX_H

#include <asterisk/channel.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ast_pbx_result {
    AST_PBX_SUCCESS = 0,
    AST_PBX_FAILED = -1,
    AST_PBX_CALL_LIMIT = -2,
};

enum ast_pbx_result ast_pbx_start(struct ast_channel *chan);

void pbx_builtin_setvar_helper(struct ast_channel *chan, const char *name, const char *value);
const char *pbx_builtin_getvar_helper(struct ast_channel *chan, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_PBX_H */
