/*
 * Asterisk compatibility shim for chan_simbox
 * compiler.h - GCC/Clang attribute macros
 */
#ifndef ASTERISK_COMPILER_H
#define ASTERISK_COMPILER_H

#define attribute_unused __attribute__((unused))
#define attribute_pure   __attribute__((pure))
#define attribute_malloc __attribute__((malloc))
#define attribute_deprecated __attribute__((deprecated))
#define attribute_sentinel __attribute__((sentinel))
#define attribute_warn_unused_result __attribute__((warn_unused_result))

#endif /* ASTERISK_COMPILER_H */
