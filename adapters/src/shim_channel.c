/*
 * Asterisk compatibility shim for chan_simbox
 * shim_channel.c - Channel allocation, lifecycle, and event wait
 */
#include <asterisk/channel.h>
#include <asterisk/logger.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <poll.h>

static const struct ast_channel_tech *registered_tech = NULL;

int ast_channel_register(const struct ast_channel_tech *tech)
{
    registered_tech = tech;
    ast_verb(2, "Registered channel tech: %s (%s)\n", tech ? tech->type : "null", tech ? tech->description : "");
    return 0;
}

void ast_channel_unregister(const struct ast_channel_tech *tech)
{
    if (registered_tech == tech) {
        registered_tech = NULL;
    }
}

struct ast_channel *ast_channel_alloc(int needqueue, int state, const char *cid_num,
                                      const char *cid_name, const char *acctcode,
                                      const char *exten, const char *context,
                                      const char *linkedid, const int amaflag,
                                      const char *name_fmt, ...)
{
    struct ast_channel *chan = (struct ast_channel *)calloc(1, sizeof(struct ast_channel));
    if (!chan) return NULL;

    chan->state = (enum ast_channel_state)state;
    if (context) strncpy(chan->context, context, sizeof(chan->context) - 1);
    if (exten) strncpy(chan->exten, exten, sizeof(chan->exten) - 1);
    if (acctcode) strncpy(chan->accountcode, acctcode, sizeof(chan->accountcode) - 1);
    strncpy(chan->language, DEFAULT_LANGUAGE, sizeof(chan->language) - 1);

    if (cid_num) {
        chan->connected.id.number.str = strdup(cid_num);
        chan->connected.id.number.valid = 1;
    }
    if (cid_name) {
        chan->connected.id.name = strdup(cid_name);
    }

    va_list ap;
    va_start(ap, name_fmt);
    vsnprintf(chan->name, sizeof(chan->name), name_fmt, ap);
    va_end(ap);

    for (int i = 0; i < 16; i++) {
        chan->fds[i] = -1;
    }
    chan->fdno = -1;

    ast_mutex_init(&chan->lock);
    AST_LIST_HEAD_INIT_NOLOCK(&chan->varshead);

    chan->tech = registered_tech;
    chan->nativeformats = ast_format_cap_alloc();
    ast_format_set(&chan->readformat, AST_FORMAT_SLINEAR, 0);
    ast_format_set(&chan->writeformat, AST_FORMAT_SLINEAR, 0);
    ast_format_set(&chan->rawreadformat, AST_FORMAT_SLINEAR, 0);
    ast_format_set(&chan->rawwriteformat, AST_FORMAT_SLINEAR, 0);

    return chan;
}

void ast_channel_free(struct ast_channel *chan)
{
    if (!chan) return;

    if (chan->connected.id.number.str) free(chan->connected.id.number.str);
    if (chan->connected.id.name) free(chan->connected.id.name);

    struct ast_var_t *var;
    while ((var = AST_LIST_REMOVE_HEAD(&chan->varshead, entries))) {
        if (var->name) free(var->name);
        if (var->value) free(var->value);
        free(var);
    }

    if (chan->nativeformats) {
        ast_format_cap_destroy(chan->nativeformats);
    }

    ast_mutex_destroy(&chan->lock);
    free(chan);
}

int ast_hangup(struct ast_channel *chan)
{
    if (!chan) return -1;
    if (chan->tech && chan->tech->hangup) {
        chan->tech->hangup(chan);
    }
    ast_channel_free(chan);
    return 0;
}

int ast_softhangup(struct ast_channel *chan, int reason)
{
    if (!chan) return -1;
    ast_channel_lock(chan);
    chan->softhangup |= reason;
    ast_channel_unlock(chan);
    return 0;
}

int ast_softhangup_nolock(struct ast_channel *chan, int reason)
{
    if (!chan) return -1;
    chan->softhangup |= reason;
    return 0;
}

int ast_setstate(struct ast_channel *chan, enum ast_channel_state state)
{
    if (!chan) return -1;
    chan->state = state;
    return 0;
}

int ast_waitfor_n_fd(int *fds, int n, int *ms, int *exception)
{
    if (!fds || n <= 0) return -1;

    struct pollfd pfd[16];
    int count = (n > 16) ? 16 : n;

    for (int i = 0; i < count; i++) {
        pfd[i].fd = fds[i];
        pfd[i].events = POLLIN | POLLPRI;
        pfd[i].revents = 0;
    }

    int timeout = (ms) ? *ms : -1;
    int res = poll(pfd, count, timeout);

    if (exception) *exception = 0;

    if (res > 0) {
        for (int i = 0; i < count; i++) {
            if (pfd[i].revents & (POLLIN | POLLPRI)) {
                if (exception && (pfd[i].revents & POLLPRI))
                    *exception = 1;
                return pfd[i].fd;
            }
        }
    }

    return (res == 0) ? -1 : -1;
}

struct ast_channel *ast_request(const char *type, struct ast_format_cap *cap,
                                const struct ast_channel *requestor, const char *data, int *cause)
{
    if (registered_tech && registered_tech->requester) {
        return registered_tech->requester(type, cap, requestor, (void *)data, cause);
    }
    if (cause) *cause = AST_CAUSE_FAILURE;
    return NULL;
}
