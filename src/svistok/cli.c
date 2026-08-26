/* Svistok-only composition fragment. */

static char* cli_changeimei (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	const char * msg;
	int fd;

        struct pvt * pvt;

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"dongle changeimei";
			e->usage   =	"Usage: dongle diagmode <device> <num>\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->word, a->n);
			}
			return NULL;
	}

	if (a->argc != 4)
	{
		return CLI_SHOWUSAGE;
	}

	

	pvt = find_device (a->argv[2]);
	if (pvt)
	{
		//pvt->changeimei=1;
		strcpy(pvt->newimei,a->argv[3]);

		ast_verb (3, "[%s] (instant) Changing imei on fd=%d, imei=%s\n", PVT_ID(pvt),pvt->audio_fd,pvt->newimei);
		ttyprog_changeimei(pvt->audio_fd,pvt->newimei);
//		disconnect_dongle(pvt);
		ast_mutex_unlock_pvt(pvt);
		ast_verb (3, "[%s] (instant) Changing imei OK\n", PVT_ID(pvt));
		ast_cli (a->fd, "[%s] (instant) Imei changed\nPlease restart\n", a->argv[2]);

		//ast_mutex_unlock_pvt (pvt);
		//ast_cli (a->fd, "[%s] Queued changeimei\nPlease restart\n", a->argv[2]);
	}
	else
	{
		ast_cli (a->fd, "Device %s not found\n", a->argv[2]);
	}

	return CLI_SUCCESS;
}

static char* cli_diagmode (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	const char * msg;
	int fd;

        struct pvt * pvt;

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"dongle diagmode";
			e->usage   =	"Usage: dongle diagmode <device>\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->word, a->n);
			}
			return NULL;
	}

	if (a->argc != 3)
	{
		return CLI_SHOWUSAGE;
	}

	

	pvt = find_device (a->argv[2]);
	if (pvt)
	{
		pvt->diagmode=1;
		ast_mutex_unlock_pvt (pvt);
		ast_cli (a->fd, "[%s] Queued Diag Mode\nPlease remove sim\n", a->argv[2]);
	}
	else
	{
		ast_cli (a->fd, "Device %s not found\n", a->argv[2]);
	}

	return CLI_SUCCESS;
}

static char* cli_dongle_update(struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	const char * msg;
	int status;
	void * msgid;

	switch (cmd)
	{
		case CLI_INIT:
			e->command = "dongle update";
			e->usage =
				"Usage: dongle update\n"
				"       update info.\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->word, a->n);
			}
			return NULL;
	}

	if (a->argc != 2)
	{
		return CLI_SHOWUSAGE;
	}


        struct pvt* pvt;
        
//        AST_RWLIST_RDLOCK (&gpublic->devices);
	AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
	{
		ast_verb(3,"readpvtinfo-- %s\n",PVT_ID(pvt));
		readpvtinfo(pvt);
		readpvtlimits(pvt);
	}
//	AST_RWLIST_UNLOCK (&gpublic->devices);
		ast_verb(3,"readpvtinfo-- OK %s\n","OK");

	make_dongles_imsi_list();

	return CLI_SUCCESS;
}

static char* cli_setgroup (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	const char * msg;
	int status;
	void * msgid;

	switch (cmd)
	{
		case CLI_INIT:
			e->command = "dongle setgroup";
			e->usage =
				"Usage: dongle setgroup <device> <group>\n"
				"       Set <group> to the dongle\n"
				"       with the specified <device>.\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->word, a->n);
			}
			return NULL;
	}

	if (a->argc != 4)
	{
		return CLI_SHOWUSAGE;
	}


        struct pvt* pvt;
