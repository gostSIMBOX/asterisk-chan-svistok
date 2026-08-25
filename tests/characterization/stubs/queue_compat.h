#ifndef SVISTOK_CHARACTERIZATION_QUEUE_COMPAT_H
#define SVISTOK_CHARACTERIZATION_QUEUE_COMPAT_H

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#define CHAN_DONGLE_AT_CMD_QUEUE_H_INCLUDED
#define CHAN_DONGLE_H_INCLUDED
#define EXPORT_DEF
#define LOG_ERROR 3
#define SVISTOK_AST_TIME_DEFINED

typedef int at_cmd_t;
typedef int at_res_t;

#define ATQ_CMD_FLAG_DEFAULT 0x00
#define ATQ_CMD_FLAG_STATIC 0x01
#define ATQ_CMD_FLAG_IGNORE 0x02

typedef struct at_queue_cmd {
    at_cmd_t cmd;
    at_res_t res;
    unsigned flags;
    struct timeval timeout;
    char *data;
    unsigned length;
} at_queue_cmd_t;

struct cpvt;

typedef struct at_queue_task {
    struct { struct at_queue_task *next; } entry;
    unsigned cmdsno;
    unsigned cindex;
    struct cpvt *cpvt;
    at_queue_cmd_t cmds[0];
} at_queue_task_t;

struct at_queue_head {
    at_queue_task_t *first;
    at_queue_task_t **last;
};

struct queue_counters {
    int at_tasks;
    int at_cmds;
    size_t d_write_bytes;
};

struct pvt {
    struct at_queue_head at_queue;
    struct queue_counters state;
    struct queue_counters stat;
    int data_fd;
    const char *id;
};

typedef struct pvt pvt_t;

struct cpvt {
    struct pvt *pvt;
};

#define PVT_STATE(pvt, field) ((pvt)->state.field)
#define PVT_STAT(pvt, field) ((pvt)->stat.field)
#define PVT_ID(pvt) ((pvt)->id)

#define AST_LIST_FIRST(head) ((head)->first)
#define AST_LIST_REMOVE_HEAD(head, field) __extension__ ({ \
    at_queue_task_t *_item = (head)->first; \
    if (_item) { \
        (head)->first = _item->field.next; \
        if (!(head)->first) (head)->last = &(head)->first; \
    } \
    _item; \
})
#define AST_LIST_INSERT_AFTER(head, listelm, elm, field) do { \
    (elm)->field.next = (listelm)->field.next; \
    (listelm)->field.next = (elm); \
    if (!(elm)->field.next) (head)->last = &(elm)->field.next; \
} while (0)
#define AST_LIST_INSERT_TAIL(head, elm, field) do { \
    (elm)->field.next = NULL; \
    *(head)->last = (elm); \
    (head)->last = &(elm)->field.next; \
} while (0)

#define ast_debug(...) ((void)0)
#define ast_log(...) ((void)0)

static const char *at_cmd2str(at_cmd_t value) { (void)value; return "cmd"; }
static const char *at_res2str(at_res_t value) { (void)value; return "res"; }
static struct timeval ast_tvnow(void) { struct timeval value = {0, 0}; return value; }
static struct timeval ast_tvadd(struct timeval left, struct timeval right) {
    left.tv_sec += right.tv_sec;
    left.tv_usec += right.tv_usec;
    return left;
}
static int ast_tvdiff_ms(struct timeval left, struct timeval right) {
    return (int)((left.tv_sec - right.tv_sec) * 1000 +
        (left.tv_usec - right.tv_usec) / 1000);
}
static void timenow(char *value) { strcpy(value, "normalized-time"); }
static const at_queue_cmd_t *at_queue_task_cmd(const at_queue_task_t *task) {
    return task ? &task->cmds[task->cindex] : NULL;
}

int at_queue_insert_const(struct cpvt *, const at_queue_cmd_t *, unsigned, int);
void at_queue_handle_result(struct pvt *, at_res_t);
void at_queue_flush(struct pvt *);
const at_queue_task_t *at_queue_head_task(const struct pvt *);
const at_queue_cmd_t *at_queue_head_cmd(const struct pvt *);

#endif
