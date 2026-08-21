#include "reader_core.c"

void main()
{
    char* dev="/dev/ttyUSB24";

    char buf[256];
    int len;

    char* apdu1="A0A4000002";
    char* apdu2="7F4D";
    char* apdu3="8F0D";

    char* apdu4="A0B0000016";
    char* apdu5="A0B20104B001";


    
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


