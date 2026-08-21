#include "tty_v2.c"
#include "ttyprog_core.c"
#include "addons.c"


void tty_log(FILE * fd, char *fmt, ...)
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


void main(int argc, char* argv[])
{
    if (argc!=4)
    {
	printf("Usage: ./programmator /dev/ttyUSB5 3-1.1.1 123.bin\n");
	return;
    }
    char* port;
    char* device;
    char* filename;
    
    port=argv[1];
    device=argv[2];
    filename=argv[3];

    if(access(filename,F_OK)!=0)
    {
	printf("  NO FILE!!!\n");
	return;
    }


    saveparams(device,"state","init");
    saveparami(device,"progress",0);

    sleep(5);

    int fd;
    fd=opentty(port);

    saveparams(device,"state","diag");
    saveparami(device,"progress",1);

    ttyprog_set_diagmode(fd);

    saveparams(device,"state","wait");
    saveparami(device,"progress",10);

    sleep(25);

    saveparams(device,"state","update");
    saveparami(device,"progress",20);

    ttyprog_sendfile(filename,fd,device);

    saveparams(device,"state","done");
    saveparami(device,"progress",100);

    closetty(port,fd);
}