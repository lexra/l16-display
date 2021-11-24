
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



#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((unsigned int)((unsigned int)((Byte0) & 0x00FF) + ((unsigned int)((Byte1) & 0x00FF) << 8) \
			+ ((unsigned int)((Byte2) & 0x00FF) << 16) + ((unsigned int)((Byte3) & 0x00FF) << 24)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((unsigned char)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)

#define BREAK_UINT32(var, ByteNum) \
          (unsigned char)((unsigned int)(((var) >>((ByteNum) * 8)) & 0x00FF))



//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

typedef struct tm SYSTEMTIME;

void GetLocalTime(SYSTEMTIME *st)
{
	struct tm *pst = NULL;
	time_t t = time(NULL);
	pst = localtime(&t);
	memcpy(st, pst, sizeof(SYSTEMTIME));
	st->tm_year += 1900;
}

int PackAfCmd(unsigned short addr, unsigned short cId, unsigned char opt, unsigned char radius, unsigned char dataLen, unsigned char *pdata)
{
	unsigned char buf[320];
	unsigned char len = (int)dataLen + 10;

	if (0X00 != opt && 0X10 != opt && 0X20 != opt && 0X30 != opt)
		return 0;
	if (len > 0X93)
		return 0;
	memset(buf, 0, 320);
	srand((unsigned)time(NULL));

	buf[0] = (unsigned char)len;
	buf[1] = 0X24;
	buf[2] = 0X01;
	memcpy(&buf[3], &addr, sizeof(unsigned short));
	buf[5] = 103;
	buf[6] = 103;
	memcpy(&buf[7], &cId, sizeof(unsigned short));
	while (0 == (buf[9] = (unsigned char)(rand() % 255)));
	buf[10] = opt;
	buf[11] = radius;
	buf[12] = dataLen;
	memcpy(&buf[13], pdata, (int)dataLen);
	memcpy(pdata, buf, len + 3);

	return len;
}

int PackDesc(const unsigned short cId, unsigned char *pbuf)
{
	if (0 == pbuf)
		return 0;

	if (cId & 0X8000)
		return 0;
	memcpy(&pbuf[3], &cId, sizeof(unsigned short));
	return 5;
}

unsigned char XorSum(unsigned char *msg_ptr, unsigned char len)
{
	unsigned char x = 0;
	unsigned char xorResult = 0;

	for (; x < len; x++, msg_ptr++)
		xorResult = xorResult ^ *msg_ptr;

	return xorResult;
}

int PackFrame(unsigned char *pcmd)
{
	unsigned char len;
	unsigned char sum;
	unsigned char buf[320];

	len = pcmd[0];
	sum = XorSum(pcmd, len + 3);
	buf[0] = 0XFE;
	buf[len + 4] = sum;
	memcpy(&buf[1], pcmd, len + 3);
	memcpy(pcmd, buf, len + 5);

	return (len + 5);
}

void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -S [TTY-SERIAL-DEV] -B [BAUD] -A [SHORT ADDR] -T [TIMEOUT]\n", file);
	printf("Example: %s -S/dev/ttyUSB0 -B38400 -A0X796F -T3\n", file), exit(1);
}

