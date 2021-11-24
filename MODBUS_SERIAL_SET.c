
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

#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))
#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)


static unsigned int calccrc(unsigned char crcbuf, unsigned int crc)
{
	unsigned char i = 0;

	crc ^= crcbuf;
	for(i = 0; i < 8; i++)
	{
		unsigned char chk;

		chk = crc & 1;
		crc >>= 1;
		crc &= 0X7FFF;
		if (1 == chk)
			crc ^= 0XA001;
		crc &= 0XFFFF;
	}

	return crc;
}

static unsigned int crc16(unsigned char *buf, unsigned short len)
{
	unsigned char hi = 0, lo = 0;
	unsigned int i = 0;
	unsigned int crc = 0xFFFF;

	for (i = 0; i < len; i++)
	{
		crc = calccrc(*buf, crc);
		buf++;
	}
	hi = crc & 0XFF;
	lo = crc >> 8;
	crc = (hi << 8) | lo;

	return crc;
}

void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -T [TTY-SERIAL-DEV] -B [BAUD-RATE] -S [SLAVE-ADDR] -R [REGISTER] -V [VALUE] -t [TIMEOUT]\n", file);
	printf("Example: %s -T/dev/ttyUSB0 -B57600 -S0X01 -R0X0001 -V0X0004 -t6\n", file), exit(1);
}

