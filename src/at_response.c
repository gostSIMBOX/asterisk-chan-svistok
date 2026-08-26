/*
 * Copyright (C) 2014-2026 Anton Dodonov (NativeMind)
 * https://github.com/Anton-Dodonov
 * http://linkedin.com/in/anton-dodonov/
 * mailto:anton.v.dodonov@gmail.com
 */

#ifdef HAVE_CONFIG_H
#include <svistok_config.h>
#endif /* HAVE_CONFIG_H */

#include <asterisk.h>
#include <asterisk/logger.h>			/* ast_debug() */
#include <asterisk/pbx.h>			/* ast_pbx_start() */

#include "at_response.h"
#include <asterisk-chan-dongle/mutils.h>		/* STRLEN() */
#include <asterisk-chan-dongle/at_queue.h>
#include "chan_dongle.h"
#include "at_parse.h"
#include <asterisk-chan-dongle/char_conv.h>
#include <asterisk-chan-dongle/manager.h>
#include "channel.h"				/* channel_queue_hangup() channel_queue_control() */
#include "dserial.c"
#include "limits.c"
 
#define DEF_STR(str)	str,STRLEN(str)

#define CCWA_STATUS_NOT_ACTIVE	0
#define CCWA_STATUS_ACTIVE	1

#define CLCC_CALL_TYPE_VOICE	0
#define CLCC_CALL_TYPE_DATA	1
#define CLCC_CALL_TYPE_FAX	2

/*
^DSFLOWRPT: N1, N2, N3, N4, N5, N6, N7 
        N1: Connection duration in seconds 
        N2: measured upload speed 
        N3: measured download speed 
        N4: number of sent data 
        N5: number of received data  
        N6: connection, supported by the maximum upload speed
        N7: connection, supported by a maximum download speed 
*/


/* magic!!! must be in same order as elements of enums in at_res_t */
static const at_response_t at_responses_list[] = {
	{ RES_PARSE_ERROR,"PARSE ERROR", 0, 0 },
	{ RES_UNKNOWN,"UNKNOWN", 0, 0 },

	{ RES_BOOT,"^BOOT",DEF_STR("^BOOT:") },
	{ RES_BUSY,"BUSY",DEF_STR("BUSY\r") },
	{ RES_CEND,"^CEND",DEF_STR("^CEND:") },

	{ RES_CMGR, "+CMGR",DEF_STR("+CMGR:") },
	{ RES_CMS_ERROR, "+CMS ERROR",DEF_STR("+CMS ERROR:") },
	{ RES_CMTI, "+CMTI",DEF_STR("+CMTI:") },
	{ RES_CNUM, "+CNUM",DEF_STR("+CNUM:") },		/* and "ERROR+CNUM:" */

	{ RES_CONF,"^CONF",DEF_STR("^CONF:") },
	{ RES_CONN,"^CONN",DEF_STR("^CONN:") },
	{ RES_COPS,"+COPS",DEF_STR("+COPS:") },
	{ RES_SPN,"^SPN",DEF_STR("^SPN:") },
	{ RES_SYSINFO,"^SYSINFO",DEF_STR("^SYSINFO:") },
	{ RES_CPIN,"+CPIN",DEF_STR("+CPIN:") },

	{ RES_CREG,"+CREG",DEF_STR("+CREG:") },
	{ RES_CSQ,"+CSQ",DEF_STR("+CSQ:") },
	{ RES_CSSI,"+CSSI",DEF_STR("+CSSI:") },
	{ RES_CSSU,"+CSSU",DEF_STR("+CSSU:") },

	{ RES_CUSD,"+CUSD",DEF_STR("+CUSD:") },
	{ RES_ERROR,"ERROR",DEF_STR("ERROR\r") },		/* and "COMMAND NOT SUPPORT\r" */
	{ RES_MODE,"^MODE",DEF_STR("^MODE:") },
	{ RES_NO_CARRIER,"NO CARRIER",DEF_STR("NO CARRIER\r") },

	{ RES_NO_DIALTONE,"NO DIALTONE",DEF_STR("NO DIALTONE\r") },
	{ RES_OK,"OK",DEF_STR("OK\r") },
	{ RES_ORIG,"^ORIG",DEF_STR("^ORIG:") },
	{ RES_RING,"RING",DEF_STR("RING\r") },

	{ RES_RSSI,"^RSSI",DEF_STR("^RSSI:") },
	{ RES_DSFLOWRPT,"^DSFLOWRPT",DEF_STR("^DSFLOWRPT:") },

	{ RES_SMMEMFULL,"^SMMEMFULL",DEF_STR("^SMMEMFULL:") },
	{ RES_SMS_PROMPT,"> ",DEF_STR("> ") },
	{ RES_SRVST,"^SRVST",DEF_STR("^SRVST:") },
	{ RES_SIMST,"^SIMST",DEF_STR("^SIMST:") },
	{ RES_CFUN_V,"+CFUN",DEF_STR("+CFUN:") },
	{ RES_ICCID,"^ICCID",DEF_STR("^ICCID:") },
	{ RES_SN,"^SN",DEF_STR("^SN:") },

//	{ RES_CVOICE,"^CVOICE",DEF_STR("^CVOICE:") },
	{ RES_CMGS,"+CMGS",DEF_STR("+CMGS:") },
	{ RES_CMGS,"+CPMS",DEF_STR("+CPMS:") },
	{ RES_CSCA,"+CSCA",DEF_STR("+CSCA:") },

	{ RES_CLCC,"+CLCC", DEF_STR("+CLCC:") },
	{ RES_CCWA,"+CCWA", DEF_STR("+CCWA:") },
	{ RES_CDS,"+CDS", DEF_STR("+CDS:") },

	/* duplicated response undef other id */
	{ RES_CNUM, "+CNUM",DEF_STR("ERROR+CNUM:") },
	{ RES_ERROR,"ERROR",DEF_STR("COMMAND NOT SUPPORT\r") },
	};
#undef DEF_STR

EXPORT_DEF const at_responses_t at_responses = { at_responses_list, 2, ITEMS_OF(at_responses_list), RES_MIN, RES_MAX};

/*!
 * \brief Get the string representation of the given AT response
 * \param res -- the response to process
 * \return a string describing the given response
 */

                                                
 
                                                                              
                                                                    
                    
 



/*!
 * \brief Handle OK response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_ok (struct pvt* pvt, at_res_t res)
{
	const at_queue_task_t * task = at_queue_head_task (pvt);
	const at_queue_cmd_t * ecmd = at_queue_task_cmd(task);

	if(!ecmd)
	{
		ast_log (LOG_ERROR, "[%s] Received unexpected 'OK'\n", PVT_ID(pvt));
		return 0;
	}

	if(ecmd->res == RES_OK || ecmd->res == RES_CMGR)
	{
		switch (ecmd->cmd)
		{
			case CMD_AT:
			case CMD_AT_Z:
			case CMD_AT_E:
			case CMD_AT_U2DIAG:
			case CMD_AT_CGMI:
			case CMD_AT_CGMM:
			case CMD_AT_CGMR:
			case CMD_AT_CMEE:
			case CMD_AT_CGSN:
			case CMD_AT_CVOICE:
			case CMD_AT_CARDLOCK:
			
			case CMD_AT_CIMI:
			case CMD_AT_CPIN:
			case CMD_AT_CCWA_SET:
			case CMD_AT_CCWA_STATUS:
			case CMD_AT_CHLD_2:
			case CMD_AT_CHLD_3:
			case CMD_AT_CSCA:
			case CMD_AT_CLCC:
			case CMD_AT_CLIR:
				ast_debug (3, "[%s] %s sent successfully\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd));
				break;

			case CMD_AT_COPS_INIT:
				ast_debug (1, "[%s] Operator select parameters set\n", PVT_ID(pvt));
				break;

			case CMD_AT_CREG_INIT:
				ast_debug (1, "[%s] registration info enabled\n", PVT_ID(pvt));
				break;

			case CMD_AT_CREG:
				ast_debug (1, "[%s] registration query sent\n", PVT_ID(pvt));
				break;

			case CMD_AT_CNUM:
				ast_debug (1, "[%s] Subscriber phone number query successed\n", PVT_ID(pvt));
				break;

/*
			case CMD_AT_CVOICE:
				ast_debug (1, "[%s] Dongle has voice support\n", PVT_ID(pvt));
				pvt->novoice = 0;
				pvt->has_voice = 1;
				break;*/
				
/*
			case CMD_AT_CLIP:
				ast_debug (1, "[%s] Calling line indication disabled\n", PVT_ID(pvt));
				break;
*/
			case CMD_AT_CSSN:
				ast_debug (1, "[%s] Supplementary Service Notification enabled successful\n", PVT_ID(pvt));
				break;

			case CMD_AT_CMGF:
				pvt->use_pdu = CONF_SHARED(pvt, smsaspdu);
				ast_debug (1, "[%s] SMS operation mode set to %s\n", PVT_ID(pvt), pvt->use_pdu ? "PDU" : "TEXT");
				break;

			case CMD_AT_CSCS:
				ast_debug (1, "[%s] UCS-2 text encoding enabled\n", PVT_ID(pvt));

				pvt->use_ucs2_encoding = 1;
				break;

			case CMD_AT_CPMS:
				ast_debug (1, "[%s] SMS storage location is established\n", PVT_ID(pvt));
				break;

			case CMD_AT_CNMI:
				ast_debug (1, "[%s] SMS new message indication enabled\n", PVT_ID(pvt));
				ast_debug (1, "[%s] Dongle has sms support\n", PVT_ID(pvt));

				pvt->has_sms = 1;

				if (!pvt->initialized)
				{
					pvt->timeout = DATA_READ_TIMEOUT;
					pvt->initialized = 1;
					ast_verb (3, "[%s] Dongle initialized and ready\n", PVT_ID(pvt));
					manager_event_device_status(PVT_ID(pvt), "Initialize");
				}
				break;

			case CMD_AT_SN:
				ast_debug (1, "[%s] S OK\n", PVT_ID(pvt));
				break;

			case CMD_AT_ICCID:
				ast_debug (1, "[%s] ICCID OK\n", PVT_ID(pvt));
				break;

			case CMD_AT_CFUN_V:
				ast_debug (1, "[%s] CFUN? OK\n", PVT_ID(pvt));
				break;


			case CMD_AT_FREQLOCK:
				ast_debug (1, "[%s] FREQLOCK OK\n", PVT_ID(pvt));
				break;



			case CMD_AT_D:
				pvt->dialing = 1;
				putfilei("sim/state",pvt->imsi,"state_dialing",1);
				if(task->cpvt != &pvt->sys_chan)
					pvt->last_dialed_cpvt = task->cpvt;
				/* passthrow */

			case CMD_AT_A:
			case CMD_AT_CHLD_2x:
