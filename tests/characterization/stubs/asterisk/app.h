#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_APP_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_APP_H
#define AST_DECLARE_APP_ARGS(name, content) struct { content } name
#define AST_APP_ARG(name) char *name;
#define AST_STANDARD_APP_ARGS(args, parse) ((void)0)
#endif
