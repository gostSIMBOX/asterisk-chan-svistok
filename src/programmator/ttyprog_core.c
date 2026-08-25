#include <stdio.h>
#include <termios.h>			/* struct termios tcgetattr() tcsetattr()  */
#include <fcntl.h>			/* O_RDWR O_NOCTTY */
#include <sys/stat.h>			/* S_IRUSR | S_IRGRP | S_IROTH */
#include <sys/ioctl.h>			
#include <string.h>			/* strrchr() */
#include <limits.h>			/* PATH_MAX */
#include <stdlib.h>			/* realpath() */
#include <unistd.h>			/* getpid() */
#include <errno.h>			/* errno */
#include <signal.h>			/* kill() */

#include <stdarg.h>

#include "crc.c"


//---- imei
unsigned char cmdi0[]={0x27,0x26,0x02,0x08,
	0x0A,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, // imei
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,      // 00 =  7*16+8 = 15*8 = 120
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x99,0x99,                                   //crc [133][134]
	0x7e						//7e [135] total=136
};                         


//---- cfun
unsigned char cmd_cfun11[]={0x29 ,0x02 ,0x00 ,0x59 ,0x6A ,0x7E};                          // >> AT+CFUN=1,1


//---- Перевод в диаг режим
unsigned char cmd0[]={0x41,0x54,0x24,0x51,0x43,0x44,0x4D,0x47,0x0D,0x0A};                          // >> AT$QCDMG..
unsigned char res0[]={0x24,0x51,0x43,0x44,0x4D,0x47,0x0D,0x0D,0x0A,0x4F,0x4B,0x0D,0x0A};           // << $QCDMG...OK..

unsigned char cmd1[]={0x41,0x54,0x5E,0x44,0x49,0x53,0x4C,0x4F,0x47,0x3D,0x32,0x35,0x35,0x0D,0x0A,0x7E,0x0C,0x14,0x3A,0x7E,0x0C,0x14,0x3A,0x7E}; // >> AT^DISLOG=255..~..:~..:~
unsigned char res1[]={0xD2,0x7A,0x7E};                                                                                                          // << Тz~

// ---- пауза и хрень
unsigned char cmd2[]={0x7E,0x0C,0x14,0x3A,0x7E}; 					// >> ~..:~
unsigned char res2[]={0xD2,0x7A,0x7E};							// << Тz~

//---- пауза и хрень повтор

//---- пауза и хрень2  (Получить ESN на CDMA?????)
unsigned char cmd4[]={0x01,0xF1,0xE1,0x7E};						// >> .сб~
unsigned char res4[]={0xD0,0xAD,0xDE,0xC7,0xE0,0x7E};					// << Р�ЮЗа~

//---- запрос версии прошивки и проца DIAG_SUBSYS_CMD_F
unsigned char cmd5[]={0x4B,0xC9,0x03,0xFF,0xE8,0x99,0x7E};				// >>  KЙ.яи™~
unsigned char res5[]={0x03,0xFF,0x31,0x31,0x2E,0x36,0x30,0x39,0x2E,0x31,0x38,0x2E,0x30,0x30,0x2E,0x30,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x45,0x31,0x35,0x35,0x30,0x46,0x43,0x44,0x36,0x41,0x54,0x43,0x50,0x55,0x20,0x56,0x65,0x72,0x2E,0x41,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x89,0x93,0x7E}; // << .я11.609.18.00.00...............E1550FCD6ATCPU Ver.A............‰“~

unsigned char cmd6[]={0xFA,0x13,0x00,0x00,0xAA,0x3D,0x7E};				// >> ъ...Є=~
unsigned char res6[]={0x00,0x00,0xAA,0x3D,0x7E};					// <<..Є=~

//----- Подготовка к жаре (7E 0C 14 3A 7E см ранее - запрос режима)
unsigned char cmd7[]={0x7E,0xFB,0x24,0xB9,0x7E};					// >> ~ы$№~
unsigned char res7[]={0xE7,0x97,0x7E};							// << з—~

