/* 
   Copyright (C) 2009 - 2010
   
   Artem Makhutov <artem@makhutov.org>
   http://www.makhutov.org
   
   Dmitry Vagin <dmitry2004@yandex.ru>
*/

#ifndef ____RINGBUFFER_H__
#define ____RINGBUFFER_H__
#ifdef SVISTOK_COMPOSED_RINGBUFFER_H_HEADER
#include SVISTOK_COMPOSED_RINGBUFFER_H_HEADER
#else

#include <sys/uio.h>			/* struct iovec */
#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */

/* SVISTOK_BASELINE_UNIT typedef rb_write_f */

/* SVISTOK_BASELINE_UNIT record ringbuffer */


/* SVISTOK_BASELINE_UNIT function rb_init */

/* SVISTOK_BASELINE_UNIT function rb_used */

/* SVISTOK_BASELINE_UNIT function rb_free */

/* SVISTOK_BASELINE_UNIT declaration rb_memcmp */

/*!< fill io vectors array with readed data (situable for writev()) and return number of io vectors updated  */
/* SVISTOK_BASELINE_UNIT declaration rb_read_all_iov */

/*!< fill io vectors array and return number of io vectors updated for reading len bytes */
/* SVISTOK_BASELINE_UNIT declaration rb_read_n_iov */

/* SVISTOK_BASELINE_UNIT declaration rb_read_until_char_iov */
EXPORT_DECL int rb_read_until_char_after_iov (const struct ringbuffer*, struct iovec iov[2], char, int after);

/* SVISTOK_BASELINE_UNIT declaration rb_read_until_mem_iov */

/*!< advice read position to len bytes */
/* SVISTOK_BASELINE_UNIT declaration rb_read_upd */

/*!< fill io vectors array with free data (situable for readv()) and return number of io vectors updated  */
/* SVISTOK_BASELINE_UNIT declaration rb_write_iov */

/*!< advice write position to len bytes */
/* SVISTOK_BASELINE_UNIT declaration rb_write_upd */

/* SVISTOK_BASELINE_UNIT declaration rb_write_core */

/* SVISTOK_BASELINE_UNIT function rb_write */

#endif /* SVISTOK_COMPOSED_RINGBUFFER_H_HEADER */
#endif /* ____RINGBUFFER_H__ */