/*        FILE * pFile;
        char filename[64]="/var/log/asterisk/sim/";*/
        
        
        pvt = find_device_ext(a->argv[2], &msg);
	if(pvt)
	{
		CONF_SHARED(pvt,group) = (int) strtol (a->argv[3], (char**) NULL, 10);
		
		/*Записываем status*/
		putfilei("sim/settings",pvt->imsi,"group",CONF_SHARED(pvt,group));

		putfileilog("sim/log",pvt->imsi,"setgroup",CONF_SHARED(pvt,group));
/*
		putgetfilei('w',"sim",pvt->imsi,"group",CONF_SHARED(pvt,group),a);
		
		strcat(filename,pvt->imsi);
		strcat(filename,".status");
		pFile=fopen(filename,"w");
		if (pFile!=NULL)
		{
			fprintf(pFile,"%d",CONF_SHARED(pvt,group));
			fclose(pFile);
		}*/
		
		pvt->selectbusy=0;
		//ast_mutex_unlock_pvt (pvt);

		//ast_cli (a->fd, "[%s]  %s\n", a->argv[2], pvt->imei);
		ast_cli (a->fd, "[%s] group = %s\n", a->argv[2], a->argv[3]);
	} else {
		ast_cli (a->fd, "[%s]Error!!!\n %s \n",a->argv[2], msg);
	}
	//#else
	//#	ast_cli (a->fd, "[%s] %s\n", a->argv[2], msg);

	return CLI_SUCCESS;
}

static char* cli_setgroupimsi (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	const char * msg;
	int status;
	void * msgid;

	switch (cmd)
	{
		case CLI_INIT:
			e->command = "dongle setgroupimsi";
			e->usage =
				"Usage: dongle setgroupimsi <imsi> <group>\n"
				"       Set <group> to the imsi\n"
				"       with the specified <imsi>.\n";
			return NULL;

		case CLI_GENERATE:
			if (a->pos == 2)
			{
				return complete_device (a->word, a->n);
			}
			return NULL;
	}

	if (a->argc != 4)
	{
		return CLI_SHOWUSAGE;
	}


        struct pvt* pvt;
        struct pvt* found=0;
/*        FILE * pFile;
        char filename[64]="/var/log/asterisk/sim/";*/
        
        
        
	AST_RWLIST_TRAVERSE(&gpublic->devices, pvt, entry)
	{
		/*ast_cli (a->fd, "[%s] %s %s\n", PVT_ID(pvt),pvt->imsi,a->argv[2]);*/
		if (!strcmp(pvt->imsi, a->argv[2]))
		{
			ast_cli (a->fd, "FOUND [%s]\n", PVT_ID(pvt));

			found = pvt;

			break;
		}
	}

	pvt=found;

	if(pvt)
	{
		CONF_SHARED(pvt,group) = (int) strtol (a->argv[3], (char**) NULL, 10);
		
		
		/*Записываем status*/
		
		putfilei("sim/settings",pvt->imsi,"group",CONF_SHARED(pvt,group));

		/*
		strcat(filename,pvt->imsi);
		strcat(filename,".status");
		pFile=fopen(filename,"w");
		if (pFile!=NULL)
		{
			fprintf(pFile,"%d",CONF_SHARED(pvt,group));
			fclose(pFile);
		}*/
		
		ast_mutex_unlock_pvt (pvt); //LOCK//!!!
		
		ast_cli (a->fd, "[%s] group = %s\n", PVT_ID(pvt),a->argv[3]);
	} else {
		ast_cli (a->fd, "[%s]Error!!!\n %s \n",a->argv[2], msg);
	}
	//#else
	//#	ast_cli (a->fd, "[%s] %s\n", a->argv[2], msg);

	return CLI_SUCCESS;
}

