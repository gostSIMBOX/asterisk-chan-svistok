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

#/* return length of lockname */
static int lock_build(const char * devname, char * buf, unsigned length)
{
	const char * basename;
	char resolved_path[PATH_MAX];

	/* follow symlinks */
	if(realpath(devname, resolved_path) != NULL)
		devname = resolved_path;
	
/*
	while(1)
	{
		len = readlink(devname, symlink, sizeof(symlink) - 1);
		if(len <= 0)
			break;
		symlink[len] = 0;
		if(symlink[0] == '/')
			devname = symlink;
		else
		{
			// TODO
			memmove()
			memcpy(symlink, devname);
		}
	}
*/

	basename = strrchr(devname, '/');
	if(basename)
		basename++;
	else
		basename = devname;

	/* TODO: use asterisk build settings for /var/lock */
	return snprintf(buf, length, "/var/lock/LOCK..%s", basename);
}

#/* return 0 on error */
static int lock_create(const char * lockfile)
{
	int fd;
	int len = 0;
	char pidb[21];
	
	fd = open(lockfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IROTH);
	if(fd >= 0)
	{
		len = snprintf(pidb, sizeof(pidb), "%d", getpid());
		len = write(fd, pidb, len);
		close(fd);
	}
	return len;
}

#/* return pid of owner, 0 if free */
static int lock_try(const char * devname)
{
	int fd;
	int len;
	int pid = 0;
	char name[256];
	char pidb[21];

	lock_build(devname, name, sizeof(name));

	/* FIXME: rise condition: some time between lock check and got lock */
	fd = open(name, O_RDONLY);
	if(fd >= 0)
	{
		len = read(fd, pidb, sizeof(pidb) - 1);
		if(len > 0)
		{
			pidb[len] = 0;
			len = strtol(pidb, NULL, 10);
			if(kill(len, 0) == 0) 
				pid = len;
		}
		close(fd);
	}
	if(pid == 0)
	{
		unlink(name);
		lock_create(name);
	}
	return pid;
}



int opentty (const char* dev)
{
    return opentty_spec (dev, 0);
}


int opentty_spec (const char* dev, int param)
{
	int		pid;
	int		fd;
	struct termios	term_attr;

printf("1\n");
	if(param==1)
	{
		fd = open (dev, O_RDWR | O_NOCTTY | O_NDELAY);
	} else
	{
		fd = open (dev, O_RDWR | O_NOCTTY);
	}

/*	return fd;*/
printf("2\n");
	if (fd < 0)
	{
		return -1;
	}
	
printf("3\n");
	if (tcgetattr (fd, &term_attr) != 0)
	{
		close(fd);
		return -1;
	}

printf("4\n");
	term_attr.c_cflag = B115200 | CS8 | CREAD | CRTSCTS;

	if(param==1)
	{
	    term_attr.c_cflag = B9600 | PARENB | CSTOPB | CS8 | CREAD; // | CRTSCTS
	}

	term_attr.c_iflag = 0;
	term_attr.c_oflag = 0;
	term_attr.c_lflag = 0;
	//term_attr.c_cc[VMIN] = 1;
	//term_attr.c_cc[VTIME] = 0;

	if (tcsetattr (fd, TCSAFLUSH, &term_attr) != 0)
	{
	}
printf("5\n");
/*
	if(param==1)
	{
	    pid = lock_try(dev);
	    if(pid != 0)
	    {
	    	close(fd);
	    	return -1;
	    }
	}
*/

	return fd;
}

#/* */
void closetty(const char * dev, int fd)
{
	char name[256];

	close(fd);

	/* remove lock */
/* !!!
	lock_build(dev, name, sizeof(name));
	unlink(name);
!!! */
}

void closetty_spec(const char * dev, int fd)
{
	char name[256];

	close(fd);

	/* remove lock */
/*
	lock_build(dev, name, sizeof(name));
	unlink(name);
*/
}

#/* */


/*
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

    return r;
}

*/


/*
void saveparami(const char*device, char*name, int value)
{
}
*/