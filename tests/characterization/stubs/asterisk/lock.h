#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_LOCK_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_LOCK_H
#include <pthread.h>
typedef pthread_mutex_t ast_mutex_t;
typedef pthread_rwlock_t ast_rwlock_t;
#define AST_MUTEX_DEFINE_STATIC(name) static ast_mutex_t name = PTHREAD_MUTEX_INITIALIZER
#endif
