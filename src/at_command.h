/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifndef CHAN_DONGLE_AT_SEND_H_INCLUDED
#define CHAN_DONGLE_AT_SEND_H_INCLUDED
#ifdef SVISTOK_COMPOSED_AT_COMMAND_H_HEADER
#include SVISTOK_COMPOSED_AT_COMMAND_H_HEADER
#else

#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */
#include "dc_config.h"		/* call_waiting_t */
#include <asterisk-chan-dongle/mutils.h>	/* enum2str_def() ITEMS_OF() */

/* SVISTOK_BASELINE_UNIT macro CCWA_CLASS_VOICE */

/* magic order !!! keep order of this values like in at_cmd2str()
*/
typedef enum {
	CMD_USER = 0,

	CMD_AT,
	CMD_AT_A,
	CMD_AT_CCWA_STATUS,
	CMD_AT_CCWA_SET,
	CMD_AT_CFUN_V,
	CMD_AT_CFUN,

	CMD_AT_CGMI,
	CMD_AT_CGMM,
	CMD_AT_CGMR,
	CMD_AT_CGSN,

	CMD_AT_CHUP,
	CMD_AT_CIMI,
//	CMD_AT_CLIP,
	CMD_AT_CLIR,

	CMD_AT_CLVL,
	CMD_AT_CMGD,
	CMD_AT_CMGF,
	CMD_AT_CMGR,

	CMD_AT_CMGS,
	CMD_AT_SMSTEXT,
	CMD_AT_CNMI,
	CMD_AT_CNUM,

	CMD_AT_COPS,
	CMD_AT_COPS_INIT,
	CMD_AT_SPN,
	CMD_AT_CPIN,
	CMD_AT_CPMS,

	CMD_AT_CREG,
	CMD_AT_CREG_INIT,
	CMD_AT_CSCS,
	CMD_AT_CSQ,

	CMD_AT_CSSN,
	CMD_AT_CUSD,
	CMD_AT_CVOICE,
	CMD_AT_CARDLOCK,
	CMD_AT_D,

	CMD_AT_DDSETEX,
	CMD_AT_DTMF,
	CMD_AT_E,

	CMD_AT_U2DIAG,
	CMD_AT_Z,
	CMD_AT_CMEE,
	CMD_AT_CSCA,

	CMD_AT_CHLD_1x,
	CMD_AT_CHLD_2x,
	CMD_AT_CHLD_2,
	CMD_AT_CHLD_3,
	CMD_AT_CLCC,
	CMD_AT_SN,
	CMD_AT_ICCID,
	CMD_AT_FREQLOCK,
	CMD_AT_CSNR,
	CMD_AT_SYSINFO,
} at_cmd_t;

/*!
 * \brief Get the string representation of the given AT command
 * \param cmd -- the command to process
 * \return a string describing the given command
 */

INLINE_DECL const char* at_cmd2str (at_cmd_t cmd)
{
	/* magic!!! must be in same order as elements of enums in at_cmd_t */
	static const char * const cmds[] = {
		"USER'S",

		"AT",
		"ATA",
		"AT+CCWA?",
		"AT+CCWA=",
		"AT+CFUN?",
		"AT+CFUN",

		"AT+CGMI",
		"AT+CGMM",
		"AT+CGMR",
		"AT+CGSN",

		"AT+CHUP",
		"AT+CIMI",
//		"AT+CLIP",
		"AT+CLIR",

		"AT+CLVL",
		"AT+CMGD",
		"AT+CMGF",
		"AT+CMGR",

		"AT+CMGS",
		"SMSTEXT",
		"AT+CNMI",
		"AT+CNUM",

		"AT+COPS?",
		"AT+COPS=",
		"AT^SPN=0",
		"AT+CPIN?",
		"AT+CPMS",

		"AT+CREG?",
		"AT+CREG=",
		"AT+CSCS",
		"AT+CSQ",

		"AT+CSSN",
		"AT+CUSD",
		"AT^CVOICE",
		"AT^CARDLOCK",
		"ATD",

		"AT^DDSETEX",
		"AT^DTMF",
		"ATE",

		"AT^U2DIAG",
		"ATZ",
		"AT+CMEE",
		"AT+CSCA",

		"AT+CHLD=1x",
		"AT+CHLD=2x",
		"AT+CHLD=2",
		"AT+CHLD=3",
		"AT+CLCC",
		"AT^SN",
		"AT^ICCID?"
		"AT^FREQLOCK?",
		"AT^CSNR?",
		"AT^SYSINFO"
	};
	return enum2str_def(cmd, cmds, ITEMS_OF(cmds), "UNDEFINED");
}


/* SVISTOK_BASELINE_UNIT record cpvt */

EXPORT_DECL const char* at_cmd2str (at_cmd_t cmd);
/* SVISTOK_BASELINE_UNIT declaration at_enque_initialization */




/* SVISTOK_BASELINE_UNIT declaration at_enque_ping */
/* SVISTOK_BASELINE_UNIT declaration at_enque_cops */





















/* SVISTOK_BASELINE_UNIT declaration at_enque_sms */
/* SVISTOK_BASELINE_UNIT declaration at_enque_pdu */
/* SVISTOK_BASELINE_UNIT declaration at_enque_ussd */
/* SVISTOK_BASELINE_UNIT declaration at_enque_dtmf */
/* SVISTOK_BASELINE_UNIT declaration at_enque_set_ccwa */
/* SVISTOK_BASELINE_UNIT declaration at_enque_reset */
/* SVISTOK_BASELINE_UNIT declaration at_enque_dial */
/* SVISTOK_BASELINE_UNIT declaration at_enque_answer */
/* SVISTOK_BASELINE_UNIT declaration at_enque_user_cmd */
/* SVISTOK_BASELINE_UNIT declaration at_enque_retrive_sms */
/* SVISTOK_BASELINE_UNIT declaration at_enque_hangup */
/* SVISTOK_BASELINE_UNIT declaration at_enque_volsync */
/* SVISTOK_BASELINE_UNIT declaration at_enque_clcc */
/* SVISTOK_BASELINE_UNIT declaration at_enque_activate */
/* SVISTOK_BASELINE_UNIT declaration at_enque_flip_hold */
/* SVISTOK_BASELINE_UNIT declaration at_enque_conference */
/* SVISTOK_BASELINE_UNIT declaration at_hangup_immediality */

#endif /* SVISTOK_COMPOSED_AT_COMMAND_H_HEADER */
#include "svistok/at_command.h"
#endif /* CHAN_DONGLE_AT_SEND_H_INCLUDED */
