/*
 * Asterisk compatibility shim for chan_simbox
 * format_cap.h - Format capabilities (hardcoded SLINEAR)
 */
#ifndef ASTERISK_FORMAT_CAP_H
#define ASTERISK_FORMAT_CAP_H

#include <asterisk/format.h>
#include <stdlib.h>
#include <stdio.h>

struct ast_format_cap {
    struct ast_format format;
};

static inline struct ast_format_cap *ast_format_cap_alloc(void)
{
    struct ast_format_cap *cap = (struct ast_format_cap *)malloc(sizeof(struct ast_format_cap));
    if (cap) {
        cap->format.id = AST_FORMAT_SLINEAR;
        cap->format.flags = 0;
    }
    return cap;
}

static inline struct ast_format_cap *ast_format_cap_destroy(struct ast_format_cap *cap)
{
    if (cap) free(cap);
    return NULL;
}

static inline void ast_format_cap_add(struct ast_format_cap *cap, const struct ast_format *format)
{
    if (cap && format) {
        cap->format = *format;
    }
}

static inline int ast_format_cap_iscompatible(const struct ast_format_cap *cap, const struct ast_format *format)
{
    return 1;
}

static inline struct ast_format_cap *ast_format_cap_joint(const struct ast_format_cap *cap1, const struct ast_format_cap *cap2)
{
    return ast_format_cap_alloc();
}

static inline char *ast_getformatname_multiple(char *buf, size_t size, const struct ast_format_cap *cap)
{
    if (buf && size > 0) {
        snprintf(buf, size, "slin");
    }
    return buf;
}

#endif /* ASTERISK_FORMAT_CAP_H */
