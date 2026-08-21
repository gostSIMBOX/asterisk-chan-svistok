/*
 * Asterisk compatibility shim for chan_simbox
 * utils.h - Memory allocation, string helpers, timeval math, threading helpers
 */
#ifndef ASTERISK_UTILS_H
#define ASTERISK_UTILS_H

#include <asterisk/asterisk.h>
#include <asterisk/lock.h>
#include <alloca.h>
#include <strings.h>

#define ast_free(p) free(p)
#define ast_malloc(s) malloc(s)
#define ast_calloc(n, s) calloc((n), (s))
#define ast_realloc(p, s) realloc((p), (s))
#define ast_strdup(s) strdup(s)
#define ast_strdupa(s) ({ const char *__s = (s); size_t __len = strlen(__s) + 1; char *__d = (char *)alloca(__len); memcpy(__d, __s, __len); __d; })
#define ast_alloca(s) alloca(s)

static inline int ast_strlen_zero(const char *s)
{
    return (!s || *s == '\0');
}

static inline void ast_slinear_saturated_add(short *input, const short *value)
{
    int res = *input + *value;
    if (res > 32767)
        *input = 32767;
    else if (res < -32768)
        *input = -32768;
    else
        *input = (short)res;
}

static inline int ast_base64encode(char *dst, const unsigned char *src, int srclen, int max)
{
    static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    int i, j = 0;
    for (i = 0; i < srclen && j < max - 4; i += 3) {
        unsigned int val = ((unsigned int)src[i] << 16) +
                           ((i + 1 < srclen) ? ((unsigned int)src[i + 1] << 8) : 0) +
                           ((i + 2 < srclen) ? ((unsigned int)src[i + 2]) : 0);
        dst[j++] = b64[(val >> 18) & 0x3F];
        dst[j++] = b64[(val >> 12) & 0x3F];
        dst[j++] = (i + 1 < srclen) ? b64[(val >> 6) & 0x3F] : '=';
        dst[j++] = (i + 2 < srclen) ? b64[val & 0x3F] : '=';
    }
    if (j < max) dst[j] = '\0';
    return j;
}

/* Timeval utilities */
static inline struct timeval ast_tvnow(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv;
}

static inline struct timeval ast_tv(time_t sec, suseconds_t usec)
{
    struct timeval t;
    t.tv_sec = sec;
    t.tv_usec = usec;
    return t;
}

static inline struct timeval ast_tvadd(struct timeval a, struct timeval b)
{
    struct timeval res;
    res.tv_sec = a.tv_sec + b.tv_sec;
    res.tv_usec = a.tv_usec + b.tv_usec;
    if (res.tv_usec >= 1000000) {
        res.tv_sec += res.tv_usec / 1000000;
        res.tv_usec %= 1000000;
    }
    return res;
}

static inline struct timeval ast_tvsub(struct timeval a, struct timeval b)
{
    struct timeval res;
    res.tv_sec = a.tv_sec - b.tv_sec;
    res.tv_usec = a.tv_usec - b.tv_usec;
    while (res.tv_usec < 0) {
        res.tv_sec--;
        res.tv_usec += 1000000;
    }
    return res;
}

static inline int ast_tvcmp(struct timeval a, struct timeval b)
{
    if (a.tv_sec < b.tv_sec)
        return -1;
    if (a.tv_sec > b.tv_sec)
        return 1;
    if (a.tv_usec < b.tv_usec)
        return -1;
    if (a.tv_usec > b.tv_usec)
        return 1;
    return 0;
}

static inline int64_t ast_tvdiff_ms(struct timeval end, struct timeval start)
{
    return ((int64_t)(end.tv_sec - start.tv_sec)) * 1000 +
           ((int64_t)(end.tv_usec - start.tv_usec)) / 1000;
}

static inline int64_t ast_tvdiff_us(struct timeval end, struct timeval start)
{
    return ((int64_t)(end.tv_sec - start.tv_sec)) * 1000000 +
           ((int64_t)(end.tv_usec - start.tv_usec));
}

static inline int ast_tvzero(struct timeval t)
{
    return (t.tv_sec == 0 && t.tv_usec == 0);
}

static inline struct timeval ast_samp2tv(unsigned int samples, unsigned int rate)
{
    struct timeval tv;
    if (rate == 0) rate = 8000;
    tv.tv_sec = samples / rate;
    tv.tv_usec = ((unsigned long long)(samples % rate) * 1000000) / rate;
    return tv;
}

static inline unsigned int ast_tv2samp(struct timeval tv, unsigned int rate)
{
    if (rate == 0) rate = 8000;
    return (tv.tv_sec * rate) + (tv.tv_usec * rate) / 1000000;
}

/* Thread utilities */
static inline int ast_pthread_create_background(pthread_t *thread, const pthread_attr_t *attr,
                                                void *(*start_routine)(void *), void *data)
{
    pthread_attr_t lattr;
    if (!attr) {
        pthread_attr_init(&lattr);
        pthread_attr_setdetachstate(&lattr, PTHREAD_CREATE_DETACHED);
        attr = &lattr;
    }
    int res = pthread_create(thread, attr, start_routine, data);
    if (attr == &lattr) {
        pthread_attr_destroy(&lattr);
    }
    return res;
}

static inline int ast_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                                     void *(*start_routine)(void *), void *data)
{
    return pthread_create(thread, attr, start_routine, data);
}

#endif /* ASTERISK_UTILS_H */