//----- Перевод в режим программирования, РЕСТАРТ!
//--?проверка ready???
unsigned char cmd8[]={0x7E,0x3A,0xA1,0x6E,0x7E};					// >> ~:Ўn~
unsigned char res8[]={0x6E,0x7E};							// << n~


/*
----- Подготовка к жаре (7E 0C 14 3A 7E см ранее - запрос режима)
000065 09:40:47.250  COM17  << 7E 0C 14 3A 7E                                  ~..:~
000066 09:40:47.265  COM17  >> 4D 36 32 78 78 2D 4F 45 4D 53 42 4C 1A C0 7E    M62xx-OEMSBL.А~

--?проверка ready???
000067 09:40:47.281  COM17  << 7E 3A A1 6E 7E                                  ~:Ўn~
000068 09:40:47.281  COM17  >> 6E 7E                                           n~

--Запрос модели???
000069 09:40:47.296  COM17  << 7E 45 D1 E5 7E                                  ~EСе~
000070 09:40:47.296  COM17  >> 55 41 57 45 49 20 45 31 35 35 00 00 00 00 00 00 UAWEI E155......
                               00 00 00 C6 1C 7E                               ...Ж.~

000072 09:40:48.437  COM17  << 7E 41 00 01 00 00 00 00 01 D0 00 00 00 F0 6D 7E ~A.......Р...рm~
000073 09:40:48.484  COM17  >> 6A D3 7E                                        jУ~
*/

void tty_log(FILE* fd,char *fmt, ...);
size_t writetty_all (int fd, const char* buf, size_t count);
int readtty_all(int fd, char* buf, int maxsize, int *readed);


void saveparams(const char*device, char*name, char*value);
void saveparami(const char*device, char*name, int value);

void ttyprog_write_read(int fd,char*cmd,int size)
{
    unsigned char readbuf[1024];
    int readed;
    int i;

    tty_log(NULL,"\n>>tty>>[%d]\n\n",size);
    writetty_all(fd,cmd,size);
    msleep(500);
    readtty_all(fd,readbuf,sizeof(readbuf),&readed);
    tty_log(NULL,"\n<<tty<<[%d]\n\n",readed);

    msleep(500);
    readtty_all(fd,readbuf,sizeof(readbuf),&readed);
    tty_log(NULL,"\n<<tty<<[%d]\n\n",readed);

    msleep(500);
    readtty_all(fd,readbuf,sizeof(readbuf),&readed);
    tty_log(NULL,"\n<<tty<<[%d]\n\n",readed);

    //tty_log(NULL,readbuf,readed,1,stdout);
    //fwrite(readbuf,readed,1,stdout);
//    for (i=0;i<readed;i++)
//	tty_log(NULL,"\n%c",readbuf[i]);
//    tty_log(NULL,"\n",readed);
    //printf("\n");

}


int msleep(unsigned long milisec)

{

    struct timespec req={0};

    time_t sec=(int)(milisec/1000);

    milisec=milisec-(sec*1000);

    req.tv_sec=sec;

    req.tv_nsec=milisec*1000000L;

    while(nanosleep(&req,&req)==-1)

        continue;

    return 1;

}


void ttyprog_set_diagmode(int fd)
{
    if(fd<0)
    {
	tty_log(NULL,"No pointer!");
	return;
    }
    ttyprog_write_read(fd,cmd0,sizeof(cmd0));
    ttyprog_write_read(fd,cmd1,sizeof(cmd1));
    ttyprog_write_read(fd,cmd2,sizeof(cmd2));
    ttyprog_write_read(fd,cmd2,sizeof(cmd2));
    ttyprog_write_read(fd,cmd4,sizeof(cmd4));
    ttyprog_write_read(fd,cmd5,sizeof(cmd5));
    ttyprog_write_read(fd,cmd6,sizeof(cmd6));
    ttyprog_write_read(fd,cmd7,sizeof(cmd7));
    ttyprog_write_read(fd,cmd8,sizeof(cmd8));
}


