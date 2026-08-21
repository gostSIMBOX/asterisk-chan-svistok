/*
 * Asterisk compatibility shim for chan_simbox
 * lock.h - Mutex, RWLock, and condition variables wrapping POSIX threads
 */
#ifndef ASTERISK_LOCK_H
#define ASTERISK_LOCK_H

#include <pthread.h>
#include <errno.h>

typedef pthread_mutex_t ast_mutex_t;
typedef pthread_rwlock_t ast_rwlock_t;
typedef pthread_cond_t ast_cond_t;

#define AST_MUTEX_INIT_VALUE PTHREAD_MUTEX_INITIALIZER
#define AST_RWLOCK_INIT_VALUE PTHREAD_RWLOCK_INITIALIZER

#define AST_MUTEX_DEFINE_STATIC(name) \
    static ast_mutex_t name = AST_MUTEX_INIT_VALUE

#define AST_RWLOCK_DEFINE_STATIC(name) \
    static ast_rwlock_t name = AST_RWLOCK_INIT_VALUE

static inline int ast_mutex_init(ast_mutex_t *pmtx)
{
    return pthread_mutex_init(pmtx, NULL);
}

static inline int ast_mutex_destroy(ast_mutex_t *pmtx)
{
    return pthread_mutex_destroy(pmtx);
}

static inline int ast_mutex_lock(ast_mutex_t *pmtx)
{
    return pthread_mutex_lock(pmtx);
}

static inline int ast_mutex_unlock(ast_mutex_t *pmtx)
{
    return pthread_mutex_unlock(pmtx);
}

static inline int ast_mutex_trylock(ast_mutex_t *pmtx)
{
    return pthread_mutex_trylock(pmtx);
}

static inline int ast_rwlock_init(ast_rwlock_t *prw)
{
    return pthread_rwlock_init(prw, NULL);
}

static inline int ast_rwlock_destroy(ast_rwlock_t *prw)
{
    return pthread_rwlock_destroy(prw);
}

static inline int ast_rwlock_rdlock(ast_rwlock_t *prw)
{
    return pthread_rwlock_rdlock(prw);
}

static inline int ast_rwlock_wrlock(ast_rwlock_t *prw)
{
    return pthread_rwlock_wrlock(prw);
}

static inline int ast_rwlock_unlock(ast_rwlock_t *prw)
{
    return pthread_rwlock_unlock(prw);
}

static inline int ast_rwlock_tryrdlock(ast_rwlock_t *prw)
{
    return pthread_rwlock_tryrdlock(prw);
}

static inline int ast_rwlock_trywrlock(ast_rwlock_t *prw)
{
    return pthread_rwlock_trywrlock(prw);
}

#endif /* ASTERISK_LOCK_H */
