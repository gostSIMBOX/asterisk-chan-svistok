/* Svistok-only composition fragment. */

EXPORT_DEF int at_enque_cfun1 (struct cpvt* cpvt)
{
        return at_enque_cmd_proc(cpvt, "AT+CFUN=1,1;+CFUN?");
}

EXPORT_DEF int at_enque_cfun5 (struct cpvt* cpvt)
{
        return at_enque_cmd_proc(cpvt, "AT+CFUN=5;+CFUN?");
}

EXPORT_DEF int at_enque_cfun6 (struct cpvt* cpvt)
{
        return at_enque_cmd_proc(cpvt, "AT+CFUN=6;+CFUN?");
}

EXPORT_DEF int at_enque_cfun_v (struct cpvt* cpvt)
{
        return at_enque_cmd_proc(cpvt, "AT+CFUN?");
}

EXPORT_DEF int at_enque_cmd_proc (struct cpvt* cpvt, const char * cmd)
{
	at_queue_cmd_t at_cmd = { CMD_USER, RES_OK,  ATQ_CMD_FLAG_DEFAULT, { ATQ_CMD_TIMEOUT_2S, 0} , NULL, 0 };
//	at_queue_cmd_t at_cmd = { CMD_AT_SMSTEXT, RES_OK,  ATQ_CMD_FLAG_DEFAULT, { ATQ_CMD_TIMEOUT_2S, 0} , NULL, 0 };

	at_cmd.length = strlen(cmd)+1;
	at_cmd.data = ast_malloc(at_cmd.length+1);
	memcpy(at_cmd.data, cmd, at_cmd.length-1);
	at_cmd.data[at_cmd.length-1]='\r';
	//at_cmd.data[at_cmd.length-1]=0;

	//ast_verb(3,"[%s] proc %s => %s (%d)\n",PVT_ID(cpvt->pvt),cmd, at_cmd.data,at_cmd);
	return at_queue_insert_const(cpvt, &at_cmd, 1, 0);


/* CORRECT
	at_queue_cmd_t at_cmd = { CMD_AT_SMSTEXT, RES_OK,  ATQ_CMD_FLAG_DEFAULT, { ATQ_CMD_TIMEOUT_2S, 0} , NULL, 0 };

	at_cmd.length = strlen(cmd)+2;
	at_cmd.data = ast_malloc(at_cmd.length);
	memcpy(at_cmd.data, cmd, at_cmd.length-2);
	at_cmd.data[at_cmd.length-2]='\r';
	at_cmd.data[at_cmd.length-1]=0;

	ast_verb(3,"[%s] proc %s => %s (%d)\n",PVT_ID(cpvt->pvt),cmd, at_cmd.data,at_cmd);
	return at_queue_insert_const(cpvt, &at_cmd, 1, 0);

*/

/*
	static at_queue_cmd_t at_cmd = ATQ_CMD_DECLARE_ST(CMD_USER, "");

        at_fill_generic_cmd(&at_cmd, "%s\r", cmd);
	return at_queue_insert_const(cpvt, &at_cmd, 1, 0);
*/

/*
	char * tmp[256];
	strcpy(tmp,cmd);
	strcat(tmp,"\r");

	ast_verb (3, "[%s] exec %s \n \n", PVT_ID(cpvt->pvt), tmp);
        at_write(cpvt->pvt,tmp,strlen(tmp)+1);*/
}

EXPORT_DEF int at_enque_cpin_v (struct cpvt* cpvt)
{
        return at_enque_cmd_proc(cpvt, "AT+CPIN?");
}

EXPORT_DEF int at_enque_iccid (struct cpvt* cpvt)
{
        return at_enque_cmd_proc(cpvt, "AT^ICCID?");
}

