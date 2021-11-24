
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
vmultim -h 192.168.3.30 -p 34000 -b0X0000 -h0X00 -s0X00 -K0X000000

// HD HD CM0 CM1 0 S K0 K1 K2 K3 K4 K5 CRC1 CRC0
BOOL vmulti_update_keyboard(pvmulti_client vmulti, BYTE shiftKeyFlags, BYTE keyCodes[KBD_KEY_CODES]);
vmulti_update_keyboard(vmulti, N->msg[5], &N->msg[6]);

*/


static void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (i = 0; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -h[host] -p[connect port] -s[SHIFT] -k[KEYS]\n", file);
	printf("Example: %s -h192.168.3.30 -p34000 -S0X00 -K0X000000\n", file), exit(1);
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
	char filename[128];
	unsigned char wb[14 * 2];
	int sd = -1;
	int v;
	int c;
	int len;
	int i;
	int res;

	int S = 0;
	long long int K = 0;


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

	while ((c = getopt(argc, argv, "h:H:p:P:s:S:k:K:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;
		case 'H':
		case 'h':
			strcpy(host, optarg);
			break;
		case 'P':
		case 'p':
			port = atoi(optarg);
			break;

		case 'S':
		case 's':
			if (strlen(optarg) > 4)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%X", &S);
				else								res = sscanf(optarg, "%x", &S);
				if (-1 == res)
					PrintUsage();
			}
			else
				S = atoi(optarg);
			S &= 0XFF;
			break;
		case 'K':
		case 'k':
			//if (strlen(optarg) > 8)					PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])				res = sscanf(optarg, "%llX", &K);
				else								res = sscanf(optarg, "%llx", &K);
				if (-1 == res)
					PrintUsage();
			}
			else
			{
				PrintUsage();
			}
			K &= 0XFFFFFF;
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


	/*if (0 > (v = fcntl(sd, F_GETFL, 0)))
		printf("(%s %d) FCNTL() fail, F_GETFL, EXIT \n", __FILE__, __LINE__), close(sd), sd = -1, exit(1);
	if (0 > fcntl(sd, F_SETFL, v | O_NONBLOCK))
		printf("(%s %d) FCNTL() F_SETFL fail, O_NONBLOCK, EXIT \n", __FILE__, __LINE__), close(sd), sd = -1, exit(1);*/



// HD HD CM0 CM1 0 S K0 K1 K2 K3 K4 K5 CRC1 CRC0
	memset(wb, 0, sizeof(wb));
	wb[0] = 0XFE, wb[1] = 0XFE;
	wb[2] = REPORTID_KEYBOARD, wb[3] = 0;
	wb[4] = 0;
	wb[5] = S;
	memcpy(&wb[6], &K, 6);
	crc = ModbusCrc16(wb, 12);
	wb[12] = HI_UINT16(crc);
	wb[13] = LO_UINT16(crc);

	wb[14 + 0] = 0XFE, wb[14 + 1] = 0XFE;
	wb[14 + 2] = REPORTID_KEYBOARD, wb[14 + 3] = 0;
	crc = ModbusCrc16(&wb[14], 12);
	wb[14 + 12] = HI_UINT16(crc);
	wb[14 + 13] = LO_UINT16(crc);

	len = sendto(sd, wb, sizeof(wb), MSG_NOSIGNAL, NULL, 0);
	if (len != sizeof(wb))
		printf("(%s %d) SENDTO fail, EXIT !\n", __FILE__, __LINE__), close(sd), sd = -1, exit(4);

	printf("> \n");
	for (i = 0; i < len; i++)
	{
		if (i == len - 1)	printf("%02X\n", wb[i]);
		else				printf("%02X-", wb[i]);
	}
	printf("UPDATE_KEYBOARD: [S]=0X%02X,%03u [K0]=0X%02X [K1]=0X%02X [K2]=0X%02X [K3]=0X%02X [K4]=0X%02X [K5]=0X%02X\n", S, S, wb[6], wb[7], wb[8], wb[9], wb[10], wb[11]); 
	printf("\n");

	if (-1 != sd)
		close(sd);

	return 0;
}