static char* cli_show_devicesd (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	struct pvt* pvt;


//FILE * pFile;
//char filename[64]="/var/log/asterisk/sim/";
//char balance[64]="  ";

//char dfilename[64]="/var/log/asterisk/sim/";
//char dbalance[64]="   ";

float pdd,pdd1,pdd2;

#define FORMAT1 "%-10.10s %-16.16s %-5.5s %-3.3s %-3.3s %4.4s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s\n"
#define FORMAT2 "%-10.10s %-16.16s %-11.11s %5s %5d %-3.3s %-3.3s %4d %5d %6d %9.2f %9.2f %9.2f %9.2f %5.1f %5.1f %5.1f %5.1f %4d/%4d/%4d\n"

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"dongle show devicesd";
			e->usage   =	"Usage: dongle show devicesd\n"
					"       Shows the state of Dongle devices.\n";
			return NULL;

		case CLI_GENERATE:
			return NULL;
	}

	if (a->argc != 3)
	{
		return CLI_SHOWUSAGE;
	}

	ast_cli (a->fd, "DONGLE    IMSI              Number----- Group Sta-Dif Prv DATT TOTAL ANSWER MINUTES_T MINUTES_W ACD_TOTAL ----ACD_L -ASRL PDDAS PDDL0 PDDL1 BALAN ERR0/ERR1/ERR2\n");

	AST_RWLIST_RDLOCK (&gpublic->devices);
	AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
	{





		//LOCK// ast_mutex_lock (&pvt->lock);
		readpvtinfo(pvt);
		readpvterrors(pvt);
		
		pdd=((float)getACD(PVT_STAT(pvt, stat_out_calls[1]),PVT_STAT(pvt, stat_wait_duration[1])));
		pdd1=(float)(((float)PVT_STAT(pvt, stat_pddl[0][1]))/1000);
		pdd2=(float)(((float)PVT_STAT(pvt, stat_pddl[1][1]))/1000);

		
		ast_cli (a->fd, FORMAT2,
			PVT_ID(pvt),
			pvt->imsi,
			PVT_STAT(pvt, number),
			PVT_STAT(pvt, balance),
			CONF_SHARED(pvt, group),
			pvt_str_state(pvt),
			pvt->provider_name,
			PVT_STAT(pvt, stat_datt[1]),
			PVT_STAT(pvt, stat_out_calls[1]),
			PVT_STAT(pvt, stat_calls_answered[1]),
			((float)PVT_STAT(pvt, stat_calls_duration[1]))/60,
			((float)PVT_STAT(pvt, stat_wait_duration[1]))/60,
			((float)getACD(PVT_STAT(pvt, stat_calls_answered[1]), PVT_STAT(pvt, stat_calls_duration[1])))/60,
			((float)PVT_STAT(pvt, stat_acdl[1]))/60,
			((float)PVT_STAT(pvt, stat_asrl[1]))/1000,
			(float)pdd,
			(float)pdd1,
			(float)pdd2,
			PVT_STAT(pvt, stat_errors[0]),
			PVT_STAT(pvt, stat_errors[1]),
			PVT_STAT(pvt, stat_errors[2])
			
		);
		//LOCK// ast_mutex_unlock (&pvt->lock);
	}
	AST_RWLIST_UNLOCK (&gpublic->devices);

	ast_cli (a->fd, "TOTAL                                                 %4d              MINUTES_T MINUTES_W ACD_TOTAL %9.2f       %5.1f %5.1f                     \n",
			total_stat_datt,
			((float)total_stat_acdl)/60,
			((float)total_stat_pddl[0])/1000,
			((float)total_stat_pddl[1])/1000
		);


#undef FORMAT1
#undef FORMAT2

	return CLI_SUCCESS;
}

