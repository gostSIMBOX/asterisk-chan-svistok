/*
 * Asterisk compatibility shim for chan_simbox
 * shim_logging.c - Logging implementation (stdout/stderr + callback hook)
 */
#include <asterisk/logger.h>
#include <asterisk/options.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <pthread.h>

int ast_opt_debug = 0;
int ast_opt_verbose = 0;

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

static const char *level_to_string(int level)
{
    switch (level) {
    case __LOG_ERROR:   return "ERROR";
    case __LOG_WARNING: return "WARNING";
    case __LOG_NOTICE:  return "NOTICE";
    case __LOG_DEBUG:   return "DEBUG";
    case __LOG_VERBOSE: return "VERBOSE";
    case __LOG_DTMF:    return "DTMF";
    default:            return "LOG";
    }
}

void ast_log(int level, const char *file, int line, const char *function, const char *fmt, ...)
{
    char timestr[32];
    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tm_buf);

    pthread_mutex_lock(&log_lock);
    fprintf(stderr, "[%s] %s[%s:%d %s]: ", timestr, level_to_string(level),
            file ? file : "unknown", line, function ? function : "");

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
    pthread_mutex_unlock(&log_lock);
}

void ast_verbose(const char *fmt, ...)
{
    pthread_mutex_lock(&log_lock);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    pthread_mutex_unlock(&log_lock);
}

void __ast_verbose(const char *file, int line, const char *func, int level, const char *fmt, ...)
{
    if (ast_opt_verbose < level && level > 0)
        return;

    pthread_mutex_lock(&log_lock);
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    fflush(stdout);
    pthread_mutex_unlock(&log_lock);
}

void __ast_debug(int level, const char *file, int line, const char *func, const char *fmt, ...)
{
    if (ast_opt_debug < level)
        return;

    pthread_mutex_lock(&log_lock);
    fprintf(stderr, "[DEBUG:%d][%s:%d %s] ", level, file ? file : "", line, func ? func : "");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
    pthread_mutex_unlock(&log_lock);
}
