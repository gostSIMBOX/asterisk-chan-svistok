/*
 * Asterisk compatibility shim for chan_simbox
 * app.h - Application registration & dialplan argument parsing
 */
#ifndef ASTERISK_APP_H
#define ASTERISK_APP_H

#include <asterisk/channel.h>
#include <asterisk/strings.h>

#define AST_DECLARE_APP_ARGS(name, arglist) \
    struct name { \
        int argc; \
        char *argv[32]; \
        arglist \
    }

#define AST_APP_ARG(name) char *name

#define AST_STANDARD_APP_ARGS(args, parse) \
    do { \
        char *__p = (parse); \
        (args).argc = 0; \
        while (__p && *__p && (args).argc < 32) { \
            (args).argv[(args).argc++] = __p; \
            char *__comma = strchr(__p, ','); \
            if (__comma) { \
                *__comma = '\0'; \
                __p = __comma + 1; \
            } else { \
                break; \
            } \
        } \
    } while (0)

#define AST_NONSTANDARD_APP_ARGS(args, parse, sep) \
    do { \
        char *__p = (parse); \
        (args).argc = 0; \
        while (__p && *__p && (args).argc < 32) { \
            (args).argv[(args).argc++] = __p; \
            char *__s = strchr(__p, (sep)); \
            if (__s) { \
                *__s = '\0'; \
                __p = __s + 1; \
            } else { \
                break; \
            } \
        } \
    } while (0)

typedef int (*ast_app_exec_cb)(struct ast_channel *chan, const char *data);

#ifdef __cplusplus
extern "C" {
#endif

int ast_register_application2(const char *app, ast_app_exec_cb exec,
                              const char *synopsis, const char *description, void *mod);
int ast_unregister_application(const char *app);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_APP_H */
