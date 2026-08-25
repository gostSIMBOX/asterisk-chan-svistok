/*
   Copyright (C) 2010 bg <bg_one@mail.ru>
*/
#ifndef CHAN_DONGLE_PDU_H_INCLUDED
#define CHAN_DONGLE_PDU_H_INCLUDED
#ifdef SVISTOK_COMPOSED_PDU_H_HEADER
#include SVISTOK_COMPOSED_PDU_H_HEADER
#else

#include <sys/types.h>			/* size_t */
#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */
#include <asterisk-chan-dongle/char_conv.h>	/* str_encoding_t */

/* SVISTOK_BASELINE_UNIT declaration pdu_digit2code */
/* SVISTOK_BASELINE_UNIT declaration pdu_build */
/* SVISTOK_BASELINE_UNIT declaration pdu_parse */
EXPORT_DECL const char * pdu_parse_cds(char ** pdu, size_t tpdu_length, char * oa, size_t oa_len, str_encoding_t * oa_enc, char ** msg, str_encoding_t * msg_enc);

/* SVISTOK_BASELINE_UNIT declaration pdu_parse_sca */

#endif /* SVISTOK_COMPOSED_PDU_H_HEADER */
#endif /* CHAN_DONGLE_PDU_H_INCLUDED */
