#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_LINKEDLISTS_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_LINKEDLISTS_H

#define AST_LIST_ENTRY(type) struct { struct type *next; }
#define AST_LIST_HEAD_NOLOCK(name, type) struct name { \
    struct type *first; \
    struct type **last; \
}
#define AST_RWLIST_HEAD(name, type) struct name { struct type *first; struct type **last; }
#define AST_RWLIST_TRAVERSE(head, var, field) \
    for ((var) = (head)->first; (var); (var) = (var)->field.next)
#define AST_RWLIST_FIRST(head) ((head)->first)
#define AST_RWLIST_NEXT(elm, field) ((elm)->field.next)
#define AST_RWLIST_HEAD_INIT(head) do { (head)->first = NULL; (head)->last = &(head)->first; } while (0)
#define AST_RWLIST_HEAD_DESTROY(head) ((void)0)
#define AST_RWLIST_WRLOCK(head) ((void)0)
#define AST_RWLIST_RDLOCK(head) ((void)0)
#define AST_RWLIST_UNLOCK(head) ((void)0)
#define AST_RWLIST_INSERT_TAIL(head, elm, field) AST_LIST_INSERT_TAIL(head, elm, field)
#define AST_RWLIST_REMOVE_HEAD(head, field) __extension__ ({ \
    __typeof__((head)->first) _item = (head)->first; \
    if (_item) { \
        (head)->first = _item->field.next; \
        if (!(head)->first) (head)->last = &(head)->first; \
    } \
    _item; \
})
#define AST_RWLIST_TRAVERSE_SAFE_BEGIN(head, var, field) \
    for ((var) = (head)->first; (var); (var) = (var)->field.next)
#define AST_RWLIST_REMOVE_CURRENT(field) ((void)0)
#define AST_RWLIST_TRAVERSE_SAFE_END
#define AST_LIST_HEAD_INIT_NOLOCK(head) do { \
    (head)->first = NULL; \
    (head)->last = &(head)->first; \
} while (0)
#define AST_LIST_FIRST(head) ((head)->first)
#define AST_LIST_REMOVE_HEAD(head, field) __extension__ ({ \
    __typeof__((head)->first) _item = (head)->first; \
    if (_item) { \
        (head)->first = _item->field.next; \
        if (!(head)->first) (head)->last = &(head)->first; \
    } \
    _item; \
})
#define AST_LIST_INSERT_AFTER(head, current, elm, field) do { \
    (elm)->field.next = (current)->field.next; \
    (current)->field.next = (elm); \
    if (!(elm)->field.next) (head)->last = &(elm)->field.next; \
} while (0)
#define AST_LIST_INSERT_TAIL(head, elm, field) do { \
    (elm)->field.next = NULL; \
    *(head)->last = (elm); \
    (head)->last = &(elm)->field.next; \
} while (0)
#define AST_LIST_TRAVERSE(head, var, field) \
    for ((var) = (head)->first; (var); (var) = (var)->field.next)
#define AST_LIST_TRAVERSE_SAFE_BEGIN(head, var, field) \
    for ((var) = (head)->first; (var); (var) = (var)->field.next)
#define AST_LIST_REMOVE_CURRENT(field) ((void)0)
#define AST_LIST_TRAVERSE_SAFE_END
#define AST_LIST_REMOVE(head, elm, field) do { \
    __typeof__((head)->first) *_cursor = &(head)->first; \
    while (*_cursor && *_cursor != (elm)) _cursor = &(*_cursor)->field.next; \
    if (*_cursor) { \
        *_cursor = (elm)->field.next; \
        if (!*_cursor) (head)->last = _cursor; \
    } \
} while (0)

#endif
