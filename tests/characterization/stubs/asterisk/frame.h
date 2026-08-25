#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_FRAME_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_FRAME_H
#define AST_FRIENDLY_OFFSET 64
enum ast_control_frame_type {
    AST_CONTROL_HANGUP = 1,
    AST_CONTROL_BUSY = 2,
    AST_CONTROL_CONGESTION = 3,
    AST_CONTROL_PROGRESS = 4,
    AST_CONTROL_ANSWER = 5,
    AST_CONTROL_RINGING = 6,
    AST_CONTROL_PROCEEDING = 7,
    AST_CONTROL_VIDUPDATE = 8,
    AST_CONTROL_SRCUPDATE = 9,
    AST_CONTROL_SRCCHANGE = 12,
    AST_CONTROL_HOLD = 10,
    AST_CONTROL_UNHOLD = 11
};
#define AST_FRAME_NULL 0
#define AST_FRAME_VOICE 1
#define AST_FRAME_CONTROL 2
#define AST_FRAME_DTMF_BEGIN 3
#define AST_FRAME_DTMF_END 4
#define AST_FORMAT_SLINEAR 1
struct ast_frame_subclass { int integer; format_t codec; struct ast_format format; };
struct ast_frame_data { void *ptr; };
struct ast_frame { int frametype; struct ast_frame_subclass subclass; struct ast_frame_data data; int datalen; int samples; int mallocd; int offset; int len; const char *src; };
extern struct ast_frame ast_null_frame;
#endif
