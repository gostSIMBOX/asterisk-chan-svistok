/*
 * Asterisk compatibility shim for chan_simbox
 * config.h - Configuration parser and AST_CONFIG_DIR
 */
#ifndef ASTERISK_CONFIG_H
#define ASTERISK_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_STATUS_FILEUNCHANGED ((struct ast_config *)-1)
#define CONFIG_STATUS_FILEINVALID   ((struct ast_config *)-2)
#define CONFIG_STATUS_FILEMISSING   ((struct ast_config *)-3)

#define CONFIG_FLAG_FILEUNCHANGED (1 << 1)
#define CONFIG_FLAG_WITHCOMMENTS  (1 << 2)
#define CONFIG_FLAG_NOCACHE       (1 << 3)

struct ast_flags {
    unsigned int flags;
};

struct ast_variable {
    char *name;
    char *value;
    struct ast_variable *next;
    int lineno;
    int object;
    int blanklines;
};

struct ast_category {
    char name[80];
    struct ast_variable *root;
    struct ast_variable *last;
    struct ast_category *next;
};

struct ast_config {
    struct ast_category *root;
    struct ast_category *last;
    struct ast_category *current;
};

#ifdef __cplusplus
extern "C" {
#endif

struct ast_config *ast_config_load(const char *filename, struct ast_flags flags);
struct ast_config *ast_config_load2(const char *filename, const char *who_asked, struct ast_flags flags);
void ast_config_destroy(struct ast_config *config);

const char *ast_variable_retrieve(const struct ast_config *config, const char *category, const char *variable);
struct ast_variable *ast_variable_browse(const struct ast_config *config, const char *category);
char *ast_category_browse(struct ast_config *config, const char *prev);

int ast_true(const char *val);
int ast_false(const char *val);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_CONFIG_H */