/* not work, ^CONN: appear before OK for CHLD_ANSWER 
				task->cpvt->answered = 1;
				task->cpvt->needhangup = 1;
*/
				CPVT_SET_FLAGS(task->cpvt, CALL_FLAG_NEED_HANGUP);
				ast_debug (1, "[%s] %s sent successfully for call id %d\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd), task->cpvt->call_idx);
				break;

			case CMD_AT_CFUN:
				/* in case of reset */
				pvt->ring = 0;
				pvt->dialing = 0;
				pvt->cwaiting = 0;
				putfilei("sim/state",pvt->imsi,"state_ring",0);
				putfilei("sim/state",pvt->imsi,"state_dialing",0);
				putfilei("sim/state",pvt->imsi,"state_cwaiting",0);
				break;
			case CMD_AT_DDSETEX:
				ast_debug (1, "[%s] %s sent successfully\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd));
				if (!pvt->initialized)
				{
					pvt->timeout = DATA_READ_TIMEOUT;
					pvt->initialized = 1;
					ast_verb (3, "[%s] Dongle initialized and ready\n", PVT_ID(pvt));
					manager_event_device_status(PVT_ID(pvt), "Initialize");
				}
				break;
			case CMD_AT_CHUP:
			case CMD_AT_CHLD_1x:
				CPVT_RESET_FLAGS(task->cpvt, CALL_FLAG_NEED_HANGUP);
				ast_debug (1, "[%s] Successful hangup for call idx %d\n", PVT_ID(pvt), task->cpvt->call_idx);
				break;

			case CMD_AT_CMGS:
				ast_debug (1, "[%s] Sending sms message in progress\n", PVT_ID(pvt));
				break;

			case CMD_AT_SMSTEXT:
				pvt->outgoing_sms = 0;
				putfilei("sim/state",pvt->imsi,"outgoing_sms",pvt->outgoing_sms);

				pvt_try_restate(pvt);

				manager_event_sent_notify(PVT_ID(pvt), "SMS", task, "Sent");
				/* TODO: move to +CMGS: handler */
				ast_verb (3, "[%s] Successfully sent SMS message %p\n", PVT_ID(pvt), task);
				ast_log (LOG_NOTICE, "[%s] Successfully sent SMS message %p\n", PVT_ID(pvt), task);
				break;

			case CMD_AT_DTMF:
				ast_debug (1, "[%s] DTMF sent successfully for call idx %d\n", PVT_ID(pvt), task->cpvt->call_idx);
				break;

			case CMD_AT_CUSD:
				pvt->outgoing_ussd = 0;
				putfilei("sim/state",pvt->imsi,"outgoing_ussd",pvt->outgoing_ussd);
				manager_event_sent_notify(PVT_ID(pvt), "USSD", task, "Sent");
				ast_verb (3, "[%s] Successfully sent USSD %p\n", PVT_ID(pvt), task);
				ast_log (LOG_NOTICE, "[%s] Successfully sent USSD %p\n", PVT_ID(pvt), task);
				break;

			case CMD_AT_COPS:
				ast_debug (1, "[%s] Provider query successfully\n", PVT_ID(pvt));
				break;

			case CMD_AT_SPN:
				ast_debug (1, "[%s] Provider2 query successfully\n", PVT_ID(pvt));
				break;

			case CMD_AT_CMGR:
				ast_debug (1, "[%s] SMS message see later\n", PVT_ID(pvt));
				break;

			case CMD_AT_CMGD:
				ast_debug (1, "[%s] SMS message deleted successfully\n", PVT_ID(pvt));
				break;

			case CMD_AT_CSQ:
				ast_debug (1, "[%s] Got signal strength result\n", PVT_ID(pvt));
				break;

			case CMD_AT_CLVL:
				pvt->volume_sync_step++;
				if(pvt->volume_sync_step == VOLUME_SYNC_DONE)
				{
					ast_debug (1, "[%s] Volume level synchronized\n", PVT_ID(pvt));
					pvt->volume_sync_step = VOLUME_SYNC_BEGIN;
				}
				break;
			case CMD_USER:
				break;
			default:
				ast_log (LOG_ERROR, "[%s] Received 'OK' for unhandled command '%s'\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd));
				break;
		}
		at_queue_handle_result (pvt, res);
	}
	else
	{
		ast_log (LOG_ERROR, "[%s] Received 'OK' when expecting '%s', ignoring\n", PVT_ID(pvt), at_res2str (ecmd->res));
	}

	return 0;
}

/*!
 * \brief Handle ERROR response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_error (struct pvt* pvt, at_res_t res)
{
	const at_queue_task_t * task = at_queue_head_task(pvt);
	const at_queue_cmd_t * ecmd = at_queue_task_cmd(task);

readpvterrors(pvt);
PVT_STAT(pvt,stat_errors[1])++;
writepvterrors(pvt);


	if (ecmd && (ecmd->res == RES_OK || ecmd->res == RES_CMGR || ecmd->res == RES_SMS_PROMPT))
	{
		switch (ecmd->cmd)
		{
        		/* critical errors */
			case CMD_AT:
			case CMD_AT_Z:
			case CMD_AT_E:
			case CMD_AT_CLCC:
				ast_log (LOG_ERROR, "[%s] Command '%s' failed\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd));
				/* mean disconnected from device */
				goto e_return;

			/* not critical errors */
			case CMD_AT_U2DIAG:
			case CMD_AT_CCWA_SET:
			case CMD_AT_CCWA_STATUS:
				ast_log (LOG_ERROR, "[%s] Command '%s' failed\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd));
				/* mean ignore error */
				break;

			case CMD_AT_CGMI:
				ast_log (LOG_ERROR, "[%s] Getting manufacturer info failed\n", PVT_ID(pvt));
				goto e_return;

			case CMD_AT_CGMM:
				ast_log (LOG_ERROR, "[%s] Getting model info failed\n", PVT_ID(pvt));
				goto e_return;

			case CMD_AT_CGMR:
				ast_log (LOG_ERROR, "[%s] Getting firmware info failed\n", PVT_ID(pvt));
				goto e_return;

			case CMD_AT_CMEE:
				ast_log (LOG_ERROR, "[%s] Setting error verbosity level failed\n", PVT_ID(pvt));
				goto e_return;


			case CMD_AT_CGSN:
				ast_log (LOG_ERROR, "[%s] Getting IMEI number failed\n", PVT_ID(pvt));
				ast_verb (3, "[%s] Getting IMEI number failed, enque_cfun1\n", PVT_ID(pvt));				
				at_enque_cfun1 (&pvt->sys_chan);
				break;
				//goto e_return;

			case CMD_AT_SN:
				ast_log (LOG_ERROR, "[%s] Getting S failed\n", PVT_ID(pvt));
				goto e_return;

			case CMD_AT_ICCID:
				ast_log (LOG_ERROR, "[%s] Getting ICCID failed\n", PVT_ID(pvt));
				pvt->nosim=1;
				goto e_return;

			case CMD_AT_CFUN_V:
				ast_log (LOG_ERROR, "[%s] Getting CFUN? failed\n", PVT_ID(pvt));
				break;

			case CMD_AT_FREQLOCK:
				ast_debug (1, "[%s] Error getting FREQLOCK info\n", PVT_ID(pvt));
				break;


			case CMD_AT_CIMI:
				ast_log (LOG_ERROR, "[%s] Getting IMSI number failed\n", PVT_ID(pvt));
				break;
				//goto e_return;

			case CMD_AT_CPIN:
				ast_log (LOG_ERROR, "[%s] Error checking PIN state\n", PVT_ID(pvt));
				//ecmd->res = RES_OK;

				break;
				//goto e_return;

			case CMD_AT_COPS_INIT:
				ast_log (LOG_ERROR, "[%s] Error setting operator select parameters\n", PVT_ID(pvt));
				break;
				//goto e_return;

			case CMD_AT_CREG_INIT:
				ast_log (LOG_ERROR, "[%s] Error enabling registration info\n", PVT_ID(pvt));
				goto e_return;

			case CMD_AT_CREG:
				ast_debug (1, "[%s] Error getting registration info\n", PVT_ID(pvt));
				break;

			case CMD_AT_CNUM:
				ast_log (LOG_WARNING, "[%s] Error checking subscriber phone number\n", PVT_ID(pvt));
				ast_verb (3, "[%s] Dongle needs to be reinitialized. The SIM card is not ready yet\n", PVT_ID(pvt));
				goto e_return;

			case CMD_AT_CARDLOCK:
				ast_verb (3, "[%s] CARDLOCK ERROR\n", PVT_ID(pvt));
				pvt->cardlock = 1;
				goto e_return;


			case CMD_AT_CVOICE:
				ast_verb (1, "[%s] Dongle has NO voice support\n", PVT_ID(pvt));
				ast_log (LOG_WARNING, "[%s] Dongle has NO voice support\n", PVT_ID(pvt));
				pvt->novoice = 1;
				pvt->has_voice = 0;
				goto e_return;
/*
				if (!pvt->initialized)
				{
					// continue initialization in other job at cmd CMD_AT_CMGF
					if (at_enque_initialization(task->cpvt, CMD_AT_CMGF))
					{
						ast_log (LOG_ERROR, "[%s] Error schedule initialization commands\n", PVT_ID(pvt));
						goto e_return;
					}
				}* /
				break;
				
/*
			case CMD_AT_CLIP:
				ast_log (LOG_ERROR, "[%s] Error enabling calling line indication\n", PVT_ID(pvt));
				goto e_return;
*/
			case CMD_AT_CSSN:
				ast_log (LOG_ERROR, "[%s] Error Supplementary Service Notification activation failed\n", PVT_ID(pvt));
				goto e_return;

			case CMD_AT_CMGF:
			case CMD_AT_CPMS:
				ast_log (LOG_ERROR, "[%s] Error setting CPMS=\n", PVT_ID(pvt));
				break;
			case CMD_AT_CNMI:
				ast_debug (1, "[%s] Command '%s' failed\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd));
				ast_debug (1, "[%s] No SMS support\n", PVT_ID(pvt));

				pvt->has_sms = 0;

				if (!pvt->initialized)
				{
					if (pvt->has_voice)
					{
						/* continue initialization in other job at cmd CMD_AT_CSQ */
						if (at_enque_initialization(task->cpvt, CMD_AT_CSQ))
						{
							ast_log (LOG_ERROR, "[%s] Error querying signal strength\n", PVT_ID(pvt));
							goto e_return;
						}

						pvt->timeout = DATA_READ_TIMEOUT;
						pvt->initialized = 1;
					/* FIXME: say 'initialized and ready' but disconnect */
//						ast_verb (3, "[%s] Dongle initialized and ready\n", PVT_ID(pvt));
					}
					goto e_return;
				}
				break;

			case CMD_AT_CSCS:
				ast_debug (1, "[%s] No UCS-2 encoding support\n", PVT_ID(pvt));

				pvt->use_ucs2_encoding = 0;
				break;

			case CMD_AT_A:
			case CMD_AT_CHLD_2x:
				ast_log (LOG_ERROR, "[%s] Answer failed for call idx %d\n", PVT_ID(pvt), task->cpvt->call_idx);
				queue_hangup (task->cpvt->channel, 0);
				break;

			case CMD_AT_CHLD_3:
				ast_log (LOG_ERROR, "[%s] Can't begin conference call idx %d\n", PVT_ID(pvt), task->cpvt->call_idx);
				queue_hangup(task->cpvt->channel, 0);
				break;

			case CMD_AT_CLIR:
				ast_log (LOG_ERROR, "[%s] Setting CLIR failed\n", PVT_ID(pvt));
				break;

			case CMD_AT_CHLD_2:
				if(!CPVT_TEST_FLAG(task->cpvt, CALL_FLAG_HOLD_OTHER) || task->cpvt->state != CALL_STATE_INIT)
					break;
				/* passthru */
			case CMD_AT_D:
				pvt->eerror=1;
readpvterrors(pvt);
PVT_STAT(pvt,stat_errors[2])++;
writepvterrors(pvt);
				ast_log (LOG_ERROR, "[%s] Dial failed\n", PVT_ID(pvt));
				//at_write(pvt,"AT+CFUN=1,1\r",sizeof("AT+CFUN=1,1\r"));
				queue_control_channel (task->cpvt, AST_CONTROL_CONGESTION);

				at_enque_cfun1(task->cpvt);
				//goto e_return;
				break; /// ??? eerror

			case CMD_AT_DDSETEX:
				ast_log (LOG_ERROR, "[%s] AT^DDSETEX failed\n", PVT_ID(pvt));
				break;

			case CMD_AT_CHUP:
			case CMD_AT_CHLD_1x:
				ast_log (LOG_ERROR, "[%s] Error sending hangup for call idx %d\n", PVT_ID(pvt), task->cpvt->call_idx);
				break;

			case CMD_AT_CMGR:
				pvt->incoming_sms = 0;
				pvt_try_restate(pvt);
				ast_log (LOG_ERROR, "[%s] Error reading SMS message\n", PVT_ID(pvt));
				break;

			case CMD_AT_CMGD:
				pvt->incoming_sms = 0;
				pvt_try_restate(pvt);
				ast_log (LOG_ERROR, "[%s] Error deleting SMS message\n", PVT_ID(pvt));
				break;

			case CMD_AT_CMGS:
			case CMD_AT_SMSTEXT:
				pvt->outgoing_sms = 0;
				putfilei("sim/state",pvt->imsi,"outgoing_sms",pvt->outgoing_sms);
				pvt_try_restate(pvt);

				manager_event_sent_notify(PVT_ID(pvt), "SMS", task, "NotSent");
				ast_verb (3, "[%s] Error sending SMS message %p\n", PVT_ID(pvt), task);
				ast_log (LOG_ERROR, "[%s] Error sending SMS message %p\n", PVT_ID(pvt), task);
				break;

			case CMD_AT_DTMF:
				ast_log (LOG_ERROR, "[%s] Error sending DTMF\n", PVT_ID(pvt));
				break;

			case CMD_AT_COPS:
				ast_debug (1, "[%s] Could not get provider name\n", PVT_ID(pvt));
				break;

			case CMD_AT_SPN:
				ast_debug (1, "[%s] Could not get provider name2\n", PVT_ID(pvt));
				break;

			case CMD_AT_CLVL:
				ast_debug (1, "[%s] Audio level synchronization failed at step %d/%d\n", PVT_ID(pvt), pvt->volume_sync_step, VOLUME_SYNC_DONE-1);
				pvt->volume_sync_step = VOLUME_SYNC_BEGIN;
				break;

			case CMD_AT_CUSD:
				pvt->outgoing_ussd = 0;
				putfilei("sim/state",pvt->imsi,"outgoing_ussd",pvt->outgoing_ussd);
				manager_event_sent_notify(PVT_ID(pvt), "USSD", task, "NotSent");
				ast_verb (3, "[%s] Error sending USSD %p\n", PVT_ID(pvt), task);
				ast_log (LOG_ERROR, "[%s] Error sending USSD %p\n", PVT_ID(pvt), task);
				break;

			default:
				ast_log (LOG_ERROR, "[%s] Received 'ERROR' for unhandled command '%s'\n", PVT_ID(pvt), at_cmd2str (ecmd->cmd));
				break;
		}
		at_queue_handle_result (pvt, res);
	}
	else if (ecmd)
	{
		ast_log (LOG_ERROR, "[%s] Received 'ERROR' when expecting '%s', ignoring\n", PVT_ID(pvt), at_res2str (ecmd->res));
	}
	else
	{
		ast_log (LOG_ERROR, "[%s] Received unexpected 'ERROR'\n", PVT_ID(pvt));
	}

	return 0;

e_return:
	at_queue_handle_result (pvt, res);

	return -1;
}

