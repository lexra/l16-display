
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

#include <cstddef>

#include "tree.hh"
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

struct ADDR_T
{
	unsigned short pa;
	unsigned short sa;
	unsigned long long int ia;
	unsigned char type;
	unsigned char lqi;
	unsigned short my;
	unsigned char has_child;
};

static tree<struct ADDR_T> tr;


struct pool_t
{
	struct list_head list;
	unsigned short pa;
	unsigned short sa;
	unsigned long long int ia;
	unsigned char type;
	unsigned char lqi;
	unsigned char flag;
	unsigned short my;
	unsigned char has_child;
};

static struct pool_t all_list;
static struct pool_t sort_list;



static void ProcessGpioRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char P;
	unsigned char V;
	unsigned char i, j;
	unsigned short cId = 0XFFFF;

	unsigned long long int ieee = 0X00124B000133B481LL;

	((unsigned char *)&ieee)[0] = pdesc[0];
	((unsigned char *)&ieee)[1] = pdesc[1];
	((unsigned char *)&ieee)[2] = pdesc[2];

	memcpy(&cId, &pdesc[3], sizeof(unsigned short));
	memcpy(&my, &pdesc[5], sizeof(unsigned short));
	P = pdesc[7];

	printf("GPIO_RSP (0X%04X), [MY]=%05u,0X%04X [IA]=0X%016LX, value=0X%02X \n", cId, my, my, ieee, pdesc[8]);

	if (P > 1)	P = 1;
	V = pdesc[8];
	for (i = 0; i < 8; i++)
	{
		j = V & (1 << i);
		if (0 != j)	j = 1;

#if 1
		if (i == 7)	printf("P%d_%d=%d\n", P, i, j);
		else			printf("P%d_%d=%d, ", P, i, j);
#else
		if (0 == P && 6 == i)			printf("P%d_%d=%d (RELAY-0 )\n", P, i, j);
		else if (0 == P && 4 == i)		printf("P%d_%d=%d (RELAY-1 )\n", P, i, j);
		else if (0 == P && 5 == i)		printf("P%d_%d=%d (RELAY-2 )\n", P, i, j);
		else if (0 == P && 3 == i)		printf("P%d_%d=%d (RELAY-3 )\n", P, i, j);
		else if (0 == P && 2 == i)		printf("P%d_%d=%d (RELAY-4 )\n", P, i, j);
		else if (1 == P && 6 == i)		printf("P%d_%d=%d (BUTTON-0)\n", P, i, j);
		else if (1 == P && 5 == i)		printf("P%d_%d=%d (BUTTON-1)\n", P, i, j);
		else if (1 == P && 3 == i)		printf("P%d_%d=%d (BUTTON-2)\n", P, i, j);
		else if (0 == P && 1 == i)		printf("P%d_%d=%d (BUTTON-3)\n", P, i, j);
		else if (0 == P && 0 == i)		printf("P%d_%d=%d (BUTTON-4)\n", P, i, j);
		else							printf("P%d_%d=%d\n", P, i, j);
#endif

	}
}

