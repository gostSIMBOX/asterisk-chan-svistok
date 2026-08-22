/*
 * Asterisk compatibility shim for chan_simbox
 * manager.h - AMI actions, events, and hooks
 */
#ifndef ASTERISK_MANAGER_H
#define ASTERISK_MANAGER_H

#include <stdio.h>
#include <stdarg.h>

struct mansession {
    int fd;
};

struct message {
    int hdrcount;
    const char *headers[64];
};

typedef int (*ast_manager_action_cb)(struct mansession *s, const struct message *m);

/* Authorization/event masks only need to remain distinct in standalone
 * mode; there is no real AMI permissions engine behind the shim. */
#define EVENT_FLAG_SYSTEM     (1 << 0)
#define EVENT_FLAG_CALL       (1 << 1)
#define EVENT_FLAG_REPORTING  (1 << 2)
#define EVENT_FLAG_CONFIG     (1 << 3)

#ifdef __cplusplus
extern "C" {
#endif

int ast_manager_register2(const char *action, int auth, ast_manager_action_cb func,
                          const void *module, const char *synopsis, const char *description);
int ast_manager_unregister(const char *action);

const char *astman_get_header(const struct message *m, const char *var);
void astman_send_ack(struct mansession *s, const struct message *m, const char *msg);
void astman_send_error(struct mansession *s, const struct message *m, const char *error);
void astman_send_response(struct mansession *s, const struct message *m, const char *resp, const char *msg);
void astman_send_listack(struct mansession *s, const struct message *m,
                         const char *msg, const char *listflag);
void astman_append(struct mansession *s, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

void manager_event(int category, const char *event, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_MANAGER_H */