/*!
 * \brief Handle ^RSSI response Here we get the signal strength.
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */




/*
^DSFLOWRPT:n,n,n,n,n,n,n
This gives you connection statistics while online, you should receive them every two seconds. The values are all in hexadecimal.
n1 is the duration of the connection in seconds
n2 is transmit (upload) speed in bytes per second (n2 *8 / 1000 will give you kbps)
n3 is receive (download) speed in bytes per second (n3 *8 / 1000 will give you kbps)
n4 is the total bytes transmitted during this session
n5 is the total bytes transmitted during this session
n6 is the negotiated QoS uplink in bytes per second (n2 *8 / 1000 will give you kbps)
n7 is the negotiated QoS downlink in bytes per second (n2 *8 / 1000 will give you kbps)
Note: n4 and n5 are 64-bit integers, for those >4GB torrent sessions! 
You can reset the connection statistics by sending AT^DSFLOWCLR.
*/





/*!
 * \brief Handle ^MODE response Here we get the link mode (GSM, UMTS, EDGE...).
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error

<sys_mode>: System mode. The values are as follows:
0    No service.
1    AMPS mode (not in use currently)
2    CDMA mode (not in use currently)
3    GSM/GPRS mode
4    HDR mode
5    WCDMA mode
6    GPS mode
<sys_submode>: System sub mode. The values are as follows:
0    No service.
1    GSM mode
2    GPRS mode
3    EDEG mode
4    WCDMA mode
5    HSDPA mode
6  HSUPA mode
7  HSDPA mode and HSUPA mode  Attachment is the detailed AT command of our


 */









extern void svistok_bridge_upstream_request_clcc(struct pvt * pvt);
           
  
                                                                                    
  
 

/*!
 * \brief Handle ^ORIG response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \retval  0 success
 * \retval -1 error
 */

extern int svistok_bridge_upstream_at_response_orig(struct pvt * pvt, const char * str);
        
                                            

                              
          
  
                                                                             
           
  


   
                                            
                                  
    

                                                               
  
                                                                                
            
  

                                                                                                       

                                       
  

                                                              
   
                          
                                
                                 
  
                               
   
                                                     
                          
                                                 
    
                                              
                                
     
                                                                              
     
        
                             
    

                     
   
  
     
  
                                                      
  
                                                                                                                     
  
          
 

#if 0
/*!
 * \brief Handle ^CONF response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_conf (struct pvt* pvt, const char* str)
{
	int call_index;
	struct cpvt * cpvt;

	/*
	 * parse CONF info in the following format:
	 * ^CONF: <call_index>
	 */

	if (sscanf (str, "^CONF:%d", &call_index) != 1)
	{
		ast_log (LOG_ERROR, "[%s] Error parsing ORIG event '%s'\n", PVT_ID(pvt), str);
		return -1;
	}

	ast_debug (1, "[%s] CONF Received call_index %d\n", PVT_ID(pvt), call_index);

//	v_stat_call_response(pvt);

	cpvt = pvt_find_cpvt(pvt, call_index);
	if(cpvt)
	{
	
		channel_change_state(cpvt, CALL_STATE_ALERTING, 0);
	}

	return 0;
}
#endif /* 0 */


/*
void set_channel_vars2(struct pvt* pvt, struct ast_channel* channel)
{
    unsigned idx;
    channel_var_t dev_vars[] =
	{
        { "DONGLENAME", PVT_ID(pvt) },
        { "DONGLEPROVIDER", pvt->provider_name },
        { "DONGLEIMEI", pvt->imei },
        { "DONGLEIMSI", pvt->imsi },
        { "DONGLES",    pvt->serial },
        { "DONGLENUMBER", pvt->subscriber_number }

//        { "LAC", pvt->location_area_code },
//        { "CELL", pvt->cell_id }
    };

    for(idx = 0; idx < ITEMS_OF(dev_vars); ++idx)
    pbx_builtin_setvar_helper (channel, dev_vars[idx].name, dev_vars[idx].value);

}
*/

