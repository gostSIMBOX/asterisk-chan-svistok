/*
 * Asterisk compatibility shim for chan_simbox
 * channel.h - Channel abstractions, callbacks, and accessors
 */
#ifndef ASTERISK_CHANNEL_H
#define ASTERISK_CHANNEL_H

#include <asterisk/asterisk.h>
#include <asterisk/lock.h>
#include <asterisk/linkedlists.h>
#include <asterisk/format.h>
#include <asterisk/format_cap.h>
#include <asterisk/frame.h>
#include <asterisk/devicestate.h>

#define AST_MAX_CONTEXT      80
#define AST_MAX_EXTENSION    80
#define MAX_LANGUAGE         20
#define DEFAULT_LANGUAGE     "en"
#define AST_MAX_ACCOUNT_CODE 20

#define AST_CAUSE_NOTDEFINED 0
#define AST_CAUSE_NORMAL_CLEARING 16
#define AST_CAUSE_USER_BUSY 17
#define AST_CAUSE_NO_ANSWER 19
#define AST_CAUSE_CALL_REJECTED 21
#define AST_CAUSE_CONGESTION 34
#define AST_CAUSE_FAILURE 38

#define AST_SOFTHANGUP_DEV       (1 << 0)
#define AST_SOFTHANGUP_ASYNCGOTO (1 << 1)
#define AST_SOFTHANGUP_SHUTDOWN  (1 << 2)
#define AST_SOFTHANGUP_TIMEOUT   (1 << 3)
#define AST_SOFTHANGUP_APPUNLOAD (1 << 4)
#define AST_SOFTHANGUP_EXPLICIT  (1 << 5)
#define AST_SOFTHANGUP_UNBRIDGE  (1 << 6)

enum ast_channel_state {
    AST_STATE_DOWN = 0,
    AST_STATE_RESERVED,
    AST_STATE_OFFHOOK,
    AST_STATE_DIALING,
    AST_STATE_RING,
    AST_STATE_RINGING,
    AST_STATE_UP,
    AST_STATE_BUSY,
    AST_STATE_DIALING_OFFHOOK,
    AST_STATE_PRERING,
    AST_STATE_MUTE = (1 << 16),
};

struct ast_jb_conf {
    char flags;
    long max_size;
    long resync_threshold;
    char impl[20];
    long target_extra;
    long max_consecutive_interp;
};

static inline void ast_jb_read_conf(struct ast_jb_conf *conf, const char *varname, const char *value)
{
    /* stub */
}

#define ast_jb_configure(chan, conf) do { } while (0)

struct ast_var_t {
    AST_LIST_ENTRY(ast_var_t) entries;
    char *name;
    char *value;
};

#define ast_var_full_name(v) ((v) ? (v)->name : "")
#define ast_var_name(v)      ((v) ? (v)->name : "")
#define ast_var_value(v)     ((v) ? (v)->value : "")

struct ast_party_number {
    char *str;
    int plan;
    int presentation;
    int valid;
};

struct ast_party_id {
    struct ast_party_number number;
    char *name;
};

struct ast_party_connected_line {
    struct ast_party_id id;
};

struct ast_channel;

struct ast_channel_tech {
    const char * const type;
    const char * const description;
    struct ast_format_cap *capabilities;
    struct ast_channel * (* const requester)(const char *type, struct ast_format_cap *cap, const struct ast_channel *requestor, void *data, int *cause);
    int (* const devicestate)(void *data);
    int (* const call)(struct ast_channel *chan, char *addr, int timeout);
    int (* const hangup)(struct ast_channel *chan);
    int (* const answer)(struct ast_channel *chan);
    struct ast_frame * (* const read)(struct ast_channel *chan);
    int (* const write)(struct ast_channel *chan, struct ast_frame *frame);
    struct ast_frame * (* const exception)(struct ast_channel *chan);
    int (* const indicate)(struct ast_channel *chan, int condition, const void *data, size_t datalen);
    int (* const fixup)(struct ast_channel *oldchan, struct ast_channel *newchan);
    int (* const send_digit_begin)(struct ast_channel *chan, char digit);
    int (* const send_digit_end)(struct ast_channel *chan, char digit, unsigned int duration);
    int (* const early_bridge)(struct ast_channel *c0, struct ast_channel *c1);
    int (* const func_channel_read)(struct ast_channel *chan, const char *function, char *data, char *buf, size_t len);
    int (* const func_channel_write)(struct ast_channel *chan, const char *function, char *data, const char *value);
};

struct ast_channel {
    char name[80];
    char context[AST_MAX_CONTEXT];
    char exten[AST_MAX_EXTENSION];
    char language[MAX_LANGUAGE];
    char accountcode[AST_MAX_ACCOUNT_CODE];
    enum ast_channel_state state;
    int fds[16];
    int fdno;
    int rings;
    int hangupcause;
    int softhangup;
    void *tech_pvt;
    const struct ast_channel_tech *tech;
    struct ast_format_cap *nativeformats;
    struct ast_format readformat;
    struct ast_format writeformat;
    struct ast_format rawreadformat;
    struct ast_format rawwriteformat;
    struct ast_party_connected_line connected;
    AST_LIST_HEAD_NOLOCK(, ast_var_t) varshead;
    ast_mutex_t lock;
    void *music_state;
    void *dsp;
    struct ast_channel *bridge;
};