static void ProcessWhoamiRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned long long int ia;
	unsigned short sa = 0XFFF0;
	unsigned short pa = 0XFFF0;
	unsigned short my = 0XFFF0;
	unsigned char nep = 0;
	unsigned short pan = 0XFFF0;
	unsigned long long int chlist = 0XFFFFFFF0LLU;
	char type[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char lqi = 0, rssi =0;
	unsigned char sprf;
	char SPF[32];
	unsigned short prof = 0XFFF0;

	char i = 0, lst[512], tmp[64];

	memset(lst, 0, 512);

	memcpy(&my, &pdesc[5], sizeof(unsigned short));
	memcpy(&sa, &pdesc[7], sizeof(unsigned short));
	memcpy(&ia, &pdesc[9], sizeof(unsigned long long int));
	memcpy(&pa, &pdesc[17], sizeof(unsigned short));
	nep = pdesc[19];
	memcpy(&prof, &pdesc[19 +nep + 1], sizeof(unsigned short));
	memcpy(&pan, &pdesc[19 +nep + 3], sizeof(unsigned short));

	memcpy(&chlist, &pdesc[19 + nep + 5], sizeof(unsigned int));

	for (i = 0; i < 16; i++)
		if ((0X00000800LLU << i) == (chlist & (0X00000800LLU << i)))
			sprintf(tmp, "%d,", 2405 + 5 * i), strcat(lst, tmp);
	if (strlen(lst) > 0)
		lst[strlen(lst) - 1] = 0;

	sprf = pdesc[19 + nep + 9];
	if (0 == sprf)			strcpy(SPF, "NETWORK_SPECIFIC");
	else if (1 == sprf)		strcpy(SPF, "HOME_CONTROLS");
	else if (2 == sprf)		strcpy(SPF, "ZIGBEEPRO");
	else if (3 == sprf)		strcpy(SPF, "GENERIC_STAR");
	else					strcpy(SPF, "GENERIC_TREE");

	if (0 == pdesc[19 + nep + 10])			strcpy(type, "COORDINATOR");
	else if (1 == pdesc[19 + nep + 10])		strcpy(type, "ROUTER");
	else if (2 == pdesc[19 + nep + 10])		strcpy(type, "END-DEVICE");
	lqi = pdesc[19 + nep + 11];
	rssi = pdesc[19 + nep + 12];

	if (0 != lqi && 255 != lqi)
		printf("WHOAMI_RSP (0X8001) , [MY]=%05u,0X%04X [SA]=0X%04X [IA]=0X%016LX [PA]=0X%04X \n[PROFILE_ID]=0X%04X [PAN_ID]=0X%04X [STACK_PROFILE]=%s [CH-LIST]=%s [TYPE]=%s \n[LQI]=0X%02X,%u [RSSI]=%d \n", 
			my, my, sa, ia, pa, prof, pan, SPF, lst, type, lqi, lqi, (char)rssi);
	else
		printf("WHOAMI_RSP (0X8001), [MY]=%05u,0X%04X [SA]=0X%04X [IA]=0X%016LX [PA]=0X%04X \n[PROFILE_ID]=0X%04X [PAN_ID]=0X%04X [STACK_PROFILE]=%s [CH-LIST]=%s [TYPE]=%s \n[LQI]=N/A [RSSI]=N/A \n", 
			my, my, sa, ia, pa, prof, pan, SPF, lst, type);
}

