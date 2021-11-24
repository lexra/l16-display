
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

#define WHOAMI_REQ_DESC				0X0001
#define WHOAMI_RSP_DESC				(WHOAMI_REQ_DESC | 0X8000)

#define PWM_SET_DESC					0X0002
#define PWM_REQ_DESC					0X0003

#define GPIO_SET_DESC					0X0004
#define GPIO_REQ_DESC					0X0005
#define PX_SET_DESC						0X0004
#define PX_REQ_DESC						0X0005

#define TEMP_REQ_DESC					0X0006
#define TEMP_RSP_DESC					(TEMP_REQ_DESC | 0X8000)

#define KEY_SET_DESC						0X0008
#define KEY_RSP_DESC					(KEY_SET_DESC | 0X8000)

#define ADC_REQ_DESC					0X0009
#define ADC_RSP_DESC					(ADC_REQ_DESC | 0X8000)

#define NV_SET_DESC						0X0012
#define NV_REQ_DESC						0X0013
#define NV_RSP_DESC						(NV_REQ_DESC | 0X8000)

#define PHOTO_REQ_DESC					0X0015
#define PHOTO_RSP_DESC					(PHOTO_REQ_DESC | 0X8000)

#define MODBUSQRY_REQ_DESC				0X0018
#define MODBUSQRY_RSP_DESC				(MODBUSQRY_REQ_DESC | 0X8000)

#define RESET_REQ_DESC					0X0019
#define RESET_RSP_DESC					(RESET_REQ_DESC | 0X8000)

#define PXSEL_SET_DESC					0X0025
#define PXSEL_REQ_DESC					0X0026
#define PXDIR_SET_DESC					0X0027
#define PXDIR_REQ_DESC					0X0028

#define PERCFG_SET_DESC					0X0029
#define PERCFG_REQ_DESC					0X002A
#define PERCFG_RSP_DESC					(PERCFG_REQ_DESC | 0X8000)

#define ADCCFG_SET_DESC					0X002B
#define ADCCFG_REQ_DESC				0X002C
#define ADCCFG_RSP_DESC				(ADCCFG_REQ_DESC | 0X8000)

#define UXBAUD_SET_DESC					0X002D
#define UXBAUD_REQ_DESC					0X002E
#define UXBAUD_RSP_DESC					(UXBAUD_REQ_DESC | 0X8000)
#define UXGCR_SET_DESC					0X002F
#define UXGCR_REQ_DESC					0X0030
#define UXGCR_RSP_DESC					(UXGCR_REQ_DESC | 0X8000)



#define RELAY_CTRL_REQ_DESC				0X0038
#define PIN_HI_LO_SET_DESC				0X0038

#define JBOX_REQ_DESC					0X0052
#define JBOX_RSP_DESC					(JBOX_REQ_DESC | 0X8000)

#define P0IEN_SET_DESC					0X0057
#define P0IEN_REQ_DESC					0X0058
#define P0IEN_RSP_DESC					(P0IEN_REQ_DESC | 0X8000)
#define P1IEN_SET_DESC					0X0059
#define P1IEN_REQ_DESC					0X0060
#define P1IEN_RSP_DESC					(P1IEN_REQ_DESC | 0X8000)
#define P2IEN_SET_DESC					0X0061
#define P2IEN_REQ_DESC					0X0062
#define P2IEN_RSP_DESC					(P2IEN_REQ_DESC | 0X8000)

#define NTEMP_REQ_DESC					0X0078
#define NTEMP_RSP_DESC					(NTEMP_REQ_DESC | 0X8000)

#define ADAM_4118_RSP_DESC				(0X0090 | 0X8000)

#define PHONIX_READ_RSP_DESC			(0X009A | 0X8000)
#define PHONIX_READ_DESC_RSP_DESC		(0X009C | 0X8000)

#define COMPILE_DATE_REQ_DESC			0X00B0
#define COMPILE_DATE_RSP_DESC			(COMPILE_DATE_REQ_DESC | 0X8000)


