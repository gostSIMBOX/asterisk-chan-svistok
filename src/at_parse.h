/*
   Copyright (C) 2010 bg <bg_one@mail.ru>
*/
#ifndef CHAN_DONGLE_AT_PARSE_H_INCLUDED
#define CHAN_DONGLE_AT_PARSE_H_INCLUDED
#ifdef SVISTOK_COMPOSED_AT_PARSE_H_HEADER
#include SVISTOK_COMPOSED_AT_PARSE_H_HEADER
#else

#include <sys/types.h>			/* size_t */

#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DECL */
#include <asterisk-chan-dongle/char_conv.h>	/* str_encoding_t */
/* SVISTOK_BASELINE_UNIT record pvt */

/* SVISTOK_BASELINE_UNIT declaration at_parse_cnum */
/* SVISTOK_BASELINE_UNIT declaration at_parse_cops */
EXPORT_DECL char* at_parse_spn (char* str);
/* SVISTOK_BASELINE_UNIT declaration at_parse_creg */
/* SVISTOK_BASELINE_UNIT declaration at_parse_cmti */
/* SVISTOK_BASELINE_UNIT declaration at_parse_cmgr */
EXPORT_DECL const char* at_parse_cds (char** str, size_t len, char* oa, size_t oa_len, str_encoding_t* oa_enc, char** msg, str_encoding_t* msg_enc);

/* SVISTOK_BASELINE_UNIT declaration at_parse_cusd */
/* SVISTOK_BASELINE_UNIT declaration at_parse_cpin */
/* SVISTOK_BASELINE_UNIT declaration at_parse_csq */
/* SVISTOK_BASELINE_UNIT declaration at_parse_rssi */
/* SVISTOK_BASELINE_UNIT declaration at_parse_mode */
EXPORT_DECL int at_parse_sysinfo (char * str, int * srvst, int * srvd, int * roamst, int * sysmode, int * simst);

/* SVISTOK_BASELINE_UNIT declaration at_parse_csca */
/* SVISTOK_BASELINE_UNIT declaration at_parse_clcc */
/* SVISTOK_BASELINE_UNIT declaration at_parse_ccwa */



#endif /* SVISTOK_COMPOSED_AT_PARSE_H_HEADER */
#endif /* CHAN_DONGLE_AT_PARSE_H_INCLUDED */
