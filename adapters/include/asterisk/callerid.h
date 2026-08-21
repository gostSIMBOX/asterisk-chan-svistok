/*
 * Asterisk compatibility shim for chan_simbox
 * callerid.h - Caller ID parsing and presentation constants
 */
#ifndef ASTERISK_CALLERID_H
#define ASTERISK_CALLERID_H

#define AST_PRES_RESTRICTION                           0x60
#define AST_PRES_ALLOWED                               0x00
#define AST_PRES_RESTRICTED                            0x20
#define AST_PRES_UNAVAILABLE                           0x40
#define AST_PRES_NUM_AMBIGUOUS                         0x60
#define AST_PRES_ALLOWED_USER_NUMBER_PASSED_SCREEN        0x01
#define AST_PRES_RESTRICTED_USER_NUMBER_PASSED_SCREEN     0x21
#define AST_PRES_ALLOWED_NETWORK_NUMBER                0x03

#define AST_PRES_ALLOWED_USER_NUMBER_NOT_SCREENED      0x00
#define AST_PRES_ALLOWED_USER_NUMBER_FAILED_SCREEN     0x02
#define AST_PRES_PROHIB_USER_NUMBER_NOT_SCREENED       0x20
#define AST_PRES_PROHIB_USER_NUMBER_PASSED_SCREEN      0x21
#define AST_PRES_PROHIB_USER_NUMBER_FAILED_SCREEN      0x22
#define AST_PRES_PROHIB_NETWORK_NUMBER                 0x23
#define AST_PRES_NUMBER_NOT_AVAILABLE                  0x43

#ifdef __cplusplus
extern "C" {
#endif

int ast_callerid_parse(char *instr, char **name, char **location);
const char *ast_describe_caller_presentation(int presentation);
int ast_parse_caller_presentation(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* ASTERISK_CALLERID_H */
