
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

#define WHOAMI_REQ_DESC					0X0001
#define WHOAMI_RSP_DESC					(WHOAMI_REQ_DESC | 0X8000)

#define PWM_SET_DESC					0X0002
#define PWM_REQ_DESC					0X0003

#define GPIO_SET_DESC					0X0004
#define GPIO_REQ_DESC					0X0005
#define PX_SET_DESC						0X0004
#define PX_REQ_DESC						0X0005

#define TEMP_REQ_DESC					0X0006
#define TEMP_RSP_DESC					(TEMP_REQ_DESC | 0X8000)

#define KEY_SET_DESC					0X0008
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
#define ADCCFG_REQ_DESC					0X002C
#define ADCCFG_RSP_DESC					(ADCCFG_REQ_DESC | 0X8000)

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

static void ProcessZdoMgmtRtgRsp(unsigned short CMD, unsigned char *pdesc)
{
	unsigned short cmd;
	unsigned short sa;
	unsigned char status, tn = 0, start = 0, cnt = 0;
	unsigned char *p;
	unsigned short da, next;
	unsigned char res;
	int i = 0;

	memcpy(&cmd, &pdesc[2], sizeof(unsigned short));
	memcpy(&sa, &pdesc[4], sizeof(unsigned short));
	status = pdesc[6];
	tn = pdesc[7];
	start = pdesc[8];
	cnt = pdesc[9];

	printf("\nZDO_MGMT_RTG_RSP, [CMD]=0X%02X \n[Status]=%d [SA]=0X%04X [TN]=%d \n", cmd, status, sa, tn);

	p = &pdesc[10];
	for (; i < cnt; i++, p += 5)
	{
		memcpy(&da, p, sizeof(unsigned short));
		memcpy(&res, p + 2, sizeof(unsigned char));
		memcpy(&next, p + 3, sizeof(unsigned short));
		if (0 == res)
			printf("DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "ACTIVE", next);
		else if (1 == res)
			printf("DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "UNDERWAY", next);
		else if (2 == res)
			printf("DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "FAIL", next);
		else if (3 == res)
			printf("DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "INACTIVE", next);
		else
			printf("DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "N/A", next);
	}
}

void ProcessZdoNwkAddrRsp(unsigned short CMD, unsigned char *pdesc)
{
	unsigned short cmd;
	unsigned char status;
	unsigned long long int ia;
	unsigned short sa, assoc;
	unsigned char start, i, num;

	memcpy(&cmd, &pdesc[2], sizeof(unsigned short));
	status = pdesc[4];
	memcpy(&ia, &pdesc[5], sizeof(unsigned long long int));
	memcpy(&sa, &pdesc[13], sizeof(unsigned short));
	start = pdesc[15];
	num = pdesc[16];

	printf("ZDO_NWK_ADDR_RSP (0X%04X): [status]=%d(%s) [IA]=0X%016"PRIX64" [SA]=0X%04X\n", cmd, status, (0 == status) ? "SUCCESS" : "FAIL" , ia, sa);

	if (num > 0 && 0 == status)
	{
		for (i = 0; i < num; i++)
		{
			memcpy(&assoc, &pdesc[17 + 2 * i], sizeof(unsigned short));
			printf("Assoc[%d]=0X%04X\n", i + start, assoc);
		}
	}
}

static unsigned char XorSum(unsigned char *msg_ptr, unsigned char len)
{
	unsigned char x = 0;
	unsigned char xorResult = 0;

	for (; x < len; x++, msg_ptr++)
		xorResult = xorResult ^ *msg_ptr;

	return xorResult;
}

static int PackFrame(unsigned char *pcmd)
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


static void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -h [host] -p [connect port] -a [short address] -s [start index]\n", file);
	printf("Example: %s -hlocalhost -p34000 -a0X0000 -s0\n", file), exit(1);
}

int main(int argc, char *argv[])
{
	int addr = 0;
	char host[128];
	int port = 34000;
	int sd = -1;
	int v;
	int i;
	int len;
	unsigned char sync[] = {0XFE, 0X00, 0X00, 0X04, 0X04};
	unsigned char frame[320];

	int start = 0;

	int res;
	int state = 0;
	time_t first;
	fd_set rset;
	struct timeval tv;
	int c;
	unsigned char ch;
	unsigned char rb[4096];

	unsigned char tl = 0;
	unsigned short cmd = 0, sa = 0;

	static int left = 0;
	static unsigned char *pb = 0;


	setbuf(stdout, 0);
	strcpy(host, "localhost");
	if (argc < 3)
		PrintUsage();

	while ((c = getopt(argc, argv, "h:p:a:A:S:s:")) != -1)
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
		case 'p':
			port = atoi(optarg);
			break;

		case 'S':
		case 's':
			start = atoi(optarg);
			if (start > 99)		PrintUsage();
			if (start < 0)			PrintUsage();
			start &= 0XFF;
			break;

		case 'A':
		case 'a':
			if (strlen(optarg) != 6)
				PrintUsage();
			if (optarg[0] != '0')
				PrintUsage();
			if (optarg[1] != 'X' && optarg[1] != 'x')
				PrintUsage();
			if (optarg[1] == 'X')	res = sscanf(optarg, "%X", &addr);
			else					res = sscanf(optarg, "%x", &addr);
			if (-1 == res)			PrintUsage();
			addr &= 0XFFFF;
			break;
		}
	}


	if (0 > (sd = CONNECT(host, port)))
		printf("TCP CONNECT TO (%s %d) fail !\n", host, port), exit(1);

	v = 1024 * 1024;
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);


	if (0 > (v = fcntl(sd, F_GETFL, 0)))
		printf("(%s %d) FCNTL() fail, F_GETFL, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > fcntl(sd, F_SETFL, v | O_NONBLOCK))
		printf("(%s %d) FCNTL() F_SETFL fail, O_NONBLOCK, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);


	len = sendto(sd, sync, sizeof(sync), MSG_NOSIGNAL, NULL, 0);
	if (len != sizeof(sync))
		printf("(%s %d) PRTOOCOL SYNC fail, EXIT !\n", __FILE__, __LINE__), close(sd), exit(1);


	frame[0] = 0X03;
	frame[1] = 0X25;
	frame[2] = 0X32;
	memcpy(&frame[3], &addr, sizeof(unsigned short));
	frame[5] = (unsigned char)start;
	len = PackFrame(frame);

	if (len != sendto(sd, frame, len, MSG_NOSIGNAL, NULL, 0))
		printf("(%s %d) SENDTO() fail, debug=(%d) !\n", __FILE__, __LINE__, len), close(sd), exit(1);

	printf("> \n");
	for (i = 0; i < len; i++)
	{
		if (i == len - 1)	printf("%02X\n", frame[i]);
		else				printf("%02X-", frame[i]);
	}
	printf("ZdoMgmtRtgReq (0X3225): [SHORT ADDR]=0X%04X [START INDEX]=%02u \n", addr, start);

	state = 0;
	time(&first);
	do
	{
		time_t now;

		time(&now);
		if ((int)(now - first) > 36)
			printf("\nWAITING 36 SECONDS for ZDO_MGMT_RTG_RSP TIMEOUT !\n\n"), close(sd), exit(1);

		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 1;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)			continue;
		if (res < 0)			printf("(%s %d) SELECT() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)
				printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (0XFE == ch)
			{
				state = 1;
				rb[0] = ch;
			}
			else
				printf("x");
			continue;
		}
		if (1 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)
				printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			state = 2;
			rb[1] = ch;
			left = (int)ch + 3;
			pb = &rb[2];
			continue;
		}
		if (2 == state)
		{
			len = read(sd, (char *)pb, left);
			if (len <= 0)
				printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
			{
				printf("(%s %d) XORSUM ERROR !\n", __FILE__, __LINE__);
				state = 0;
				continue;
			}
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		cmd &= 0XFFFF;
		if (cmd != 0XB245)
		{
			printf(".");
			continue;
		}
		memcpy(&sa, &rb[4], sizeof(unsigned short));
		sa &= 0XFFFF;
		if (addr != 0XFFFF && addr != 0XFFFD)
		{
			if (sa != addr)				continue;

			printf("\n");
			for (i = 0; i < rb[1] + 5; i++)
			{
				if (i == rb[1] + 5 - 1)	printf("%02X\n", rb[i]);
				else					printf("%02X-", rb[i]);
			}
			ProcessZdoMgmtRtgRsp(0, rb);
			break;
		}

		printf("\n");
		for (i = 0; i < rb[1] + 5; i++)
		{
			if (i == rb[1] + 5 - 1)	printf("%02X\n", rb[i]);
			else					printf("%02X-", rb[i]);
		}
		ProcessZdoMgmtRtgRsp(0, rb);

		continue;
	}
	while(1);

	close(sd);
	printf("\n");
	return 0;
}