static char* cli_show_devicesi (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	struct pvt* pvt;


//FILE * pFile;
//char filename[64]="/var/log/asterisk/sim/";
//char balance[64]="  ";

//char dfilename[64]="/var/log/asterisk/sim/";
//char dbalance[64]="   ";

float pdd;
int diff_end;

#define FORMAT1 "%-10.10s %-16.16s %-5.5s %-3.3s %-3.3s %4.4s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s\n"
#define FORMAT2 "%-10.10s %-16.16s %-11.11s %5d %-3.3s %-3.3s %4d %4d-%4d %5d %6d:%6d %9.2f:%9.2f %9.2f %9.2f %9.2f %5.1f %5.1f %5.1f %5.1f %5s\n"

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"dongle show devicesi";
			e->usage   =	"Usage: dongle show devicesi\n"
					"       Shows the state of Dongle devices.\n";
			return NULL;

		case CLI_GENERATE:
			return NULL;
	}

	if (a->argc != 3)
	{
		return CLI_SHOWUSAGE;
	}

	ast_cli (a->fd, "DONGLE    IMSI              Number----- Group Sta Prv Diff DATT-IATT TOTAL ANSWEO:ANSWEI MINUTES_O:MINUTES_I MINUTES_W ACD_TOTAL ----ACD_L -ASRL PDDAS PDDL0 PDDL1 BALAN\n");

	AST_RWLIST_RDLOCK (&gpublic->devices);
	AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
	{





		//LOCK// ast_mutex_lock (&pvt->lock);
		readpvtinfo(pvt);
		
		diff_end=(int)((long)time(NULL)-PVT_STAT(pvt,stat_call_end));
		if (diff_end>999) {diff_end=0;}
		
		pdd=((float)getACD(PVT_STAT(pvt, stat_out_calls[2]),PVT_STAT(pvt, stat_wait_duration[2])));
		
		
		ast_cli (a->fd, FORMAT2,
			PVT_ID(pvt),
			pvt->imsi,
			PVT_STAT(pvt, number),
			CONF_SHARED(pvt, group),
			pvt_str_state(pvt),
			pvt->provider_name,
			diff_end,
			PVT_STAT(pvt, stat_datt[2]),
			PVT_STAT(pvt, stat_iatt),
			PVT_STAT(pvt, stat_out_calls[2]),
			PVT_STAT(pvt, stat_calls_answered[2]),
			PVT_STAT(pvt, stat_in_answered),
			((float)PVT_STAT(pvt, stat_calls_duration[2]))/60,
			((float)PVT_STAT(pvt, stat_in_duration))/60,
			((float)PVT_STAT(pvt, stat_wait_duration[2]))/60,
			((float)getACD(PVT_STAT(pvt, stat_calls_answered[2]), PVT_STAT(pvt, stat_calls_duration[2])))/60,
			((float)PVT_STAT(pvt, stat_acdl[2]))/60,
			((float)PVT_STAT(pvt, stat_asrl[2]))/1000,
			pdd,
			(((float)PVT_STAT(pvt, stat_pddl[0][2]))/1000),
			(((float)PVT_STAT(pvt, stat_pddl[1][2]))/1000),
			PVT_STAT(pvt, balance)
			
		);
		//LOCK// ast_mutex_unlock (&pvt->lock);
	}
	AST_RWLIST_UNLOCK (&gpublic->devices);

#undef FORMAT1
#undef FORMAT2

	return CLI_SUCCESS;
}

static char* cli_show_devicesl (struct ast_cli_entry* e, int cmd, struct ast_cli_args* a)
{
	struct pvt* pvt;



#define FORMAT1 "%-10.10s %-16.16s %-5.5s %-3.3s %-3.3s %4.4s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s %6.6s\n"
#define FORMAT2 "%-10.10s %-16.16s %-11.11s %5s %5d %-3.3s %-3.3s %8.1f %5.1f %5.1f\n"

	switch (cmd)
	{
		case CLI_INIT:
			e->command =	"dongle show devicesl";
			e->usage   =	"Usage: dongle show devicesl\n"
					"       Shows the state of Dongle devices.\n";
			return NULL;

		case CLI_GENERATE:
			return NULL;
	}

	if (a->argc != 3)
	{
		return CLI_SHOWUSAGE;
	}

	ast_cli (a->fd, "DONGLE    IMSI              Number----- BALAN Group Sta Prv LIMT0--- LIMT1 LIMT2\n");

	AST_RWLIST_RDLOCK (&gpublic->devices);
	AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
	{





		//LOCK// ast_mutex_lock (&pvt->lock);
		readpvtinfo(pvt);
		readpvtlimits(pvt);

		
		
		ast_cli (a->fd, FORMAT2,
			PVT_ID(pvt),
			pvt->imsi,
			PVT_STAT(pvt, number),
			PVT_STAT(pvt, balance),
			CONF_SHARED(pvt, group),
			pvt_str_state(pvt),
			pvt->provider_name,
			((float)(PVT_STAT(pvt, limit[0])))/60,
			((float)(PVT_STAT(pvt, limit[1])))/60,
			((float)(PVT_STAT(pvt, limit[2])))/60
			
		);
		//LOCK// ast_mutex_unlock (&pvt->lock);
	
//ast_cli (a->fd,"%d\n",PVT_STAT(pvt, limit[1]));

	}
	AST_RWLIST_UNLOCK (&gpublic->devices);



#undef FORMAT1
#undef FORMAT2

	return CLI_SUCCESS;
}
