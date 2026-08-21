/*
 * Asterisk compatibility shim for chan_simbox
 * alaw.h - A-law audio transcoding tables & macros
 */
#ifndef ASTERISK_ALAW_H
#define ASTERISK_ALAW_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char __ast_lin2a[16384];
extern short __ast_alaw[256];

#define AST_LIN2A(a) (__ast_lin2a[((unsigned short)(a)) >> 2])
#define AST_ALAW(a)  (__ast_alaw[(unsigned char)(a)])

void ast_alaw_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_ALAW_H */
