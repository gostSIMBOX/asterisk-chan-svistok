/*
 * Asterisk compatibility shim for chan_simbox
 * shim_config.c - Minimal INI-style Asterisk config parser
 */
#include <asterisk/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static char *trim(char *s)
{
    if (!s) return NULL;
    while (isspace((unsigned char)*s)) s++;
    if (*s == 0) return s;
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}

struct ast_config *ast_config_load2(const char *filename, const char *who_asked, struct ast_flags flags)
{
    FILE *f = fopen(filename, "r");
    if (!f) {
        /* Also try /etc/asterisk/<filename> */
        char path[512];
        snprintf(path, sizeof(path), "/etc/asterisk/%s", filename);
        f = fopen(path, "r");
    }
    if (!f) {
        return CONFIG_STATUS_FILEMISSING;
    }

    struct ast_config *cfg = (struct ast_config *)calloc(1, sizeof(struct ast_config));
    if (!cfg) {
        fclose(f);
        return CONFIG_STATUS_FILEINVALID;
    }

    char line[1024];
    struct ast_category *current_cat = NULL;
    int lineno = 0;

    while (fgets(line, sizeof(line), f)) {
        lineno++;
        char *p = trim(line);
        if (!p || *p == '\0' || *p == ';' || *p == '#')
            continue;

        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end) {
                *end = '\0';
                char *catname = trim(p + 1);
                struct ast_category *cat = (struct ast_category *)calloc(1, sizeof(struct ast_category));
                if (cat) {
                    strncpy(cat->name, catname, sizeof(cat->name) - 1);
                    if (!cfg->root) {
                        cfg->root = cat;
                    } else {
                        cfg->last->next = cat;
                    }
                    cfg->last = cat;
                    current_cat = cat;
                }
            }
            continue;
        }

        if (current_cat) {
            char *eq = strchr(p, '=');
            char *arrow = strstr(p, "=>");
            char *val = NULL;
            char *name = NULL;

            if (arrow) {
                *arrow = '\0';
                name = trim(p);
                val = trim(arrow + 2);
            } else if (eq) {
                *eq = '\0';
                name = trim(p);
                val = trim(eq + 1);
            }

            if (name && val) {
                struct ast_variable *v = (struct ast_variable *)calloc(1, sizeof(struct ast_variable));
                if (v) {
                    v->name = strdup(name);
                    v->value = strdup(val);
                    v->lineno = lineno;
                    if (!current_cat->root) {
                        current_cat->root = v;
                    } else {
                        current_cat->last->next = v;
                    }
                    current_cat->last = v;
                }
            }
        }
    }

    fclose(f);
    cfg->current = cfg->root;
    return cfg;
}

struct ast_config *ast_config_load(const char *filename, struct ast_flags flags)
{
    return ast_config_load2(filename, "chan_simbox", flags);
}

void ast_config_destroy(struct ast_config *config)
{
    if (!config || config == CONFIG_STATUS_FILEUNCHANGED ||
        config == CONFIG_STATUS_FILEINVALID || config == CONFIG_STATUS_FILEMISSING)
        return;

    struct ast_category *cat = config->root;
    while (cat) {
        struct ast_category *next_cat = cat->next;
        struct ast_variable *var = cat->root;
        while (var) {
            struct ast_variable *next_var = var->next;
            if (var->name) free(var->name);
            if (var->value) free(var->value);
            free(var);
            var = next_var;
        }
        free(cat);
        cat = next_cat;
    }
    free(config);
}

/* CONFIG_STATUS_FILEUNCHANGED/FILEINVALID/FILEMISSING (config.h) are
 * small-negative-int sentinel "error pointers" ast_config_load2()
 * returns on failure — not NULL. A caller that doesn't check for these
 * before passing the result to ast_variable_retrieve()/ast_variable_
 * browse() (e.g. after a config file genuinely doesn't exist, the
 * normal case when running standalone without a real dongle.conf) will
 * dereference one of these bogus addresses and crash. Found the hard
 * way: sdd-asterisk-chan-simbox's Phase 5.2 smoke test crashed with
 * SIGSEGV at exactly 0xfffffffffffffffd (-3, CONFIG_STATUS_FILEMISSING)
 * via chan_dongle.c's real reload_config() -> dc_gconfig_fill() ->
 * here. Guarding here (not in chan_dongle.c, which is read-only) is the
 * correct fix — a config-missing/invalid result should behave the same
 * as a NULL config to every caller, silently. */
static int is_bad_config(const struct ast_config *config)
{
    return config == NULL ||
           config == CONFIG_STATUS_FILEUNCHANGED ||
           config == CONFIG_STATUS_FILEINVALID ||
           config == CONFIG_STATUS_FILEMISSING;
}

const char *ast_variable_retrieve(const struct ast_config *config, const char *category, const char *variable)
{
    if (is_bad_config(config) || !category || !variable) return NULL;

    struct ast_category *cat = config->root;
    while (cat) {
        if (strcasecmp(cat->name, category) == 0) {
            struct ast_variable *var = cat->root;
            while (var) {
                if (strcasecmp(var->name, variable) == 0) {
                    return var->value;
                }
                var = var->next;
            }
        }
        cat = cat->next;
    }
    return NULL;
}

struct ast_variable *ast_variable_browse(const struct ast_config *config, const char *category)
{
    if (is_bad_config(config) || !category) return NULL;

    struct ast_category *cat = config->root;
    while (cat) {
        if (strcasecmp(cat->name, category) == 0) {
            return cat->root;
        }
        cat = cat->next;
    }
    return NULL;
}

char *ast_category_browse(struct ast_config *config, const char *prev)
{
    if (is_bad_config(config)) return NULL;

    if (!prev) {
        config->current = config->root;
        return config->current ? config->current->name : NULL;
    }

    if (config->current && strcasecmp(config->current->name, prev) == 0) {
        config->current = config->current->next;
        return config->current ? config->current->name : NULL;
    }

    struct ast_category *cat = config->root;
    while (cat) {
        if (strcasecmp(cat->name, prev) == 0) {
            config->current = cat->next;
            return config->current ? config->current->name : NULL;
        }
        cat = cat->next;
    }
    return NULL;
}

int ast_true(const char *val)
{
    if (!val) return 0;
    return (!strcasecmp(val, "yes") || !strcasecmp(val, "true") ||
            !strcasecmp(val, "y") || !strcasecmp(val, "t") ||
            !strcasecmp(val, "1") || !strcasecmp(val, "on"));
}

int ast_false(const char *val)
{
    if (!val) return 0;
    return (!strcasecmp(val, "no") || !strcasecmp(val, "false") ||
            !strcasecmp(val, "n") || !strcasecmp(val, "f") ||
            !strcasecmp(val, "0") || !strcasecmp(val, "off"));
}
