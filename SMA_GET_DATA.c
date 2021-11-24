
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


typedef unsigned short u16;
static u16 fcstab[256] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};


u16 pppfcs16(register u16 fcs, register unsigned char *cp, register int len)
{
       while (len--)
           fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];

       return (fcs);
}


int MkSmaNetFrame(unsigned char * pbuf, unsigned char len, const unsigned char * pdata)
{
	unsigned short fcs;
	unsigned char *p = pbuf;

	*p = 0X7E, p++;
	*p = 0XFF, p++;
	*p = 0X03, p++;
	*p = 0X40, p++;
	*p = 0X41, p++;
	if (len > 0)	memcpy(p, pdata, len), p+= len;
	fcs = pppfcs16(0XFFFF, pbuf + 1, len + 4);
	*p = HI_UINT16(fcs); p++;
	*p = LO_UINT16(fcs); p++;
	*p = 0X7E, p++;
	return (p - pbuf);
}


int MkSmaDataTelegram(unsigned char *pbuf, unsigned short src, unsigned short dst, unsigned char ctrl, unsigned char PktCnt, unsigned char cmd, unsigned char len, const unsigned char *pdata)
{
	unsigned char *p = pbuf;

	memcpy(p, &src, 2), p+= 2;
	memcpy(p, &dst, 2), p+= 2;
	*p = ctrl, p++;
	*p = PktCnt, p++;
	*p = cmd, p++;
	if (len > 0)	memcpy(p, pdata, len), p+= len;
	return (p -pbuf);
}


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

	printf("Usage: %s -h [host] -p [connect port] -a [Short Address] -r [Radius] -o [Tx Options] -S [Source] -D [Destination] -T [Channel Type] -I [Channel Index]\n", file);
	printf("Example: %s -hlocalhost -p34000 -a0X796F -r30 -o0X20 -S0X0001 -D0X0002 -T0X090F -I0X00\n", file), exit(1);
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

	int chType = 0X090F;
	int chIdx = 0;

	int offs = 0;
	//unsigned int osn = 0, nsn = 0;
	unsigned char user[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	//unsigned short V[128];// = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	int src = 0X0001, dst = 0X0002, dev = 0X0000;//, NA = 0X0003;
	unsigned short cs;

	unsigned char telegram[512];
	unsigned char frame[512];
	unsigned char buffer[512];
	int datalen = 0;
	unsigned char *p;

	int sd = -1;
	unsigned int v;
	int i;
	int len = 7;
	unsigned char sync[] = {0XFE, 0X00, 0X00, 0X04, 0X04};
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

	while ((c = getopt(argc, argv, "h:p:a:A:r:R:o:O:s:S:d:D:t:T:i:I:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;

		case 'i':
		case 'I':
			if (strlen(optarg) != 6)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &chIdx);
			else											res = sscanf(optarg, "%x", &chIdx);
			if (-1 == res)									PrintUsage();
			chIdx &= 0XFF;
			break;

		case 't':
		case 'T':
			if (strlen(optarg) != 6)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &chType);
			else											res = sscanf(optarg, "%x", &chType);
			if (-1 == res)									PrintUsage();
			chType &= 0XFFFF;
			break;

		case 's':
		case 'S':
			if (strlen(optarg) != 6)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &src);
			else											res = sscanf(optarg, "%x", &src);
			if (-1 == res)									PrintUsage();
			src &= 0XFFFF;
			break;
		case 'd':
		case 'D':
			if (strlen(optarg) != 6)							PrintUsage();
			if ('0' != optarg[0])							PrintUsage();
			if ('X' != optarg[1] && 'x' != optarg[1])			PrintUsage();
			if ('X' == optarg[1])							res = sscanf(optarg, "%X", &dst);
			else											res = sscanf(optarg, "%x", &dst);
			if (-1 == res)									PrintUsage();
			dst &= 0XFFFF;
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
			if (0XFFF0 == addr)							PrintUsage();
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

	memcpy(&user[0], &chType, sizeof(unsigned short));
	user[2] = (unsigned char)chIdx;

	len = MkSmaDataTelegram(telegram, src, dst, 0X80, 0X00, 0X0B, 3, user);
	len = MkSmaNetFrame(frame, len, telegram);

	PackDesc(0X00EA, buffer);
	memcpy(&buffer[5], frame, len);
	if (0 == PackAfCmd((unsigned short)addr, 0X00EA, (unsigned char)txopts, (unsigned char)radius, (unsigned char)len + 5, buffer))
		printf("(%s %d) PackAfCmd() error !!\n", __FILE__, __LINE__), close(sd), exit(1);
	len = PackFrame(buffer);
	if (len != sendto(sd, buffer, len, MSG_NOSIGNAL, NULL, 0))
		printf("(%s %d) SENDTO() fail, debug=(%d) !\n", __FILE__, __LINE__, len), close(sd), exit(1);
	printf(">\n");
	for (i = 0; i < len; i++)
	{
		if (i == len - 1)	printf("%02X\n", buffer[i]);
		else				printf("%02X-", buffer[i]);
	}
	printf("SMA_GET_DATA (0X0124,0X00EA): [SA]=0X%04X [RADIUS]=0X%02X,%03u [TXOPTS]=0X%02X [SRC]=0X%04X [DST]=0X%04X\n", addr, radius, radius, txopts, src, dst);




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
			if ((int)(now - first) > t)							printf("\nWAITING %d SECONDS for SMA_GET_DATA_RSP (0X80E2) TIMEOUT !\n\n", t), 	close(sd), exit(1);
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
		if ((0X00EA | 0X8000) != cId)
		{
			printf(".");
			continue;
		}
		if (cmd == 0X8144)								memcpy(&sa, &rb[8], sizeof(unsigned short)), sa &= 0XFFFF, addr = sa;
		if (cmd == 0X8244 && 2 == rb[8])					memcpy(&a, &rb[9], sizeof(unsigned long long int)), sa = (unsigned short)(a & 0XFFFF), addr = sa, offs = 9;
		if (sa != addr)
		{
			printf(".");
			continue;
		}
		datalen = rb[20 + offs];

		// ctrl
		if ((0X40 & rb[37 + offs]) != 0X40)
		{
			printf(".");
			continue;
		}

		// cmd
		if (0X0B != rb[39 + offs])
		{
			printf(".");
			continue;
		}

		// CH TYPE
		memcpy(&v, &rb[40 + offs], sizeof(unsigned short)), v &= 0XFFFF;
		//if (chType != v)
		if (0X090F != v)
		{
			printf(".");
			continue;
		}

		// CH IDX
		memcpy(&chIdx, &rb[42 + offs], sizeof(unsigned short)), chIdx &= 0XFF;

		if (0X090F == chType)
		{
			if (datalen != 7 + 8 + 7 + 65)
			{
				printf(".");
				continue;
			}
		}

		if (0X7E != rb[21 + 7 + 0 + offs] ||0XFF != rb[21 + 7 + 1 + offs] || 0X03 != rb[21 + 7 + 2 + offs])
		{
			printf(".");
			continue;
		}

		// fcs
		//memcpy(&cs, &rb[29 + 19 + 65 + offs], sizeof(unsigned short));
		cs = BUILD_UINT16(rb[29 + 19 + 66 + offs], rb[29 + 19 + 65 + offs]);
		if (cs != pppfcs16(0XFFFF, &rb[29 + offs], 19 + 4))
		{
			printf("x");
			continue;
		}

		// network addr
		if ((0X80 & rb[37 + offs]) != 0X80)
		{
			memcpy(&dev, &rb[35 + offs], sizeof(unsigned short)), dev &= 0XFFFF;
			if (src != dev)
			{
				printf("x");
				continue;
			}
		}

		memcpy(&dev, &rb[33 + offs], sizeof(unsigned short)), dev &= 0XFFFF;
		printf(".");
		break;
	}
	while(1);
	((unsigned char *)&ieee)[0] = rb[21 + offs];
	((unsigned char *)&ieee)[1] = rb[22 + offs];
	((unsigned char *)&ieee)[2] = rb[23 + offs];
	memcpy(&my, &rb[26 + offs], sizeof(unsigned short));

	printf("\nSMA_GET_DATA_RSP (0X%04X), [IA]=0X%016"PRIX64" [SA]=0X%04X [MY]=%05u,0X%04X [DEV-ADDR]=0X%04X [CH TYPE]=0X%04X [CH IDX]=0X%02X\n", cId, ieee, sa, my, my, dev, chType, chIdx);


	p = &rb[43 + offs];

	for (i = 0; i < 22; i++)
	{
		if (15 == i ||16 == i || 17 == i || 18 == i || 19 == i)			memcpy(&v, p, 4), v &= 0XFFFFFFFF, p += 4, printf("K[%u]=0X%08X,%u \n", i + 1, v, v);
		else if (20 == i ||21 == i)									memcpy(&v, p, 1), v &= 0XFF, p++, printf("K[%u]=0X%02X,%u \n", i + 1, v, v);
		else														memcpy(&v, p, 2), v &= 0XFFFF, p += 2, printf("K[%u]=0X%04X,%u \n", i + 1, v, v);
	}

	printf("\n");
	close(sd);
	return 0;
}

