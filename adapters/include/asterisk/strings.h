/*
 * Asterisk compatibility shim for chan_simbox
 * strings.h - Dynamic string buffer helpers & string manipulation
 */
#ifndef ASTERISK_STRINGS_H
#define ASTERISK_STRINGS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static inline size_t ast_copy_string(char *dst, const char *src, size_t size)
{
    size_t len = 0;
    if (size > 0) {
        while (*src && --size) {
            *dst++ = *src++;
            len++;
        }
        *dst = '\0';
    }
    while (*src++) len++;
    return len;
}

struct ast_str {
    size_t len;   /* Allocated capacity */
    size_t used;  /* String length */
    char str[1];  /* Null-terminated buffer */
};

static inline struct ast_str *ast_str_create(size_t init_len)
{
    if (init_len < 16) init_len = 16;
    struct ast_str *buf = (struct ast_str *)malloc(sizeof(struct ast_str) + init_len);
    if (!buf) return NULL;
    buf->len = init_len;
    buf->used = 0;
    buf->str[0] = '\0';
    return buf;
}

static inline void ast_str_reset(struct ast_str *buf)
{
    if (buf) {
        buf->used = 0;
        buf->str[0] = '\0';
    }
}

static inline char *ast_str_buffer(const struct ast_str *buf)
{
    return buf ? (char *)buf->str : (char *)"";
}

static inline size_t ast_str_strlen(const struct ast_str *buf)
{
    return buf ? buf->used : 0;
}

static inline size_t ast_str_size(const struct ast_str *buf)
{
    return buf ? buf->len : 0;
}

static inline int ast_str_make_space(struct ast_str **buf, size_t new_len)
{
    if (!buf || !*buf) return -1;
    if (new_len <= (*buf)->len) return 0;

    size_t target_len = (*buf)->len * 2;
    if (target_len < new_len) target_len = new_len + 64;

    struct ast_str *new_buf = (struct ast_str *)realloc(*buf, sizeof(struct ast_str) + target_len);
    if (!new_buf) return -1;
    new_buf->len = target_len;
    *buf = new_buf;
    return 0;
}

static inline int ast_str_set_va(struct ast_str **buf, size_t max_len, const char *fmt, va_list ap)
{
    if (!buf || !*buf) return -1;
    va_list ap_copy;
    va_copy(ap_copy, ap);
    int needed = vsnprintf((*buf)->str, (*buf)->len + 1, fmt, ap);
    if (needed < 0) {
        va_end(ap_copy);
        return -1;
    }
    if ((size_t)needed > (*buf)->len) {
        if (ast_str_make_space(buf, (size_t)needed) < 0) {
            va_end(ap_copy);
            return -1;
        }
        needed = vsnprintf((*buf)->str, (*buf)->len + 1, fmt, ap_copy);
    }
    va_end(ap_copy);
    (*buf)->used = (size_t)needed;
    return needed;
}

static inline int ast_str_set(struct ast_str **buf, size_t max_len, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

static inline int ast_str_set(struct ast_str **buf, size_t max_len, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int res = ast_str_set_va(buf, max_len, fmt, ap);
    va_end(ap);
    return res;
}

static inline int ast_str_append_va(struct ast_str **buf, size_t max_len, const char *fmt, va_list ap)
{
    if (!buf || !*buf) return -1;
    va_list ap_copy;
    va_copy(ap_copy, ap);
    size_t offset = (*buf)->used;
    size_t space_left = ((*buf)->len > offset) ? ((*buf)->len - offset) : 0;
    int needed = vsnprintf((*buf)->str + offset, space_left + 1, fmt, ap);
    if (needed < 0) {
        va_end(ap_copy);
        return -1;
    }
    if ((size_t)needed > space_left) {
        if (ast_str_make_space(buf, offset + (size_t)needed) < 0) {
            va_end(ap_copy);
            return -1;
        }
        needed = vsnprintf((*buf)->str + offset, (*buf)->len - offset + 1, fmt, ap_copy);
    }
    va_end(ap_copy);
    (*buf)->used = offset + (size_t)needed;
    return needed;
}

static inline int ast_str_append(struct ast_str **buf, size_t max_len, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

static inline int ast_str_append(struct ast_str **buf, size_t max_len, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int res = ast_str_append_va(buf, max_len, fmt, ap);
    va_end(ap);
    return res;
}

static inline void ast_str_truncate(struct ast_str *buf, ssize_t len)
{
    if (!buf) return;
    if (len < 0) {
        if ((size_t)(-len) >= buf->used)
            buf->used = 0;
        else
            buf->used += len;
    } else {
        if ((size_t)len < buf->used)
            buf->used = (size_t)len;
    }
    buf->str[buf->used] = '\0';
}

#endif /* ASTERISK_STRINGS_H */
