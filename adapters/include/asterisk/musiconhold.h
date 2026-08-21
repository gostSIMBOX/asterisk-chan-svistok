/*
 * Asterisk compatibility shim for chan_simbox
 * musiconhold.h - Music on hold no-op stubs
 */
#ifndef ASTERISK_MUSICONHOLD_H
#define ASTERISK_MUSICONHOLD_H

#include <asterisk/channel.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline int ast_moh_start(struct ast_channel *chan, const char *mclass, const char *interpclass)
{
    return 0;
}

static inline void ast_moh_stop(struct ast_channel *chan)
{
}

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_MUSICONHOLD_H */
