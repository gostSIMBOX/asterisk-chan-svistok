/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifndef CHAN_DONGLE_AT_RESPONSE_H_INCLUDED
#define CHAN_DONGLE_AT_RESPONSE_H_INCLUDED
#ifdef SVISTOK_COMPOSED_AT_RESPONSE_H_HEADER
#include SVISTOK_COMPOSED_AT_RESPONSE_H_HEADER
#else

#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */

/* SVISTOK_BASELINE_UNIT record pvt */
/* SVISTOK_BASELINE_UNIT record iovec */

/* magic order!!! keep this enum order same as in at_responses_list */
typedef enum {
	RES_PARSE_ERROR = -1,
	RES_MIN = RES_PARSE_ERROR,
	RES_UNKNOWN = 0,

	RES_BOOT,
	RES_BUSY,
	RES_CEND,

	RES_CMGR,
	RES_CMS_ERROR,
	RES_CMTI,
	RES_CNUM,

	RES_CONF,
	RES_CONN,
	RES_COPS,
	RES_SPN,
	RES_SYSINFO,
	RES_CPIN,

	RES_CREG,
	RES_CSQ,
	RES_CSSI,
	RES_CSSU,

	RES_CUSD,
	RES_ERROR,
	RES_MODE,
	RES_NO_CARRIER,

	RES_NO_DIALTONE,
	RES_OK,
	RES_ORIG,
	RES_RING,

	RES_RSSI,
	RES_DSFLOWRPT,
	RES_SMMEMFULL,
	RES_SMS_PROMPT,
	RES_SRVST,
	RES_SIMST,
	RES_CFUN_V,
	RES_ICCID,
	RES_SN,

//	RES_CVOICE,
	RES_CMGS,
	RES_CPMS,
	RES_CSCA,
	RES_CLCC,
	RES_CCWA,
	RES_CDS,
	RES_MAX = RES_CCWA,
} at_res_t;

/*! response description */
/* SVISTOK_BASELINE_UNIT typedef at_response_t */

/*! responses control */
/* SVISTOK_BASELINE_UNIT typedef at_responses_t */

/*! responses description */
EXPORT_DECL const at_responses_t at_responses;
/* SVISTOK_BASELINE_UNIT declaration at_res2str */
/* SVISTOK_BASELINE_UNIT declaration at_response */

#endif /* SVISTOK_COMPOSED_AT_RESPONSE_H_HEADER */
#endif /* CHAN_DONGLE_AT_RESPONSE_H_INCLUDED */
