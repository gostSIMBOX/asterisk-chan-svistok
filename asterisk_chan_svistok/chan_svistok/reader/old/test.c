#include "reader_core.h"
#include "../programmator/tty_v2.c"

char ATR[]="3B1695D0016B2F0D00";


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
	printf("%02X ",buf[i]);
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

int emu_send_atr(int fd)
{
    char buf[256];
    int len;

    hex2buf(ATR,buf,&len);

    printf("Sending ATR\n");
    printf_buf(buf, len);
//    nsleep(SIMTIMERESET);
    writetty_all(fd, buf, len);
//    nsleep(SIMTIMERESET);
    printf("Ok\n");

}

int emu_send_apdu(int fd, char* apdu)
{
    char buf[256];
    int len;

    hex2buf(apdu,buf,&len);

    printf("Sending APDU\n");
    printf_buf(buf, len);
    nsleep(SIMTIMERESET);
    writetty_all(fd, buf, len);
    nsleep(SIMTIMERESET);
    printf("Ok\n");

}


int emu_init(char * dev)
{
    int r=0;

    int flags;
    int flags_before;
    int fd=0;
    int dsr_was;
    int dsr_now;

    char buf[256];
    int len;

    printf("emu start on %s\n", dev);

    fd=opentty_spec(dev,1);
    if(fd==0) return fd;
    printf("connected to %s on %d\n", dev, fd);


//    ioctl(fd, TIOCMGET, TIOCM_DSR);
    
while(1)
    {

	readtty_all(fd, buf, 256, &len);
	if(len>0)
{
        printf("reading buffer\n");
        printf_buf(buf, len);
}

        flags_before=flags;
        ioctl(fd, TIOCMGET, &flags);
        if (flags_before!=flags)
        printf("flags was %d, now %d\n", flags_before,flags);


	dsr_was=flags_before&TIOCM_DSR;
	dsr_now=flags&TIOCM_DSR;

	if(dsr_was!=dsr_now)
        printf("dsr was %d, now %d\n", dsr_was,dsr_now);

	if((dsr_was==0)&&(dsr_now==TIOCM_DSR)) 
	{
	    if(flags_before==0) printf("INIT\n");
            printf("RESET FOUND\n");
	    r++;
	    if(r>1) 
	    {
		
		emu_send_atr(fd);
		readtty_all(fd, buf, 256, &len);
    		printf_buf(buf, len);
		emu_send_apdu(fd,"9000");
		readtty_all(fd, buf, 256, &len);
    		printf_buf(buf, len);
		emu_send_apdu(fd,"9000");
		readtty_all(fd, buf, 256, &len);
    		printf_buf(buf, len);

		 printf("loop\n");
		 while(1)
		{

	readtty_all(fd, buf, 256, &len);
	if(len>0)
    	    printf_buf(buf, len);

		}
	    }
	}

//        ioctl(fd, TIOCMIWAIT, TIOCM_DSR | TIOCM_CD );
//        printf("WAIT END\n");
    }

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

int sim_send(int fd, char* cmd, int len)
{
    return 0;
}

int sim_read(int fd, char* buf, int len)
{
    return 0;
}



/*

 Функция выполнения команды СИМ-ки

 Коды ошибок:
 01 - ошибка записи первых 5-ти байт команды
 02 - ошибка чтения локального эха первых 5-ти байт
 03 - ошибка чтения первого байта ответа карты
 04 - ошибка чтения второго байта ответа карты NINO
 05!- длина выходных данных команды превысила размер буфера BufferSize
 06 - ошибка чтения выходных данных команды NIFO
 07 - ошибка чтения второго байта ответа карты NIFO
 08 - ошибка записи параметров команды FINO
 09 - ошибка чтения локального эха параметров команды FINO
 0A - ошибка чтения второго байта ответа карты FINO
 80 - FIX-ME!!! Недоделано! NIFO
 81 - FIX-ME!!! Недоделано! FINO

*/

int sim_cmd(int fd, char *cmd, int cmd_len, char *res, int *res_len)
{
   int r;
   int readcount;
   char buf[1024];
   *res_len=0;

   if(cmd_len<5) return 1;
   r=sim_send(fd,cmd,5);
   if(r!=5) return 1;

   //Считаем локальное эхо
   r=sim_read(fd,buf,5);
   if(r!=5) {printf("wrong echo");printf_buf(buf,r); return 2;}

   //Цикл обработки ответа карты}
   //u=true;
   //while(u)
   //{
    r=sim_read(fd,buf,1);
    if(r!=1) return 3;


    if((cmd[0]=0x04)||(cmd[0]=0x44)||(cmd[0]=0xFA))
    { 
     //NoInput/NoOutput
     res[0]=buf[0];
     r=sim_read(fd,buf,1);
     if(r!=1) return 4;
     res[1]=buf[1];
     *res_len=2;
     return 0;
    }

    if((cmd[0]=0xF2)||(cmd[0]=0xB0)||(cmd[0]=0xB2)||(cmd[0]=0xC0))
    { 
     //NoInput/FixedOutput
     if(buf[0]==0x60)
     {
	if((buf[0]==cmd[0])||(buf[0]==cmd[0]+1))
	{
	    //Считаем результаты
	    readcount=cmd[3];
	    if(readcount==0) readcount=256;
	    readcount+=2;
            //if ReadCount>BufferSize  return 5;
	    r=sim_read(fd,res,readcount);
	    *res_len=r;
	    if (r!=readcount) return 6;
	    return 0;
	} else {
	    return 80;
	}
     } else {
	 // 60 - error?
         res[0]=buf[0];
	 r=sim_read(fd,buf,1);
         if(r!=1) return 4;
         res[1]=buf[1];
         *res_len=2;
         return 0;
     }
    }

    if((cmd[0]=0xD6)||(cmd[0]=0xDC)||(cmd[0]=0x20)||(cmd[0]=0x24)||(cmd[0]=0x26)||(cmd[0]=0x28)||(cmd[0]=0x2C)||(cmd[0]=0xA4)||(cmd[0]=0xA2)||(cmd[0]=0x32)||(cmd[0]=0x88))
    { 
     //FixedInput/NoOutput
     if(buf[0]==0x60)
     {
	if((buf[0]==cmd[0])||(buf[0]==cmd[0]+1))
	{
	    //Считаем результаты
	    readcount=cmd[3];
	    if(readcount==0) readcount=256;
	    readcount+=2;
            //if ReadCount>BufferSize  return 5;
	    r=sim_read(fd,res,readcount);
	    *res_len=r;
	    if (r!=readcount) return 6;
	    return 0;
	} else {
	    return 80;
	}
     } else {
	 // 60 - error?
         res[0]=buf[0];
	 r=sim_read(fd,buf,1);
         if(r!=1) return 4;
         res[1]=buf[1];
         *res_len=2;
         return 0;
     }
    }




/*

    if (Command=$D6) or (Command=$DC) or (Command=$20) or (Command=$24) or
     (Command=$26) or (Command=$28) or (Command=$2C) or (Command=$A4) or
     (Command=$A2) or (Command=$32) or (Command=$88) then begin
     {FixedInput/NoOutput}
     Pr_Cmd:=True;
     if AnswerB<>$60 then begin
      Pr:=False;
      if (AnswerB=Command) or (AnswerB=(Command+1)) then begin
       Pr:=True;
       {Запишем параметры}
       WriteCount:=P3;
       FillChar(WBuffer[0],BufferSize,$0);
       Move(Parameter[0],WBuffer[0],WriteCount);
       WriteC:=WriteCard(Port,WBuffer,WriteCount);
       if WriteC<>WriteCount then begin
        Error:=$08;
        RunCommand:=ResBuffer;
        Exit;
       end;
       {Считаем локальное эхо}
       FillChar(RBuffer[0],BufferSize,$0);
       ReadC:=ReadCard(Port,RBuffer,WriteCount);
       if ReadC<>WriteCount then begin
        Error:=$09;
        RunCommand:=ResBuffer;
        Exit;
       end;
      end;
      if (AnswerB=(not Command)) or (AnswerB=(not (Command+1))) then begin
       {FIX-ME!!! Инверсия ответа карты}
       Error:=$81;
       RunCommand:=ResBuffer;
       Exit;
      end;
      if Pr=False then begin
       ResBuffer[0]:=AnswerB;
       FillChar(RBuffer[0],BufferSize,$0);
       ReadC:=ReadCard(Port,RBuffer,1);
       if ReadC<>1 then begin
        Error:=$0A;
        RunCommand:=ResBuffer;
        Exit;
       end;
       ResBuffer[1]:=RBuffer[0];
       U:=False;
      end;
     end;
    end; 
    if Pr_Cmd=False then begin
     {Unknown}
     ResBuffer[0]:=AnswerB;
     FillChar(RBuffer[0],BufferSize,$0);
     ReadC:=ReadCard(Port,RBuffer,0);
     if ReadC=BufferSize then ReadC:=ReadC-1;
     if ReadC<>0 then begin
      Move(RBuffer[0],ResBuffer[1],ReadC);
     end;
     U:=False;
    end;       
   end;
   RunCommand:=ResBuffer;
  end;
*/

}


void main()
{
    char* dev="/dev/ttyUSB24";

//    char* dev="/dev/ttyUSB33";
    char buf[256];
    int len;

    char* apdu1="A0A4000002";
    char* apdu2="7F4D";
    char* apdu3="8F0D";

    char* apdu4="A0B0000016";
    char* apdu5="A0B20104B001";


    
    int fd;
    int i;

//    fd=emu_init(dev);  return;

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

    hex2buf(apdu5,buf,&len);
    printf_buf(buf, len);
    writetty_all(fd, buf, len);
    sleep(1);
    printf("reading answer\n",dev);
    readtty_all(fd, buf, 256, &len);
    printf_buf(buf, len);
    sleep(1);

    
    closetty_spec(dev,fd);
}


