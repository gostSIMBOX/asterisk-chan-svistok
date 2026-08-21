/*
 * Asterisk compatibility shim for chan_simbox
 * linkedlists.h - Intrusive linked lists and rwlists
 */
#ifndef ASTERISK_LINKEDLISTS_H
#define ASTERISK_LINKEDLISTS_H

#include <asterisk/lock.h>
#include <stddef.h>

#define AST_LIST_ENTRY(type) \
    struct { \
        struct type *next; \
    }

#define AST_LIST_HEAD(name, type) \
    struct name { \
        struct type *first; \
        struct type *last; \
        ast_mutex_t lock; \
    }

#define AST_LIST_HEAD_NOLOCK(name, type) \
    struct name { \
        struct type *first; \
        struct type *last; \
    }

#define AST_LIST_HEAD_STATIC(name, type) \
    struct name { \
        struct type *first; \
        struct type *last; \
        ast_mutex_t lock; \
    } name = { NULL, NULL, AST_MUTEX_INIT_VALUE }

#define AST_LIST_HEAD_NOLOCK_STATIC(name, type) \
    struct name { \
        struct type *first; \
        struct type *last; \
    } name = { NULL, NULL }

#define AST_LIST_HEAD_INIT(head) do { \
    (head)->first = NULL; \
    (head)->last = NULL; \
    ast_mutex_init(&(head)->lock); \
} while (0)

#define AST_LIST_HEAD_INIT_NOLOCK(head) do { \
    (head)->first = NULL; \
    (head)->last = NULL; \
} while (0)

#define AST_LIST_HEAD_DESTROY(head) do { \
    ast_mutex_destroy(&(head)->lock); \
} while (0)

#define AST_LIST_FIRST(head) ((head)->first)
#define AST_LIST_LAST(head)  ((head)->last)
#define AST_LIST_NEXT(elm, field) ((elm)->field.next)
#define AST_LIST_EMPTY(head) ((head)->first == NULL)

#define AST_LIST_TRAVERSE(head, var, field) \
    for ((var) = (head)->first; (var); (var) = (var)->field.next)

#define AST_LIST_TRAVERSE_SAFE_BEGIN(head, var, field) { \
    typeof(head) __list_head = (head); \
    typeof((head)->first) __cur, __next; \
    void **__prev_ptr = (void **)&((head)->first); \
    size_t __offset = 0; \
    int __removed = 0; \
    for (__cur = (head)->first; __cur; ) { \
        __next = __cur->field.next; \
        __offset = (size_t)((char *)&(__cur->field.next) - (char *)__cur); \
        __removed = 0; \
        (var) = __cur;

#define AST_LIST_REMOVE_CURRENT(field) do { \
        *__prev_ptr = (void *)__next; \
        if (__cur == __list_head->last) { \
            __list_head->last = NULL; \
        } \
        __removed = 1; \
        __cur->field.next = NULL; \
} while (0)

#define AST_LIST_TRAVERSE_SAFE_END \
        if (!__removed) { \
            __prev_ptr = (void **)((char *)__cur + __offset); \
        } \
        __cur = __next; \
    } \
}

#define AST_LIST_INSERT_HEAD(head, elm, field) do { \
    (elm)->field.next = (head)->first; \
    (head)->first = (elm); \
    if (!(head)->last) \
        (head)->last = (elm); \
} while (0)

#define AST_LIST_INSERT_TAIL(head, elm, field) do { \
    (elm)->field.next = NULL; \
    if ((head)->last) \
        (head)->last->field.next = (elm); \
    else \
        (head)->first = (elm); \
    (head)->last = (elm); \
} while (0)

#define AST_LIST_INSERT_AFTER(head, listelm, elm, field) do { \
    (elm)->field.next = (listelm)->field.next; \
    (listelm)->field.next = (elm); \
    if ((head)->last == (listelm)) \
        (head)->last = (elm); \
} while (0)

