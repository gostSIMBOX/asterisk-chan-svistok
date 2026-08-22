/*
 * Asterisk compatibility shim for chan_simbox
 * shim_manager.c - Manager interface response and header helpers
 */
#include <asterisk/manager.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
#include "simbox_internal_linux.h"
#endif

#ifdef __linux__
static int payload_header(const char *payload, const char *name,
                          char *out, size_t out_size)
{
    size_t name_len;
    const char *line;

    if (!payload || !name || !out || out_size == 0) return 0;
    name_len = strlen(name);
    line = payload;
    while (*line) {
        const char *end = strstr(line, "\r\n");
        size_t line_len = end ? (size_t)(end - line) : strlen(line);
        if (line_len > name_len + 1 &&
            strncmp(line, name, name_len) == 0 && line[name_len] == ':') {
            const char *value = line + name_len + 1;
            size_t value_len;
            while (value < line + line_len && *value == ' ') value++;
            value_len = (size_t)((line + line_len) - value);
            if (value_len >= out_size) value_len = out_size - 1;
            memcpy(out, value, value_len);
            out[value_len] = '\0';
            return 1;
        }
        if (!end) break;
        line = end + 2;
    }
    out[0] = '\0';
    return 0;
}

static char *payload_message_lines(const char *payload)
{
    const char *line;
    char *result;
    size_t used = 0;
    size_t capacity;

    if (!payload) return NULL;
    capacity = strlen(payload) + 1;
    result = (char *)malloc(capacity);
    if (!result) return NULL;
    result[0] = '\0';

    line = payload;
    while (*line) {
        const char *end = strstr(line, "\r\n");
        size_t line_len = end ? (size_t)(end - line) : strlen(line);
        const char *colon = memchr(line, ':', line_len);
        if (colon && (size_t)(colon - line) > strlen("MessageLine") &&
            strncmp(line, "MessageLine", strlen("MessageLine")) == 0) {
            const char *value = colon + 1;
            size_t value_len;
            while (value < line + line_len && *value == ' ') value++;
            value_len = (size_t)((line + line_len) - value);
            if (used > 0) result[used++] = '\n';
            memcpy(result + used, value, value_len);
            used += value_len;
            result[used] = '\0';
        }
        if (!end) break;
        line = end + 2;
    }
    return result;
}
#endif

static void bridge_manager_event(const char *event, const char *payload)
{
#ifdef __linux__
    char device[128];

    if (!event || !payload || !payload_header(payload, "Device", device, sizeof(device))) {
        return;
    }

    if (strcmp(event, "DongleCallStateChange") == 0) {
        char state[64];
        if (payload_header(payload, "NewState", state, sizeof(state))) {
            simbox_event_bridge_fire_call_state(device, state);
        }
    } else if (strcmp(event, "DongleNewSMS") == 0) {
        char sender[512];
        char *text;
        if (!payload_header(payload, "From", sender, sizeof(sender))) return;
        text = payload_message_lines(payload);
        if (text) {
            simbox_event_bridge_fire_incoming_sms(device, sender, text);
            free(text);
        }
    } else if (strcmp(event, "DongleNewUSSD") == 0) {
        char *text = payload_message_lines(payload);
        if (text) {
            simbox_event_bridge_fire_ussd_response(device, text);
            free(text);
        }
    }
#else
    (void)event;
    (void)payload;
#endif
}

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

void astman_send_listack(struct mansession *s, const struct message *m,
                         const char *msg, const char *listflag)
{
    (void)listflag;
    astman_send_ack(s, m, msg);
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
                          const void *module, const char *synopsis, const char *description)
{
    return 0;
}

int ast_manager_unregister(const char *action)
{
    return 0;
}

void manager_event(int category, const char *event, const char *fmt, ...)
{
    va_list ap;
    va_list copy;
    int needed;
    char *payload;

    (void)category;
    if (!event || !fmt) return;

    va_start(ap, fmt);
    va_copy(copy, ap);
    needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed < 0) {
        va_end(ap);
        return;
    }

    payload = (char *)malloc((size_t)needed + 1);
    if (!payload) {
        va_end(ap);
        return;
    }
    vsnprintf(payload, (size_t)needed + 1, fmt, ap);
    va_end(ap);

    bridge_manager_event(event, payload);
    free(payload);
}
