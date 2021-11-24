
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <netinet/tcp.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>



//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////


void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -T [TTY-SERIAL-DEV] -M [FORMAT] -D [DURATION] -f [FILE]\n", file);
	printf("Example: %s -T /dev/ttyUSB0 -M 0X19 -D 0XFF -f/dev/fd/0\n", file), exit(1);
}

int main(int argc, char *argv[])
{
	char ttyname[256];
	int ttyfd = -1;
	struct termios term;
	unsigned char buf[1024 * 4];

	struct stat lbuf;
	int fmt = 0X19, dura = 0X0A;
	char fname[256];
	unsigned char fbuf[1024 * 4], *p;
	unsigned char cs = 0;

	int fd = -1;
	int v = 0;
	int j = 0;
	int len = 0;
	int l = 0;
	int left = 0;
	int c;
	int res;


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	setbuf(stdout, 0);
	strcpy(fname, "/dev/fd/0");
	strcpy(ttyname, "/dev/ttyUSB0");
	if (argc < 3)											PrintUsage();
	while ((c = getopt(argc, argv, "t:T:m:M:d:D:f:F:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;
		case 't':
		case 'T':
			if (strlen(optarg) == 0)							PrintUsage();
			if(0 > stat(optarg, &lbuf))						PrintUsage();
			if(!S_ISCHR(lbuf.st_mode))						PrintUsage();
			strcpy(ttyname, optarg);
			break;
		case 'f':
		case 'F':
			if (strlen(optarg) == 0)							PrintUsage();
			if(0 > stat(optarg, &lbuf))						PrintUsage();
			strcpy(fname, optarg);
			break;
		case 'm':
		case 'M':
			if (strlen(optarg) != 4)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &fmt);
			else											res = sscanf(optarg, "%x", &fmt);
			if (-1 == res)									PrintUsage();
			fmt &= 0XFF;
			if (0X17 != fmt && 0X18 != fmt && 0X19 != fmt)	fmt = 0X19;
			break;
		case 'd':
		case 'D':
			if (strlen(optarg) > 4)							PrintUsage();
			if ('0' == optarg[0] && ('x' == optarg[1] || 'X' == optarg[1]))
			{
				if ('X' == optarg[1])						res = sscanf(optarg, "%X", &dura);
				else										res = sscanf(optarg, "%x", &dura);
				if (-1 == res)								PrintUsage();
			}
			else
			{
				dura = atoi(optarg);
			}
			dura &= 0XFF;
			break;
		}
	}


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	if (0 > (ttyfd = open(ttyname, O_RDWR | O_NOCTTY | O_NDELAY)))
		printf("(%s %d) SERIAL DEVICE( \"/dev/tty[M|S|USB][0-9]\") OPEN() fail, please run as root and retry later, EXIT \n", __FILE__, __LINE__), exit(1);
	if (!isatty(ttyfd))
		printf("(%s %d) DEVICE( \"%s\"), ttyfd=%d, not ISATTY(), EXIT \n", __FILE__, __LINE__, ttyname, ttyfd), close(ttyfd), exit(1);


	if (0 > (fd = open(fname, O_RDONLY)))					printf("OPEN FAIL, `%s` , PLEASE RUN as ROOT !\n", fname), exit(1);
	p = fbuf, memset(p, 0, sizeof(fbuf));
	while (1)
	{
		if (0 > (l = read(fd, p, 80)))							printf("READ FAIL, %s \n", fname), close(fd), close(ttyfd), exit(1);
		if (0 == l)										break;
		p += l;
	}
	len = p - fbuf;
	if (len > 2048)											printf("FAIL, %s FILE SIZE > 1024\n", fname), close(fd), close(ttyfd), exit(1);
	close(fd);

	memset(&term, 0, sizeof(struct termios));

  #if defined(MAC)	//MSG_NOSIGNAL
	tcgetattr(ttyfd, &term);
  #endif // MAC

	term.c_cflag |= B19200, term.c_cflag |= CLOCAL, term.c_cflag |= CREAD, term.c_cflag &= ~PARENB, term.c_cflag &= ~CSTOPB, term.c_cflag &= ~CSIZE, term.c_cflag |= CS8, term.c_iflag = IGNPAR, term.c_cc[VMIN] = 1, term.c_cc[VTIME] = 0;
	cfsetispeed(&term, B19200), cfsetospeed(&term, B19200);
	tcsetattr(ttyfd, TCSANOW, &term);
	if (0 > (v = fcntl(ttyfd, F_GETFL, 0)))						printf("(%s %d) FCNTL() F_GETFL(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	if (0 > fcntl(ttyfd, F_SETFL, v | O_NONBLOCK))				printf("(%s %d) FCNTL() F_SETFL,O_NONBLOCK(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	tcflush(ttyfd, TCIFLUSH);


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	p = buf, memset(p, 0, sizeof(buf));
	*p = 0XFF, p++;
	*p = 0X07, p++;
	*p = 0XF8, p++;

	for (j = 0; j < len; j++)
	{
		*p = fbuf[j], p++;
		cs ^= fbuf[j];
	}

	*p = 0, cs ^= 0, p++;
	*p = cs, p++;

	*p = 0XFF, p++;
	*p = fmt, p++;
	*p = (0XFF ^ fmt), p++;
	*p = dura, p++;
	*p = dura, p++;
	*p = 0XFF, p++;
	*p = 0XF4, p++;
	*p = 0X0B, p++;

	left = (p - buf);
	p = buf;
	while (left > 0)
	{
		l = write(ttyfd, p, left);
		if (0 > l)											printf("TTYFD WRITE FAIL, %s \n", ttyname), close(ttyfd), exit(1);
		if (0 == l)										break;
		p += l, left -= l;
	}
	close(ttyfd);

	printf("L16_SERIAL_LIVEPLAY DONE \n");
	return 0;
}


