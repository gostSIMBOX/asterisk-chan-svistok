/*
 * Asterisk compatibility shim for chan_simbox
 * shim_logging.c - Logging implementation (stdout/stderr + callback hook)
 */
#include <asterisk/logger.h>
#include <asterisk/options.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#ifdef __linux__
#include "simbox_internal_linux.h"
#endif

int ast_opt_debug = 0;
int ast_opt_verbose = 0;

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

#ifdef __linux__
static void capture_user_command_response(char *message)
{
    static const char marker[] = "] Got Response for user's command:'";
    char *marker_pos;
    char *response;
    char *end;
    char device[128];
    size_t device_len;

    if (!message || message[0] != '[') return;
    marker_pos = strstr(message, marker);
    if (!marker_pos) return;
    device_len = (size_t)(marker_pos - (message + 1));
    if (device_len == 0 || device_len >= sizeof(device)) return;
    memcpy(device, message + 1, device_len);
    device[device_len] = '\0';

    response = marker_pos + strlen(marker);
    end = message + strlen(message);
    while (end > response && (end[-1] == '\r' || end[-1] == '\n')) end--;
    if (end > response && end[-1] == '\'') end--;
    *end = '\0';
    simbox_at_response_bridge_capture(device, response);
}
#endif

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
    char *message = NULL;
    int needed;
    time_t now = time(NULL);
    struct tm tm_buf;
    localtime_r(&now, &tm_buf);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", &tm_buf);

    va_list ap;
    va_start(ap, fmt);
    va_list copy;
    va_copy(copy, ap);
    needed = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (needed >= 0) {
        message = (char *)malloc((size_t)needed + 1);
        if (message) vsnprintf(message, (size_t)needed + 1, fmt, ap);
    }
    va_end(ap);

    if (!message) return;
    pthread_mutex_lock(&log_lock);
    fprintf(stderr, "[%s] %s[%s:%d %s]: %s", timestr, level_to_string(level),
            file ? file : "unknown", line, function ? function : "", message);
    fflush(stderr);
    pthread_mutex_unlock(&log_lock);

#ifdef __linux__
    if (level == __LOG_NOTICE) capture_user_command_response(message);
#endif
    free(message);
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
