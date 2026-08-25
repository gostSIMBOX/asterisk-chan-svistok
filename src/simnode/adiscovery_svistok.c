
#include "adiscovery_core.c"

void alog(FILE* fd,char *fmt, ...)
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


static int pvt_adiscovery (struct pvt * pvt)
{
    if(CONF_UNIQ(pvt, data_tty)[0] != 0)
    {
	ast_copy_string (PVT_STATE(pvt, data_tty),  CONF_UNIQ(pvt, data_tty), sizeof(PVT_STATE(pvt, data_tty)));
	return usbdev_get_audioport(CONF_UNIQ(pvt, data_tty),PVT_STATE(pvt, audio_tty));
    }
    return !1;

}


