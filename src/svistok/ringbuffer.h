#ifndef SVISTOK_RINGBUFFER_H_INCLUDED
#define SVISTOK_RINGBUFFER_H_INCLUDED

EXPORT_DECL int rb_read_until_char_after_iov (const struct ringbuffer*, struct iovec iov[2], char, int after);

#endif /* SVISTOK_RINGBUFFER_H_INCLUDED */