EXPORT_DEF int at_enque_initialization_modem(struct cpvt* cpvt)
{
	static const at_queue_cmd_t st_cmds1[] = {
		ATQ_CMD_DECLARE_ST(CMD_AT, cmd_at),
//!!!		ATQ_CMD_DECLARE_ST(CMD_AT_Z, cmd2),		/* optional,  reload configuration */
		ATQ_CMD_DECLARE_ST(CMD_AT_E, cmd3),		/* disable echo */

//!!!		ATQ_CMD_DECLARE_ST(CMD_AT, cmd69),		// HANGUP

//		ATQ_CMD_DECLARE_ST(CMD_AT_U2DIAG, cmd80),		/* optional, Enable or disable some devices */
		ATQ_CMD_DECLARE_ST(CMD_AT_CGMI, cmd5),		/* Getting manufacturer info */

		ATQ_CMD_DECLARE_ST(CMD_AT_CGMM, cmd7),		/* Get Product name */
		ATQ_CMD_DECLARE_ST(CMD_AT_CGMR, cmd8),		/* Get software version */
		ATQ_CMD_DECLARE_ST(CMD_AT_CMEE, cmd9),		/* set MS Error Report to 'ERROR' only  TODO: change to 1 or 2 and add support in response handlers */

//		ATQ_CMD_DECLARE_ST(CMD_AT_SN,   cmd99),		/* SN Read */

//		ATQ_CMD_DECLARE_ST(CMD_AT_CVOICE, cmd17),	/* read the current voice mode, and return sampling rate、data bit、frame period */
//		ATQ_CMD_DECLARE_ST(CMD_AT_CARDLOCK, cmd97),

		ATQ_CMD_DECLARE_ST(CMD_AT_CGSN, cmd10),		/* IMEI Read */ // Ne prochitalsya - rebutnut (vozmozhno posle pereproshivki)

//		ATQ_CMD_DECLARE_ST(CMD_AT_SYSINFO, cmd93),
		//ATQ_CMD_DECLARE_ST(CMD_AT_CPIN, cmd12),		/* check is password authentication requirement and the remainder validation times */

		ATQ_CMD_DECLARE_ST(CMD_AT_CFUN_V, cmd92)	/* check CFUN */

	};


	unsigned in, out;

	pvt_t * pvt = cpvt->pvt;
	at_queue_cmd_t cmds[ITEMS_OF(st_cmds1)];

	/* customize list */
	out=0;
	for(in = 0; in < ITEMS_OF(st_cmds1); in++)
	{

		memcpy(&cmds[out], &st_cmds1[in], sizeof(st_cmds1[in]));
		out++;
	}


	if(out > 0)
		return at_queue_insert(cpvt, cmds, out, 0);
	return 0;
}

EXPORT_DEF int at_enque_initialization_sim(struct cpvt* cpvt)
{
if(strstr(cpvt->pvt->model,"MULTIBAND")==NULL)
    at_enque_initialization_sim_e (cpvt);
else
    at_enque_initialization_sim_mb (cpvt);
}

EXPORT_DEF int at_enque_initialization_sim_e(struct cpvt* cpvt)
{
	static const at_queue_cmd_t st_cmds2[] = {

//		ATQ_CMD_DECLARE_ST(CMD_AT, cmd_at),
		ATQ_CMD_DECLARE_ST(CMD_AT_SN,   cmd99),		/* SN Read */
		ATQ_CMD_DECLARE_ST(CMD_AT_ICCID, cmd98),	/* ICCID Read */
		ATQ_CMD_DECLARE_ST(CMD_AT_SPN, cmd96),		/* Read operator from SIM */



		ATQ_CMD_DECLARE_ST(CMD_AT_CIMI, cmd11),		/* IMSI Read */

//		ATQ_CMD_DECLARE_ST(CMD_AT_CFUN_V, cmd92),	/* CFUN? Read */


		ATQ_CMD_DECLARE_ST(CMD_AT_FREQLOCK, cmd95),

		ATQ_CMD_DECLARE_ST(CMD_AT_COPS_INIT, cmd13),	/* Read operator name */
////

		ATQ_CMD_DECLARE_STI(CMD_AT_CREG_INIT,cmd14),	/* GSM registration status setting */
		ATQ_CMD_DECLARE_ST(CMD_AT_CREG, cmd15),		/* GSM registration status */
		ATQ_CMD_DECLARE_ST(CMD_AT_CNUM, cmd16),		/* Get Subscriber number */


//		ATQ_CMD_DECLARE_ST(CMD_AT_CSCA, cmd6),		/* Get SMS Service center address */
//		ATQ_CMD_DECLARE_ST(CMD_AT_CLIP, cmd18),		/* disable  Calling line identification presentation in unsolicited response +CLIP: <number>,<type>[,<subaddr>,<satype>[,[<alpha>][,<CLI validitity>]] */
		ATQ_CMD_DECLARE_ST(CMD_AT_CSSN, cmd19),		/* activate Supplementary Service Notification with CSSI and CSSU */
		ATQ_CMD_DECLARE_ST(CMD_AT_CMGF, cmd81),		/* Set Message Format */

		ATQ_CMD_DECLARE_STI(CMD_AT_CSCS, cmd21),	/* UCS-2 text encoding */

//		ATQ_CMD_DECLARE_ST(CMD_AT_CPMS, cmd22),		/* SMS Storage Selection */
			/* pvt->initialized = 1 after successful of CMD_AT_CNMI */
		ATQ_CMD_DECLARE_ST(CMD_AT_CNMI, cmd23),		/* New SMS Notification Setting +CNMI=[<mode>[,<mt>[,<bm>[,<ds>[,<bfr>]]]]] */

//		ATQ_CMD_DECLARE_ST(CMD_AT_SYSINFO, cmd93),

//		ATQ_CMD_DECLARE_ST(CMD_AT_CCWA_SET, cmd70),

		ATQ_CMD_DECLARE_ST(CMD_AT_CSQ, cmd24),		/* Query Signal quality */
		ATQ_CMD_DECLARE_ST(CMD_AT_CSNR, cmd93),
		};

	unsigned in, out;
	pvt_t * pvt = cpvt->pvt;

	at_queue_cmd_t cmds[ITEMS_OF(st_cmds2)];

	/* customize list */
	out=0;
	for(in = 0; in < ITEMS_OF(st_cmds2); in++)
	{
		memcpy(&cmds[out], &st_cmds2[in], sizeof(st_cmds2[in]));
		out++;
	}



	if(out > 0)
		return at_queue_insert(cpvt, cmds, out, 0);
	return 0;
}

