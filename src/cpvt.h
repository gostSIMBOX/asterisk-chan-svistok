/*
   Copyright (C) 2010 bg <bg_one@mail.ru>
*/
#ifndef CHAN_DONGLE_CPVT_H_INCLUDED
#define CHAN_DONGLE_CPVT_H_INCLUDED
#ifdef SVISTOK_COMPOSED_CPVT_H_HEADER
#include SVISTOK_COMPOSED_CPVT_H_HEADER
#else

#include <asterisk.h>
#include <asterisk/linkedlists.h>		/* AST_LIST_ENTRY() */
#include <asterisk/frame.h>			/* AST_FRIENDLY_OFFSET */

#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */
#include <asterisk-chan-dongle/mixbuffer.h>	/* struct mixstream */
#include <asterisk-chan-dongle/mutils.h>	/* enum2str() ITEMS_OF() */

/* SVISTOK_BASELINE_UNIT macro FRAME_SIZE */

/* SVISTOK_BASELINE_UNIT typedef call_state_t */
/* SVISTOK_BASELINE_UNIT macro CALL_STATES_NUMBER */

/* SVISTOK_BASELINE_UNIT typedef call_flag_t */


/* */
typedef struct cpvt {
	AST_LIST_ENTRY (cpvt)	entry;				/*!< linked list pointers */

	struct ast_channel*	channel;			/*!< Channel pointer */
	struct ast_channel*	requestor;			/*!< Channel pointer */
	struct pvt		*pvt;				/*!< pointer to device structure */

	short			call_idx;			/*!< device call ID */
/* SVISTOK_BASELINE_UNIT macro MIN_CALL_IDX */
/* SVISTOK_BASELINE_UNIT macro MAX_CALL_IDX */

	call_state_t		state;				/*!< see also call_state_t */
	int			flags;				/*!< see also call_flag_t */

/* TODO: join with flags */
	unsigned int		dir:1;				/*!< call direction */
/* SVISTOK_BASELINE_UNIT macro CALL_DIR_OUTGOING */
/* SVISTOK_BASELINE_UNIT macro CALL_DIR_INCOMING */

	int			rd_pipe[2];			/*!< pipe for split readed from device */
/* SVISTOK_BASELINE_UNIT macro PIPE_READ */
/* SVISTOK_BASELINE_UNIT macro PIPE_WRITE */

	struct mixstream	mixstream;			/*!< mix stream */
	char			a_read_buf[FRAME_SIZE + AST_FRIENDLY_OFFSET];/*!< audio read buffer */
	struct ast_frame	a_read_frame;			/*!< readed frame buffer */

//	size_t			write;				/*!< write position in pvt->a_write_buf */
//	size_t			used;				/*!< bytes used in pvt->a_write_buf */
//	char			a_write_buf[FRAME_SIZE * 5];	/*!< audio write buffer */
//	struct ringbuffer	a_write_rb;			/*!< audio ring buffer */


	long int answered;
} cpvt_t;

/* SVISTOK_BASELINE_UNIT macro CPVT_SET_FLAGS */
/* SVISTOK_BASELINE_UNIT macro CPVT_RESET_FLAGS */
/* SVISTOK_BASELINE_UNIT macro CPVT_TEST_FLAG */
/* SVISTOK_BASELINE_UNIT macro CPVT_TEST_FLAGS */

/* SVISTOK_BASELINE_UNIT macro CPVT_IS_MASTER */
/* SVISTOK_BASELINE_UNIT macro CPVT_IS_ACTIVE */
/* SVISTOK_BASELINE_UNIT macro CPVT_IS_SOUND_SOURCE */


/* SVISTOK_BASELINE_UNIT declaration cpvt_alloc */
/* SVISTOK_BASELINE_UNIT declaration cpvt_free */

/* SVISTOK_BASELINE_UNIT declaration pvt_find_cpvt */
/* SVISTOK_BASELINE_UNIT declaration pvt_call_dir */

#/* */
/* SVISTOK_BASELINE_UNIT function call_state2str */

#endif /* SVISTOK_COMPOSED_CPVT_H_HEADER */
#endif /* CHAN_DONGLE_CPVT_H_INCLUDED */
