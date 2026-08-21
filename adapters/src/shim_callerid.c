/*
 * Asterisk compatibility shim for chan_simbox
 * shim_callerid.c - CallerID parser and presentation stringifiers
 */
#include <asterisk/callerid.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

int ast_callerid_parse(char *instr, char **name, char **location)
{
    if (!instr) {
        if (name) *name = NULL;
        if (location) *location = NULL;
        return -1;
    }

    char *start = instr;
    while (isspace((unsigned char)*start)) start++;

    char *angle = strchr(start, '<');
    if (angle) {
        *angle = '\0';
        char *end = angle - 1;
        while (end >= start && isspace((unsigned char)*end)) {
            *end = '\0';
            end--;
        }
        if (name) *name = start;

        char *loc_start = angle + 1;
        char *loc_end = strchr(loc_start, '>');
        if (loc_end) *loc_end = '\0';
        if (location) *location = loc_start;
    } else {
        if (name) *name = NULL;
        if (location) *location = start;
    }

    return 0;
}

const char *ast_describe_caller_presentation(int presentation)
{
    switch (presentation & AST_PRES_RESTRICTION) {
    case AST_PRES_ALLOWED:                  return "Allowed";
    case AST_PRES_RESTRICTED:               return "Restricted";
    case AST_PRES_UNAVAILABLE:              return "Unavailable";
    case AST_PRES_NUM_AMBIGUOUS:            return "Ambiguous";
    case AST_PRES_ALLOWED_USER_NUMBER_PASSED_SCREEN: return "Allowed (User/Passed)";
    case AST_PRES_RESTRICTED_USER_NUMBER_PASSED_SCREEN: return "Restricted (User/Passed)";
    case AST_PRES_ALLOWED_NETWORK_NUMBER:   return "Allowed (Network)";
    default:                                return "Unknown";
    }
}

int ast_parse_caller_presentation(const char *name)
{
    if (!name) return -1;
    if (strcasecmp(name, "allowed") == 0 || strcasecmp(name, "allowed_not_screened") == 0)
        return AST_PRES_ALLOWED_USER_NUMBER_NOT_SCREENED;
    if (strcasecmp(name, "allowed_passed_screen") == 0)
        return AST_PRES_ALLOWED_USER_NUMBER_PASSED_SCREEN;
    if (strcasecmp(name, "allowed_failed_screen") == 0)
        return AST_PRES_ALLOWED_USER_NUMBER_FAILED_SCREEN;
    if (strcasecmp(name, "prohib") == 0 || strcasecmp(name, "prohib_not_screened") == 0)
        return AST_PRES_PROHIB_USER_NUMBER_NOT_SCREENED;
    if (strcasecmp(name, "prohib_passed_screen") == 0)
        return AST_PRES_PROHIB_USER_NUMBER_PASSED_SCREEN;
    if (strcasecmp(name, "prohib_failed_screen") == 0)
        return AST_PRES_PROHIB_USER_NUMBER_FAILED_SCREEN;
    if (strcasecmp(name, "unavailable") == 0)
        return AST_PRES_NUMBER_NOT_AVAILABLE;
    return -1;
}
