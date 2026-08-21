/*
 * Asterisk compatibility shim for chan_simbox
 * format.h - Audio/video media format representations
 */
#ifndef ASTERISK_FORMAT_H
#define ASTERISK_FORMAT_H

#include <stdint.h>
#include <stdbool.h>

#define AST_FORMAT_SLINEAR  64
#define AST_FORMAT_ULAW     4
#define AST_FORMAT_ALAW     8
#define AST_FORMAT_GSM      2
#define AST_FORMAT_TESTLAW  (1 << 30)

struct ast_format {
    int id;
    int flags;
};

static inline struct ast_format *ast_format_set(struct ast_format *format, int id, int flags)
{
    if (format) {
        format->id = id;
        format->flags = flags;
    }
    return format;
}

static inline struct ast_format *ast_format_copy(struct ast_format *dst, const struct ast_format *src)
{
    if (dst && src) {
        dst->id = src->id;
        dst->flags = src->flags;
    }
    return dst;
}

static inline int ast_format_is_slinear(const struct ast_format *format)
{
    return (format && format->id == AST_FORMAT_SLINEAR);
}

static inline const char *ast_getformatname(const struct ast_format *format)
{
    if (!format) return "unknown";
    switch (format->id) {
    case AST_FORMAT_SLINEAR: return "slin";
    case AST_FORMAT_ULAW:    return "ulaw";
    case AST_FORMAT_ALAW:    return "alaw";
    case AST_FORMAT_GSM:     return "gsm";
    default:                 return "unknown";
    }
}

#endif /* ASTERISK_FORMAT_H */