/*!
 * \brief Handle ^CEND response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_cend (struct pvt * pvt, const char* str)
{
	int bs_out_calls;
	int bs_in_calls;

	int bs_out_duration;
	int bs_in_duration;
	
	int bs_acdl;
	int bs_asrl;
	
	long int pdd=0;
	long int pdds=0;

	
	int call_index = 0;
	long int duration_total=0;
	long int duration   = 0;
	int end_status = 0;
	int cc_cause   = 0;
	struct cpvt * cpvt;
	struct ast_channel * chan;
	long int fassec,pddcsec;
	long int td;

	//long duration_total;

	char buffer[64];

	request_clcc(pvt);

	/*
	 * parse CEND info in the following format:
	 * ^CEND:<call_index>,<duration>,<end_status>[,<cc_cause>]
	 */

	if (sscanf (str, "^CEND:%d,%d,%d,%d", &call_index, &duration, &end_status, &cc_cause) != 4)
	{
		ast_debug (1, "[%s] Could not parse all CEND parameters\n", PVT_ID(pvt));
	}

	ast_debug (1,	"[%s] CEND: call_index %d duration %d end_status %d cc_cause %d Line disconnected\n"
				, PVT_ID(pvt), call_index, duration, end_status, cc_cause);

	cpvt = pvt_find_cpvt(pvt, call_index);
	if (cpvt)
	{
		PVT_STAT(pvt, calls_duration[cpvt->dir]) += duration;
		
		
		
		if (cpvt->dir==CALL_DIR_OUTGOING)
		{
			//bs
			getfilei_def("bs/state",    pvt->cell_id,  "acdl",&bs_acdl,ACDLINIT*1000);
			getfilei_def("bs/state",    pvt->cell_id,  "asrl",&bs_asrl,ASRLINIT);
			bs_acdl = (((bs_acdl)*(ACDL_BS-1))+(duration*1000))/ACDL_BS;
			if (duration>0)
				bs_asrl=(bs_asrl*(ASRL_BS-1)+100000)/ASRL_BS;
			else
				bs_asrl = (bs_asrl*(ASRL_BS-1))/ASRL_BS;
			putfilei("bs/state",    pvt->cell_id,  "acdl",bs_acdl);
			putfilei("bs/state",    pvt->cell_id,  "asrl",bs_asrl);
		
			putfilei("sim/state",pvt->imsi,"state_in",0);
			putfilei("sim/state",pvt->imsi,"state_out",0);
			putfilei("sim/state",pvt->imsi,"state_active",0);
			putfilei("sim/state",pvt->imsi,"busy",0);

			putfiles("sim/state",pvt->imsi,"pro","");
			putfiles("sim/state",pvt->imsi,"cap","");

			readpvtinfo(pvt);
			
			
			
			
			/*
			if (!getfilei("dongles",PVT_ID(pvt),"stat_calls_duration",&PVT_STAT(pvt, stat_calls_duration[1]))) {PVT_STAT(pvt, stat_calls_duration[1])=0;}                
			if (!getfilei("sim",    pvt->imsi,  "stat_calls_duration",&PVT_STAT(pvt, stat_calls_duration[2]))) {PVT_STAT(pvt, stat_calls_duration[2])=0;}

			if (!getfilei("dongles",PVT_ID(pvt),"stat_calls_answered",&PVT_STAT(pvt, stat_calls_answered[1]))) {PVT_STAT(pvt, stat_calls_answered[1])=0;}
			if (!getfilei("sim",    pvt->imsi,  "stat_calls_answered",&PVT_STAT(pvt, stat_calls_answered[2]))) {PVT_STAT(pvt, stat_calls_answered[2])=0;}

			if (!getfilei("dongles",PVT_ID(pvt),"stat_acdl",&PVT_STAT(pvt, stat_acdl[1]))) {PVT_STAT(pvt, stat_acdl[1])=ACDLINIT*100;}
			if (!getfilei("sim",    pvt->imsi,  "stat_acdl",&PVT_STAT(pvt, stat_acdl[2]))) {PVT_STAT(pvt, stat_acdl[2])=ACDLINIT*100;}

			if (!getfilei("dongles",PVT_ID(pvt),"stat_datt",&PVT_STAT(pvt, stat_datt[1]))) {PVT_STAT(pvt, stat_datt[1])=0;}
			if (!getfilei("sim",    pvt->imsi,  "stat_datt",&PVT_STAT(pvt, stat_datt[2]))) {PVT_STAT(pvt, stat_datt[2])=0;}
			*/
			
			PVT_STAT(pvt, stat_calls_duration[1]) += duration;
			PVT_STAT(pvt, stat_calls_duration[2]) += duration;

			PVT_STAT(pvt, stat_out_calls[1]) ++;
			PVT_STAT(pvt, stat_out_calls[2]) ++;


			v_stat_call_end(pvt,duration);
			//PVT_STAT(pvt,stat_call_end)=(long)time(NULL);
			
			
			duration_total=PVT_STAT(pvt,stat_call_end)-PVT_STAT(pvt,stat_call_start);
			
			if(duration_total>90)
			{
				putfilei("sim",pvt->imsi,"outdone",1);
			}
			
			td=(long)time(NULL)-PVT_STAT(pvt,stat_call_start)-duration;

			PVT_STAT(pvt, stat_wait_duration[1]) +=td;
			PVT_STAT(pvt, stat_wait_duration[2]) +=td;

			if(PVT_STAT(pvt,stat_call_response)>0)
			{
				pdd=(long)time(NULL)-PVT_STAT(pvt,stat_call_response)-duration;

				pdds=PVT_STAT(pvt,stat_call_response)-PVT_STAT(pvt,stat_call_start);
			} else 
			{
				pdd=0;
				pdds=PVT_STAT(pvt,stat_call_end)-PVT_STAT(pvt,stat_call_start)-duration;
			}

		    ast_verb(3,"time: %s st=%ld res=%ld proc=%ld en=%ld dur=%d \n",PVT_ID(pvt),PVT_STAT(pvt,stat_call_start),PVT_STAT(pvt,stat_call_response),PVT_STAT(pvt,stat_call_process),PVT_STAT(pvt,stat_call_end),duration);


		


		
			if(cpvt->answered>0)
			{
			    PVT_STAT(pvt, stat_calls_answered[1]) ++;
			    PVT_STAT(pvt, stat_calls_answered[2]) ++;

			    PVT_STAT(pvt, stat_iatt)++;

			    PVT_STAT(pvt, stat_satt)++;

			    
			    PVT_STAT(pvt, stat_datt[1])=0;
			    PVT_STAT(pvt, stat_datt[2])=0;
			    

			total_stat_datt=0 ;
			
			
			total_stat_acdl = (total_stat_acdl*(ACDL-1)+(duration*1000))/ACDL;
			total_stat_pddl[1] = (long int)((total_stat_pddl[1]*(PDDL-1)+(td*1000))/PDDL);

			PVT_STAT(pvt, stat_asrl[1]) = (PVT_STAT(pvt, stat_asrl[1])*(ASRL-1)+100000)/ASRL;
			PVT_STAT(pvt, stat_asrl[2]) = (PVT_STAT(pvt, stat_asrl[2])*(ASRL-1)+100000)/ASRL;

			PVT_STAT(pvt, stat_acdl[1]) = (PVT_STAT(pvt, stat_acdl[1])*(ACDL-1)+(duration*1000))/ACDL;
			PVT_STAT(pvt, stat_acdl[2]) = (PVT_STAT(pvt, stat_acdl[2])*(ACDL-1)+(duration*1000))/ACDL;

			PVT_STAT(pvt, stat_pddl[1][1]) = (long int)(((float)PVT_STAT(pvt, stat_pddl[1][1])*(PDDL-1)+(float)(td*1000))/PDDL);
			PVT_STAT(pvt, stat_pddl[1][2]) = (long int)(((float)PVT_STAT(pvt, stat_pddl[1][2])*(PDDL-1)+(float)(td*1000))/PDDL);

			} else 
			{

			total_stat_datt++ ;
			total_stat_pddl[0] = (long int)((total_stat_pddl[0]*(PDDL-1)+(td*1000))/PDDL);


			PVT_STAT(pvt, stat_asrl[1]) = (PVT_STAT(pvt, stat_asrl[1])*(ASRL-1))/ASRL;
			PVT_STAT(pvt, stat_asrl[2]) = (PVT_STAT(pvt, stat_asrl[2])*(ASRL-1))/ASRL;

			if(strcmp(pvt->numberb,pvt->numberb_before)!=0)
			{
			    PVT_STAT(pvt, stat_datt[1])++;
			    PVT_STAT(pvt, stat_datt[2])++;
			}
			    strcpy(pvt->numberb_before,pvt->numberb);

			PVT_STAT(pvt, stat_pddl[0][1]) = (long int)(((float)PVT_STAT(pvt, stat_pddl[0][1])*(PDDL-1)+(float)(td*1000))/PDDL);
			PVT_STAT(pvt, stat_pddl[0][2]) = (long int)(((float)PVT_STAT(pvt, stat_pddl[0][2])*(PDDL-1)+(float)(td*1000))/PDDL);
			}
			
			
			//запись логов
			/*
			
			IMSI,
			NUMBERA,
			NUMBERB,
			NUMBERMY,
			DONGLES,
			DONGLENAME,
			IAXME,
			TOTALSEC_i,
			BILLSEC_i,
			DONGLEIMEI,
			DONGLEIMSI,
			LAC,
			CELL,
			END_STATUS_i,
			CC_CAUSE_i
			
			*/

ast_verb(3,"PDDCHECK2 : pdd=%d, pdds=%d", pdd,pdds);

if (PVT_STAT(pvt,stat_call_fas)>0)
    fassec=(long)time(NULL)-PVT_STAT(pvt,stat_call_fas);
else
   fassec=0;

if (PVT_STAT(pvt,stat_call_pddc)>0)
    pddcsec=(long)time(NULL)-PVT_STAT(pvt,stat_call_pddc);
else
    pddcsec=(long)time(NULL)-PVT_STAT(pvt,stat_call_start);

			callendout(pvt->imsi,
				pvt->numbera,
				pvt->numberb,
				"NUMBERMY",
				pvt->serial,
				PVT_ID(pvt),
				PVT_STAT(pvt,stat_call_start),
				cpvt->answered,
				duration_total,
				duration,
				fassec,
				pddcsec,
				pvt->imei,
				pvt->imsi,
				pvt->location_area_code,
				pvt->cell_id,
				end_status,
				cc_cause,
				pvt->spec,
				pvt->qos,
				pvt->vip,
				pdd,
				pdds,
				pvt->naprstr,
				pvt->im,
				pvt->uid,
				pvt->procur,
				pvt->capcur,

				pvt->fas,
				pvt->epdd,
				pvt->fpdd,
				pvt->hem,
				pvt->hoa,
				pvt->em_type
			);

			*pvt->procur=0;
			*pvt->capcur=0;

			
			//limits_temp(pvt);

			//limits_final(pvt,duration);
			

			//Запишем переменные
//			chan=cpvt->channel;
//			if (chan!=NULL)
//			{
//			chan=NULL;
//			ast_debug (1,"[%s]",pvt->imsi);
//			ast_debug (1,"[%s]",pvt->imei);
/*

			ast_debug (1,"[%s]",pvt->location_area_code);
			ast_debug (1,"[%s]",pvt->pvt->cell_id);
			ast_debug (1,"[%s]",pvt->duration);
			ast_debug (1,"[%s]",pvt->imsi);
			ast_debug (1,"[%s]",pvt->imsi);
			ast_debug (1,"[%s]",pvt->imsi);
			ast_debug (1,"[%s]",pvt->imsi);*/

//			chan=NULL;

//			chan=cpvt->channel;
/*
			chan=NULL;

			ast_verb(3,"---1");
			set_channel_vars2(pvt,chan);
			ast_verb(3,"---2");
			*/

			snprintf (buffer,64,"%d",duration);
			//pbx_builtin_setvar_helper(cpvt->requestor, "BILLSEC", buffer);
			snprintf (buffer,64,"%d",duration_total);
			//pbx_builtin_setvar_helper(cpvt->requestor, "TOTALSEC", buffer);
			
			//pbx_builtin_setvar_helper(cpvt->requestor, "NUMBERB2", pvt->numberb);
			//pbx_builtin_setvar_helper(cpvt->requestor, "NUMBERA2", pvt->numbera);
			
			/*
			//ast_channel_unlock(chan);

			
			ast_verb(3,"---3");*/
//			}
			
			
			/*
			putfilei("dongles",PVT_ID(pvt),"stat_calls_answered",PVT_STAT(pvt, stat_calls_answered[1]));
			putfilei("sim",pvt->imsi,      "stat_calls_answered",PVT_STAT(pvt, stat_calls_answered[2]));

			putfilei("dongles",PVT_ID(pvt),"stat_calls_duration",PVT_STAT(pvt, stat_calls_duration[1]));
			putfilei("sim",pvt->imsi,      "stat_calls_duration",PVT_STAT(pvt, stat_calls_duration[2]));

			putfilei("dongles",PVT_ID(pvt),"stat_acdl",PVT_STAT(pvt, stat_acdl[1]));
			putfilei("sim",pvt->imsi,      "stat_acdl",PVT_STAT(pvt, stat_acdl[2]));

			putfilei("dongles",PVT_ID(pvt),"stat_datt",PVT_STAT(pvt, stat_datt[1]));
			putfilei("sim",pvt->imsi,      "stat_datt",PVT_STAT(pvt, stat_datt[2]));
			*/
			
			writepvtinfo(pvt);
		} else if (cpvt->dir==CALL_DIR_INCOMING) {
		
			v_stat_call_end(pvt,duration);
			
			PVT_STAT(pvt,stat_call_end)=(long)time(NULL);

			putfilei("sim/state",pvt->imsi,"state_in",0);
			putfilei("sim/state",pvt->imsi,"state_out",0);
			putfilei("sim/state",pvt->imsi,"state_active",0);

			putfilei("sim/state",pvt->imsi,"busy",0);
			putfilei("sim/state",pvt->imsi,"indone",1);


			readpvtinfo(pvt);
			
			PVT_STAT(pvt, stat_iatt)=0;
			PVT_STAT(pvt, stat_in_duration) += duration;
			PVT_STAT(pvt, stat_in_answered) ++;

			writepvtinfo(pvt);

			duration_total=PVT_STAT(pvt,stat_call_end)-PVT_STAT(pvt,stat_call_start);

			callendin(pvt->imsi,
				pvt->numberb,
				"NUMBERMY",
				pvt->serial,
				PVT_ID(pvt),
				duration_total,
				duration,
				pvt->imei,
				pvt->imsi,
				pvt->location_area_code,
				pvt->cell_id,
				end_status,
				cc_cause,
				pvt->uid
			);


		}

		CPVT_RESET_FLAGS(cpvt, CALL_FLAG_NEED_HANGUP);

		
		
		

		
		change_channel_state(cpvt, CALL_STATE_RELEASED, cc_cause);
		manager_event_cend(PVT_ID(pvt), call_index, duration, end_status, cc_cause);
	}
	else
	{
//		ast_log (LOG_ERROR, "[%s] CEND event for unknown call idx '%d'\n", PVT_ID(pvt), call_index);
	}

	return 0;
}

