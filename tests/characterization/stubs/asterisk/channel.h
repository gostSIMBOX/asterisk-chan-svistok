#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_CHANNEL_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_CHANNEL_H

#define AST_MAX_CONTEXT 80
#define AST_MAX_EXTENSION 80
#define MAX_LANGUAGE 40

struct ast_channel;
struct ast_jb_conf { unsigned int flags; long max_size; long resync_threshold; char impl[32]; long target_extra; };
struct ast_channel_tech {
    const char *type;
    const char *description;
    void *requester;
    void *call;
    void *hangup;
    void *answer;
    void *send_digit_begin;
    void *send_digit_end;
    void *read;
    void *write;
    void *exception;
    void *fixup;
    void *devicestate;
    void *indicate;
    void *func_channel_read;
    void *func_channel_write;
    format_t capabilities;
};
int ast_waitfor_n_fd(int *fds, int count, int *milliseconds, int *exception);

#endif
