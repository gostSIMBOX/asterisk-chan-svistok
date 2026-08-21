/*
 * Asterisk compatibility shim for chan_simbox
 * shim_manager.c - Manager interface response and header helpers
 */
#include <asterisk/manager.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void astman_send_response(struct mansession *s, const struct message *m, const char *resp, const char *msg)
{
    if (s && s->fd >= 0) {
        dprintf(s->fd, "Response: %s\r\nMessage: %s\r\n\r\n", resp ? resp : "Success", msg ? msg : "");
    }
}

void astman_send_ack(struct mansession *s, const struct message *m, const char *msg)
{
    astman_send_response(s, m, "Success", msg);
}

void astman_send_error(struct mansession *s, const struct message *m, const char *error)
{
    astman_send_response(s, m, "Error", error);
}

void astman_append(struct mansession *s, const char *fmt, ...)
{
    if (!s || s->fd < 0) return;
    va_list ap;
    va_start(ap, fmt);
    vdprintf(s->fd, fmt, ap);
    va_end(ap);
}

const char *astman_get_header(const struct message *m, const char *var)
{
    if (!m || !var) return "";
    for (int i = 0; i < m->hdrcount; i++) {
        if (m->headers[i]) {
            char *eq = strchr(m->headers[i], ':');
            if (eq) {
                size_t keylen = eq - m->headers[i];
                if (strncasecmp(m->headers[i], var, keylen) == 0 && var[keylen] == '\0') {
                    char *val = eq + 1;
                    while (*val == ' ') val++;
                    return val;
                }
            }
        }
    }
    return "";
}

int ast_manager_register2(const char *action, int auth, ast_manager_action_cb func,
                          const char *module, const char *synopsis, const char *description)
{
    return 0;
}

int ast_manager_unregister(const char *action)
{
    return 0;
}
