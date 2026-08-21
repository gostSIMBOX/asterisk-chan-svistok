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
    printf("%s %s=%s\n",device,name,value);
}

void saveparami(const char*device, char*name, int value)
{
    FILE * fd;
    char filename[256]="/var/svistok/devices/";
    strcat(filename,device);
    strcat(filename,".");
    strcat(filename,name);

    fd=fopen(filename,"w");
    if (fd)
    {
    fprintf(fd,"%d",value);
        fclose(fd);
    }
    printf("%s %s=%d\n",device,name,value);
}
