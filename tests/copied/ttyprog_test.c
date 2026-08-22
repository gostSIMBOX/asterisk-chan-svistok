#include "tty_v2.c"
#include "ttyprog_core.c"

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


void main()
{
    char* port="/dev/ttyUSB2";
    int fd;
    fd=opentty(port);
    //ttyprog_set_diagmode(fd);
    ttyprog_changeimei(fd,"444444444444466");
    closetty(port,fd);
}