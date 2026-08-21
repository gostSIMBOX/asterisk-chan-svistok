/*
 * Asterisk compatibility shim for chan_simbox
 * module.h - Module metadata and lifecycle macros
 */
#ifndef ASTERISK_MODULE_H
#define ASTERISK_MODULE_H

enum ast_module_load_result {
    AST_MODULE_LOAD_SUCCESS = 0,
    AST_MODULE_LOAD_DECLINE = 1,
    AST_MODULE_LOAD_FAILURE = 2,
};

#define AST_MODFLAG_DEFAULT 0
#define AST_MODFLAG_GLOBAL_SYMBOLS (1 << 0)

struct ast_module {
    int dummy;
};

struct ast_module_info {
    const char *key;
    int flags;
    const char *description;
    int (*load)(void);
    int (*unload)(void);
    int (*reload)(void);
    struct ast_module *self;
    const char *name;
};

#define AST_MODULE_INFO(keystr, flags_val, desc, fields...) \
    static struct ast_module_info __mod_info = { \
        .key = keystr, \
        .flags = flags_val, \
        .description = desc, \
        fields \
    }; \
    static struct ast_module_info *ast_module_info = &__mod_info

#define AST_MODULE_INFO_STANDARD(keystr, desc) \
    AST_MODULE_INFO(keystr, AST_MODFLAG_DEFAULT, desc, .load = load_module, .unload = unload_module)

#define ast_module_ref(mod) do { } while (0)
#define ast_module_unref(mod) do { } while (0)
#define ast_update_use_count() do { } while (0)

#endif /* ASTERISK_MODULE_H */