/*!
 * \brief Handle +CSCA response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */
extern int svistok_bridge_upstream_at_response_csca(struct pvt * pvt, char * str);
                     
  
                                                                               
            
  
                                                                     

                                                                 
          
 

/*!
 * \brief Handle ^CONN response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_conn (struct pvt* pvt, const char* str)
{
	int call_index;
	int call_type;
	struct cpvt * cpvt;

	pvt->ring = 0;
	pvt->dialing = 0;
	pvt->cwaiting = 0;

	putfilei("sim/state",pvt->imsi,"state_ring",0);
	putfilei("sim/state",pvt->imsi,"state_dialing",0);
	putfilei("sim/state",pvt->imsi,"state_cwaiting",0);

	// !!!! request_clcc(pvt);

	/*
	 * parse CONN info in the following format:
	 * ^CONN:<call_index>,<call_type>
	 */
	if (sscanf (str, "^CONN:%d,%d", &call_index, &call_type) != 2)
	{
		ast_log (LOG_ERROR, "[%s] Error parsing CONN event '%s'\n", PVT_ID(pvt), str);
		return -1;
	}

	ast_debug (1, "[%s] CONN Received call_index %d call_type %d\n", PVT_ID(pvt), call_index, call_type);

	if (call_type == CLCC_CALL_TYPE_VOICE)
	{
		cpvt = pvt_find_cpvt(pvt, call_index);
		if(cpvt)
		{
/* FIXME: delay until CLCC handle?
*/
			PVT_STAT(pvt, calls_answered[cpvt->dir]) ++;
			cpvt->answered=1;
			change_channel_state(cpvt, CALL_STATE_ACTIVE, 0);

// answer
			if(CONF_SHARED(pvt,group)==295)
			{
				ast_debug (1, "[%s] ^CONN&group==295\n", PVT_ID(pvt));
				if(cpvt->dir == CALL_DIR_OUTGOING)
				{
				    	ast_debug (1, "[%s] OUTGOING => hangup\n", PVT_ID(pvt));
					at_enque_hangup(cpvt,call_index);
					return 0;
				} else {
				    	ast_debug (1, "[%s] INCOMING\n", PVT_ID(pvt));
				}
			}

			request_clcc(pvt);
			if(CPVT_TEST_FLAG(cpvt, CALL_FLAG_CONFERENCE))
				at_enque_conference(cpvt);
		}
		else
		{
			at_enque_hangup(&pvt->sys_chan, call_index);
			ast_log (LOG_ERROR, "[%s] answered incoming call with not exists call idx %d, hanging up!\n", PVT_ID(pvt), call_index);
		}
	}
	else
		ast_log (LOG_ERROR, "[%s] answered not voice incoming call type '%d' idx %d, skipped\n", PVT_ID(pvt), call_type, call_index);
	return 0;
}


static int start_pbx(struct pvt* pvt, const char * number, int call_idx, call_state_t state)
{
	struct cpvt* cpvt;

	/* TODO: pass also Subscriber number or other DID info for exten  */
	struct ast_channel * channel = new_channel (pvt, AST_STATE_RING, number, call_idx, CALL_DIR_INCOMING, state, pvt->has_subscriber_number ? pvt->subscriber_number : CONF_SHARED(pvt, exten), NULL);
	//ast_log (LOG_ERROR, "[%s] !!! new_channel %s,%s,\n", PVT_ID(pvt),number,pvt->has_subscriber_number ? pvt->subscriber_number : CONF_SHARED(pvt, exten));

	if (!channel)
	{
		ast_log (LOG_ERROR, "[%s] Unable to allocate channel for incoming call\n", PVT_ID(pvt));

		if (at_enque_hangup (&pvt->sys_chan, call_idx))
		{
			ast_log (LOG_ERROR, "[%s] Error sending AT+CHUP command\n", PVT_ID(pvt));
		}

		return -1;
	}
	cpvt = ast_channel_tech_pvt(channel); //cpvt=channel->tech_pvt;
// FIXME: not execute if channel_new() failed
	CPVT_SET_FLAGS(cpvt, CALL_FLAG_NEED_HANGUP);

	// ast_pbx_start() usually failed if asterisk.conf minmemfree set too low, try drop buffer cache sync && echo 3 > /proc/sys/vm/drop_caches
	if (ast_pbx_start (channel))
	{
		ast_channel_tech_pvt_set(channel,NULL);//channel->tech_pvt = NULL;
		cpvt_free(cpvt);

		ast_hangup (channel);
		ast_log (LOG_ERROR, "[%s] Unable to start pbx on incoming call\n", PVT_ID(pvt));
		// TODO: count fails and reset incoming when count reach limit ?
		return -1;
	}

	
/*
	if (ast_pbx_start (cpvt->requestor))
	{
		ast_log (LOG_ERROR, "[%s] !!! Unable to start pbx on cpvt->requestor\n", PVT_ID(pvt));
		return -1;
	}

	pbx_builtin_setvar_helper (cpvt->requestor, "DONGLENAME", PVT_ID(pvt));
	pbx_builtin_setvar_helper (cpvt->channel, "DONGLENAME", PVT_ID(pvt));

	pbx_builtin_setvar_helper (cpvt->requestor, "DONGLEIMEI", pvt->imei);
	pbx_builtin_setvar_helper (cpvt->channel, "DONGLEIMEI", pvt->imei);

	pbx_builtin_setvar_helper (cpvt->requestor, "DONGLEIMSI", pvt->imsi);
	pbx_builtin_setvar_helper (cpvt->channel, "DONGLEIMSI", pvt->imsi);

	pbx_builtin_setvar_helper (cpvt->requestor, "DONGLENUMBER", pvt->subscriber_number);
	pbx_builtin_setvar_helper (cpvt->channel, "DONGLENUMBER", pvt->subscriber_number);
*/

/*                { "DONGLENAME", PVT_ID(pvt) },
                { "DONGLEIMEI", pvt->imei },
                { "DONGLEIMSI", pvt->imsi },
                { "DONGLENUMBER", pvt->subscriber_number }*/

        //set_channel_vars(pvt, cpvt->requestor);
        //set_channel_vars(pvt, cpvt->channel);

	return 0;
}

