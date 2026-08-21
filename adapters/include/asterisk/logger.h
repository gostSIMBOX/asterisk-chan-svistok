/*
 * Asterisk compatibility shim for chan_simbox
 * logger.h - Logging macros and prototypes
 */
#ifndef ASTERISK_LOGGER_H
#define ASTERISK_LOGGER_H

#include <stdio.h>
#include <stdarg.h>

#define __LOG_ERROR    0
#define __LOG_WARNING  1
#define __LOG_NOTICE   2
#define __LOG_EVENT    3
#define __LOG_DEBUG    4
#define __LOG_VERBOSE  5
#define __LOG_DTMF     6

#define LOG_ERROR      __LOG_ERROR, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define LOG_WARNING    __LOG_WARNING, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define LOG_NOTICE     __LOG_NOTICE, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define LOG_EVENT      __LOG_EVENT, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define LOG_DEBUG      __LOG_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define LOG_VERBOSE    __LOG_VERBOSE, __FILE__, __LINE__, __PRETTY_FUNCTION__
#define LOG_DTMF       __LOG_DTMF, __FILE__, __LINE__, __PRETTY_FUNCTION__

#ifdef __cplusplus
extern "C" {
#endif

void ast_log(int level, const char *file, int line, const char *function, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

void ast_verbose(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

void __ast_verbose(const char *file, int line, const char *func, int level, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

void __ast_debug(int level, const char *file, int line, const char *func, const char *fmt, ...)
    __attribute__((format(printf, 5, 6)));

#define ast_verb(level, ...) __ast_verbose(__FILE__, __LINE__, __PRETTY_FUNCTION__, level, __VA_ARGS__)
#define ast_debug(level, ...) __ast_debug(level, __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_LOGGER_H */
