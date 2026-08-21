#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include "share_mysql.h"
#include <mysql.h>

MYSQL *mysql;

char mysql_host = "localhost";
char mysql_user = "svistok";
char mysql_pass = "svistok4385";
char mysql_db = "svistok";

void a_mysql_connect ()
{
    mysql=mysql_init(0);
    if(!mysql)
    {
        st_verb(3,"MYSQL:cannot creare socket (%s)",mysql_error(mysql));
        return;
    }
    if (!(mysql_real_connect(mysql, mysql_host, mysql_user, mysql_pass, mysql_db, 0, NULL, 0)))
    {
        st_verb(3,"MYSQL:cannot connect (%s)",mysql_error(mysql));
	mysql=NULL;
        return;
    }

    if (!(mysql_connect(&mysql,"localhost","svistok","svistok4385")))
}

int mysql_getfilei (char* devtype, char* fileitem, char* filetype, int *value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  

  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    fscanf(pFile,"%d",value);
    fclose(pFile);
    return 1;
  }
  return 0;

}

int mysql_getfilel (char* devtype, char* fileitem, char* filetype, long int *value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  

  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    fscanf(pFile,"%ld",value);
    fclose(pFile);
    return 1;
  }
  return 0;

}

int mysql_getfiles (char* devtype, char* fileitem, char* filetype, char *value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  

  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    fscanf(pFile,"%s",value);
    fclose(pFile);
    return 1;
  }
  return 0;

}


int mysql_putfilei (char* devtype, char* fileitem, char* filetype, int value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%d",value);
		fclose(pFile);
		return 1;
	}
	return 0;
	

//    chmod(filename,511);
}


int mysql_putfilel (char* devtype, char* fileitem, char* filetype, long int value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%ld",value);
		fclose(pFile);
		return 1;
	}
	return 0;
	

//    chmod(filename,511);
}




int mysql_putfiles (char* devtype, char* fileitem, char* filetype, char* value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%s",value);
		fclose(pFile);
		return 1;
	}
	return 0;
	
//    chmod(filename,511);
}


int putfileslog (char* devtype, char* fileitem, char* filetype, char* value)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
char dn[256];
timenow(dn);

  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"a");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%s %s\n",dn,value);
		fclose(pFile);
		return 1;
	}
	return 0;
	
//    chmod(filename,511);
}

int putfileslog2 (char* devtype, char* fileitem, char* filetype, const char* valueformat, va_list va)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
char dn[256];
timenow(dn);

  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
	pFile=fopen(filename,"a");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%s",dn);
		fprintf(pFile,valueformat,va);
		fclose(pFile);
		return 1;
	}
	return 0;
	
//    chmod(filename,511);
}





int putgetfilei (char putget, char* devtype, char* fileitem, char* filetype, int value,  struct ast_cli_args* a)
{
  /*
  devtype="imsi"
  fileitem="250000000"
  filetype="balance"
  putget=g
  putget=p
  */
  char filename[128];
  char path[64]="/var/svistok/";

  char slash[5]="/";
  char dot[5]=".";
  
  FILE* pFile;
  
  

  strcpy(filename, path);

  strcat(filename, devtype);
  strcat(filename, slash);
  strcat(filename, fileitem);
  strcat(filename, dot);
  strcat(filename, filetype);
  
  /*ast_cli (a->fd, "!!!%s!!!\n", filename);*/
  /* return 0;*/
  
  if (putget=='r')
  {
  ast_cli (a->fd, "!!!r!!!\n", filename);

  pFile=fopen(filename,"r");
  if (pFile!=NULL)
  {
    fscanf(pFile,"%d",&value);
    fclose(pFile);
    return 1;
  }
  return 0;
  }

  if (putget=='w')
  {
  ast_cli (a->fd, "!!!w!!!\n", filename);

	pFile=fopen(filename,"w");
	if (pFile!=NULL)
	{
		fprintf(pFile,"%d",value);
		fclose(pFile);
		return 1;
	}
	return 0;
  }
  return 0;
}


