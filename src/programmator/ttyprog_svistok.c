#include "ttyprog_core.c"

void tty_log(FILE* fd,char *fmt, ...)
{
    char buf[512];

    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf,fmt,ap);
    va_end(ap);
    if(fd==NULL)
    {
        ast_verb(3,"%s",buf);
    } else {
    fprintf(fd,"%s",buf);
    }
}