/*!
 * \brief Handle +CLCC response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_clcc (struct pvt* pvt, char* str)
{
	struct cpvt * cpvt;
	unsigned call_idx, dir, state, mode, mpty, type;
	unsigned all = 0;
	unsigned held = 0;
	char * number;
	char *p;

	if (pvt->initialized)
	{
		/* I think man is good until he proves the reverse */
		AST_LIST_TRAVERSE(&pvt->chans, cpvt, entry)
		{
			CPVT_RESET_FLAGS(cpvt, CALL_FLAG_ALIVE);
		}

		for(;;)
		{
			if(at_parse_clcc(str, &call_idx, &dir, &state, &mode, &mpty, &number, &type) == 0)
			{
				ast_debug (3, "[%s] CLCC callidx %u dir %u state %u mode %u mpty %u number %s type %u\n",  PVT_ID(pvt), call_idx, dir, state, mode, mpty, number, type);
				if(mode == CLCC_CALL_TYPE_VOICE && state <= CALL_STATE_WAITING)
				{
					cpvt = pvt_find_cpvt(pvt, call_idx);
					if(cpvt)
					{
						/* cpvt alive */
						CPVT_SET_FLAGS(cpvt, CALL_FLAG_ALIVE);
						if(dir == cpvt->dir)
						{
							if(mpty)
								CPVT_SET_FLAGS(cpvt, CALL_FLAG_MULTIPARTY);
							else
								CPVT_RESET_FLAGS(cpvt, CALL_FLAG_MULTIPARTY);
							if(dir == CALL_DIR_INCOMING && (state == CALL_STATE_INCOMING || state == CALL_STATE_WAITING))
							{
								if(cpvt->channel)
								{
									/* FIXME: unprotected channel access */
									ast_channel_rings_set(cpvt->channel, ast_channel_rings(cpvt->channel)+pvt->rings);
									//cpvt->channel->rings += pvt->rings;
									pvt->rings = 0;
								}
							}
							if(state != cpvt->state)
							{
								change_channel_state(cpvt, state, 0);
							}
						}
						else
						{
							ast_log (LOG_ERROR, "[%s] CLCC call idx %d direction mismatch %d/%d\n", PVT_ID(pvt), cpvt->call_idx, dir, cpvt->dir);
						}
					}
					else if(dir == CALL_DIR_INCOMING && (state == CALL_STATE_INCOMING || state == CALL_STATE_WAITING))
					{
						if(state == CALL_STATE_INCOMING)
						{
							PVT_STAT(pvt, in_calls) ++;
							
							strcpy(pvt->numberb,number);

//							putfiles("sim/state",pvt->imsi,"last_numberb",number);
							putfiles("sim/state",pvt->imsi,"numberb",number);
							
							putfilei("sim/state",pvt->imsi,"busy",1);

							putfilei("sim/state",pvt->imsi,"state_in",1);
							putfilei("sim/state",pvt->imsi,"state_out",0);
							putfilei("sim/state",pvt->imsi,"state_active",1);


							
							v_stat_call_start(pvt);
						}
						else
							PVT_STAT(pvt, cw_calls) ++;
						if(pvt_enabled(pvt))
						{
							/* TODO: give dialplan level user tool for checking device is voice enabled or not  */
							if(start_pbx(pvt, number, call_idx, state) == 0)
							{
								PVT_STAT(pvt, in_calls_handled) ++;
								if(!pvt->has_voice)
									ast_log (LOG_WARNING, "[%s] pbx started for device not voice capable\n", PVT_ID(pvt));
							}
							else
								PVT_STAT(pvt, in_pbx_fails) ++;
						}
					}

					all++;
					switch(state)
					{
						case CALL_STATE_WAITING:
							pvt->cwaiting = 1;
							pvt->ring = 0;
							pvt->dialing = 0;

							putfilei("sim/state",pvt->imsi,"state_ring",0);
							putfilei("sim/state",pvt->imsi,"state_dialing",0);
							putfilei("sim/state",pvt->imsi,"state_cwaiting",1);
							
							break;

						case CALL_STATE_ONHOLD:
							held++;
							break;

						case CALL_STATE_DIALING:
						case CALL_STATE_ALERTING:
							pvt->dialing = 1;
							pvt->cwaiting = 0;
							pvt->ring = 0;

							putfilei("sim/state",pvt->imsi,"state_ring",0);
							putfilei("sim/state",pvt->imsi,"state_dialing",1);
							putfilei("sim/state",pvt->imsi,"state_cwaiting",0);

							break;

						case CALL_STATE_INCOMING:
							pvt->ring = 1;
							pvt->dialing = 0;
							pvt->cwaiting = 0;
							


							putfilei("sim/state",pvt->imsi,"state_ring",1);
							putfilei("sim/state",pvt->imsi,"state_dialing",0);
							putfilei("sim/state",pvt->imsi,"state_cwaiting",0);

							break;
						default:;
					}
				}
			}
			else
			{
				ast_log (LOG_ERROR, "[%s] can't parse CLCC line '%s'\n", PVT_ID(pvt), str);
			}
			p = strchr(str, '\r');
			if(p)
			{
				++p;
				if(p[0] == '\n')
					++p;
				if(p[0])
				{
					str = p;
					continue;
				}
			}
			/* or -1 ? */
			return 0;
		}
		/* unhold first held call */
		if(all == held)
		{
/* HW BUG 2: when no active call exists not way to enable voice again on activated from hold call
	call will be activated but no voice
*/
			ast_debug (1, "[%s] all %u call held, try activate some\n",  PVT_ID(pvt), all);
			if(at_enque_flip_hold(&pvt->sys_chan))
			{
				ast_log (LOG_ERROR, "[%s] can't flip active and hold/waiting calls \n", PVT_ID(pvt));
			}
		}

		/* dead cpvt only here */
		AST_LIST_TRAVERSE(&pvt->chans, cpvt, entry)
		{
			if(!CPVT_TEST_FLAG(cpvt, CALL_FLAG_ALIVE))
				change_channel_state(cpvt, CALL_STATE_RELEASED, 0);
		}
	}
	return 0;
}

/*!
 * \brief Handle +CCWA response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

extern int svistok_bridge_upstream_at_response_ccwa(struct pvt * pvt, char * str);
       

    
                        
                            
                 
                            
               
                                                     
                             
                           
                                                                                                
   
    
                                                         
           

                                                  
           
           
                 
  
                                                                                                       
   
                                                                
                                                                                                           
   
           
  

                      
  
                                                                        
                                      
   
                                                                                               
                                 
    
                 
                      
                      
    
   
      
                                                                              
  
          
 

/*!
 * \brief Handle RING response
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_ring (struct pvt* pvt)
{
	if (pvt->initialized)
	{
		pvt->ring = 1;
		pvt->dialing = 0;
		pvt->cwaiting = 0;

		putfilei("sim/state",pvt->imsi,"state_ring",1);
		putfilei("sim/state",pvt->imsi,"state_dialing",0);
		putfilei("sim/state",pvt->imsi,"state_cwaiting",0);

		pvt->rings++;

		request_clcc(pvt);

		/* We only want to syncronize volume on the first ring and if no channels yes */
		if (pvt->volume_sync_step == VOLUME_SYNC_BEGIN && PVT_NO_CHANS(pvt))
		{
			if (at_enque_volsync (&pvt->sys_chan))
			{
				ast_log (LOG_ERROR, "[%s] Error synchronize audio level\n", PVT_ID(pvt));
			}
			else
				pvt->volume_sync_step++;
		}
	}

	return 0;
}

/*!
 * \brief Handle +CMTI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

extern int svistok_bridge_upstream_at_response_cmti(struct pvt * pvt, const char * str);
          
                                 

                
  
                                                            

                                   
   
                                                                                                      
   
                           
   
                                                                                     
    
                                                                                          
              
    
       
                             
   

           
  
     
  
                                                                                                               
            
  
 

/*!
 * \brief Handle +CMGR response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_cmgr (struct pvt* pvt, const char * str, size_t len)
{
	char		oa[512] = "";
	char*		msg = NULL;
	str_encoding_t	oa_enc;
	str_encoding_t	msg_enc;
	const char*	err;
	char*		err_pos;
	char*		cmgr;
	ssize_t		res;
	char		sms_utf8_str[4096];
	char*		number;
	char		from_number_utf8_str[1024];
	char		text_base64[16384];
	size_t		msg_len;

	const struct at_queue_cmd * ecmd = at_queue_head_cmd (pvt);

	manager_event_message("DongleNewCMGR", PVT_ID(pvt), str);
	if (ecmd)
	{
	    if (ecmd->res == RES_CMGR || ecmd->cmd == CMD_USER)
	    {
		at_queue_handle_result (pvt, RES_CMGR);
		pvt->incoming_sms = 0;
		pvt_try_restate(pvt);

		cmgr = err_pos = ast_strdupa (str);
		err = at_parse_cmgr (&err_pos, len, oa, sizeof(oa), &oa_enc, &msg, &msg_enc);
		if (err)
		{
			ast_log (LOG_WARNING, "[%s] Error parsing incoming message '%s' at possition %d: %s\n", PVT_ID(pvt), str, (int)(err_pos - cmgr), err);
			return 0;
		}

		ast_debug (1, "[%s] Successfully read SMS message\n", PVT_ID(pvt));

		/* last chance to define encodings */
		if (oa_enc == STR_ENCODING_UNKNOWN)
			oa_enc = pvt->use_ucs2_encoding ? STR_ENCODING_UCS2_HEX : STR_ENCODING_7BIT;

		if (msg_enc == STR_ENCODING_UNKNOWN)
			msg_enc = pvt->use_ucs2_encoding ? STR_ENCODING_UCS2_HEX : STR_ENCODING_7BIT;

		/* decode number and message */
		res = str_recode (RECODE_DECODE, oa_enc, oa, strlen(oa), from_number_utf8_str, sizeof (from_number_utf8_str));
		if (res < 0)
		{
			ast_log (LOG_ERROR, "[%s] Error decode SMS originator address: '%s', message is '%s'\n", PVT_ID(pvt), oa, str);
			number = oa;
			return 0;
		}
		else
			number = from_number_utf8_str;

		msg_len = strlen(msg);
		res = str_recode (RECODE_DECODE, msg_enc, msg, msg_len, sms_utf8_str, sizeof (sms_utf8_str));
		if (res < 0)
		{
			ast_log (LOG_ERROR, "[%s] Error decode SMS text '%s' from encoding %d, message is '%s'\n", PVT_ID(pvt), msg, msg_enc, str);
			return 0;
		}
		else
		{
			msg = sms_utf8_str;
			msg_len = res;
		}

		ast_verb (1, "[%s] Got SMS from %s: '%s'\n", PVT_ID(pvt), number, msg);
		ast_base64encode (text_base64, (unsigned char*)msg, msg_len, sizeof(text_base64));

		manager_event_new_sms(PVT_ID(pvt), number, msg);
		manager_event_new_sms_base64(PVT_ID(pvt), number, text_base64);
		{
			channel_var_t vars[] = 
			{
				{ "SMS", msg } ,
				{ "SMS_BASE64", text_base64 },
				{ "CMGR", (char *)str },


			{ "DONGLENAME", PVT_ID(pvt) },
			{ "DONGLEPROVIDER", pvt->provider_name },
			{ "DONGLEIMEI", pvt->imei },
			{ "DONGLEIMSI", pvt->imsi },
			{ "DONGLES",    pvt->serial },
			{ "DONGLENUMBER", pvt->subscriber_number },


				{ NULL, NULL },
			};
			start_local_channel (pvt, "sms", number, vars);
		}
	    }
	    else
	    {
		ast_log (LOG_ERROR, "[%s] Received '+CMGR' when expecting '%s' response to '%s', ignoring\n", PVT_ID(pvt),
				at_res2str (ecmd->res), at_cmd2str (ecmd->cmd));
	    }
	}
	else
	{
		ast_log (LOG_WARNING, "[%s] Received unexpected '+CMGR'\n", PVT_ID(pvt));
	}

	return 0;
}