#define RTS_TIMEOUT_SET_DESC			0X00BA
#define RTS_TIMEOUT_RSP_DESC			(RTS_TIMEOUT_SET_DESC | 0X8000)




/////////////////////////////////////////////////////////////////////////////////////////
//

#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((unsigned int)((unsigned int)((Byte0) & 0x00FF) + ((unsigned int)((Byte1) & 0x00FF) << 8) \
			+ ((unsigned int)((Byte2) & 0x00FF) << 16) + ((unsigned int)((Byte3) & 0x00FF) << 24)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)




/////////////////////////////////////////////////////////////////////////////////////////
//

static int seg_size = 20;


void ProcessWhoamiRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned char nep = 0;

	nep = pdesc[19];
	if (1 == pdesc[19 + nep + 10])		seg_size = 80;
	else								seg_size = 20;
}

void ProcessAfIncoming(unsigned short cmd, unsigned char *pbuf)
{
	unsigned short cId = 0X00;

	memcpy(&cId, &pbuf[6], sizeof(unsigned short));
	if (WHOAMI_RSP_DESC == cId)
	{
		ProcessWhoamiRsp(pbuf[20], &pbuf[21]);
		return;
	}
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

	if (WHOAMI_REQ_DESC == cId || NTEMP_REQ_DESC == cId || 0X001B == cId || 0X00B7 == cId)
		return 5;
	if (PWM_SET_DESC == cId)
		return 7;
	if (PWM_REQ_DESC == cId)
		return 5;
	if (GPIO_SET_DESC == cId)
		return 7;
	if (GPIO_REQ_DESC == cId)
		return 6;
	if (ADC_REQ_DESC == cId)
		return 5;
	if (PIN_HI_LO_SET_DESC == cId)
		return 8;
	if (PXDIR_SET_DESC == cId || PXSEL_SET_DESC == cId)
		return 7;
	if (PXDIR_REQ_DESC == cId || PXSEL_REQ_DESC == cId)
		return 6;
	if (TEMP_REQ_DESC == cId)
		return 5;
	if (PHOTO_REQ_DESC == cId)
		return 5;
	if (KEY_SET_DESC == cId)
		return 7;
	if (RESET_REQ_DESC == cId)
		return 7;
	if (NV_REQ_DESC == cId)
		return 7;
	if (PERCFG_SET_DESC == cId)
		return 6;
	if (PERCFG_REQ_DESC == cId)
		return 5;
	if (JBOX_REQ_DESC == cId)
		return 5;
	if (COMPILE_DATE_REQ_DESC == cId)
		return 5;
	if (ADCCFG_REQ_DESC == cId)
		return 5;
	if (UXGCR_REQ_DESC == cId || UXBAUD_REQ_DESC == cId)
		return 6;
	if (UXGCR_SET_DESC == cId || UXBAUD_SET_DESC == cId)
		return 7;
	if (P0IEN_REQ_DESC == cId || P1IEN_REQ_DESC == cId || P2IEN_REQ_DESC == cId)
		return 5;
	if (P0IEN_SET_DESC == cId || P1IEN_SET_DESC == cId || P2IEN_SET_DESC == cId)
		return 6;
	if (RTS_TIMEOUT_SET_DESC == cId)
		return 6;
	if (MODBUSQRY_REQ_DESC == cId)
		return 11;

	return 0;
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

int CONNECT(char *domain, int port)
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


void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -h [host] -p [connect port] -a [Short Address] -r [Radius] -o [Tx Options] -S [START_PAGE] -E [END_PAGE]\n", file);
	printf("Example: %s -hlocalhost -p34000 -a0X796F -r30 -o0X20 -S0 -E0\n", file), exit(1);
}

