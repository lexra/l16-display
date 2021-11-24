
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

#include "list.h"


#if !defined(MSG_NOSIGNAL)
 #define MSG_NOSIGNAL									0
#endif // MSG_NOSIGNAL



/////////////////////////////////////////////////////////////////////////////////////////
//

#define REPORTID_MTOUCH         0x01
#define REPORTID_FEATURE        0x02
#define REPORTID_MOUSE          0x03
#define REPORTID_RELATIVE_MOUSE 0x04
#define REPORTID_DIGI           0x05
#define REPORTID_JOYSTICK       0x06
#define REPORTID_KEYBOARD       0x07
#define REPORTID_MESSAGE        0x10
#define REPORTID_CONTROL        0x40


#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((unsigned int)((unsigned int)((Byte0) & 0x00FF) + ((unsigned int)((Byte1) & 0x00FF) << 8) \
			+ ((unsigned int)((Byte2) & 0x00FF) << 16) + ((unsigned int)((Byte3) & 0x00FF) << 24)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)




/////////////////////////////////////////////////////////////////////////////////////////
//

typedef struct tm SYSTEMTIME;

void GetLocalTime(SYSTEMTIME *st)
{
	struct tm *pst = NULL;
	time_t t = time(NULL);
	pst = localtime(&t);
	memcpy(st, pst, sizeof(SYSTEMTIME));
	st->tm_year += 1900;
}

static int CONNECT(char *domain, int port)
{
	int sock_fd;
	struct hostent *site;
	struct sockaddr_in me;
	int v;
 
	site = gethostbyname(domain);
	if (0 == site)
		printf("(%s %d) GETHOSTBYNAME() FAIL !\n", __FILE__, __LINE__), exit(1);
	if (0 >= site->h_length)
		printf("(%s %d) 0 >= site->h_length \n", __FILE__, __LINE__), exit(1);

	if (0 > (sock_fd = socket(AF_INET, SOCK_STREAM, 0)))
		printf("(%s %d) SOCKET() FAIL !\n", __FILE__, __LINE__), exit(1);

	v = 1;
	if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
		printf("(%s %d) SETSOCKOPT() FAIL !\n", __FILE__, __LINE__), exit(1);

	if (0 == memset(&me, 0, sizeof(struct sockaddr_in)))
		printf("(%s %d) MEMSET() FAIL !\n", __FILE__, __LINE__), exit(1);

	memcpy(&me.sin_addr, site->h_addr_list[0], site->h_length);
	me.sin_family = AF_INET;
	me.sin_port = htons(port);

	return (0 > connect(sock_fd, (struct sockaddr *)&me, sizeof(struct sockaddr))) ? -1 : sock_fd;
}


/*
vmultim -h 192.168.3.30 -p 34000 -b0X0000 -h0X00 -x640 -y480 -X0 -y0 -t0

// HD HD CM0 CM1 B0 B1 H X Y RX RY T CRC1 CRC0
BOOL vmulti_update_joystick(pvmulti_client vmulti, USHORT buttons, BYTE hat, BYTE x, BYTE y, BYTE rx, BYTE ry, BYTE throttle);
vmulti_update_joystick(vmulti, 
	BUILD_UINT16(N->msg[4], N->msg[5]), N->msg[6], N->msg[7], N->msg[8], N->msg[9], N->msg[10], N->msg[11]);

*/


static void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (i = 0; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -h[host] -p[connect port] -b[BUTTON] -H[HAT] -x[X] -y[Y] -X[RX] -Y[RY] -t[THROTTLE]\n", file);
	printf("Example: %s -h192.168.3.30 -p34000 -b0X0000 -H0X00 -x80 -y60 -X0 -y0 -t0\n", file), exit(1);
}

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

unsigned int ModbusCrc16(unsigned char *buf, unsigned short len)
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