void make_bs_list()
{
    struct pvt* pvt;
    int i;
    int bs_count=0;
    int bs_max=256;
    char bs[bs_max][10];
    int  bs_online[bs_max];
    int  bs_connected[bs_max];
    int  bs_rssisum[bs_max];
    
    long int bs_acdl_sum[bs_max];
    long int bs_acd_sum[bs_max];

    long int bs_asrl_sum[bs_max];
    long int bs_asr_sum[bs_max];

    char bs_provider[bs_max][64];
    
    //Получим список и количество Cell

	AST_RWLIST_RDLOCK (&gpublic->devices);
	AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
	{
	if (pvt!=NULL)
	{
        if(*pvt->cell_id!=0)
	    for(i=0;i<bs_count;i++)
	    {
		if(strcmp(bs[i],pvt->cell_id)==0)
		{
			bs_online[i]++;
			bs_rssisum[i]+=pvt->rssi;

			bs_acdl_sum[i]+=PVT_STAT(pvt,stat_acdl[2]);
			bs_acd_sum[i]+=PVT_STAT(pvt,stat_acdl[2]);
			bs_asrl_sum[i]+=PVT_STAT(pvt,stat_asrl[2]);
			bs_asr_sum[i]+=PVT_STAT(pvt,stat_asrl[2]);

			break;
		}
	    }
	    if(!(i<bs_count))
	    {
		bs_count++;
		bs_online[bs_count-1]=1;
		bs_connected[bs_count-1]=0;
		bs_rssisum[bs_count-1]=pvt->rssi;
		
		bs_acdl_sum[i]=PVT_STAT(pvt,stat_acdl[2]);
		bs_acd_sum[i]=PVT_STAT(pvt,stat_acdl[2]);
		bs_asrl_sum[i]=PVT_STAT(pvt,stat_asrl[2]);
		bs_asr_sum[i]=PVT_STAT(pvt,stat_asrl[2]);

		strcpy(bs[bs_count-1],pvt->cell_id);
		strcpy(bs_provider[bs_count-1],pvt->provider_name);
	    }
	}
	}
	AST_RWLIST_UNLOCK (&gpublic->devices);


    char filename_bs[128]="/var/svistok/bs.list";
    //char filename_bs_item[128]="";

    FILE* pFile_bs;
    
    pFile_bs=fopen(filename_bs,"w");
    if (pFile_bs!=NULL)
	{
	    for(i=0;i<bs_count;i++)
	    {
		fprintf(pFile_bs,"%s\n",bs[i]);
		putfilei("bs/state",bs[i],"connected",bs_connected[i]);
		putfilei("bs/state",bs[i],"online",bs_online[i]);
		putfiles("bs/state",bs[i],"provider_name",bs_provider[i]);
		putfilei("bs/state",bs[i],"rssi",bs_rssisum[i]/bs_online[i]);

//		putfilei("bs/state",bs[i],"acdl",bs_acdl_sum[i]/bs_online[i]);
		putfilei("bs/state",bs[i],"acd",bs_acd_sum[i]/bs_online[i]);
//		putfilei("bs/state",bs[i],"asrl",bs_asrl_sum[i]/bs_online[i]);
		putfilei("bs/state",bs[i],"asr",bs_asr_sum[i]/bs_online[i]);

	    }
	    fclose(pFile_bs);
	}
ast_verb(3,"%s","--bsok\n");
    
    
}
void make_dongles_imsi_list()
{
    struct pvt* pvt;
    int bs_count=0;

    char filename_dongles[128]="/var/svistok/dongles.list";
    char filename_imsi[128]="/var/svistok/imsi.list";

    FILE* pFile_dongles;
    FILE* pFile_imsi;

ast_verb(3,"%s","--writedongles\n");

	pFile_dongles=fopen(filename_dongles,"w");
	if (pFile_dongles!=NULL)
	{
		AST_RWLIST_RDLOCK (&gpublic->devices);
		AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
		{
			if (*pvt->imei!=0)
			{
			    fprintf(pFile_dongles,"%s\n",PVT_ID(pvt));
			    putfiles("dongles/state",PVT_ID(pvt),"str_state",pvt_str_state(pvt));
			} else if (pvt->nosim)
			{
			    fprintf(pFile_dongles,"%s\n",PVT_ID(pvt));
			    putfiles("dongles/state",PVT_ID(pvt),"str_state",pvt_str_state(pvt));
			}
		}
	        AST_RWLIST_UNLOCK (&gpublic->devices);

		
		fclose(pFile_dongles);
	}
ast_verb(3,"%s","--writeimsi\n");

        pFile_imsi=fopen(filename_imsi,"w");
	if (pFile_imsi!=NULL)
	{
		AST_RWLIST_RDLOCK (&gpublic->devices);
		AST_RWLIST_TRAVERSE (&gpublic->devices, pvt, entry)
		{
			if (*pvt->imsi!=0)
			{
				fprintf(pFile_imsi,"%s\n",pvt->imsi);
				putfiles("sim/state",pvt->imsi,"dongle",PVT_ID(pvt));
			}
		}
	        AST_RWLIST_UNLOCK (&gpublic->devices);
		fclose(pFile_imsi);
	}

ast_verb(3,"%s","--writedongles_ok\n");

//    make_bs_list();
}

void clear_state()
{
    system("rm /var/svistok/sim/state/*");
    system("rm /var/svistok/dongles.list");
    system("rm /var/svistok/imsi.list");
    system("rm /var/svistok/bs.list");
    return;
}

void IAXME_get()
{
    //char buf
    ast_verb(3,"get IAXME\n");
    //strcpy(IAXME1,pbx_builtin_getvar_helper(NULL,"IAXME"));
    ast_verb(3,"get IAXME OK %s\n",IAXME1);
}