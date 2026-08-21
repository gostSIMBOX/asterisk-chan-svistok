/*
 * Asterisk compatibility shim for chan_simbox
 * asterisk.h - Master include & core definitions
 */
#ifndef ASTERISK_H
#define ASTERISK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/time.h>

#include <asterisk/compiler.h>
#include <asterisk/compat.h>
#include <asterisk/utils.h>
#include <asterisk/strings.h>
#include <asterisk/logger.h>
#include <asterisk/config.h>

#define ASTERISK_FILE_VERSION(file, version)
#define ASTERISK_GPL_KEY "GPL"

#define AST_MODULE "chan_dongle"

#define ASTERISK_VERSION_NUM 110000

#define AST_PTHREADT_NULL ((pthread_t) -1)
#define AST_PTHREADT_STOP ((pthread_t) -2)

#ifndef ARRAY_LEN
#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef S_OR
#define S_OR(a, b) ((a) && *(a) ? (a) : (b))
#endif

#ifndef S_COR
#define S_COR(a, b, c) ((a) && *(a) ? (b) : (c))
#endif

#endif /* ASTERISK_H */