/*!
 * \brief Handle +CMGR response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */





/*!
 * \brief Send an SMS message from the queue.
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

extern int svistok_bridge_upstream_at_response_sms_prompt(struct pvt * pvt);
                                     
                                         
  
                                               
  
               
  
                                                                                                               
                                                    
  
     
  
                                                                            
  

          
 

/*!
 * \brief Handle CUSD response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_cusd (struct pvt * pvt, char * str, size_t len)
{
	static const char * const types[] = {
		"USSD Notify",
		"USSD Request",
		"USSD Terminated by network",
		"Other local client has responded",
		"Operation not supported",
		"Network time out",
	};

	ssize_t		res;
	int		type;
	char*		cusd;
	int		dcs;
	char		cusd_utf8_str[1024];
	char		text_base64[16384];
	str_encoding_t	ussd_encoding;
	char		typebuf[2];
	const char*	typestr;

	manager_event_message("DongleNewCUSD", PVT_ID(pvt), str);

	if (at_parse_cusd (str, &type, &cusd, &dcs))
	{
		ast_verb (1, "[%s] Error parsing CUSD: '%.*s'\n", PVT_ID(pvt), (int) len, str);
		return -1;
	}

	if(type < 0 || type >= (int)ITEMS_OF(types))
	{
		ast_log (LOG_WARNING, "[%s] Unknown CUSD type: %d\n", PVT_ID(pvt), type);
	}

	typestr = enum2str(type, types, ITEMS_OF(types));

	typebuf[0] = type + '0';
	typebuf[1] = 0;

	// FIXME: strictly check USSD encoding and detect encoding
// || dcs == 1
	if ((dcs == 0 || dcs == 15 || dcs == 1) && !pvt->cusd_use_ucs2_decoding)
		ussd_encoding = STR_ENCODING_7BIT_HEX;
	else
		ussd_encoding = STR_ENCODING_UCS2_HEX;
	res = str_recode (RECODE_DECODE, ussd_encoding, cusd, strlen (cusd), cusd_utf8_str, sizeof (cusd_utf8_str));
	if(res >= 0)
	{
		cusd = cusd_utf8_str;
	}
/*	else
	{
		ast_log (LOG_ERROR, "[%s] Error decode CUSD: %s\n", PVT_ID(pvt), cusd);
		return -1;
	}*/

	ast_verb (1, "[%s] Got USSD type %d '%s': '%s'\n", PVT_ID(pvt), type, typestr, cusd);
	ast_base64encode (text_base64, (unsigned char*)cusd, strlen(cusd), sizeof(text_base64));

	// TODO: pass type
	manager_event_new_ussd(PVT_ID(pvt), cusd);
	manager_event_message("DongleNewUSSDBase64", PVT_ID(pvt), text_base64);

	{
		channel_var_t vars[] = 
		{
			{ "USSD_TYPE", typebuf },
			{ "USSD_TYPE_STR", ast_strdupa(typestr) },
			{ "USSD", cusd },
			{ "USSD_BASE64", text_base64 },

			{ "DONGLENAME", PVT_ID(pvt) },
			{ "DONGLEPROVIDER", pvt->provider_name },
			{ "DONGLEIMEI", pvt->imei },
			{ "DONGLEIMSI", pvt->imsi },
			{ "DONGLES",    pvt->serial },
			{ "DONGLENUMBER", pvt->subscriber_number },

			{ NULL, NULL },
		};
		start_local_channel(pvt, "ussd", "ussd", vars);
	}

	return 0;
}

/*!
 * \brief Handle +CPIN response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
   \ 50 wait
 */

static int at_response_cpin (struct pvt* pvt, char* str, size_t len)
{
	int rv = at_parse_cpin (str, len);

	ast_log (LOG_ERROR, "[%s] rv %d\n", PVT_ID(pvt), rv);

	switch(rv)
	{
		case 0:
			pvt->pinrequired=0;
			if(pvt->cfun==1)
			{
			    if(pvt->sim_start==0)
			    {
				pvt->sim_start=1;
				at_enque_initialization_sim (&pvt->sys_chan);

			    }
			    return rv; //AAA
			}
			if(pvt->cfun==5)
			{
				at_enque_sn (&pvt->sys_chan);
				at_enque_iccid (&pvt->sys_chan);
			}
			putfilei("dongles/state",PVT_ID(pvt),"pinrequired",pvt->pinrequired);
			break;
		case -1:
			ast_log (LOG_ERROR, "[%s] Error parsing +CPIN message: %s\n", PVT_ID(pvt), str);
			break;
		case 1:
			ast_log (LOG_ERROR, "Dongle %s needs PIN code!\n", PVT_ID(pvt));
			pvt->pinrequired=1;
			putfilei("dongles/state",PVT_ID(pvt),"pinrequired",pvt->pinrequired);
			at_enque_iccid (&pvt->sys_chan);

			return 50;
			break;
		case 2:
			ast_log (LOG_ERROR, "Dongle %s needs PUK code!\n", PVT_ID(pvt));
			pvt->pinrequired=2;
			putfilei("dongles/state",PVT_ID(pvt),"pinrequired",pvt->pinrequired);
			at_enque_iccid (&pvt->sys_chan);

			return 51;
			break;
	}
	return rv;
}

/*!
 * \brief Handle ^SMMEMFULL response This event notifies us, that the sms storage is full
 * \param pvt -- pvt structure
 * \retval  0 success
 * \retval -1 error
 */

extern int svistok_bridge_upstream_at_response_smmemfull(struct pvt * pvt);
                                         
          
 

/*!
 * \brief Handle +CSQ response Here we get the signal strength and bit error rate
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */



/*!
 * \brief Handle +CNUM response Here we get our own phone number
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

extern int svistok_bridge_upstream_at_response_cnum(struct pvt * pvt, char * str);
            

            
  
                                                                                    
                                    
                                  
           
  

                                                                                      
                                

           
 

/*!
 * \brief Handle +COPS response Here we get the GSM provider name
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */








/*!
 * \brief Handle +CREG response Here we get the GSM registration status
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

static int at_response_creg (struct pvt* pvt, char* str, size_t len)
{
	int	d;
	char*	lac;
	char*	ci;

        char dn[256];
	timenow(dn);

	if (at_enque_cops (&pvt->sys_chan))
	{
		ast_log (LOG_ERROR, "[%s] Error sending query for provider name\n", PVT_ID(pvt));
	}

//AAA
/*
	if (at_enque_spn (&pvt->sys_chan))
	{
		ast_log (LOG_ERROR, "[%s] Error sending query for provider name2\n", PVT_ID(pvt));
	}
*/

	putfileslog("dongles/state",PVT_ID(pvt),"laccell",str);

	if (at_parse_creg (str, len, &d, &pvt->gsm_reg_status, &lac, &ci))
	{
		ast_verb (1, "[%s] Error parsing CREG: '%.*s'\n", PVT_ID(pvt), (int) len, str);
		return 0;
	}



	if (d)
	{
//#ifdef ISSUE_CCWA_STATUS_CHECK
		/* only if gsm_registered 0 -> 1 ? */
//		if(!pvt->gsm_registered && CONF_SHARED(pvt, callwaiting) != CALL_WAITING_AUTO)
//			at_enque_set_ccwa(&pvt->sys_chan, 0, 0, CONF_SHARED(pvt, callwaiting));
//#endif
		pvt->gsm_registered = 1;
		manager_event_device_status(PVT_ID(pvt), "Register");
		putfilei("sim/state",pvt->imsi,"gsm_registered",1);
	}
	else
	{
		pvt->gsm_registered = 0;
		manager_event_device_status(PVT_ID(pvt), "Unregister");
		putfilei("sim/state",pvt->imsi,"gsm_registered",0);
	}

	if (lac)
	{
		ast_copy_string (pvt->location_area_code, lac, sizeof (pvt->location_area_code));
		putfiles("sim/state",pvt->imsi,"lac",pvt->location_area_code);
		putfiles("dongles/state",PVT_ID(pvt),"lac",pvt->location_area_code);

//		ast_verb(3,"%s %s %s",dn,pvt->location_area_code,pvt->cell_id);
//		putfileslog2("dongles/state",PVT_ID(pvt),"laccell","%s %s %s",dn,pvt->location_area_code,pvt->cell_id);
	}

	if (ci)
	{
		ast_copy_string (pvt->cell_id, ci, sizeof (pvt->cell_id));
		putfiles("sim/state",pvt->imsi,"cell",pvt->cell_id);
		putfiles("dongles/state",PVT_ID(pvt),"cell",pvt->cell_id);
//		putfileslog2("dongles/state",PVT_ID(pvt),"laccell","%s %s %s",dn,pvt->location_area_code,pvt->cell_id);
	}

	return 0;
}

/*!
 * \brief Handle AT+CGMI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */




/*!
 * \brief Handle AT+CGMM response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

#/* */
static int at_response_cgmm (struct pvt* pvt, const char* str)
{
	unsigned i;
	/* NOTE: in order of appears, replace with sorter and binary search */
	static const char * const seven_bit_modems[] = {
		"E1550",
		"E1750",
		"E160X",
		"E150",
		"E173",
		"E1552",
		"E171",
		"E153",
	};

	ast_copy_string (pvt->model, str, sizeof (pvt->model));

	writepvtstate(pvt);

/* 	putfiles("dongles/state",PVT_ID(pvt),"model",pvt->model);
	putfiles("dongles/state",PVT_ID(pvt),"data",PVT_STATE(pvt,data_tty));
	putfiles("dongles/state",PVT_ID(pvt),"audio",PVT_STATE(pvt,audio_tty));
	putfiles("dongles/state",PVT_ID(pvt),"dev",PVT_STATE(pvt,dev)); */

putfilei("dongles/state",PVT_ID(pvt),"rssi",-1);
putfilei("dongles/state",PVT_ID(pvt),"mode",-1);
putfilei("dongles/state",PVT_ID(pvt),"submode",-1);
	

	pvt->cusd_use_7bit_encoding = 0;
	pvt->cusd_use_ucs2_decoding = 1;
	for(i = 0; i < ITEMS_OF(seven_bit_modems); ++i)
	{
		if(!strcmp (pvt->model, seven_bit_modems[i]))
		{
			pvt->cusd_use_7bit_encoding = 1;
			pvt->cusd_use_ucs2_decoding = 0;
			break;
		}
	}
	return 0;
}