int main(int argc, char *argv[])
{
	int addr = 0X0000;
	unsigned char frame[320];
	unsigned char rb[4096];
	int state = 0;
	unsigned char LEN_Token = 0, tempDataLen = 0, FSC_Token = 0;

	char ttyname[256];
	int ttyfd = -1;
	struct termios term;
	struct stat lbuf;

	int baud = 38400;
	int timeout = 24;
	time_t first;
	time_t now;
	fd_set rset;
	struct timeval tv;
	int res;
	int l;
	SYSTEMTIME st;
	int c;
	int v;

	char ch;
	unsigned char *p = rb;
	const unsigned char sync[] = {0XFE, 0X00, 0X00, 0X04, 0X04};
	unsigned short cmd = 0X0000;
	unsigned short cId = 0X0000, sa = 0X0000;
	unsigned long long int ieee = 0X00124B000133B481LL;
	unsigned short my;
	int done = 1;
	int i;


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	setbuf(stdout, 0);
	strcpy(ttyname, "/dev/ttyUSB0");
	if (argc < 3)											PrintUsage();
	while ((c = getopt(argc, argv, "s:S:b:B:t:T:A:a:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;
		case 's':
		case 'S':
			if (strlen(optarg) == 0)							PrintUsage();
			if(0 > stat(optarg, &lbuf))						PrintUsage();
			if(!S_ISCHR(lbuf.st_mode))						PrintUsage();
			strcpy(ttyname, optarg);
			break;
		case 'b':
		case 'B':
			baud = atoi(optarg);
			break;
		case 't':
		case 'T':
			timeout = atoi(optarg);
			if (timeout <= 0)								PrintUsage();
			break;
		case 'A':
		case 'a':
			if (strlen(optarg) != 6)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &addr);
			else											res = sscanf(optarg, "%x", &addr);
			if (-1 == res)									PrintUsage();
			addr &= 0XFFFF;
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

	term.c_cflag |= B9600, term.c_cflag |= CLOCAL, term.c_cflag |= CREAD, term.c_cflag &= ~PARENB, term.c_cflag &= ~CSTOPB, term.c_cflag &= ~CSIZE, term.c_cflag |= CS8, term.c_iflag = IGNPAR, term.c_cc[VMIN] = 1, term.c_cc[VTIME] = 0;
	if (baud == 230400)									cfsetispeed(&term, B230400), cfsetospeed(&term, B230400);
	else if (baud == 115200)								cfsetispeed(&term, B115200), cfsetospeed(&term, B115200);
	else if (baud == 57600)									cfsetispeed(&term, B57600), cfsetospeed(&term, B57600);
	else if (baud == 38400)									cfsetispeed(&term, B38400), cfsetospeed(&term, B38400);
	else if (baud == 19200)									cfsetispeed(&term, B19200), cfsetospeed(&term, B19200);
	else if (baud == 9600)									cfsetispeed(&term, B9600), cfsetospeed(&term, B9600);
	else if (baud == 4800)									cfsetispeed(&term, B4800), cfsetospeed(&term, B4800);
	else													cfsetispeed(&term, B9600), cfsetospeed(&term, B9600);
	tcsetattr(ttyfd, TCSANOW, &term);
	if (0 > (v = fcntl(ttyfd, F_GETFL, 0)))						printf("(%s %d) FCNTL() F_GETFL(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	if (0 > fcntl(ttyfd, F_SETFL, v | O_NONBLOCK))				printf("(%s %d) FCNTL() F_SETFL,O_NONBLOCK(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	tcflush(ttyfd, TCIFLUSH);


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////


	l = write(ttyfd, sync, sizeof(sync));
	if (l != sizeof(sync))									printf("(%s %d) WTITE SYNC fail, EXIT !\n", __FILE__, __LINE__), close(ttyfd), exit(1);

	if (0 == (l = PackDesc(0X0003, frame)))					printf("(%s %d) PackDesc() error !!\n", __FILE__, __LINE__), close(ttyfd), exit(1);
	if (0 == PackAfCmd((unsigned short)addr, 0X0003, 0X00, 30, (unsigned char)l, frame))
		printf("(%s %d) PackAfCmd() error !!\n", __FILE__, __LINE__), close(ttyfd), exit(1);
	l = PackFrame(frame);
	if (l != write(ttyfd, frame, l))								printf("(%s %d) SENDTO() fail, debug=(%d) !\n", __FILE__, __LINE__, l), close(ttyfd), exit(1);
	printf("> \n");
	for (i = 0; i < l; i++)
	{
		if (i == l - 1)										printf("%02X\n", frame[i]);
		else												printf("%02X-", frame[i]);
	}

	time(&first);
	do
	{
		time(&now);
		if ((int)(now - first) > timeout)
		{
			if (0 == done)									printf("\n");
			printf("(%s %d) TIMEOUT !\n", __FILE__, __LINE__), close(ttyfd), exit(done);
		}

		FD_ZERO(&rset);
		FD_SET(ttyfd, &rset);
		memset(&tv, 0, sizeof(tv)), tv.tv_sec = 1;

		if (0 == (res = select(ttyfd + 1, &rset, 0, 0, &tv)))		continue;
		if (res < 0)										printf("(%s %d) SELECT() fail !\n", __FILE__, __LINE__), close(ttyfd), exit(11);
		l = read(ttyfd, (char *)&ch, sizeof(ch));
		if (0 >= l)										printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(ttyfd), exit(12);
		if (0 == state)
		{
		        if (ch == 0XFE)								*p++ = ch, state = 1;
			continue;
		}
		if (1 == state)	
		{
			LEN_Token = ch;
			tempDataLen = 0;
			*p++ = ch, state = 2;
			continue;
		}
		if (2 == state)	
		{
			*p++ = ch, state = 3;
			continue;
		}
		if (3 == state)	
		{
			if (LEN_Token)								state = 4;
			else											state = 5;
			*p++ = ch;
			continue;
		}
		if (4 == state)
		{
			*p++ = ch;
			tempDataLen++;
			if (tempDataLen >= LEN_Token)					state = 5;
			continue;
		}
		if (5 == state)
		{
			*p++ = ch, FSC_Token = ch;
			if(FSC_Token == XorSum(&rb[1], LEN_Token + 3))
			{
				printf(".");
				memcpy(&cmd, &rb[2], sizeof(unsigned short)), cmd &= 0XFFFF;
				if (cmd == 0X8144)
				{
					memcpy(&sa, &rb[8], sizeof(unsigned short)), sa &= 0XFFFF;
					memcpy(&cId, &rb[6], sizeof(unsigned short)), cId &= 0XFFFF;
					((unsigned char *)&ieee)[0] = rb[21];
					((unsigned char *)&ieee)[1] = rb[22];
					((unsigned char *)&ieee)[2] = rb[23];
					memcpy(&my, &rb[26], sizeof(unsigned short)), my &= 0XFFFF;
					if (cId == 0X8003)
					{
						GetLocalTime(&st);
						done = 0;
						if (addr != 0XFFFF)
						{
							if ((int)sa == addr)
							{
								printf("[%04d-%02d-%02d %02d:%02d:%02d %010u]\n", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)now);
								printf("PWM_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [PwmValue]=%u\n", cId, sa, ieee, my, my, BUILD_UINT16(rb[28], rb[29]));
								close(ttyfd), exit(0);
							}
						}
						else
						{
								printf("[%04d-%02d-%02d %02d:%02d:%02d %010u]\n", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)now);
								printf("PWM_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [PwmValue]=%u\n", cId, sa, ieee, my, my, BUILD_UINT16(rb[28], rb[29]));
						}
					}
				}
			}
			else
			{
				printf("x");
			}
			state = 0;
			p = rb;
			continue;
		}
		state = 0;
		p = rb;
	}
	while(1);

	printf("NEVER REACHED\n");
	return 0;
}




