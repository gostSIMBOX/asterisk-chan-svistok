/* Svistok-only composition fragment. */

static int at_response_cardlock (struct pvt* pvt, const char* str)
{
	char buf[256];
	strcpy(buf,str);
	char cl;
	cl=buf[11];
	buf[11]=0;

	pvt->cardlock = 0;

	
	if((strcmp(buf,"^CARDLOCK: ")==0)&&(cl=='1'))
	{
		pvt->cardlock = 1;
		
		ast_verb(3,"LOCK CARDLOCKRESPONSE=%s cardlock=%d \n",str,pvt->cardlock);
		
		return 50;
	}

	ast_verb(3,"CARDLOCKRESPONSE=%s cl=%c cardlock=%d\n",str,cl,pvt->cardlock);

	return 0;
}

static int at_response_cds (struct pvt* pvt, const char * str, size_t len)
{
/*    char *cds;
    *cds=memchr(str,"\r",len);
    if(cds)
    {
	cds=cds+1;
	ast_verb(3,"[%s] CDS: %s\n",PVT_ID(pvt),cds);
	
    }*/

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



	//const struct at_queue_cmd * ecmd = at_queue_head_cmd (pvt);
	//manager_event_message("DongleNewCMGR", PVT_ID(pvt), str);
	//if (ecmd)
	//{
	//    if (ecmd->res == RES_CMGR || ecmd->cmd == CMD_USER)
	//    {
	//	at_queue_handle_result (pvt, RES_CMGR);
	//	pvt->incoming_sms = 0;
	//	pvt_try_restate(pvt);

		cmgr = err_pos = ast_strdupa (str);
		err = at_parse_cds (&err_pos, len, oa, sizeof(oa), &oa_enc, &msg, &msg_enc);

/*
	FILE * fp;
	fp=fopen("/var/log/cds.log","a");
	if(fp)
	{
	    fprintf(fp,"%s\n",err_pos);
	    fclose(fp);
	}*/

		if (err)
		{
			ast_log (LOG_WARNING, "[%s] Error parsing incoming CDS message '%s' at possition %d: %s\n", PVT_ID(pvt), str, (int)(err_pos - cmgr), err);
			return 0;
		}

		ast_debug (1, "[%s] Successfully read CDS message\n", PVT_ID(pvt));

		/* last chance to define encodings */
		if (oa_enc == STR_ENCODING_UNKNOWN)
			oa_enc = pvt->use_ucs2_encoding ? STR_ENCODING_UCS2_HEX : STR_ENCODING_7BIT;

		if (msg_enc == STR_ENCODING_UNKNOWN)
			msg_enc = pvt->use_ucs2_encoding ? STR_ENCODING_UCS2_HEX : STR_ENCODING_7BIT;

		/* decode number and message */
		res = str_recode (RECODE_DECODE, oa_enc, oa, strlen(oa), from_number_utf8_str, sizeof (from_number_utf8_str));
		if (res < 0)
		{
			ast_log (LOG_ERROR, "[%s] Error decode CDS originator address: '%s', message is '%s'\n", PVT_ID(pvt), oa, str);
			number = oa;
			return 0;
		}
		else
			number = from_number_utf8_str;

		msg_len = strlen(msg);
		res = str_recode (RECODE_DECODE, msg_enc, msg, msg_len, sms_utf8_str, sizeof (sms_utf8_str));
		if (res < 0)
		{
			ast_log (LOG_ERROR, "[%s] Error decode CDS text '%s' from encoding %d, message is '%s'\n", PVT_ID(pvt), msg, msg_enc, str);
			return 0;
		}
		else
		{
			msg = sms_utf8_str;
			msg_len = res;
		}

		ast_verb (1, "[%s] Got CDS from %s: '%s'\n", PVT_ID(pvt), number, msg);


		{
			channel_var_t vars[] = 
			{
				{ "CDS", msg } ,
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
			start_local_channel (pvt, "cds", number, vars);
		}


/*		ast_base64encode (text_base64, (unsigned char*)msg, msg_len, sizeof(text_base64));

		manager_event_new_sms(PVT_ID(pvt), number, msg);
		manager_event_new_sms_base64(PVT_ID(pvt), number, text_base64);
		{
			channel_var_t vars[] = 
			{
				{ "SMS", msg } ,
				{ "SMS_BASE64", text_base64 },
				{ "CMGR", (char *)str },
				{ NULL, NULL },
			};
			start_local_channel (pvt, "sms", number, vars);
		}
*/
//	    }

/*
	    else
	    {
		ast_log (LOG_ERROR, "[%s] Received '+CMGR' when expecting '%s' response to '%s', ignoring\n", PVT_ID(pvt),
				at_res2str (ecmd->res), at_cmd2str (ecmd->cmd));
	    }
	}
	else
	{
		ast_log (LOG_WARNING, "[%s] Received unexpected '+CMGR'\n", PVT_ID(pvt));
	}*/

	return 0;
}

static int at_response_cfun_v (struct pvt* pvt, const char* str)
{
    ast_verb(3,"[%s]CFUN str:%s\n",PVT_ID(pvt),str);



    if ((str[0]=='+')&&(str[1]=='C')&&(str[2]=='F')&&(str[3]=='U')&&(str[4]=='N')&&(str[5]==':')&&(str[6]==' '))
    {
     if((str[7]>='0')&&(str[7]<='9'))
     {
	putfiles("dongles/state",PVT_ID(pvt),"cfun",str+7);
	pvt->cfun=str[7]-'0';
	ast_verb(3,"parsed CFUN=%d\n",pvt->cfun);
	if (pvt->cfun==5)
	{
		if((pvt->simst==4)||(pvt->simst==3)||(pvt->simst==1)) //||(pvt->simst==0)
		{
		    
		    if(pvt->sim_ready==0)
		    {
			pvt->sim_ready=1;
			//at_enque_cpin_v (&pvt->sys_chan);


		    }
		}

	    return 0;
	} else if (pvt->cfun==1)
	{
		if(pvt->simst==255)
		{ 
			ast_verb (3, "[%s] No SIM, but not offline sending cfun5 if nosim2offline(=%d)==1 \n", PVT_ID(pvt),nosim2offline);
			if (nosim2offline==1)	at_enque_cfun5 (&pvt->sys_chan);
			pvt->nosim=1;
		} else if((pvt->simst==4)||(pvt->simst==3)||(pvt->simst==1))
		{
		    
		    if(pvt->sim_ready==0)
		    {
			pvt->sim_ready=1;
			at_enque_cpin_v (&pvt->sys_chan);
		    }
		}

///AAA
			    if(pvt->sim_start==0)
			    {
				pvt->sim_start=1;
				at_enque_initialization_sim (&pvt->sys_chan);
			    }


	    return 0;
	} else if (pvt->cfun==4)
	{
	    ast_verb(3,"[%s] CFUN=4!!!!!!\n",PVT_ID(pvt));
	    at_enque_cfun6 (&pvt->sys_chan);
	    return 0;

	} else {
	    ast_verb(3,"UNKNOWN CFUN str:%s\n",str);
	}
     } else {
	    ast_verb(3,"CFUN NOT digit %d",str[7]);
	    }
    } else {
	    ast_verb(3,"NOT CFUN str:%s",str);
    }
	return 0;
}

static int at_response_cvoice (struct pvt* pvt, const char* str)
{
	//ast_copy_string (pvt->firmware, str, sizeof (pvt->firmware));
	//putfiles("dongles",PVT_ID(pvt),"firmware",pvt->firmware);

	pvt->novoice = 0;
	pvt->has_voice = 1;



	if(strcmp(str,"^CVOICE:0,8000,16,20")!=0)
	{
		pvt->novoice = 1;
		pvt->has_voice = 0;
		
		ast_verb(3,"NO VOICE CVOICERESPONSE=%s novoice=%d has_voice=%d\n",str,pvt->novoice,pvt->has_voice);
		
		return 50;
	}

	ast_verb(3,"CVOICERESPONSE=%s novoice=%d has_voice=%d\n",str,pvt->novoice,pvt->has_voice);

	return 0;
}

static int at_response_dsflowrpt (struct pvt* pvt, const char* str)
{
	//int rssi = at_parse_dsflowrpt (str);
	/*
	if (rssi == -1)
	{
		ast_debug (2, "[%s] Error parsing RSSI event '%s'\n", PVT_ID(pvt), str);
		return -1;
	}

	pvt->rssi = rssi;
	*/

	//char dn[256];
	//timenow(dn);

	putfilel("dongles/state",PVT_ID(pvt),"dsflowrpt.time",(long)time(NULL));
	return 0;
}

static int at_response_freqlock (struct pvt* pvt, const char* str)
{
	char buf[256];
	strcpy(buf,str);
	char cl;
	cl=buf[11];
	buf[11]=0;

	pvt->freqlock = -1;
	if((strcmp(buf,"^FREQLOCK: ")==0)&&(cl=='1'))
	{
		pvt->freqlock = atoi(buf+12);
	} else if((strcmp(buf,"^FREQLOCK: ")==0)&&(cl=='0'))
	{
		pvt->freqlock = 0;
	}

	ast_verb(3,"FREQLOCK RESPONSE=%s freq=%d \n",str,pvt->freqlock);
	putfilei("dongles/state",PVT_ID(pvt),"freqlock",pvt->freqlock);

	return 0;
}

static int at_response_iccid (struct pvt* pvt, const char* str)
{
	pvt->nosim=0;
	pvt->eerror=0;

    ast_verb(3,"ICCID:%s\n",str);

if ((str[0]=='^')&&(str[1]=='I')&&(str[2]=='C')&&(str[3]=='C')&&(str[4]=='I')&&(str[5]=='D')&&(str[6]==':')&&(str[7]==' '))
{
	ast_copy_string (pvt->iccid, str+8, sizeof (pvt->iccid));

	putfiles("dongles/state",PVT_ID(pvt),"iccid",pvt->iccid);
} else if(strstr(str,"SIM not inserted"))
{
	pvt->nosim=1;
	return -1;
}

	return 0;
}

static int at_response_simst (struct pvt* pvt, const char* str)
{
    // ^SIMST:255
    int tmp=0;

    if ((str[0]=='^')&&(str[1]=='S')&&(str[2]=='I')&&(str[3]=='M')&&(str[4]=='S')&&(str[5]=='T')&&(str[6]==':'))
    {

	ast_verb(3,"Found SIMST %s\n",str+7);
	tmp=0;
	if(str[7]=='1') tmp=1;
	if((str[7]=='2')&&(str[8]=='5')&&(str[9]=='5')) tmp=255;
//	tmp=atoi(str+7);
	ast_verb(3,"atoi=%d\n",tmp);
	at_enque_sysinfo (&pvt->sys_chan);
//	at_enque_cfun_v (&pvt->sys_chan);

	return 1;
    }

    return 0;
}

static int at_response_sn (struct pvt* pvt, const char* str)
{
if ((str[0]=='^')&&(str[1]=='S')&&(str[2]=='N')&&(str[3]==':')&&(str[4]==' '))
{
	ast_copy_string (pvt->serial, str+5, sizeof (pvt->serial));

	dserial_changename(pvt);

/*
	putfiles("dongles/state",PVT_ID(pvt),"imsi",pvt->imsi);
	putfiles("dongles/state",PVT_ID(pvt),"imei",pvt->imei);
	putfiles("dongles/state",PVT_ID(pvt),"serial",pvt->serial);
	putfiles("dongles/state",PVT_ID(pvt),"iccid",pvt->iccid);

	putfiles("dongles/state",PVT_ID(pvt),"model",pvt->model);
	putfiles("dongles/state",PVT_ID(pvt),"firmware",pvt->firmware);
	putfiles("dongles/state",PVT_ID(pvt),"audio",PVT_STATE(pvt,audio_tty));
	putfiles("dongles/state",PVT_ID(pvt),"data",PVT_STATE(pvt,data_tty));
	putfiles("dongles/state",PVT_ID(pvt),"dev",PVT_STATE(pvt,dev));
*/

	writepvtstate(pvt);
}
	return 0;
}

static int at_response_spn (struct pvt* pvt, char* str)
{
	char* provider_name2 = at_parse_spn (str);

	if (provider_name2)
	{
		ast_copy_string (pvt->provider_name2, provider_name2, sizeof (pvt->provider_name2));
		putfiles("sim/state",pvt->imsi,"provider_name2",pvt->provider_name2);
		putfiles("dongles/state",PVT_ID(pvt),"operator2",pvt->provider_name2);
		return 0;
	}

	ast_copy_string (pvt->provider_name2, "", sizeof (pvt->provider_name2));
	putfiles("sim/state",pvt->imsi,"provider_name2",pvt->provider_name2);
	putfiles("dongles/state",PVT_ID(pvt),"operator2",pvt->provider_name2);

	return -1;
}

static int at_response_srvst (struct pvt* pvt, const char* str)
{
    // ^SRVST:1
    int tmp=0;

    if ((str[0]=='^')&&(str[1]=='S')&&(str[2]=='R')&&(str[3]=='V')&&(str[4]=='S')&&(str[5]=='T')&&(str[6]==':'))
    {

	ast_verb(3,"Found SRVST %s\n",str+7);
	at_enque_sysinfo (&pvt->sys_chan);
//	at_enque_cfun_v (&pvt->sys_chan);
	return 1;
    }

    return 0;
}

static int at_response_sysinfo (struct pvt* pvt, char* str, size_t len)
{
    int srvst;
    int srvd;
    int roamst;
    int sysmode;
    int simst;

	ast_verb (2, "[%s] BEFORE parsing SYSINFO event '%.*s'\n", PVT_ID(pvt), (int) len, str);

	int rv = at_parse_sysinfo (str, &srvst, &srvd, &roamst, &sysmode, &simst);
	if(rv)
	{
		ast_verb (2, "[%s] Error parsing SYSINFO event '%.*s'\n", PVT_ID(pvt), (int) len, str);
	}
	else
	{
    		ast_verb (2, "[%s] SYSINFO parsed: %d,%d,%d,%d,simst=%d\n", PVT_ID(pvt), srvst,srvd,roamst,sysmode,simst);

		pvt->srvst = srvst;
		pvt->simst = simst;

		putfilei("dongles/state",PVT_ID(pvt),"srvst",srvst);
		putfilei("dongles/state",PVT_ID(pvt),"simst",simst);


	}

	ast_verb (2, "[%s] at_enque_cfun_v SYSINFO event '%.*s'\n", PVT_ID(pvt), (int) len, str);

	at_enque_cfun_v (&pvt->sys_chan);

	return rv;
}

static int at_response_unknown (struct pvt* pvt, const char* str)
{
    return 0;
}
