#include "reader_core.c"

void main()
{
    char* dev="/dev/ttyUSB25";

    int fd;
    int i;

    fd=emu_init(dev);  return;
    closetty_spec(dev,fd);
}