EXPORT_DEF int at_enque_initialization_sim_mb(struct cpvt* cpvt)
{
	static const at_queue_cmd_t st_cmds2[] = {

//		ATQ_CMD_DECLARE_ST(CMD_AT, cmd_at),
//		ATQ_CMD_DECLARE_ST(CMD_AT_SN,   cmd99),		/* SN Read */
//		ATQ_CMD_DECLARE_ST(CMD_AT_ICCID, cmd98),	/* ICCID Read */
//		ATQ_CMD_DECLARE_ST(CMD_AT_SPN, cmd96),		/* Read operator from SIM */



		ATQ_CMD_DECLARE_ST(CMD_AT_CIMI, cmd11),		/* IMSI Read */

//		ATQ_CMD_DECLARE_ST(CMD_AT_CFUN_V, cmd92),	/* CFUN? Read */


// 		ATQ_CMD_DECLARE_ST(CMD_AT_FREQLOCK, cmd95),

///		ATQ_CMD_DECLARE_ST(CMD_AT_COPS_INIT, cmd13),	/* Read operator name */


		ATQ_CMD_DECLARE_STI(CMD_AT_CREG_INIT,cmd14),	/* GSM registration status setting */
		ATQ_CMD_DECLARE_ST(CMD_AT_CREG, cmd15),		/* GSM registration status */
//		ATQ_CMD_DECLARE_ST(CMD_AT_CNUM, cmd16),		/* Get Subscriber number */


//		ATQ_CMD_DECLARE_ST(CMD_AT_CSCA, cmd6),		/* Get SMS Service center address */
//		ATQ_CMD_DECLARE_ST(CMD_AT_CLIP, cmd18),		/* disable  Calling line identification presentation in unsolicited response +CLIP: <number>,<type>[,<subaddr>,<satype>[,[<alpha>][,<CLI validitity>]] */
		ATQ_CMD_DECLARE_ST(CMD_AT_CSSN, cmd19),		/* activate Supplementary Service Notification with CSSI and CSSU */
		ATQ_CMD_DECLARE_ST(CMD_AT_CMGF, cmd81),		/* Set Message Format */

//		ATQ_CMD_DECLARE_STI(CMD_AT_CSCS, cmd21),	/* UCS-2 text encoding */

//		ATQ_CMD_DECLARE_ST(CMD_AT_CPMS, cmd22),		/* SMS Storage Selection */
			/* pvt->initialized = 1 after successful of CMD_AT_CNMI */
		ATQ_CMD_DECLARE_ST(CMD_AT_CNMI, cmd23),		/* New SMS Notification Setting +CNMI=[<mode>[,<mt>[,<bm>[,<ds>[,<bfr>]]]]] */

//		ATQ_CMD_DECLARE_ST(CMD_AT_SYSINFO, cmd93),

//		ATQ_CMD_DECLARE_ST(CMD_AT_CCWA_SET, cmd70),

		ATQ_CMD_DECLARE_ST(CMD_AT_CSQ, cmd24),		/* Query Signal quality */
		ATQ_CMD_DECLARE_ST(CMD_AT_CSNR, cmd93),
		};

	unsigned in, out;
	pvt_t * pvt = cpvt->pvt;

	at_queue_cmd_t cmds[ITEMS_OF(st_cmds2)];

	/* customize list */
	out=0;
	for(in = 0; in < ITEMS_OF(st_cmds2); in++)
	{
		memcpy(&cmds[out], &st_cmds2[in], sizeof(st_cmds2[in]));
		out++;
	}



	if(out > 0)
		return at_queue_insert(cpvt, cmds, out, 0);
	return 0;
}

EXPORT_DEF int at_enque_sn (struct cpvt* cpvt)
{
        return at_enque_cmd_proc(cpvt, "AT^SN");
}

EXPORT_DEF int at_enque_spn (struct cpvt* cpvt)
{
/*
	static const char cmd[] = "AT^SPN=0\r";
	static at_queue_cmd_t at_cmd = ATQ_CMD_DECLARE_ST(CMD_AT_SPN, cmd);
	return at_queue_insert_const(cpvt, &at_cmd, 1, 0);
*/
        return at_enque_cmd_proc(cpvt, "AT^SPN=0");
}

EXPORT_DEF int at_enque_sysinfo (struct cpvt* cpvt)
{
	return;
	//if(strstr(cpvt->pvt->model,"MULTIBAND")!=NULL)	return;
        //return at_enque_cmd_proc(cpvt, "AT^SYSINFO");
}
