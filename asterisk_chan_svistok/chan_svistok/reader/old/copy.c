#include "reader_core.h"
#include "../programmator/tty_v2.c"




//include "../programmator/ttyprog_core.c"

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

void nsleep(long int msec)
{
    struct timespec ts;

//    sleep(1);

    ts.tv_sec = msec/1000;
    ts.tv_nsec = (msec%1000)*1000000;
//    printf ("sleep %ld\n",ts.tv_nsec)
    nanosleep(&ts, &ts);
}

void setrtsdtr(int fd, int rts, int dtr)
{
    int flags;
    ioctl(fd, TIOCMGET, &flags);
    printf("flags was %d\n", flags);

    flags &= ~TIOCM_RTS;
    flags &= ~TIOCM_DTR;

    if(rts!=0) flags |= TIOCM_RTS;
    if(dtr!=0) flags |= TIOCM_DTR;

    ioctl(fd, TIOCMSET, &flags);
    printf("flags now %d\n", flags);

}

int hex2int(char h)
{
    if ((h>='0')&&(h<='9')) return h-'0';
    if ((h>='A')&&(h<='F')) return h-'A'+10;
    if ((h>='a')&&(h<='f')) return h-'a'+10;
    return 0;
}

void hex2buf(const char*hex, char*buf, int* len)
{
    int i;
    *len=strlen(hex)/2;
    for(i=0;i<*len;i++)
    {
	buf[i]=(hex2int(hex[i*2]))*16+(hex2int(hex[i*2+1]));
    }
}

void printf_buf(unsigned char *buf, int len)
{
    int i;
    printf("%d bytes:", len);
    for(i=0;i<len;i++) 
	printf("%x ",buf[i]);
    printf("\n");
}




int sim_reset(int fd)
{
    setrtsdtr(fd,1,0);
    nsleep(SIMTIMERESET);
    setrtsdtr(fd,1,1);
    nsleep(SIMTIMEPOST);
    setrtsdtr(fd,0,1);
    nsleep(SIMTIMEPOST);
}


//return fd or 0 - false
int sim_init(char * dev)
{
    unsigned char buf[256];
    int readed;
    int fd=0;
    int i;

    printf("try %s\n", dev);
    fd=opentty_spec(dev,1);
    if(fd==0) return fd;
    printf("connected to %s on %d\n", dev, fd);
    sim_reset(fd);
    printf("reading ATR on %d\n", fd);

    readtty_all(fd, buf, 256, &readed);

    if(readed==0) 
    {
	printf("No ATR\n");
	closetty_spec(dev,fd);
	return 0;
    }

    printf_buf(buf,readed);

    if(buf[0]==0x03)
    {
	printf("Reverse ATR. Reconnecting...\n");
	closetty_spec(dev,fd);
        printf("try again %s\n", dev);
        fd=opentty_spec(dev,2);
        if(fd==0) return fd;

        readtty_all(fd, buf, 256, &readed);

        if(readed==0) 
	{
	    printf("No reverse ATR\n");
	    closetty_spec(dev,fd);
	    return 0;
        }

        printf_buf(buf,readed);
    }

    if(buf[0]!=0x3B)
    {
        printf("ATR[0]!=3B\n");
        closetty_spec(dev,fd);
        return 0;
    }

    printf("ATR OK\n");
    return fd;
}

void main()
{
    char* dev="/dev/ttyUSB30";
    char buf[256];
    int len;

    char* apdu1="A0A4000002";
    char* apdu2="7F4D";
    char* apdu3="8F0D";

    
    int fd;
    int i;

    fd=sim_init(dev);
    if(fd==0)
    {
	printf("No sim at %s\n",dev);
	return;
    }

    hex2buf(apdu1,buf,&len);
    printf_buf(buf, len);
    writetty_all(fd, buf, len);
    sleep(1);
    printf("reading answer\n",dev);
    readtty_all(fd, buf, 256, &len);
    printf_buf(buf, len);
    sleep(1);

    hex2buf(apdu2,buf,&len);
    printf_buf(buf, len);
    writetty_all(fd, buf, len);
    sleep(1);
    printf("reading answer\n",dev);
    readtty_all(fd, buf, 256, &len);
    printf_buf(buf, len);
    sleep(1);

    hex2buf(apdu1,buf,&len);
    printf_buf(buf, len);
    writetty_all(fd, buf, len);
    sleep(1);
    printf("reading answer\n",dev);
    readtty_all(fd, buf, 256, &len);
    printf_buf(buf, len);
    sleep(1);

    hex2buf(apdu3,buf,&len);
    printf_buf(buf, len);
    writetty_all(fd, buf, len);
    sleep(1);
    printf("reading answer\n",dev);
    readtty_all(fd, buf, 256, &len);
    printf_buf(buf, len);
    sleep(1);

    
    closetty_spec(dev,fd);
}