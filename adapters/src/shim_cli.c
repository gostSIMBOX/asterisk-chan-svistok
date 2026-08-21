/*
 * Asterisk compatibility shim for chan_simbox
 * shim_cli.c - CLI output formatting and command dispatch
 */
#include <asterisk/cli.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static struct ast_cli_entry *cli_entries = NULL;
static int num_cli_entries = 0;

void ast_cli(int fd, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    if (fd >= 0 && fd <= 2) {
        vfprintf((fd == 2) ? stderr : stdout, fmt, ap);
    } else if (fd > 2) {
        vdprintf(fd, fmt, ap);
    }
    va_end(ap);
}

int ast_cli_register_multiple(struct ast_cli_entry *e, int len)
{
    cli_entries = e;
    num_cli_entries = len;
    return 0;
}

int ast_cli_unregister_multiple(struct ast_cli_entry *e, int len)
{
    if (cli_entries == e) {
        cli_entries = NULL;
        num_cli_entries = 0;
    }
    return 0;
}

char *ast_cli_complete(const char *word, const char * const choices[], int state)
{
    int i = 0, which = 0;
    size_t wordlen = word ? strlen(word) : 0;

    if (!choices) return NULL;

    while (choices[i]) {
        if (!wordlen || strncasecmp(choices[i], word, wordlen) == 0) {
            if (++which > state) {
                return strdup(choices[i]);
            }
        }
        i++;
    }
    return NULL;
}
