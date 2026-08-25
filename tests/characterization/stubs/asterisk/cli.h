#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_CLI_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_CLI_H
#include <asterisk/strings.h>
struct ast_cli_entry { const char *command; const char *usage; void *handler; };
struct ast_cli_args { int fd; int argc; char **argv; const char *word; int n; int pos; };
#define CLI_INIT 1
#define CLI_GENERATE 2
#define CLI_SUCCESS ((char *)0)
#define CLI_FAILURE ((char *)1)
#define CLI_SHOWUSAGE ((char *)2)
#define AST_CLI_DEFINE(fn, summary) { 0, summary, (void *)(fn) }
char *ast_cli_complete(const char *, const char * const *, int);
#endif