void ttyprog_changeimei(int fd ,char * newimei)
{
    int i;
    int c;
    int m[15];

    if(fd<0)
    {
	tty_log(NULL,"No pointer!");
	return;
    }
//    for (i=0;i<8;i++)
//	cmdi0[4+i]=0;
//    cmdi0[4+i]=0x0A;

    for(i=0;i<15;i++)
	m[i]=newimei[i]-'0';

    cmdi0[4]=m[0]*16+0x0A;
    cmdi0[5]=m[2]*16+m[1];
    cmdi0[6]=m[4]*16+m[3];
    cmdi0[7]=m[6]*16+m[5];
    cmdi0[8]=m[8]*16+m[7];
    cmdi0[9]=m[10]*16+m[9];
    cmdi0[10]=m[12]*16+m[11];
    cmdi0[11]=m[14]*16+m[13];

    crcdat(cmdi0,133,cmdi0+133);

//    for(i=0;i<136;i++)
//	putchar(cmdi0[i]);


//    ttyprog_write_read(fd,cmd0,sizeof(cmd0));
    ttyprog_write_read(fd,cmdi0,sizeof(cmdi0));
//    ttyprog_write_read(fd,cmd_cfun11,sizeof(cmd_cfun11));
}






size_t writetty_all (int fd, const char* buf, size_t count)
{
	ssize_t out_count;
	size_t total = 0;
	unsigned errs = 10;
	
	while (count > 0)
	{
		out_count = write (fd, buf, count);
		if (out_count <= 0)
		{
			if(errno == EINTR || errno == EAGAIN)
			{
				errs--;
				if(errs != 0)
					continue;
			}
			break;
		}
		errs = 10;
		count -= out_count;
		buf += out_count;
		total += out_count;
	}
	return total;
}




int readtty_all(int fd, char* buf, int maxsize, int *readed)
{
    *readed=0;
    int bytes;
    int r;

    r=ioctl(fd,FIONREAD, &bytes);

    if(r==-1)
    {
	return r;
    }

    if(bytes>0)
    {
	*readed = read(fd, buf, maxsize);
    }

    return *readed;
}



int readtty_blocked(int fd, char* buf, int maxsize, int *readed)
{
    *readed=0;
    int bytes;
    int r;

/*
    r=ioctl(fd,FIONREAD, &bytes);

    if(r==-1)
    {
	return r;
    }

    if(bytes>0)
    {*/

	*readed = read(fd, buf, maxsize);
/*
    }*/

    return *readed;
}











void ttyprog_sendfile(char*filename, int fd, char* device)
{
    long int cnt;
    long int cnt_pause;
    long int cnt_total;

    int readed;

    FILE * fdf;

    char buf[4200];
    char readbuf[4000];

    fdf=fopen(filename,"rb");
    if(!fdf)
    {
	printf("Cannot open file");
	return;
    }

    cnt=0;
    cnt_pause=0;
    fseek(fdf, 0, SEEK_END);
    cnt_total= ftell(fdf);
    fseek(fdf, 0, SEEK_SET);

    while(!feof(fdf))
    {
	readed=fread(buf,1,sizeof(buf),fdf);
        writetty_all(fd, buf, readed);

        cnt+=readed;

        readtty_blocked(fd,readbuf,sizeof(readbuf),&readed);
        tty_log(NULL,"\n<<[%d]\n\n",readed);
        fwrite(readbuf,readed,1,stdout);
        printf("\n");
	msleep(25);

    if (cnt-cnt_pause>200000)
    {
      printf ("\ncnt=[%ld/%ld]\n",cnt,cnt_total);
      saveparami(device,"progress",(int)(cnt*80/cnt_total)+20);
      sleep(1);
      cnt_pause=cnt;
    }


    }
}


/*
    if (cnt-cnt_pause>200000)
    {
      printf ("\n[%ld]",cnt);
      sleep(1);
      cnt_pause=cnt;
    }

    //if ((cnt%5000)==0)
    //{
     // printf ("+\n");
    //}

    }

    printf ("Done. Wait 1 Sec\n");
    sleep(1);
    
    */