#define ast_channel_name(c) ((c) ? (c)->name : "unknown")
#define ast_channel_state(c) ((c) ? (c)->state : AST_STATE_DOWN)
#define ast_channel_state_set(c, s) do { if (c) (c)->state = (s); } while (0)
#define ast_channel_tech(c) ((c) ? (c)->tech : NULL)
#define ast_channel_tech_set(c, t) do { if (c) (c)->tech = (t); } while (0)
#define ast_channel_tech_pvt(c) ((c) ? (c)->tech_pvt : NULL)
#define ast_channel_tech_pvt_set(c, p) do { if (c) (c)->tech_pvt = (p); } while (0)
#define ast_channel_rings(c) ((c) ? (c)->rings : 0)
#define ast_channel_rings_set(c, r) do { if (c) (c)->rings = (r); } while (0)
#define ast_channel_hangupcause(c) ((c) ? (c)->hangupcause : 0)
#define ast_channel_hangupcause_set(c, h) do { if (c) (c)->hangupcause = (h); } while (0)
#define ast_channel_nativeformats(c) ((c) ? (c)->nativeformats : NULL)
#define ast_channel_nativeformats_set(c, f) do { if (c) (c)->nativeformats = (f); } while (0)
#define ast_channel_readformat(c) (&(c)->readformat)
#define ast_channel_writeformat(c) (&(c)->writeformat)
#define ast_channel_rawreadformat(c) (&(c)->rawreadformat)
#define ast_channel_rawwriteformat(c) (&(c)->rawwriteformat)
#define ast_channel_exten(c) ((c)->exten)
#define ast_channel_exten_set(c, e) do { if (c) { strncpy((c)->exten, (e), sizeof((c)->exten)-1); (c)->exten[sizeof((c)->exten)-1] = '\0'; } } while (0)
#define ast_channel_context(c) ((c)->context)
#define ast_channel_context_set(c, ctx) do { if (c) { strncpy((c)->context, (ctx), sizeof((c)->context)-1); (c)->context[sizeof((c)->context)-1] = '\0'; } } while (0)
#define ast_channel_language(c) ((c)->language)
#define ast_channel_language_set(c, l) do { if (c) { strncpy((c)->language, (l), sizeof((c)->language)-1); (c)->language[sizeof((c)->language)-1] = '\0'; } } while (0)
#define ast_channel_accountcode(c) ((c)->accountcode)
#define ast_channel_accountcode_set(c, a) do { if (c) { strncpy((c)->accountcode, (a), sizeof((c)->accountcode)-1); (c)->accountcode[sizeof((c)->accountcode)-1] = '\0'; } } while (0)
#define ast_channel_fd(c, i) ((c)->fds[i])
#define ast_channel_set_fd(c, i, fd) do { if (c && (i) >= 0 && (i) < 16) (c)->fds[i] = (fd); } while (0)
#define ast_channel_fdno(c) ((c)->fdno)
#define ast_channel_set_fdno(c, n) do { if (c) (c)->fdno = (n); } while (0)
#define ast_channel_varshead(c) (&(c)->varshead)
#define ast_channel_connected(c) (&(c)->connected)
#define ast_channel_lock(c) ast_mutex_lock(&(c)->lock)
#define ast_channel_unlock(c) ast_mutex_unlock(&(c)->lock)
#define ast_channel_trylock(c) ast_mutex_trylock(&(c)->lock)
#define ast_channel_bridge_peer(c) ((c) ? (c)->bridge : NULL)
#define ast_bridged_channel(c) ((c) ? (c)->bridge : NULL)
#define ast_channel_softhangup_internal_flag(c) ((c) ? (c)->softhangup : 0)
#define ast_channel_softhangup_internal_flag_set(c, f) do { if (c) (c)->softhangup = (f); } while (0)
#define ast_channel_softhangup_internal_flag_add(c, f) do { if (c) (c)->softhangup |= (f); } while (0)
#define ast_channel_linkedid(c) ((c) ? "linkedid" : NULL)

#define CHANNEL_DEADLOCK_AVOIDANCE(chan) do { \
    if (chan) { \
        ast_channel_unlock(chan); \
        usleep(1); \
        ast_channel_lock(chan); \
    } \
} while (0)

#ifdef __cplusplus
extern "C" {
#endif

void ast_channel_get_var(const struct ast_channel *parent, char *varname1, char *value);

int ast_channel_register(const struct ast_channel_tech *tech);
void ast_channel_unregister(const struct ast_channel_tech *tech);

struct ast_channel *ast_channel_alloc(int needqueue, int state, const char *cid_num,
                                      const char *cid_name, const char *acctcode,
                                      const char *exten, const char *context,
                                      const char *linkedid, const int amaflag,
                                      const char *name_fmt, ...)
    __attribute__((format(printf, 10, 11)));

void ast_channel_free(struct ast_channel *chan);
int ast_hangup(struct ast_channel *chan);
int ast_softhangup(struct ast_channel *chan, int reason);
int ast_softhangup_nolock(struct ast_channel *chan, int reason);
int ast_setstate(struct ast_channel *chan, enum ast_channel_state state);

int ast_queue_frame(struct ast_channel *chan, struct ast_frame *f);
int ast_queue_hangup(struct ast_channel *chan);
int ast_queue_control(struct ast_channel *chan, enum ast_control_frame_type control);
int ast_queue_control_data(struct ast_channel *chan, enum ast_control_frame_type control,
                           const void *data, size_t datalen);

int ast_waitfor_n_fd(int *fds, int n, int *ms, int *exception);

struct ast_channel *ast_request(const char *type, struct ast_format_cap *cap, const struct ast_channel *requestor, const char *data, int *cause);
static inline void ast_set_callerid(struct ast_channel *chan, const char *cid_num, const char *cid_name, const char *cid_ani) { }

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_CHANNEL_H */
