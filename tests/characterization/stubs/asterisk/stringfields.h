#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_STRINGFIELDS_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_STRINGFIELDS_H
#define AST_DECLARE_STRING_FIELDS(content) content
#define AST_STRING_FIELD(name) char *name
#define ast_string_field_set(object, field, value) ((void)0)
#endif
