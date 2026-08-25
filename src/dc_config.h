/*
   Copyright (C) 2010 bg <bg_one@mail.ru>
*/
#ifndef CHAN_DONGLE_DC_CONFIG_H_INCLUDED
#define CHAN_DONGLE_DC_CONFIG_H_INCLUDED
#ifdef SVISTOK_COMPOSED_DC_CONFIG_H_HEADER
#include SVISTOK_COMPOSED_DC_CONFIG_H_HEADER
#else

#include <asterisk.h>
#include <asterisk/channel.h>		/* AST_MAX_CONTEXT MAX_LANGUAGE */

#include <asterisk-chan-dongle/export.h>	/* EXPORT_DECL EXPORT_DEF */
#include <asterisk-chan-dongle/mutils.h>

/* SVISTOK_BASELINE_UNIT macro CONFIG_FILE */
/* SVISTOK_BASELINE_UNIT macro DEVNAMELEN */
/* SVISTOK_BASELINE_UNIT macro IMEI_SIZE */
/* SVISTOK_BASELINE_UNIT macro IMSI_SIZE */
#define SERIAL_SIZE		16
#define DEVPATHLEN		512

/* SVISTOK_BASELINE_UNIT typedef dev_state_t */
EXPORT_DECL const char * const dev_state_strs[4];

/* SVISTOK_BASELINE_UNIT typedef call_waiting_t */

/* SVISTOK_BASELINE_UNIT function dc_cw_setting2str */

/* SVISTOK_BASELINE_UNIT typedef dc_dtmf_setting_t */

/*
 Config API
 Operations
 	convert from string to native
 	convent from native to string
 	get native value
	get alternative presentation

 	set native value ?

	types:
		string of limited length
		integer with limits
		enum
		boolean
*/

/* Global inherited (shared) settings */
typedef struct dc_sconfig
{
	char			context[AST_MAX_CONTEXT];	/*!< the context for incoming calls; 'default '*/
	char			exten[AST_MAX_EXTENSION];	/*!< exten, not overwrite valid subscriber_number */
	char			language[MAX_LANGUAGE];		/*!< default language 'en' */
	int			group;				/*!< group number for group dialling 0 */
	int			agroup;				/*!< group number for group dialling 0 */
	int			rxgain;				/*!< increase the incoming volume 0 */
	int			txgain;				/*!< increase the outgoint volume 0 */
	int			u2diag;				/*!< -1 */
	int			callingpres;			/*!< calling presentation */

	unsigned int		usecallingpres:1;		/*! -1 */
	unsigned int		autodeletesms:1;		/*! 0 */
	unsigned int		resetdongle:1;			/*! 1 */
	unsigned int		disablesms:1;			/*! 0 */
	unsigned int		smsaspdu:1;			/*! 0 */
	dev_state_t		initstate;			/*! DEV_STATE_STARTED */
//	unsigned int		disable:1;			/*! 0 */

	call_waiting_t		callwaiting;			/*!< enable/disable/auto call waiting CALL_WAITING_AUTO */
	dc_dtmf_setting_t	dtmf;				/*!< off/inband/relax incoming DTMF detection, default DC_DTMF_SETTING_RELAX */

	int			mindtmfgap;			/*!< minimal time in ms from end of previews DTMF and begining of next */
/* SVISTOK_BASELINE_UNIT macro DEFAULT_MINDTMFGAP */

	int			mindtmfduration;		/*!< minimal DTMF duration in ms */
/* SVISTOK_BASELINE_UNIT macro DEFAULT_MINDTMFDURATION */

	int			mindtmfinterval;		/*!< minimal DTMF interval beetween ends in ms, applied only on same digit */
/* SVISTOK_BASELINE_UNIT macro DEFAULT_MINDTMFINTERVAL */
} dc_sconfig_t;

/* Global settings */
/* SVISTOK_BASELINE_UNIT typedef dc_gconfig_t */

/* Local required (unique) settings */
typedef struct dc_uconfig
{
	/* unique settings */
	char			id[DEVNAMELEN];			/*!< id from dongle.conf */
	char			audio_tty[DEVPATHLEN];		/*!< tty for audio connection */
	char			data_tty[DEVPATHLEN];		/*!< tty for AT commands */
	char			imei[IMEI_SIZE+1];		/*!< search device by imei */
	char			imsi[IMSI_SIZE+1];		/*!< search device by imsi */
	char			serial[SERIAL_SIZE+2];		/*!< search device by s */
	char			dev[DEVPATHLEN];
	char			net[DEVPATHLEN];
} dc_uconfig_t;

/* all Config settings join in one place */
/* SVISTOK_BASELINE_UNIT typedef pvt_config_t */
/* SVISTOK_BASELINE_UNIT macro SCONFIG */
/* SVISTOK_BASELINE_UNIT macro UCONFIG */

/* SVISTOK_BASELINE_UNIT declaration dc_dtmf_setting2str */
/* SVISTOK_BASELINE_UNIT declaration dc_sconfig_fill_defaults */
/* SVISTOK_BASELINE_UNIT declaration dc_sconfig_fill */
/* SVISTOK_BASELINE_UNIT declaration dc_gconfig_fill */
/* SVISTOK_BASELINE_UNIT declaration dc_config_fill */


#endif /* SVISTOK_COMPOSED_DC_CONFIG_H_HEADER */
#endif /* CHAN_DONGLE_DC_CONFIG_H_INCLUDED */
