/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifndef CHAN_DONGLE_CHANNEL_H_INCLUDED
#define CHAN_DONGLE_CHANNEL_H_INCLUDED
#ifdef SVISTOK_COMPOSED_CHANNEL_H_HEADER
#include SVISTOK_COMPOSED_CHANNEL_H_HEADER
#else

#include <asterisk.h>
#include <asterisk/frame.h>		/* enum ast_control_frame_type */

#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */


/* SVISTOK_BASELINE_UNIT typedef channel_var_t */

/* SVISTOK_BASELINE_UNIT record pvt */
/* SVISTOK_BASELINE_UNIT record cpvt */

EXPORT_DECL struct ast_channel_tech channel_tech;

/* SVISTOK_BASELINE_UNIT declaration new_channel */
/* SVISTOK_BASELINE_UNIT declaration queue_control_channel */
/* SVISTOK_BASELINE_UNIT declaration queue_hangup */
/* SVISTOK_BASELINE_UNIT declaration start_local_channel */
/* SVISTOK_BASELINE_UNIT declaration change_channel_state */
/* SVISTOK_BASELINE_UNIT declaration channels_loop */



#endif /* SVISTOK_COMPOSED_CHANNEL_H_HEADER */
#endif /* CHAN_DONGLE_CHANNEL_H_INCLUDED */
