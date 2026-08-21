/*
 * Asterisk compatibility shim for chan_simbox
 * ast_version.h - Asterisk version reporting
 */
#ifndef ASTERISK_AST_VERSION_H
#define ASTERISK_AST_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

const char *ast_get_version(void);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_AST_VERSION_H */