#define AST_LIST_REMOVE_HEAD(head, field) ({ \
    typeof((head)->first) __cur = (head)->first; \
    if (__cur) { \
        (head)->first = __cur->field.next; \
        if (!(head)->first) \
            (head)->last = NULL; \
        __cur->field.next = NULL; \
    } \
    __cur; \
})

#define AST_LIST_REMOVE(head, elm, field) do { \
    if ((head)->first == (elm)) { \
        (head)->first = (elm)->field.next; \
        if ((head)->last == (elm)) \
            (head)->last = NULL; \
    } else { \
        typeof((head)->first) __cur; \
        for (__cur = (head)->first; __cur && __cur->field.next != (elm); __cur = __cur->field.next); \
        if (__cur) { \
            __cur->field.next = (elm)->field.next; \
            if ((head)->last == (elm)) \
                (head)->last = __cur; \
        } \
    } \
    (elm)->field.next = NULL; \
} while (0)

#define AST_LIST_LOCK(head)   ast_mutex_lock(&(head)->lock)
#define AST_LIST_UNLOCK(head) ast_mutex_unlock(&(head)->lock)

/* RWLIST */
#define AST_RWLIST_ENTRY(type) AST_LIST_ENTRY(type)

#define AST_RWLIST_HEAD(name, type) \
    struct name { \
        struct type *first; \
        struct type *last; \
        ast_rwlock_t lock; \
    }

#define AST_RWLIST_HEAD_STATIC(name, type) \
    struct name { \
        struct type *first; \
        struct type *last; \
        ast_rwlock_t lock; \
    } name = { NULL, NULL, AST_RWLOCK_INIT_VALUE }

#define AST_RWLIST_HEAD_INIT(head) do { \
    (head)->first = NULL; \
    (head)->last = NULL; \
    ast_rwlock_init(&(head)->lock); \
} while (0)

#define AST_RWLIST_HEAD_DESTROY(head) do { \
    ast_rwlock_destroy(&(head)->lock); \
} while (0)

#define AST_RWLIST_RDLOCK(head)   ast_rwlock_rdlock(&(head)->lock)
#define AST_RWLIST_WRLOCK(head)   ast_rwlock_wrlock(&(head)->lock)
#define AST_RWLIST_UNLOCK(head)   ast_rwlock_unlock(&(head)->lock)
#define AST_RWLIST_TRYRDLOCK(head) ast_rwlock_tryrdlock(&(head)->lock)
#define AST_RWLIST_TRYWRLOCK(head) ast_rwlock_trywrlock(&(head)->lock)

#define AST_RWLIST_TRAVERSE(head, var, field) AST_LIST_TRAVERSE(head, var, field)
#define AST_RWLIST_TRAVERSE_SAFE_BEGIN(head, var, field) AST_LIST_TRAVERSE_SAFE_BEGIN(head, var, field)
#define AST_RWLIST_TRAVERSE_SAFE_END AST_LIST_TRAVERSE_SAFE_END
#define AST_RWLIST_REMOVE_CURRENT(field) AST_LIST_REMOVE_CURRENT(field)
#define AST_RWLIST_INSERT_HEAD(head, elm, field) AST_LIST_INSERT_HEAD(head, elm, field)
#define AST_RWLIST_INSERT_TAIL(head, elm, field) AST_LIST_INSERT_TAIL(head, elm, field)
#define AST_RWLIST_INSERT_AFTER(head, listelm, elm, field) AST_LIST_INSERT_AFTER(head, listelm, elm, field)
#define AST_RWLIST_REMOVE_HEAD(head, field) AST_LIST_REMOVE_HEAD(head, field)
#define AST_RWLIST_REMOVE(head, elm, field) AST_LIST_REMOVE(head, elm, field)
#define AST_RWLIST_FIRST(head) AST_LIST_FIRST(head)
#define AST_RWLIST_LAST(head) AST_LIST_LAST(head)
#define AST_RWLIST_NEXT(elm, field) AST_LIST_NEXT(elm, field)
#define AST_RWLIST_EMPTY(head) AST_LIST_EMPTY(head)

#endif /* ASTERISK_LINKEDLISTS_H */
