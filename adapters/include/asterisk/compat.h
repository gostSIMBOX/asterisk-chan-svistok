/*
 * Asterisk compatibility shim for chan_simbox
 * compat.h - Compiler and standard C compatibility
 */
#ifndef ASTERISK_COMPAT_H
#define ASTERISK_COMPAT_H

#include <asterisk/compiler.h>
#include <iconv.h>

#ifndef ICONV_CONST
#define ICONV_CONST
#endif

#ifndef ICONV_T
#define ICONV_T iconv_t
#endif

#endif /* ASTERISK_COMPAT_H */
