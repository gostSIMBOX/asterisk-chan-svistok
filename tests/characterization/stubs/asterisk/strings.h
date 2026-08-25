#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_STRINGS_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_STRINGS_H
#include <string.h>
#define ast_strlen_zero(value) (!(value) || !*(value))
struct ast_str { char *value; };
struct ast_str *ast_str_create(size_t);
void ast_str_append(struct ast_str **, size_t, const char *, ...);
const char *ast_str_buffer(const struct ast_str *);
#endif