void ProcessAfIncoming(unsigned short cmd, unsigned char *pbuf)
{
	unsigned short cId = 0, sa = 0XFFF0, gId = 0XFFF0;
	unsigned char lqi = 0X00;
	unsigned char DataLen = 0X00;
	unsigned char sn = 0X00, sec = 0;
	unsigned char br = 0X00;
	unsigned char len = 0X00;
	unsigned int ts = 0XFFF0;

	int i;

	unsigned short my;
	unsigned long long int ieee = 0X00124B000133B481LL;


	memcpy(&cId, &pbuf[6], sizeof(unsigned short));
	memcpy(&gId, &pbuf[4], sizeof(unsigned short));
	memcpy(&sa, &pbuf[8], sizeof(unsigned short));
	memcpy(&lqi, &pbuf[13], sizeof(char));
	br = pbuf[12];
	sec = pbuf[14];
	memcpy(&ts, &pbuf[15], sizeof(unsigned int));
	sn = pbuf[19];
	DataLen = pbuf[20];
	len = pbuf[1];

	if (0 != lqi && 0XFF != lqi)
		printf("AF_INCOMING (0X8144), [cId]=0X%04X [SA]=0X%04X [gId]=0X%04X [br]=%d [Lqi]=%03d,0X%02X [sec]=%d [SN]=%03u [len]=%03d [plen]=%03d [ts]=%06u\n", 
			cId, sa, gId, br, lqi, lqi, sec, sn, len, DataLen, ts);
	else
		printf("AF_INCOMING (0X8144), [cId]=0X%04X [SA]=0X%04X [gId]=0X%04X [br]=%d [Lqi]=N/A,0X00 [sec]=%d [SN]=%03u [len]=%03d [plen]=%03d [ts]=%06u\n", 
			cId, sa, gId, br, sec, sn, len, DataLen, ts);

	if (COMPILE_DATE_RSP_DESC == cId)
	{
		unsigned char sl = 0, x = 0;
		char szDate[256];

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];


		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		sl = pbuf[28];

		memset(szDate, 0, sizeof(szDate));
		for (x = 0; x < sl; x++)
			szDate[x] = pbuf[29 + x];

		printf("COMPILE_DATE_RSP, [MY]=%05u,0X%04X [IA]=0X%016LX ", my, my, ieee);
		printf("[COMPILE_DATE]=%s\n", szDate);

		return;
	}


	if (RESET_RSP_DESC == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		printf("RESET_RSP, [MY]=%05u,0X%04X \n", my, my);
		return;
	}

	if (0X8018 == cId)
	{
		unsigned short start;
		unsigned char slave, fn, bc;

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		memcpy(&my, &pbuf[26], sizeof(unsigned short));

		printf("MODBUS_RSP, [MY]=%05u,0X%04X [IA]=0X%016LX\n", my, my, ieee);


		memcpy(&start, &pbuf[28], sizeof(unsigned short));
		printf("[START REG]=0X%04X \n", start);
		slave = pbuf[30];
		printf("[SLAVE]=0X%02X \n", slave);
		fn = pbuf[31];
		printf("[FN]=0X%02X \n", fn);
		bc = pbuf[32];
		printf("[BC]=0X%02X \n", bc);

		for (i = 0; i < (int)bc + 5; i++)
		{
			if (i == (int)bc + 5 - 1)
				printf("0X%02X \n", pbuf[30 + i]);
			else if (i == 0)
				printf("[OUTPUT]: 0X%02X-", pbuf[30 + i]);
			else
				printf("0X%02X-", pbuf[30 + i]);
		}

		return;
	}


	if ((GPIO_SET_DESC | 0X8000) == cId || (GPIO_REQ_DESC | 0X8000) == cId)
	{
		ProcessGpioRsp(DataLen, &pbuf[21]);
		return;
	}

	if ((PWM_SET_DESC | 0X8000) == cId || (PWM_REQ_DESC | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		printf("PWM_RSP (0X%04X), [IA]=0X%016LX [SA]=0X%04X [MY]=%05u,0X%04X [PwmValue]=%d\n", cId, ieee, sa, my, my, BUILD_UINT16(pbuf[28], pbuf[29]));
		return;
	}

	if (WHOAMI_RSP_DESC == cId)
	{
		ProcessWhoamiRsp(DataLen, &pbuf[21]);
		return;
	}

	if (0X80B8 == cId)
	{
		unsigned short start;
		unsigned short data;
		unsigned char slave, fn;
		unsigned long long int ieee = 0X00124B000133B481LL;

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		printf("MODBUS_PRESET_RSP, [MY]=%05u,0X%04X\n", my, my);
		slave = pbuf[28];
		printf("[SLAVE]=0X%02X \n", slave);
		fn = pbuf[29];
		printf("[FN]=0X%02X \n", fn);
		start = BUILD_UINT16(pbuf[31], pbuf[30]);
		printf("[START REG]=0X%04X \n", start);
		data = BUILD_UINT16(pbuf[33], pbuf[32]);
		printf("[DATA]=0X%04X \n", data);
		return;
	}


	printf("Unknown cId=0X%04X \n", cId);
	return;
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

static int PackDesc(const unsigned short cId, unsigned char *pbuf)
{
	if (0 == pbuf)
		return 0;

	if (cId & 0X8000)
		return 0;
	memcpy(&pbuf[3], &cId, sizeof(unsigned short));

	if (WHOAMI_REQ_DESC == cId || NTEMP_REQ_DESC == cId)
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
	char filename[128];
	int i = 0;

	memset(filename, 0, sizeof(filename));
	strcpy(filename, __FILE__);
	for (; i <128; i++)
		if ('.' == filename[i])	filename[i] = 0;

	printf("Usage: %s -h [host] -p [connect port]\n", filename);
	printf("Example: %s -hlocalhost -p34000\n", filename), exit(1);
}

int main(int argc, char *argv[])
{
	char host[128];
	int port = 34000;

	int srsp = 0;
	unsigned short cId = 0;
	unsigned short sa = 0, pa = 0, my = 0;
	unsigned long long int ia;
	unsigned char n, type, lqi;
	int t = 48;

	int ack = 0;

	int sd = -1;
	int v;
	int i, count = 0, total = 0;
	int len;
	unsigned char sync[] = {0XFE, 0X00, 0X00, 0X04, 0X04};
	unsigned char frame[320];

	int res;
	int state = 0;
	time_t first;
	fd_set rset;
	struct timeval tv;
	int c;
	unsigned char ch;
	unsigned char rb[4096];

	unsigned char tl = 0;
	unsigned short cmd;

	static int left = 0;
	static unsigned char *pb = 0;

	struct list_head *pos, *q;
	struct pool_t *a, *a1, *a2;
	int f = 0;

	tree<ADDR_T>::pre_order_iterator pre;
	tree<ADDR_T>::iterator itr;
	tree<ADDR_T>::iterator top;
	struct ADDR_T ca;
	int old_depth = -1, depth = 0;


	setbuf(stdout, 0);
	strcpy(host, "localhost");
	if (argc < 3)
		PrintUsage();


	while ((c = getopt(argc, argv, "h:H:p:P:")) != -1)
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
		}
	}

	INIT_LIST_HEAD(&all_list.list);
	INIT_LIST_HEAD(&sort_list.list);

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


	if (0 == (len = PackDesc(WHOAMI_REQ_DESC, frame)))
		printf("(%s %d) PackDesc() error !!\n", __FILE__, __LINE__), close(sd), exit(1);

	if (0 == PackAfCmd(0XFFFF, WHOAMI_REQ_DESC, 0X20, 30, (unsigned char)len, frame))
		printf("(%s %d) PackAfCmd() error !!\n", __FILE__, __LINE__), close(sd), exit(1);

	len = PackFrame(frame);
	if (len != sendto(sd, frame, len, MSG_NOSIGNAL, NULL, 0))
		printf("(%s %d) SENDTO() fail, debug=(%d) !\n", __FILE__, __LINE__, len), close(sd), exit(1);


	state = 0;
	time(&first);
	do
	{
		time_t now;

		time(&now);
		if (0 == srsp)
		{
			if ((int)(now - first) > 1)
			{
				printf("\nWAITING for AF_DATA_REQUEST_SRSP (0X0164), TIMEOUT !\n\n");

				list_for_each_safe(pos, q, &all_list.list)
				{
					a = list_entry(pos, struct pool_t, list);
					list_del(pos);
					free(a);
				}

				list_for_each_safe(pos, q, &sort_list.list)
				{
					a = list_entry(pos, struct pool_t, list);
					list_del(pos);
					free(a);
				}
				close(sd), exit(1);
			}
		}
		else
		{
			if ((int)(now - first) > t)
				break;
		}

		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 1;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)				continue;
		if (res < 0)
		{
			printf("(%s %d) SELECT() fail !\n", __FILE__, __LINE__);
			list_for_each_safe(pos, q, &all_list.list)
			{
				a = list_entry(pos, struct pool_t, list);
				list_del(pos);
				free(a);
			}

			list_for_each_safe(pos, q, &sort_list.list)
			{
				a = list_entry(pos, struct pool_t, list);
				list_del(pos);
				free(a);
			}
			close(sd), exit(1);
		}
		if (0 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)
			{
				printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__);
				list_for_each_safe(pos, q, &all_list.list)
				{
					a = list_entry(pos, struct pool_t, list);
					list_del(pos);
					free(a);
				}

				list_for_each_safe(pos, q, &sort_list.list)
				{
					a = list_entry(pos, struct pool_t, list);
					list_del(pos);
					free(a);
				}
				close(sd), exit(1);
			}
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
		if (0 == srsp)
		{
			if (cmd == 0X0164)
			{
				srsp = 1;
				printf(".\nAF_DATA_REQUEST_SRSP (0X0164), %s !\n", 0 == rb[4] ? "SUCCESS" : "FAIL");
				if (0 != rb[4])	
				{
					list_for_each_safe(pos, q, &all_list.list)
					{
						a = list_entry(pos, struct pool_t, list);
						list_del(pos);
						free(a);
					}

					list_for_each_safe(pos, q, &sort_list.list)
					{
						a = list_entry(pos, struct pool_t, list);
						list_del(pos);
						free(a);
					}
					close(sd), exit(1);
				}
			}
		}

		if (cmd != 0X8144 && cmd != 0X8244)
		{
			printf(".");
			continue;
		}

		memcpy(&cId, &rb[6], sizeof(unsigned short));
		cId &= 0XFFFF;
		if ((WHOAMI_REQ_DESC | 0X8000) != cId)
		{
			printf(".");
			continue;
		}

		memcpy(&sa, &rb[8], sizeof(unsigned short));
		sa &= 0XFFFF;
		memcpy(&pa, &rb[21 + 17], sizeof(unsigned short));
		pa &= 0XFFFF;
		memcpy(&ia, &rb[21 + 9], sizeof(unsigned long long int));
		ia &= 0XFFFFFFFFFFFFFFFFLL;
		memcpy(&my, &rb[21 + 5], sizeof(unsigned short));
		my &= 0XFFFF;

		n = rb[21 + 19];
		type = rb[21 + n + 29];
		lqi = rb[21 + n + 30];
		if (255 == lqi)				lqi = 0;
		if (0X0000 == sa)			ack = 1;
		if (cmd == 0X8144)
		{
			f = 0;
			list_for_each(pos, &all_list.list)
			{
				a = list_entry(pos, struct pool_t, list);
				if (a->sa == sa)
					f = 1;
			}
			if (0 == f)
			{
				printf("+");

				a = (struct pool_t *)malloc(sizeof(struct pool_t));
				memset(a, 0, sizeof(struct pool_t));
				a->sa = sa, a->pa = pa, a->ia = ia, a->type = type, a->lqi = lqi, a->my = my;
				if(0X0000 == a->sa)		a->pa = 0XFFFF;
				list_add_tail(&a->list, &all_list.list);
			}
			else
				printf(".");
		}
		else
			printf("-");
	}
	while(1);

	close(sd);



	if (0 == ack)
	{
		list_for_each_safe(pos, q, &all_list.list)
		{
			a = list_entry(pos, struct pool_t, list);
			list_del(pos);
			free(a);
		}

		list_for_each_safe(pos, q, &sort_list.list)
		{
			a = list_entry(pos, struct pool_t, list);
			list_del(pos);
			free(a);
		}

		printf("NO COORDINATOR TO BE RENDING ... !!\n");
		return 1;
	}


	list_for_each(pos, &all_list.list)
	{
		a = list_entry(pos, struct pool_t, list);
		if (a->sa == 0X0000)
			break;
	}
	a->flag = 1;

	a1 = (struct pool_t *)malloc(sizeof(struct pool_t));
	memset(a1, 0, sizeof(struct pool_t));
	a1->sa = a->sa, a1->pa = a->pa, a1->ia = a->ia, a1->type = a->type, a1->lqi = a->lqi;
	list_add_tail(&a1->list, &sort_list.list);

	i = 0;
	do
	{
		if (i++ >= 30000)
			break;

		f = 0;
		list_for_each(pos, &sort_list.list)
		{
			a1 = list_entry(pos, struct pool_t, list);
			if (a1->flag == 0)
			{
				f = 1;
				break;
			}
		}
		if (0 == f)
			break;

		a1->flag = 1;

		list_for_each(pos, &all_list.list)
		{
			a = list_entry(pos, struct pool_t, list);
			if (1 == a->flag)
				continue;
			if (a->pa == a1->sa)
			{
				a->flag = 1;
				a1->has_child = 1;

				a2 = (struct pool_t *)malloc(sizeof(struct pool_t));
				memset(a2, 0, sizeof(struct pool_t));
				a2->sa = a->sa, a2->pa = a->pa, a2->ia = a->ia, a2->type = a->type, a2->lqi = a->lqi;
				list_add_tail(&a2->list, &sort_list.list);
			}
		}
	} while(1);


	list_for_each(pos, &sort_list.list)
	{
		a = list_entry(pos, struct pool_t, list);
		if (a->flag != 1)
			continue;

		a->flag = 2;
		ca.pa = a->pa;
		ca.sa = a->sa;
		ca.ia = a->ia;
		ca.lqi = a->lqi;
		ca.type = a->type;
		ca.has_child = a->has_child;

		if (a->sa == 0X0000)
		{
			top = tr.begin();
			tr.insert(top, ca);
			continue;
		}

		f = 0;
		for (itr = tr.begin(); itr != tr.end(); itr++)
		{
			if (a->pa == itr->sa)
			{
				f = 1;
				break;
			}
		}

		if (1 == f)
		{
			tr.append_child(itr, ca);
		}
		else
			printf("NO PARENT FOUND\n");
	}


	total = 0;
	list_for_each(pos, &all_list.list)
	{
		a = list_entry(pos, struct pool_t, list);
		if (0 == a->sa)
			a->pa = 0XFFFF;
		if (0 == total)
			printf("\n");
		printf("NODE(%03d), [IA]=0X%016LX [SA]=0X%04X [PA]=0X%04X [MY]=%05u,0X%04X [TYPE]=%u [LQI]=%03u\n", total, a->ia, a->sa, a->pa, a->my, a->my, a->type, a->lqi);
		total++;
	}
	if (total > 1)
		printf("Total %d NODES \n", total);
	else if (total == 1 || total == 0)
		printf("Total %d NODE \n", total);



	printf("\n[ZIGBEE TREE]: \n");
	count = 0;
	for (pre = tr.begin(); pre != tr.end(); pre++)
	{
		depth = tr.depth(pre);
		if (depth <= old_depth)		// ´«¦æ
		{
			printf("\n");
			for (i = 0; i < depth; i++)

#if 1
				printf("\t\t");
#else
				printf("                ");
#endif

		}

#if 1
		if (pre->has_child)
		{
			if (pre->type == 0)
				printf("%c[0X%04X]%03d ->\t", 'C', pre->sa, pre->lqi);
			else if (pre->type == 1)
				printf("%c[0X%04X]%03d ->\t", 'R', pre->sa, pre->lqi);
			else
				printf("%c[0X%04X]%03d ->\t", 'D', pre->sa, pre->lqi);
		}
		else
		{
			if (pre->type == 0)
				printf("%c[0X%04X]%03d\t", 'C', pre->sa, pre->lqi);
			else if (pre->type == 1)
				printf("%c[0X%04X]%03d\t", 'R', pre->sa, pre->lqi);
			else
				printf("%c[0X%04X]%03d\t", 'D', pre->sa, pre->lqi);
		}
#else
		if (pre->has_child)
		{
			if (pre->type == 0)
				printf("%c[%06X]%03u -> ", 'C', (unsigned int)(pre->ia & 0XFFFFFF), pre->lqi);
			else if (pre->type == 1)
				printf("%c[%06X]%03u -> ", 'R', (unsigned int)(pre->ia & 0XFFFFFF), pre->lqi);
			else
				printf("%c[%06X]%03u -> ", 'D', (unsigned int)(pre->ia & 0XFFFFFF), pre->lqi);
		}
		else
		{
			if (pre->type == 0)
				printf("%c[%06X]%03u ", 'C', (unsigned int)(pre->ia & 0XFFFFFF), pre->lqi);
			else if (pre->type == 1)
				printf("%c[%06X]%03u ", 'R', (unsigned int)(pre->ia & 0XFFFFFF), pre->lqi);
			else
				printf("%c[%06X]%03u ", 'D', (unsigned int)(pre->ia & 0XFFFFFF), pre->lqi);
		}
#endif

		old_depth = depth;
		count++;
	}
	if (count > 0)
	{
		if (total == count)
			printf("\n\nDone. %d rendered. \n\n", count);
		else
			printf("\n\nDone. %d rendered, %d not rendered. \n\n", count, total - count);
	}
	else
	{
			printf("\n\nDone. %d rendered. \n\n", count);
	}

	list_for_each_safe(pos, q, &all_list.list)
	{
		a = list_entry(pos, struct pool_t, list);
		list_del(pos);
		free(a);
	}

	list_for_each_safe(pos, q, &sort_list.list)
	{
		a = list_entry(pos, struct pool_t, list);
		list_del(pos);
		free(a);
	}

	return 0;
}

