#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_H

#include <stdint.h>
#include <stddef.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <asterisk/strings.h>

#define ASTERISK_VERSION_NUM 110000
#define attribute_unused __attribute__((unused))
#define LOG_ERROR 1
#define LOG_WARNING 2
#define LOG_NOTICE 3
#define DEFAULT_LANGUAGE "en"
#define S_OR(value, fallback) ((value) ? (value) : (fallback))
#define ast_strdup strdup
#define ast_free free
#define ast_malloc malloc
#define ast_calloc calloc
#define ast_strdupa strdup
#define AST_STATE_RING 4
#define AST_STATE_DOWN 0
#define AST_STATE_RESERVED 1
#define AST_STATE_DIALING 2
#define AST_STATE_RINGING 3
#define AST_STATE_UP 5
#define AST_DEVICE_INVALID -1
#define AST_DEVICE_NOT_INUSE 0
#define AST_DEVICE_INUSE 1
#define AST_FORMAT_AUDIO_MASK ((format_t)~0ULL)
#define ast_strlen_zero(value) (!(value) || !*(value))
#define AST_MODULE "chan_svistok"
#define MODULE_VERSION "legacy"
#define PACKAGE_REVISION "oracle"
#define MODULE_URL "local"
#define MODULE_BUGREPORT "local"
#define ast_string_field_set(object, field, value) ((void)0)
#define ASTERISK_FILE_VERSION(...)
#define AST_PTHREADT_NULL ((pthread_t)0)
#define AST_PTHREADT_STOP ((pthread_t)-1)
#define ASTERISK_GPL_KEY "GPL"
#define AST_MODFLAG_DEFAULT 0
#define AST_MODULE_INFO(...)
#define AST_FORMAT_CAP_FLAG_DEFAULT 0

typedef unsigned long long format_t;
struct ast_config;
struct ast_format { int id; };
struct ast_format_cap;
struct ast_module;
struct ast_timer;
struct ast_dsp;
struct ast_flags { unsigned int flags; };
struct ast_variable { const char *name; const char *value; struct ast_variable *next; };
struct ast_var_t { struct { struct ast_var_t *next; } entries; const char *name; const char *value; };
struct ast_var_head { struct ast_var_t *first; struct ast_var_t **last; };
struct ast_party_number { int presentation; };
struct ast_party_id { struct ast_party_number number; };
struct ast_party_connected_line { struct ast_party_id id; };
struct ast_channel_tech;
struct ast_channel {
    void *tech_pvt;
    int rings;
    int hangupcause;
    int _state;
    const char *name;
    const char *linkedid;
    const struct ast_channel_tech *tech;
    format_t nativeformats;
    format_t writeformat;
    format_t readformat;
    int fdno;
    struct ast_party_connected_line connected;
};

const char *ast_variable_retrieve(struct ast_config *, const char *, const char *);
struct ast_variable *ast_variable_browse(struct ast_config *, const char *);
void *ast_channel_tech_pvt(struct ast_channel *);
struct ast_party_connected_line *ast_channel_connected(struct ast_channel *);
struct ast_channel *ast_bridged_channel(struct ast_channel *);
struct ast_channel *ast_channel_alloc(int, int, const char *, const char *, const char *, const char *, const char *, const char *, int, const char *, ...);
struct ast_channel *ast_request(const char *, ...);
struct ast_var_head *ast_channel_varshead(const struct ast_channel *);
const char *ast_var_full_name(const struct ast_var_t *);
const char *ast_var_value(const struct ast_var_t *);
struct ast_config *ast_config_load(const char *, struct ast_flags);
struct ast_format_cap *ast_format_cap_alloc();
struct ast_timer *ast_timer_open(void);

struct ast_module_info_stub { struct ast_module *self; };
static struct ast_module_info_stub *ast_module_info;

void getfilei_def(char *, char *, char *, int *, int);
void getfiles_def(char *, char *, char *, char *, char *);
void getfiles_def2(char *, char *, char *, char *, char *);

#ifndef SVISTOK_AST_TIME_DEFINED
static inline struct timeval ast_tvnow(void) { struct timeval value = {0, 0}; return value; }
static inline int ast_tvcmp(struct timeval left, struct timeval right) {
    return left.tv_sec == right.tv_sec ?
        (left.tv_usec > right.tv_usec) - (left.tv_usec < right.tv_usec) :
        (left.tv_sec > right.tv_sec) - (left.tv_sec < right.tv_sec);
}
static inline struct timeval ast_tvadd(struct timeval left, struct timeval right) {
    struct timeval value = {left.tv_sec + right.tv_sec, left.tv_usec + right.tv_usec};
    return value;
}
static inline int ast_tvdiff_ms(struct timeval left, struct timeval right) {
    return (int)((left.tv_sec - right.tv_sec) * 1000 +
        (left.tv_usec - right.tv_usec) / 1000);
}
#endif

#endif
