
DIE!!!


#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>


#ifdef IN_SIMBOX
#include <asterisk.h>
#include <asterisk/linkedlists.h>		/* AST_LIST_ENTRY() */
#endif


//include "tty.h"


#define MAXUSBDEVS 512
#define MAXSYSDEVS 512

typedef struct iface {
   unsigned int enabled;
   unsigned int ep[7];
   unsigned char port[256];
} iface_t;

typedef struct defdev {
	unsigned int model;
	unsigned int mode;       //1 - work, 2 - diag, 3 - unknown
	unsigned int idVendor;
	unsigned int idProduct;
	iface_t iface[4];
	int dataport_num;
	int audioport_num;
} defdev_t;



defdev_t defdevs[] = {

	{200, 1, 0x12d1, 0x1001, {
				    {1,{0x01,0x00,0x00,0x81,0x82,0x00,0x00},{0}},
				    {1,{0x00,0x02,0x00,0x00,0x00,0x83,0x00},{0}},
				    {1,{0x00,0x00,0x03,0x00,0x00,0x00,0x84},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}}
				}, 2, 1
	},

	{1550, 2, 0x12d1, 0x1003, {
				    {1,{0x01,0x00,0x00,0x81,0x00,0x00,0x00},{0}},
				    {1,{0x00,0x02,0x00,0x00,0x82,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}}
				}, 1, -1
	},

	{1550, 3, 0x12d1, 0x14ac, {
				    {1,{0x01,0x00,0x00,0x81,0x82,0x00,0x00},{0}},
				    {0,{0x00,0x02,0x00,0x00,0x00,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {1,{0x00,0x00,0x03,0x00,0x00,0x00,0x00},{0}}
				}, 0, -1
	},
	
	{173, 3, 0x12d1, 0x140c, {
				    {1,{0x01,0x00,0x00,0x81,0x82,0x00,0x00},{0}},
				    {1,{0x00,0x02,0x00,0x00,0x00,0x83,0x84},{0}},
				    {1,{0x00,0x00,0x03,0x00,0x00,0x00,0x00},{0}},
				    {1,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}}
				}, 0, -1
	},

	{1550, 3, 0x12d1, 0x1003, {
				    {1,{0x01,0x00,0x00,0x81,0x82,0x00,0x00},{0}},
				    {1,{0x00,0x02,0x00,0x00,0x00,0x83,0x00},{0}},
				    {1,{0x00,0x00,0x03,0x00,0x00,0x00,0x84},{0}},
				    {1,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}}
				}, 1, -1
	},

	{173, 2, 0x12d1, 0x1001, {
				    {1,{0x01,0x00,0x00,0x81,0x00,0x00,0x00},{0}},
				    {1,{0x00,0x02,0x00,0x00,0x82,0x00,0x00},{0}},
				    {1,{0x00,0x00,0x03,0x00,0x00,0x83,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}}
				}, 2, -1
	},


	{173, 3, 0x12d1, 0x1436, {
				    {1,{0x01,0x00,0x00,0x81,0x82,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {1,{0x00,0x00,0x03,0x00,0x00,0x00,0x00},{0}}
				}, 0, -1
	},

reader
/*
	{1001, 1002, 0x67b, 0x2303, {
				    {1,{0x00,0x02,0x00,0x81,0x00,0x83,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}}
				}, 0, -1
	},
*/

//1616
	{1616, 1, 0x67b, 0x2303, {
				    {1,{0x00,0x02,0x00,0x81,0x00,0x83,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}},
				    {0,{0x00,0x00,0x00,0x00,0x00,0x00,0x00},{0}}
				}, 0, 0
	}



	};



typedef struct sysdev {
	unsigned char devfile[256];
	unsigned int idVendor;
	unsigned int idProduct;
	iface_t iface[4];
} sysdev_t;

typedef struct usbdev {
	unsigned int model;
	unsigned int mode; // 0-hz  // 1-active // 2-diag
	unsigned char dataport[256];
	unsigned char audioport[256];
	unsigned char devfile[256];
} usbdev_t;


usbdev_t usbdevs[MAXUSBDEVS];
sysdev_t sysdevs[MAXSYSDEVS];

int unsigned usbdevs_count;
int unsigned sysdevs_count;


static const char sys_drivers[3][256] = {"/sys/bus/usb/drivers/option","/sys/bus/usb/drivers/usbserial_generic","/sys/bus/usb/drivers/pl2303"};

static const char sys_bus_usb_devices[] = "/sys/bus/usb/devices";

/*
bEndpointAddress
1550 diag
idProduct 1003
idVendor 12d1
0 /ep_01 /ep_81
1 /ep_02 /ep_82

1550 normal
idProduct 1001
idVendor 12d1
0 /ep_01 /ep_81 /ep_82
1 /ep_02 /ep_83
2 /ep_03 /ep_84



*/

void alog(FILE * fd, char *fmt,...);

int get_file_sys_bus_usb_devices(char * device,char * file, unsigned int * value)
{
    char filename[256];
    strcpy(filename, sys_bus_usb_devices);
    strcat(filename,"/");
    strcat(filename,device);
    strcat(filename,"/");
    strcat(filename,file);
    return get_file_x(filename, value);
}

// ep_02/bEndpointAddress
int get_file_endpoint(char * device,unsigned int ifaceno, char * ep, unsigned int * value)
{
    char filename[256];
    char ifaceno_c[256];
    
    sprintf(ifaceno_c,"%d",ifaceno);

    strcpy(filename, sys_bus_usb_devices);
    strcat(filename,"/");
    strcat(filename,device);
    strcat(filename,":1.");
    strcat(filename,ifaceno_c);
    strcat(filename,"/");
    strcat(filename,ep);
    strcat(filename,"/bEndpointAddress");
    
    return get_file_x(filename, value);
}


int get_file_x(const char * filename, unsigned int * value)
{
//	printf ("get_file_x %s\n",filename);
	int assign;
	*value=0;
	FILE * file = fopen(filename, "r");
	if(file) {
		assign = fscanf(file, "%x", value);
		fclose(file);
	}

	return assign;
}


void sysdevs_find()
{
	sysdevs_count=0;
	char tmp1[256];
	int tmp2;

	char devfile[256];
	char port[256];
	int ep83,ep84;
	int ifaceno;
	int i,o,p;
	int tmp; 
	struct dirent * entry;

	for(p=0;p<3;p++)
	{

	DIR * dir = opendir(sys_drivers[p]);

	if(dir) 
		while((entry = readdir(dir)) != NULL) 
		{
			if(sscanf(entry->d_name, "%99[^:]:%d.%d", devfile,&tmp2, &ifaceno) == 3)
			if(tmp2==1)
			{ //%39[^.]

//				strcat(devfile,":");
//				strcat(devfile,tmp2);
//    

				//нашли
				//проверим, есть ли устройство
				
				
				for(i=0;i<sysdevs_count;i++)
					if(strcmp(devfile,(sysdevs[i]).devfile)==0) break;
				if(i==sysdevs_count)
				{
					//устройства еще нет, добавляем
					strcpy(sysdevs[sysdevs_count].devfile,devfile);
					//получаем vendorid,productid
					get_file_sys_bus_usb_devices(sysdevs[sysdevs_count].devfile,"idVendor",&(sysdevs[sysdevs_count].idVendor));
					get_file_sys_bus_usb_devices(sysdevs[sysdevs_count].devfile,"idProduct",&(sysdevs[sysdevs_count].idProduct));
					//чистим интерфейсы
					for(o=0;o<4;o++)
						sysdevs[sysdevs_count].iface[o].enabled=0;
					sysdevs_count++;
				}
				
				
				//получаем и записываем тип интерфейса
				//строим путь интерфейса

                                sysdevs[i].iface[ifaceno].enabled=1;
				
				get_file_endpoint(sysdevs[i].devfile,ifaceno,"ep_01",&(sysdevs[i].iface[ifaceno].ep[0]));
				get_file_endpoint(sysdevs[i].devfile,ifaceno,"ep_02",&(sysdevs[i].iface[ifaceno].ep[1]));
				get_file_endpoint(sysdevs[i].devfile,ifaceno,"ep_03",&(sysdevs[i].iface[ifaceno].ep[2]));
				get_file_endpoint(sysdevs[i].devfile,ifaceno,"ep_81",&(sysdevs[i].iface[ifaceno].ep[3]));
				get_file_endpoint(sysdevs[i].devfile,ifaceno,"ep_82",&(sysdevs[i].iface[ifaceno].ep[4]));
				get_file_endpoint(sysdevs[i].devfile,ifaceno,"ep_83",&(sysdevs[i].iface[ifaceno].ep[5]));
				get_file_endpoint(sysdevs[i].devfile,ifaceno,"ep_84",&(sysdevs[i].iface[ifaceno].ep[6]));
				sysdev_getport(sysdevs[i].devfile,ifaceno,&(sysdevs[i].iface[ifaceno].port));
				
				
			}
		}
		closedir(dir);
	}

}


void sysdev_show(FILE *fd,sysdev_t * sysdev)
{
	    int o;

		alog(fd,">>sysdev %s %x %x\n",sysdev->devfile,sysdev->idVendor,sysdev->idProduct);

		for(o=0;o<4;o++)
			if(sysdev->iface[o].enabled!=0)

				alog(fd,">       iface=%d { %2x %2x %2x %2x %2x %2x %2x %s } \n",o,
				sysdev->iface[o].ep[0],
				sysdev->iface[o].ep[1],
				sysdev->iface[o].ep[2],
				sysdev->iface[o].ep[3],
				sysdev->iface[o].ep[4],
				sysdev->iface[o].ep[5],
				sysdev->iface[o].ep[6],
				sysdev->iface[o].port
			    );
				

}

void sysdevs_show(FILE *fd)
{
    int i,o,j;
    
	for(i=0;i<sysdevs_count;i++)
	{
	    sysdev_show(fd,&(sysdevs[i]));
	}
	alog(fd,">>sysdevs_count=%d\n",sysdevs_count);

}


void usbdevs_show(FILE *fd,int mode)
{
    int i;
    
	for(i=0;i<usbdevs_count;i++)
	{
	    if((mode==-1)||(usbdevs[i].mode==mode))
	    {
		alog(fd,">>usbdev %d %d %s %s %s\n",usbdevs[i].model,usbdevs[i].mode,usbdevs[i].devfile,usbdevs[i].dataport,usbdevs[i].audioport);
	    }
	}
	
	alog(fd,">>usbdevs_count=%d\n",usbdevs_count);

}

void saveparams(const char*device, char*name, char*value)
{
    FILE * fd;
    char filename[256]="/var/svistok/devices/";
    strcat(filename,device);
    strcat(filename,".");
    strcat(filename,name);

    fd=fopen(filename,"w");
    if (fd)
    {
	fprintf(fd,"%s",value);
        fclose(fd);
    }

}

void saveparami(const char*device, char*name, int *value)
{
    FILE * fd;
    char filename[256]="/var/svistok/devices/";
    strcat(filename,device);
    strcat(filename,".");
    strcat(filename,name);

    fd=fopen(filename,"w");
    if (fd)
    {
	fprintf(fd,"%d",*value);
        fclose(fd);
    }

}

void usbdevs_filelist(char *filename,int mode)
{
    int i;
    FILE * fd;
    fd=fopen(filename,"w");
    if (fd)
    {
    for(i=0;i<usbdevs_count;i++)
    {
        if((mode==-1)||(usbdevs[i].mode==mode))
        {
	fprintf(fd,"%s\n",usbdevs[i].devfile);
	saveparami(usbdevs[i].devfile,"model",&(usbdevs[i].model));
	saveparams(usbdevs[i].devfile,"dataport",usbdevs[i].dataport);
//	alog("adiscovery: >>usbdev %d %d %s %s %s\n",usbdevs[i].model,us
        }
    }
    fclose(fd);
    }
}

void usbdevs_filelist_2(char *filename,int mode)
{
    int i;
    FILE * fd;
    fd=fopen(filename,"w");
    if (fd)
    {
    for(i=0;i<usbdevs_count;i++)
    {
        if((mode==-1)||(usbdevs[i].mode==mode))
        {
	fprintf(fd,"./fupdate4.sh %s %s %d\n",usbdevs[i].dataport, usbdevs[i].devfile,usbdevs[i].model);
//	saveparami(usbdevs[i].devfile,"model",&(usbdevs[i].model));
//	saveparams(usbdevs[i].devfile,"dataport",usbdevs[i].dataport);
//	alog("adiscovery: >>usbdev %d %d %s %s %s\n",usbdevs[i].model,us
        }
    }
    fclose(fd);
    }
}

void usbdevs_filelist_3(char *filename,int mode)
{
    int i;
    FILE * fd;
    fd=fopen(filename,"w");
    if (fd)
    {
    for(i=0;i<usbdevs_count;i++)
    {
        if((mode==-1)||(usbdevs[i].mode==mode))
        {
	fprintf(fd,"/usr/simbox/system/u2diag.sh %s\n", usbdevs[i].dataport);
//	saveparami(usbdevs[i].devfile,"model",&(usbdevs[i].model));
//	saveparams(usbdevs[i].devfile,"dataport",usbdevs[i].dataport);
//	alog("adiscovery: >>usbdev %d %d %s %s %s\n",usbdevs[i].model,us
        }
    }
    fclose(fd);
    }
}

void usbdevs_filelist_4(char *filename,int mode)
{
    int i;
    FILE * fd;
    fd=fopen(filename,"w");
    if (fd)
    {
    for(i=0;i<usbdevs_count;i++)
    {
        if((mode==-1)||(usbdevs[i].mode==mode))
        {
	fprintf(fd,"/usr/simbox/system/unlocker.sh %s %s &\n", usbdevs[i].dataport,usbdevs[i].devfile);
//	saveparami(usbdevs[i].devfile,"model",&(usbdevs[i].model));
//	saveparams(usbdevs[i].devfile,"dataport",usbdevs[i].dataport);
//	alog("adiscovery: >>usbdev %d %d %s %s %s\n",usbdevs[i].model,us
        }
    }
    fclose(fd);
    }
}


void sysdevs_log(const char* filename)
{

    FILE * fp;
    if(fp=fopen(filename,"w"))
    {
	sysdevs_show(fp);
        fclose(fp);
    }
}

void usbdevs_log(const char* filename,int mode)
{

    FILE * fp;
    fp=fopen(filename,"w");

    if(fp)
    {
	usbdevs_show(fp,mode);
        fclose(fp);
    }
}


void usbdev_found(usbdev_t * usbdev)
{
#ifdef IN_SIMBOX
	pvt_config_t settings;
	struct pvt * pvt;
#endif
	char newname[64]="dongle0";
	char tmp[2]={0,0};
	int i;

//	printf("Found device  %d %d data=%s audio=%s\n",usbdev->model, usbdev->mode, usbdev->dataport,usbdev->audioport);
	alog(NULL,"adiscovery: Found device %s %d %d data=%s audio=%s\n",usbdev->devfile,usbdev->model, usbdev->mode, usbdev->dataport,usbdev->audioport);

#ifdef IN_SIMBOX
if(usbdev->mode==1)
{
alog(NULL,"adiscovery: mode=1 => create\n");


strcpy(settings.shared.context,"default-dongle-incoming");
strcpy(settings.shared.exten,"12345");
strcpy(settings.shared.language,"en");

settings.shared.group=0;
settings.shared.agroup=0;
settings.shared.rxgain=0;
settings.shared.txgain=0;
settings.shared.u2diag=0;
settings.shared.callingpres=0;
settings.shared.usecallingpres=-1;
settings.shared.resetdongle=1;
settings.shared.disablesms=0;
settings.shared.smsaspdu=1;
settings.shared.autodeletesms=1;
settings.shared.initstate=DEV_STATE_STARTED;
//settings->shared.disable=0;
settings.shared.callwaiting=CALL_WAITING_DISALLOWED;
settings.shared.dtmf=DC_DTMF_SETTING_RELAX;
settings.shared.mindtmfgap=DEFAULT_MINDTMFGAP;
settings.shared.mindtmfduration=DEFAULT_MINDTMFDURATION;
settings.shared.mindtmfinterval=DEFAULT_MINDTMFINTERVAL;





for(i=0;i<strlen(usbdev->dataport);i++)
  if((usbdev->dataport[i]>='0')&&(usbdev->dataport[i]<='9'))
  {
     tmp[0]=usbdev->dataport[i];
     strcat(newname, tmp);
  }

strcpy(settings.unique.id,newname);

strcpy(settings.unique.audio_tty,usbdev->audioport);
strcpy(settings.unique.data_tty,usbdev->dataport);
strcpy(settings.unique.dev,usbdev->devfile);
*settings.unique.imei=0;
*settings.unique.imsi=0;
*settings.unique.serial=0;


	pvt = pvt_create(&settings);


	if(pvt)
	{
		strcpy(PVT_STATE(pvt,dev),usbdev->devfile);

		AST_RWLIST_WRLOCK(&gpublic->devices);
		AST_RWLIST_INSERT_TAIL(&gpublic->devices, pvt, entry);
		AST_RWLIST_UNLOCK(&gpublic->devices);
	        alog(NULL,"adiscovery: pvt_create OK\n");
	} else {
	    alog(NULL,"adiscovery: pvt_create FAILED\n");
	}

}
#endif

	
//	sysdevs_show();
	return;
}

void usbdev_lost(usbdev_t * usbdev)
{
#ifdef IN_SIMBOX
	struct pvt * pvt;
#endif

	alog(NULL,"adiscovery: Lost device %s %d %d data=%s audio=%s\n",usbdev->devfile,usbdev->model, usbdev->mode, usbdev->dataport,usbdev->audioport);

#ifdef IN_SIMBOX
        AST_RWLIST_TRAVERSE(&gpublic->devices, pvt, entry)
        {
		if(strcmp(CONF_UNIQ(pvt,data_tty),usbdev->dataport)==0)
		{
		    alog(NULL,"adiscovery: pvtdestroy[%s]\n",PVT_ID(pvt));
            	    //pvt_destroy(pvt);
		    pvt->desired_state=DEV_STATE_REMOVED;
		    alog(NULL,"adiscovery: pvtdestroy[%s] OK\n",PVT_ID(pvt));
                }
        }


// pvt->must_remove=1
//	sysdevs_show();
#endif
	return;
}

int sysdev_getport(char * device,unsigned int ifaceno, char * port)
{
    char filename[256];
    char ifaceno_c[256];

    sprintf(ifaceno_c,"%d",ifaceno);

    strcpy(filename, sys_bus_usb_devices);
    strcat(filename,"/");
    strcat(filename,device);
    strcat(filename,":1.");
    strcat(filename,ifaceno_c);
    strcat(filename,"/");

    return iface_getport(filename, port);
}


int iface_getport(char* filename, char* port)
{
	//printf("\n\n- iface_getport %s\n\n",filename);
	//strcpy(port,"ttyUSB?");

	*port=0;
	struct dirent * entry;
	
	DIR * dir = opendir(filename);
//alog("adiscovery:2 %s",filename);
	
	if(dir) {
		while((entry = readdir(dir)) != NULL) {
			if(strstr(entry->d_name,"ttyUSB"))
			{
			    strcpy(port,"/dev/");
			    strcat(port,entry->d_name);
			    closedir(dir);
			 return 1;
			 
			}
		}
		closedir(dir);
	}
	return -1;
}


int sysdev_getdef(sysdev_t * sysdev)
{
    int i,o,p;
    int u1;
    
    for(i=0;i<sizeof(defdevs)/sizeof(defdev_t);i++)
    {
	u1=(sysdev->idVendor==defdevs[i].idVendor)  &&  (sysdev->idProduct==defdevs[i].idProduct);
	for(o=0;o<4;o++)
	    {
//		u1=u1 && (sysdev->iface[o].enabled==defdevs[i].iface[o].enabled)
		if(sysdev->iface[o].enabled==1)
		for(p=0;p<7;p++)
		    u1=u1 && (sysdev->iface[o].ep[p]==defdevs[i].iface[o].ep[p]);
	    }
	if(u1)
	{
	    return i;
	}
    }
    
    alog(NULL,"Warning: sysdev not in defdevs\n");
    sysdev_show(NULL,sysdev);
    return -1;
}


void usbdev_createdef(sysdev_t * sysdev, usbdev_t * usbdev, int defdev)
{
    usbdev->model=defdevs[defdev].model;
    usbdev->mode=defdevs[defdev].mode;

    if(defdevs[defdev].dataport_num!=-1)
        strcpy(usbdev->dataport,sysdev->iface[defdevs[defdev].dataport_num].port);
    else
	*usbdev->dataport=0;

	
    if(defdevs[defdev].audioport_num!=-1)
	strcpy(usbdev->audioport,sysdev->iface[defdevs[defdev].audioport_num].port);
    else
	*usbdev->audioport=0;

    strcpy(usbdev->devfile,sysdev->devfile);
}

void usbdevs_find()
{
    int intdefdev;
    int i,o;
    
    
    
	//убираем старые
	for(o=0;o<usbdevs_count;o++)
	{
		for(i=0;i<sysdevs_count;i++)
		{
			intdefdev=sysdev_getdef(&(sysdevs[i]));
			if(intdefdev>=0)
			{
				usbdev_createdef(&(sysdevs[i]),&usbdevs[usbdevs_count],intdefdev);

				if(
				    (usbdevs[o].model==usbdevs[usbdevs_count].model) &&
				    (usbdevs[o].mode==usbdevs[usbdevs_count].mode) &&
				    (strcmp(usbdevs[o].dataport,usbdevs[usbdevs_count].dataport)==0) &&
				    (strcmp(usbdevs[o].audioport,usbdevs[usbdevs_count].audioport)==0) &&
				    (strcmp(usbdevs[o].devfile,usbdevs[usbdevs_count].devfile)==0) 

				    ) break ;
			}


		}
		if(i==sysdevs_count)
		{
			//нет такого, потерян devs[i]
			usbdev_lost(&(usbdevs[o]));
			memcpy(&(usbdevs[o]),&(usbdevs[usbdevs_count-1]),sizeof(usbdev_t));
			usbdevs_count--;
		}
	}

	//находим новые
	for(i=0;i<sysdevs_count;i++)
	{
//printf("1\n");
		intdefdev=sysdev_getdef(&(sysdevs[i]));
		if(intdefdev>=0)
		{
//printf("2\n");
			usbdev_createdef(&(sysdevs[i]),&usbdevs[usbdevs_count],intdefdev);
//printf("3\n");
			for(o=0;o<usbdevs_count;o++)
			{
				if(
				    (usbdevs[o].model==usbdevs[usbdevs_count].model) &&
				    (usbdevs[o].mode==usbdevs[usbdevs_count].mode) &&
				    (strcmp(usbdevs[o].dataport,usbdevs[usbdevs_count].dataport)==0) &&
				    (strcmp(usbdevs[o].audioport,usbdevs[usbdevs_count].audioport)==0) 
				    ) break ;
			}
//printf("4\n");
			if(o==usbdevs_count)
			{
					usbdevs_count++;
					usbdev_found(&(usbdevs[o]));
			}
		}
	}


}


int usbdev_get_audioport (char* dataport, char* audioport)
{
    int i;

    *audioport=0;
//    alog(NULL,"looking %s",dataport);
    for(i=0;i<usbdevs_count;i++)
{           
// alog("adiscovery: test %s %s",usbdevs[i].dataport,dataport);
	if(usbdevs[i].mode==1)
	if(strcmp(usbdevs[i].dataport,dataport)==0)
	{
	    strcpy(audioport,usbdevs[i].audioport);
            alog(NULL, "found %s",audioport);
	    
	    return 1;
	}
}
	
	return !1;
}