int main(int argc, char *argv[])
{
	char host[128];
	int port = 34000;

	int crc = 0;
	int B = 0, T = 0, H = 0;
	int X = 80, Y = 60;
	int RX = 0, RY = 0;
	char filename[128];
	unsigned char wb[14];
	int sd = -1;
	int v;
	int c;
	int len;
	int i;
	int res;


	memset(filename, 0, sizeof(filename));
	strcpy(filename, __FILE__);
	for (i = 0; i < sizeof(filename); i++)
	{
		if ('.' == filename[i])
			filename[i] = 0;
	}

	setbuf(stdout, 0);
	strcpy(host, "localhost");
	if (argc < 3)
		PrintUsage();

	while ((c = getopt(argc, argv, "h:p:P:b:B:x:X:y:Y:t:T:H:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;
		case 'h':
			strcpy(host, optarg);
			break;
		case 'P':
		case 'p':
			port = atoi(optarg);
			break;

		case 'T':
		case 't':
			if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &T);
				else								res = sscanf(optarg, "%x", &T);
				if (-1 == res)
					PrintUsage();
			}
			else
				T = atoi(optarg);
			T &= 0XFF;
			break;
		case 'B':
		case 'b':
			//if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &B);
				else								res = sscanf(optarg, "%x", &B);
				if (-1 == res)
					PrintUsage();
			}
			else
				B = atoi(optarg);
			B &= 0XFFFF;
			break;

		case 'H':
			if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &H);
				else								res = sscanf(optarg, "%x", &H);
				if (-1 == res)
					PrintUsage();
			}
			else
				H = atoi(optarg);
			H &= 0XFF;
			break;


		case 'X':
			if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &RX);
				else								res = sscanf(optarg, "%x", &RX);
				if (-1 == res)
					PrintUsage();
			}
			else
				RX = atoi(optarg);
			RX &= 0XFF;
			break;

		case 'x':
			if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &X);
				else								res = sscanf(optarg, "%x", &X);
				if (-1 == res)
					PrintUsage();
			}
			else
				X = atoi(optarg);
			X &= 0XFF;
			break;

		case 'Y':
			if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &RY);
				else								res = sscanf(optarg, "%x", &RY);
				if (-1 == res)
					PrintUsage();
			}
			else
				RY = atoi(optarg);
			RY &= 0XFF;
			break;

		case 'y':
			if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &Y);
				else								res = sscanf(optarg, "%x", &Y);
				if (-1 == res)
					PrintUsage();
			}
			else
				Y = atoi(optarg);
			Y &= 0XFF;
			break;
		}
	}


///////////////////////////////////////////////////////////
// 
	if (0 > (sd = CONNECT(host, port)))
		printf("CONNECT (%s %d) fail !\n", host, port), exit(2);

	v = (64 * 1024);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__), close(sd), sd = -1, exit(3);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__), close(sd), sd = -1, exit(3);
	/*
	if (0 > (v = fcntl(sd, F_GETFL, 0)))
		printf("(%s %d) FCNTL() fail, F_GETFL, EXIT \n", __FILE__, __LINE__), close(sd), sd = -1, exit(1);
	if (0 > fcntl(sd, F_SETFL, v | O_NONBLOCK))
		printf("(%s %d) FCNTL() F_SETFL fail, O_NONBLOCK, EXIT \n", __FILE__, __LINE__), close(sd), sd = -1, exit(1);
	*/


// HD HD CM0 CM1 B0 B1 H X Y RX RY T CRC1 CRC0
	memset(wb, 0, sizeof(wb));
	wb[0] = 0XFE, wb[1] = 0XFE;
	wb[2] = REPORTID_JOYSTICK, wb[3] = 0;
	memcpy(&wb[7], &X, sizeof(unsigned short));
	memcpy(&wb[4], &B, sizeof(unsigned short));
	wb[6] = H;
	wb[7] = X;
	wb[8] = Y;
	wb[9] = RX;
	wb[10] = RY;
	wb[11] = T;
	crc = ModbusCrc16(wb, 12);
	wb[12] = HI_UINT16(crc);
	wb[13] = LO_UINT16(crc);
	len = sendto(sd, wb, sizeof(wb), MSG_NOSIGNAL, NULL, 0);
	if (len != sizeof(wb))
		printf("(%s %d) SENDTO fail, EXIT !\n", __FILE__, __LINE__), close(sd), sd = -1, exit(4);
	memset(&wb[4], 0, sizeof(unsigned short));
	crc = ModbusCrc16(wb, 12);
	wb[12] = HI_UINT16(crc);
	wb[13] = LO_UINT16(crc);
	len = sendto(sd, wb, sizeof(wb), MSG_NOSIGNAL, NULL, 0);
	if (len != sizeof(wb))
		printf("(%s %d) SENDTO fail, EXIT !\n", __FILE__, __LINE__), close(sd), sd = -1, exit(4);

	printf("> \n");
	for (i = 0; i < len; i++)
	{
		if (i == len - 1)	printf("%02X\n", wb[i]);
		else				printf("%02X-", wb[i]);
	}

	if (-1 != sd)
		close(sd);

	printf("UPDATE_JOYSTICK: [B]=0X%04X [X]=0X%02X,%03u [Y]=0X%02X,%03u [RX]=0X%02X,%03u [RY]=0X%02X,%03u [H]=0X%02X,%03u [T]=0X%02X,%d\n", 
		B, X, X, Y, Y, RX, RX, RY, RY, H, H, T, T);
	printf("\n");
	return 0;
}



