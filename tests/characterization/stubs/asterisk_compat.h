#ifndef SVISTOK_CHARACTERIZATION_ASTERISK_COMPAT_H
#define SVISTOK_CHARACTERIZATION_ASTERISK_COMPAT_H

#ifndef attribute_unused
#define attribute_unused __attribute__((unused))
#endif

void ast_verb(int level, const char *format, ...);

#endif
