/*
 * Asterisk compatibility shim for chan_simbox
 * ulaw.h - u-law audio transcoding tables & macros
 */
#ifndef ASTERISK_ULAW_H
#define ASTERISK_ULAW_H

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned char __ast_lin2mu[16384];
extern short __ast_mulaw[256];

#define AST_LIN2MU(a) (__ast_lin2mu[((unsigned short)(a)) >> 2])
#define AST_MULAW(a)  (__ast_mulaw[(unsigned char)(a)])

void ast_ulaw_init(void);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_ULAW_H */