/*!
 * \brief Handle AT+CGMR response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */




/*!
 * \brief Handle AT+CGSN response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */

















/*!
 * \brief Handle AT+CIMI response
 * \param pvt -- pvt structure
 * \param str -- string containing response (null terminated)
 * \param len -- string lenght
 * \retval  0 success
 * \retval -1 error
 */




extern void svistok_bridge_upstream_at_response_busy(struct pvt * pvt, enum ast_control_frame_type control);
                                      
                                

                           
                               

         
  
                                              
                                        
  
 
















/*!
 * \brief Do response
 * \param pvt -- pvt structure
 * \param iovcnt -- number of elements array pvt->d_read_iov
 * \param at_res -- result type
 * \retval  0 success
 * \retval -1 error
   \ 50 - wait
 */

int at_response (struct pvt* pvt, const struct iovec iov[2], int iovcnt, at_res_t at_res)
{
	char*		str;
	size_t		len;
	const struct at_queue_cmd*	ecmd = at_queue_head_cmd(pvt);
	char dn[256];


	if(iov[0].iov_len + iov[1].iov_len > 0)
	{
		len = iov[0].iov_len + iov[1].iov_len - 1;

	    
		timenow(dn);

		at_log(pvt,dn,strlen(dn));
		at_log(pvt," << ",4);
		if (iovcnt == 2)
		{
			ast_debug (5, "[%s] iovcnt == 2\n", PVT_ID(pvt));

			str = alloca(len + 1);
			if (!str)
			{
				ast_debug (1, "[%s] buffer overflow\n", PVT_ID(pvt));
				return -1;
			}
			memcpy (str,                  iov[0].iov_base, iov[0].iov_len);
			memcpy (str + iov[0].iov_len, iov[1].iov_base, iov[1].iov_len);
		}
		else
		{
			str = iov[0].iov_base;
    	        }

		at_log(pvt,str,len);
		at_log(pvt,"\n",1);


		str[len] = '\0';

/*		ast_debug (5, "[%s] [%.*s]\n", PVT_ID(pvt), (int) len, str);
*/

		if(ecmd && ecmd->cmd == CMD_USER) {
			ast_verb(1, "[%s] Got Response for user's command:'%s'\n", PVT_ID(pvt), str);
			ast_log(LOG_NOTICE, "[%s] Got Response for user's command:'%s'\n", PVT_ID(pvt), str);
		}

		switch (at_res)
		{
			case RES_BOOT:
			case RES_CSSI:
			case RES_CSSU:

				return 0;

			case RES_SYSINFO:
				ast_verb(1, "[%s] GOT SYSINFO !!!! '%s'\n", PVT_ID(pvt), str);
				at_response_sysinfo (pvt, str, len);
				return 0;

			case RES_SRVST:
				ast_verb(1, "[%s] GOT SRVST !!!! '%s'\n", PVT_ID(pvt), str);
				at_response_srvst (pvt, str);
				return 0;
			case RES_SIMST:
				ast_verb(1, "[%s] GOT SIMST !!!! '%s'\n", PVT_ID(pvt), str);
				at_response_simst (pvt, str);
				return 0;
			case RES_CFUN_V:
				ast_verb(1, "[%s] GOT CFUN_V !!!! '%s'\n", PVT_ID(pvt), str);
				at_response_cfun_v(pvt, str);
				return 0;

			case RES_ICCID:
				ast_verb(1, "[%s] GOT ICCID '%s'\n", PVT_ID(pvt), str);
				at_response_iccid(pvt, str);
				return 0;

			case RES_SN:
				ast_verb(1, "[%s] GOT SN '%s'\n", PVT_ID(pvt), str);
				at_response_sn(pvt, str);
				return 0;

			case RES_CDS:
				ast_verb(1, "[%s] GOT CDS '%s'\n", PVT_ID(pvt), str);
				at_response_cds(pvt, str, len);
				return 0;


//			case RES_CVOICE:
			case RES_CMGS:
			case RES_CPMS:
				return 0;
			case RES_CONF:
				v_stat_call_response(pvt);
// ??? Why not			return at_response_conf (pvt, str);
				return 0;

			case RES_OK:
				at_response_ok (pvt, at_res);
				return 0;

			case RES_RSSI:
				/* An error here is not fatal. Just keep going. */
				at_response_rssi (pvt, str);
				break;
			case RES_DSFLOWRPT:
				/* An error here is not fatal. Just keep going. */
				at_response_dsflowrpt (pvt, str);
				break;

			case RES_MODE:
				/* An error here is not fatal. Just keep going. */
				at_response_mode (pvt, str, len);
				return 0;






			case RES_ORIG:
				return at_response_orig (pvt, str);

			case RES_CEND:
				return at_response_cend (pvt, str);

			case RES_CONN:
				return at_response_conn (pvt, str);

			case RES_CREG:
				/* An error here is not fatal. Just keep going. */
				at_response_creg (pvt, str, len);
				return 0;

			case RES_COPS:
				/* An error here is not fatal. Just keep going. */
				at_response_cops (pvt, str);
				return 0;

			case RES_SPN:
				/* An error here is not fatal. Just keep going. */
				at_response_spn (pvt, str);
				return 0;

			case RES_CSQ:
				/* An error here is not fatal. Just keep going. */
				at_response_csq (pvt, str);
				break;

			case RES_CMS_ERROR:
			case RES_ERROR:
				return at_response_error (pvt, at_res);

			case RES_RING:
				return at_response_ring (pvt);

			case RES_SMMEMFULL:
				return at_response_smmemfull (pvt);
/*
			case RES_CLIP:
				return at_response_clip (pvt, str, len);
*/
			case RES_CMTI:
				return at_response_cmti (pvt, str);

			case RES_CMGR:
				return at_response_cmgr (pvt, str, len);

			case RES_SMS_PROMPT:
				return at_response_sms_prompt (pvt);

			case RES_CUSD:
				/* An error here is not fatal. Just keep going. */
				at_response_cusd (pvt, str, len);
				break;
			case RES_CLCC:
				return at_response_clcc (pvt, str);

			case RES_CCWA:
				return at_response_ccwa (pvt, str);

			case RES_BUSY:
				ast_log (LOG_ERROR, "[%s] Receive BUSY\n", PVT_ID(pvt));
				at_response_busy(pvt, AST_CONTROL_BUSY);
				break;

			case RES_NO_DIALTONE:
				ast_log (LOG_ERROR, "[%s] Receive NO DIALTONE\n", PVT_ID(pvt));
				at_response_busy(pvt, AST_CONTROL_CONGESTION);
				break;
			case RES_NO_CARRIER:
				ast_log (LOG_ERROR, "[%s] Receive NO CARRIER\n", PVT_ID(pvt));
				at_response_busy(pvt, AST_CONTROL_CONGESTION);
				break;
			case RES_CPIN:
				/* fatal */
				//return at_response_cpin (pvt, str, len);
				at_response_cpin (pvt, str, len);
				return 0;
//				break;
			case RES_CNUM:
				/* An error here is not fatal. Just keep going. */
				at_response_cnum (pvt, str);
				return 0;

			case RES_CSCA:
				/* An error here is not fatal. Just keep going. */
				at_response_csca (pvt, str);
				return 0;

			case RES_PARSE_ERROR:
				ast_log (LOG_ERROR, "[%s] Error parsing result\n", PVT_ID(pvt));
				return -1;

			case RES_UNKNOWN:
				if (ecmd)
				{
					switch (ecmd->cmd)
					{

						case CMD_AT_CVOICE:
							ast_debug (1, "[%s] Got AT^CVOICE data\n", PVT_ID(pvt));
							return at_response_cvoice(pvt, str);

						case CMD_AT_CARDLOCK:
							ast_debug (1, "[%s] Got AT^CARDLOCK data\n", PVT_ID(pvt));
							return at_response_cardlock(pvt, str);

						case CMD_AT_CGMI:
							ast_debug (1, "[%s] Got AT_CGMI data (manufacturer info)\n", PVT_ID(pvt));
							return at_response_cgmi (pvt, str);

						case CMD_AT_CGMM:
							ast_debug (1, "[%s] Got AT_CGMM data (model info)\n", PVT_ID(pvt));
							return at_response_cgmm (pvt, str);

						case CMD_AT_CGMR:
							ast_debug (1, "[%s] Got AT+CGMR data (firmware info)\n", PVT_ID(pvt));
							return at_response_cgmr (pvt, str);

						case CMD_AT_CGSN:
							ast_debug (1, "[%s] Got AT+CGSN data (IMEI number)\n", PVT_ID(pvt));
							return at_response_cgsn (pvt, str);

						case CMD_AT_SN:
							ast_debug (1, "[%s] Got AT^SN data\n", PVT_ID(pvt));
							return at_response_sn (pvt, str);

						case CMD_AT_ICCID:
							ast_debug (1, "[%s] Got AT^ICCID data\n", PVT_ID(pvt));
							return at_response_iccid (pvt, str);

						case CMD_AT_CFUN_V:
							ast_debug (1, "[%s] Got AT+CFUN? data\n", PVT_ID(pvt));
							return at_response_cfun_v (pvt, str);


						case CMD_AT_FREQLOCK:
							ast_debug (1, "[%s] Got AT^FREQLOCK data\n", PVT_ID(pvt));
							return at_response_freqlock (pvt, str);


						case CMD_AT_CIMI:
							ast_debug (1, "[%s] Got AT+CIMI data (IMSI number)\n", PVT_ID(pvt));
							return at_response_cimi (pvt, str);
						default:
							break;
					}
				}
				/*
				if(at_response_unknown_simst (pvt, str))
				{
					ast_debug (1, "[%s] Parsed unknown result: '%.*s'\n", PVT_ID(pvt), (int) len, str);
					break;
				}*/
				ast_debug (1, "[%s] Ignoring unknown result: '%.*s'\n", PVT_ID(pvt), (int) len, str);
				break;
		}
	}

	return 0;
}
