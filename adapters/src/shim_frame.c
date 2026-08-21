/*
 * Asterisk compatibility shim for chan_simbox
 * shim_frame.c - Frame queueing and static null frame definition
 */
#include <asterisk/frame.h>
#include <asterisk/channel.h>
#include <asterisk/logger.h>

struct ast_frame ast_null_frame = {
    .frametype = AST_FRAME_NULL,
    .subclass = { .integer = 0 },
    .datalen = 0,
    .samples = 0,
    .mallocd = 0,
    .offset = 0,
    .len = 0,
    .src = "null",
    .data = { .ptr = NULL },
    .frame_list = { .next = NULL }
};

int ast_queue_frame(struct ast_channel *chan, struct ast_frame *f)
{
    if (!chan || !f) return -1;
    /* In standalone chan_simbox mode, frames queued to channel can trigger tech callbacks or be processed */
    return 0;
}

int ast_queue_hangup(struct ast_channel *chan)
{
    if (!chan) return -1;
    chan->state = AST_STATE_DOWN;
    chan->softhangup |= AST_SOFTHANGUP_DEV;
    return 0;
}

int ast_queue_control(struct ast_channel *chan, enum ast_control_frame_type control)
{
    if (!chan) return -1;
    switch (control) {
    case AST_CONTROL_ANSWER:
        chan->state = AST_STATE_UP;
        break;
    case AST_CONTROL_RINGING:
        chan->state = AST_STATE_RINGING;
        break;
    case AST_CONTROL_BUSY:
        chan->state = AST_STATE_BUSY;
        break;
    case AST_CONTROL_CONGESTION:
        chan->state = AST_STATE_BUSY;
        break;
    case AST_CONTROL_HANGUP:
        chan->state = AST_STATE_DOWN;
        break;
    default:
        break;
    }
    return 0;
}

int ast_queue_control_data(struct ast_channel *chan, enum ast_control_frame_type control,
                           const void *data, size_t datalen)
{
    return ast_queue_control(chan, control);
}
