#include "adiscovery_core.c"


//void ast_verb(int level, char *fmt, va_list ap);

void alog(FILE * fd, char *fmt, ...)
{
    char buf[256];

    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf,fmt,ap);
    if (fd==NULL)
    {
	printf("%s",buf);
    } else
    {
	fprintf(fd,"%s",buf);
    }
    va_end(ap);
}




int main(int argc, char * argv[])
{

//printf("0\n");
    sysdevs_find();
//printf("1\n");
    sysdevs_show(NULL);
//printf("2\n");
    usbdevs_find();
//printf("3\n");
    usbdevs_show(NULL,-1);
    
    while(1)
    {
	sleep(1);
	sysdevs_find();
	usbdevs_find();


                sysdevs_log("/var/svistok/lists/sysdevs.list");
                usbdevs_log("/var/svistok/lists/usbdevs_all.list",-1);
                usbdevs_log("/var/svistok/lists/usbdevs_active.list",1);
                usbdevs_log("/var/svistok/lists/usbdevs_diag.list",2);
                usbdevs_log("/var/svistok/lists/usbdevs_unknown.list",0);

                usbdevs_filelist("/var/svistok/lists/devices_diag.list",2);
                usbdevs_filelist("/var/svistok/lists/readers.list",1002);

	//<---->sysdevs_show();
    }

    return 0;
}