int main(int argc, char *argv[])
{
	char host[128];
	int port = 34000;

	unsigned int addr = 0X0000;
	unsigned int radius = 30;
	unsigned int txopts = 0X20;
	int srsp = 0;
	unsigned short cId = 0;
	unsigned short sa = 0;
	int t = 6;

	int start = 0, end = 0;

	int sd = -1;
	int v;
	int i;
	int len = 7;
	unsigned char sync[] = {0XFE, 0X00, 0X00, 0X04, 0X04};
	unsigned char frame[320];
	unsigned long long int ieee = 0X00124B000133B481LL;
	unsigned short my;
	unsigned long long int a;

	int res;
	int state = 0;
	time_t first;
	time_t now;
	fd_set rset;
	struct timeval tv;
	int c;
	unsigned char ch;
	unsigned char rb[4096];

	unsigned char tl = 0;
	unsigned short cmd;

	static int left = 0;
	static unsigned char *pb = 0;


	setbuf(stdout, 0);
	strcpy(host, "localhost");
	if (argc < 3)											PrintUsage();

	while ((c = getopt(argc, argv, "h:p:a:A:r:R:o:O:s:S:e:E:")) != -1)
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
			start = atoi(optarg);
			if (start < 0)									PrintUsage();
			if (start > 0X1F)								PrintUsage();
			break;
		case 'e':
		case 'E':
			end = atoi(optarg);
			if (end < 0)									PrintUsage();
			if (end > 0X1F)								PrintUsage();
			break;
		case 'h':
			strcpy(host, optarg);
			break;
		case 'p':
			port = atoi(optarg);
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
			if (0XFFFF == addr)							PrintUsage();
			if (0XFFFE == addr)							PrintUsage();
			if (0XFFFD == addr)							PrintUsage();
			break;
		case 'R':
		case 'r':
			if (strlen(optarg) > 5)							PrintUsage();
			if ('0' == optarg[0] && ('X' == optarg[1] || 'x' == optarg[1]))
			{
				if ('X' == optarg[1])						res = sscanf(optarg, "%X", &radius);
				else										res = sscanf(optarg, "%x", &radius);
				if (-1 == res)								PrintUsage();
			}
			else
				radius = atoi(optarg);
			radius &= 0XFF;
			if (radius > 60)								PrintUsage();
			break;
		case 'O':
		case 'o':
			if (strlen(optarg) != 4)							PrintUsage();
			if (optarg[0] != '0')							PrintUsage();
			if (optarg[1] != 'X' && optarg[1] != 'x')			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &txopts);
			else											res = sscanf(optarg, "%x", &txopts);
			if (-1 == res)									PrintUsage();
			txopts &= 0XFF;
			break;
		}
	}
	if (end < start)
		start = i, start = end, end = i;



/////////////////////////////////////////////////////////////////////////////////////////
//

	if (0 > (sd = CONNECT(host, port)))						printf("TCP CONNECT TO (%s %d) fail !\n", host, port), exit(1);
	v = 1024 * 1024;
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > (v = fcntl(sd, F_GETFL, 0)))						printf("(%s %d) FCNTL() fail, F_GETFL, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > fcntl(sd, F_SETFL, v | O_NONBLOCK))				printf("(%s %d) FCNTL() F_SETFL fail, O_NONBLOCK, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);



