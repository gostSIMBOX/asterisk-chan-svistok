/*
 * Asterisk compatibility shim for chan_simbox
 * cli.h - CLI command structures and macros
 */
#ifndef ASTERISK_CLI_H
#define ASTERISK_CLI_H

#include <stdio.h>
#include <stdarg.h>

#define CLI_SUCCESS   ((char *)0)
#define CLI_FAILURE   ((char *)1)
#define CLI_SHOWUSAGE ((char *)2)

#define CLI_INIT     1
#define CLI_HANDLER  2
#define CLI_GENERATE 3

struct ast_cli_entry;

struct ast_cli_args {
    int fd;
    int argc;
    const char * const *argv;
    const char *line;
    const char *word;
    int pos;
    int n;
};

struct ast_cli_entry {
    const char * const cmda[8];
    char * (*handler)(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a);
    const char *summary;
    const char *usage;
    const char *command;
    char *_full_cmd;
    int cmdlen;
    int inuse;
};

#define AST_CLI_DEFINE(func, sum) \
    { { "" }, func, sum, NULL, NULL, NULL, 0, 0 }

#ifdef __cplusplus
extern "C" {
#endif

void ast_cli(int fd, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));

int ast_cli_register_multiple(struct ast_cli_entry *e, int len);
int ast_cli_unregister_multiple(struct ast_cli_entry *e, int len);
char *ast_cli_complete(const char *word, const char * const choices[], int state);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_CLI_H */