int main(int argc, char *argv[])
{
	char ttyname[256];
	int ttyfd = -1;
	struct termios term;

	int len = 0;
	int res;
	int baud = 57600;
	int timeout = 6;
	int slave = 1;
	int reg = 0X0000;
	unsigned char sb[8];
	unsigned char rb[1024];
	int i;
	time_t first;
	time_t now;
	int state = 0;
	fd_set rset;
	int maxfd;
	struct timeval tv;
	unsigned char *p = 0;
	//int bc = 1;
	unsigned short crc = 0;
	unsigned short w;
	int c;
	struct stat lbuf;
	int v = 0;
	int left = 0;
	int f = 0;
	int value = 0;


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	setbuf(stdout, 0);
	strcpy(ttyname, "/dev/ttyUSB0");
	if (argc < 3)											PrintUsage();
	while ((c = getopt(argc, argv, "B:t:T:t:R:S:V:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;
		case 'B':
			baud = atoi(optarg);
			if (baud != 9600 && baud != 19200 && baud != 38400 && baud != 57600)
				baud = 57600;
			break;
		case 't':
			timeout = atoi(optarg);
			if (timeout < 3)								timeout = 3;
			if (timeout > 30)								timeout = 30;
			break;
		case 'T':
			if (strlen(optarg) == 0)							PrintUsage();
			if(0 > stat(optarg, &lbuf))						PrintUsage();
			if(!S_ISCHR(lbuf.st_mode))						PrintUsage();
			strcpy(ttyname, optarg);
			break;
		case 'S':
			if (strlen(optarg) != 4)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &slave);
			else											res = sscanf(optarg, "%x", &slave);
			if (-1 == res)									PrintUsage();
			slave &= 0XFF;
			if (slave == 0)								PrintUsage();
			break;
		case 'R':
			if (strlen(optarg) != 6)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &reg);
			else											res = sscanf(optarg, "%x", &reg);
			if (-1 == res)									PrintUsage();
			reg &= 0XFFFF;
			break;
		case 'V':
			if (
				'0' == optarg[0] &&
				('X' == optarg[1] || 'x' == optarg[1])
			)
			{
				if (strlen(optarg) != 6)						PrintUsage();
				if ('X' == optarg[1])						res = sscanf(optarg, "%X", &value);
				else										res = sscanf(optarg, "%x", &value);
				if (-1 == res)								PrintUsage();
				value &= 0XFFFF;
				break;
			}
			value = atoi(optarg);
			value &= 0XFFFF;
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

	memset(&term, 0, sizeof(struct termios));

  #if defined(MAC)	//MSG_NOSIGNAL
	tcgetattr(ttyfd, &term);
  #endif // MAC

	term.c_cflag |= B19200, term.c_cflag |= CLOCAL, term.c_cflag |= CREAD, term.c_cflag &= ~PARENB, term.c_cflag &= ~CSTOPB, term.c_cflag &= ~CSIZE, term.c_cflag |= CS8, term.c_iflag = IGNPAR, term.c_cc[VMIN] = 1, term.c_cc[VTIME] = 0;

	if (baud == 9600)				cfsetispeed(&term, B9600), cfsetospeed(&term, B9600);
	else if (baud == 19200)			cfsetispeed(&term, B19200), cfsetospeed(&term, B19200);
	else if (baud == 38400)			cfsetispeed(&term, B38400), cfsetospeed(&term, B38400);
	else if (baud == 57600)			cfsetispeed(&term, B57600), cfsetospeed(&term, B57600);
	else							cfsetispeed(&term, B57600), cfsetospeed(&term, B57600);

	tcsetattr(ttyfd, TCSANOW, &term);
	if (0 > (v = fcntl(ttyfd, F_GETFL, 0)))						printf("(%s %d) FCNTL() F_GETFL(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	if (0 > fcntl(ttyfd, F_SETFL, v | O_NONBLOCK))				printf("(%s %d) FCNTL() F_SETFL,O_NONBLOCK(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	tcflush(ttyfd, TCIFLUSH);


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	sb[0] = slave;
	sb[1] = (unsigned char)6;
	sb[2] = (unsigned char)(reg >> 8);
	sb[3] = (unsigned char)(reg & 0XFF);
	sb[4] = (unsigned char)(value >> 8);
	sb[5] = (unsigned char)(value & 0XFF);
	sb[6] = (unsigned char)(crc16(sb, 6) >> 8);
	sb[7] = (unsigned char)(crc16(sb, 6) & 0XFF);

	left = 8, p = sb;
	while (left > 0)
	{
		len = write(ttyfd, p, left);
		p += len, left -= len;
	}

	printf("MODBUS (PRESET,0X06) SERIAL REQ: ");
	for (i = 0; i < 8; i++)
	{
		if (i == 8 - 1)			printf("%02X\n", sb[i]);
		else					printf("%02X-", sb[i]);
	}
	printf("> \n");
	printf("[SLAVE-ADDR]=0X%02X [FN]=0X%02X [REGISTER]=0X%04X [VALUE]=0X%04X,%u [BAUD]=%d [TIMEOUT]=%d\n", slave, sb[1], reg, value, value, baud, timeout);
	for (i = 0; i < 8; i++)
	{
		if (i == 8 - 1)		printf("%02X\n", sb[i]);
		else				printf("%02X-", sb[i]);
	}

	state = 0;
	time(&first);
	do
	{
		time(&now);
		if ((int)(now - first) > timeout)
		{
			printf("\nTIMEOUT\n"), close(ttyfd), exit(1);
			break;
		}
		FD_ZERO(&rset);
		FD_SET(ttyfd, &rset);
		maxfd = 0;
		if (maxfd < ttyfd)											maxfd = ttyfd;
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 1;
		res = select(maxfd + 1, &rset, 0, 0, &tv);
		if (0 > res)			printf("\nSELECT FAIL\n"), close(ttyfd), exit(1);
		if (0 == res)			continue;
		if (0 == state)
		{
			p = rb, memset(p, 0, sizeof(rb));
			len = read(ttyfd, p, 1);
			if (len <= 0)		printf("ERR & QUIT\n"), close(ttyfd), exit(1);
			if (0 == *p)
			{
				state = 0;
				printf("1");
				continue;
			}
			if (slave != (int)(*p))
			{
				state = 0;
				printf("1");
				continue;
			}
			p++, state = 1;
			printf(".");
			continue;
		}
		if (1 == state)
		{
			len = read(ttyfd, p, 1);
			if (len <= 0)			printf("ERR & QUIT\n"), close(ttyfd), exit(1);
			if (sb[1] != *p)
			{
				state = 0;
				printf("6");
				continue;
			}
			p++, state = 2;
			printf(".");
			continue;
		}
		if (2 == state)
		{
			len = read(ttyfd, p, 1);
			if (len <= 0)			printf("ERR & QUIT\n"), tcflush(ttyfd, TCIFLUSH), close(ttyfd), exit(1);
			if (0 == f)
			{
				p++, f = 1;
				printf(".");
				continue;
			}
			f = 0;
			if (BUILD_UINT16(rb[3], rb[2]) != reg)
			{
				printf(" (X REG=0X%04X) ", BUILD_UINT16(rb[3], rb[2]));
				state = 0;
				continue;
			}
			p++;
			printf(".");
			state = 3, left = 4;
		}

		if (0 == f)				f = 1, left = 4;
		len = read(ttyfd, p, left);
		if (len <= 0)		printf("ERR & QUIT\n"), close(ttyfd), exit(1);
		p += len, left -= len;
		if (left <= 0)
		{
			crc = crc16(rb, 6);
			if (BUILD_UINT16(rb[7], rb[6]) == crc)
			{
				for (i = 0; i < len; i++)		printf(".");
				printf(" (CRC OK) ");
				break;
			}
			printf(" (CRC ERR) ");
			for (i = 0; i < len; i++)			printf("x");
			printf("\n");
			tcflush(ttyfd, TCIFLUSH);
			close(ttyfd), exit(1);
		}
	}
	while(1);
	tcflush(ttyfd, TCIFLUSH);
	close(ttyfd);

	for (i = 0; i < 8; i++)					printf(".");
	printf("\n");
	printf("MODBUS (PRESET,0X06) SERIAL RSP: ");
	for (i = 0; i < 8; i++)
	{
		if (i == 8 - 1)						printf("%02X\n", rb[i]);
		else								printf("%02X-", rb[i]);
	}
	printf("[REG]=0X%04X ", reg);
	printf("[SLAVE]=0X%02X ", rb[0]);
	printf("[FN]=0X%02X ", rb[1]);
	w = BUILD_UINT16(rb[5], rb[4]);
	printf("[VALUE]=0X%04X,%05u,%0*d ", w, w, 6, (short)w);
	printf("[CRC16]=0X%04X\n", crc);

	printf("\n");
	return 0;
}