/////////////////////////////////////////////////////////////////////////////////////////
//

	len = sendto(sd, sync, sizeof(sync), MSG_NOSIGNAL, NULL, 0);
	if (len != sizeof(sync))									printf("(%s %d) PRTOOCOL SYNC fail, EXIT !\n", __FILE__, __LINE__), close(sd), exit(1);
	PackDesc(0X00E5, frame);
	frame[5] = (unsigned char)start;
	frame[6] = (unsigned char)end;
	if (0 == PackAfCmd((unsigned short)addr, 0X00E5, (unsigned char)txopts, (unsigned char)radius, (unsigned char)7, frame))
		printf("(%s %d) PackAfCmd() error !!\n", __FILE__, __LINE__), close(sd), exit(1);
	len = PackFrame(frame);
	if (len != sendto(sd, frame, len, MSG_NOSIGNAL, NULL, 0))
		printf("(%s %d) SENDTO() fail, debug=(%d) !\n", __FILE__, __LINE__, len), close(sd), exit(1);
	printf(">\n");
	for (i = 0; i < len; i++)
	{
		if (i == len - 1)	printf("%02X\n", frame[i]);
		else				printf("%02X-", frame[i]);
	}
	printf("L16_PAGE_START_END (0X0124,0X00E5): [SA]=0X%04X [RADIUS]=0X%02X,%03u [TXOPTS]=0X%02X [START_PAGE]=%02u [END_PAGE]=%02u\n", addr, radius, radius, txopts, start, end);



/////////////////////////////////////////////////////////////////////////////////////////
// 

	srsp = 0, t = 6, state = 0, time(&first);
	do
	{
		time(&now);
		if (0 == srsp)
		{
			if ((int)(now - first) > 1)							printf("\nWAITING for AF_DATA_REQUEST_SRSP (0X0164), TIMEOUT !\n\n"), close(sd), exit(1);
		}
		else
		{
			if ((int)(now - first) > t)							printf("\nWAITING %d SECONDS for L16_PAGE_START_END_RSP (0X80E5) TIMEOUT !\n\n", t), 	close(sd), exit(1);
		}
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv)), tv.tv_sec = 1;
		if (0 == (res = select(sd + 1, &rset, 0, 0, &tv)))			continue;
		if (res < 0)										printf("(%s %d) SELECT() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)
		{
			if (0 >= (len = read(sd, &ch, 1)))					printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (0XFE == ch)								state = 1, rb[0] = ch;
			else											printf("x");
			continue;
		}
		if (1 == state)
		{
			if (0 >= (len = read(sd, &ch, 1)))					printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			state = 2, rb[1] = ch, left = (int)ch + 3, pb = &rb[2];
			continue;
		}
		if (2 == state)
		{
			if (0 >= (len = read(sd, (char *)pb, left)))			printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len, left -= len;
			if (left > 0)									continue;
			state = 0, tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
			{
				state = 0, printf("(%s %d) XORSUM ERROR !\n", __FILE__, __LINE__);
				continue;
			}
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short)),  cmd &= 0XFFFF;
		if (0 == srsp)
		{
			if (cmd == 0X0164)
			{
				srsp = 1, printf(".\n(%s %d) AF_DATA_REQUEST_SRSP (0X0164), %s !\n", __FILE__, __LINE__, 0 == rb[4] ? "SUCCESS" : "FAIL");
				if (0 != rb[4])								close(sd), exit(1);
			}
		}
		if (cmd != 0X8144 && cmd != 0X8244)
		{
			printf(".");
			continue;
		}
		memcpy(&cId, &rb[6], sizeof(unsigned short));
		if ((0X00E5 | 0X8000) != cId)
		{
			printf(".");
			continue;
		}
		if (cmd == 0X8144)								memcpy(&sa, &rb[8], sizeof(unsigned short)), sa &= 0XFFFF;
		if (cmd == 0X8244 && 2 == rb[8])					memcpy(&a, &rb[9], sizeof(unsigned long long int)), sa = (unsigned short)(a & 0XFFFF);
		if (sa != addr)
		{
			printf(".");
			continue;
		}
		printf(".");
		break;
	}
	while(1);
	close(sd);

	memcpy(&my, &rb[26], sizeof(unsigned short));
	((unsigned char *)&ieee)[0] = rb[21], ((unsigned char *)&ieee)[1] = rb[22], ((unsigned char *)&ieee)[2] = rb[23];
	printf("\nL16_PAGE_START_END_RSP (0X%04X), [IA]=0X%016"PRIX64" [SA]=0X%04X [MY]=%05u,0X%04X\n", cId, ieee, sa, my, my);

	return 0;
}


