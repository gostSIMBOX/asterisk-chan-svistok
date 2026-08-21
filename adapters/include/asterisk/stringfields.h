/*
 * Asterisk compatibility shim for chan_simbox
 * stringfields.h - String fields pool allocator macros
 */
#ifndef ASTERISK_STRINGFIELDS_H
#define ASTERISK_STRINGFIELDS_H

#include <stdlib.h>
#include <string.h>

typedef const char * ast_string_field;

struct ast_string_field_pool {
    struct ast_string_field_pool *prev;
    size_t size;
    size_t used;
    size_t active;
    char base[0];
};

struct ast_string_field_mgr {
    struct ast_string_field_pool *pool;
    size_t size;
};

#define AST_DECLARE_STRING_FIELDS(field_list) \
    struct ast_string_field_mgr __field_mgr; \
    field_list

#define AST_STRING_FIELD(name) ast_string_field name

#define ast_string_field_init(structure, size) 0
#define ast_string_field_free_memory(structure) do { } while (0)
#define ast_string_field_set(structure, field, value) do { \
    (structure)->field = (value); \
} while (0)
#define ast_string_field_build(structure, field, fmt, ...) do { \
    /* no-op stub */ \
} while (0)

#endif /* ASTERISK_STRINGFIELDS_H */
