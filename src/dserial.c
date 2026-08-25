#define DSERIALS_FILE "/var/svistok/serials.conf"
#define MAXDSERIALS 2049


typedef struct dserial
{
	char name[64];
	char serial[64];
	char crc[64];
} dserial_t;


struct dserial dserials[MAXDSERIALS];
int dserials_count;

void dserial_init()
{
	FILE *fp;
	char line[256];
	dserials_count=0;
	
	char name[64];
	char serial[64];
	char crc[64];

	
	ast_verb(3,"dserial_init %d\n",0);

	fp=fopen(DSERIALS_FILE,"r");
	if(fp==NULL) return;


	while(fgets(line,sizeof(line),fp))
	{
//		ast_verb(3,"line=%s\n",line);
		sscanf(line,"%s %s %s",name,serial,crc);
		
//		ast_verb(3,"name=%s serial=%s crc=%s\n",name,serial,crc);
		strcpy(dserials[dserials_count].name,name);
		strcpy(dserials[dserials_count].serial,serial);
		strcpy(dserials[dserials_count].crc,crc);
//		ast_verb(3,"dserial %i %s %s %s\n",dserials_count,dserials[dserials_count].name,dserials[dserials_count].serial,dserials[dserials_count].crc);
		dserials_count++;
		if(dserials_count==MAXDSERIALS) return;
	}
	fclose(fp);
}

char* dserial_getname(char *serial)
{
	int i;
//	ast_verb(3,"== dserial_find %d\n",dserials_count);
	for(i=0;i<dserials_count;i++)
	{
		if(strcmp(serial,dserials[i].serial)==0)
		{
			ast_verb(3,"== dserial_found %d\n",i);
			return dserials[i].name;
		}
//		ast_verb(3,"== dserial_strcmp %d %d [%s] [%s]\n",i,strcmp(serial,dserials[i].serial),serial,dserials[i].serial);
	}
	return NULL;
}


void dserial_changename(struct pvt * pvt)
{
	char* changename;
	ast_verb(3,"changename %s",pvt->serial);
	changename=dserial_getname(pvt->serial);
	if (changename)
	{
		ast_verb(3,"== %s",changename);
		strcpy(PVT_ID(pvt),changename);
		if(*pvt->imei!=0)
		putfiles("dongles/statistics",pvt->imei,"imei_name",changename);
	}
}


void dserial_clearname(struct pvt * pvt)
{
	int i;

	char newname[256]="dongle0";
	char tmp[2]={0,0};

	ast_verb(3,"clearname %s %s => %s",PVT_ID(pvt),pvt->serial, newname);
	
    for(i=0;i<strlen(CONF_UNIQ(pvt, data_tty));i++)
    {
	if((CONF_UNIQ(pvt, data_tty)[i]>='0')&&(CONF_UNIQ(pvt, data_tty)[i]<='9'))
	{
	    tmp[0]=CONF_UNIQ(pvt, data_tty)[i];
	    strcat(newname, tmp);
	}
    }

	ast_verb(3,"clearname %s %s => %s",PVT_ID(pvt),pvt->serial, newname);

	    //CONF_UNIQ(pvt, data_tty)
	if (newname)
	{
		strcpy(PVT_ID(pvt),newname);
	}
}
