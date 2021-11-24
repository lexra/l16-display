
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

#include <signal.h>
#include <netinet/tcp.h>

#include <math.h>
#include <regex.h>

#include "cgic.h"



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


/*
<ZTOOL>
	<REQ_PACKET>FE-0F-24-01-6F-79-65-65-01-00-00-20-0A-05-00-00-00-00-00-12</REQ_PACKET>
</ZTOOL>

PwmExtSet.cgi?host=127.0.0.1&port=34000&cmd=0X0124&cid=0X0002&pan=0X8504&addr=0X00124B000111B1C0&radius=15&txopts=0X30&value=1250

*/

static unsigned char XorSum(unsigned char *msg_ptr, unsigned char len)
{
	unsigned char x = 0;
	unsigned char xorResult = 0;

	for (; x < len; x++, msg_ptr++)
		xorResult = xorResult ^ *msg_ptr;

	return xorResult;
}

/*
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
*/

static void ProcessWhoamiRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned long long int ia;
	unsigned short sa = 0XFFF0;
	unsigned short pa = 0XFFF0;
	unsigned short my = 0XFFF0;
	unsigned char nep = 0;
	unsigned short pan = 0XFFF0;
	unsigned long chlist = 0XFFFFFFF0;
	unsigned short ch = 0;
	char type[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char lqi = 0, rssi =0;
	unsigned char sprf;
	char SPF[32];
	unsigned short prof = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short));
	memcpy(&sa, &pdesc[7], sizeof(unsigned short));
	memcpy(&ia, &pdesc[9], sizeof(unsigned long long int));
	memcpy(&pa, &pdesc[17], sizeof(unsigned short));
	nep = pdesc[19];
	memcpy(&prof, &pdesc[19 +nep + 1], sizeof(unsigned short));
	memcpy(&pan, &pdesc[19 +nep + 3], sizeof(unsigned short));

	memcpy(&chlist, &pdesc[19 + nep + 5], sizeof(unsigned long));
	if (0x00000800 == chlist)		ch = 2405;
	else if (0x00001000 == chlist)	ch = 2410;
	else if (0x00002000 == chlist)	ch = 2415;
	else if (0x00004000 == chlist)	ch = 2420;
	else if (0x00008000 == chlist)	ch = 2425;
	else if (0x00010000 == chlist)	ch = 2430;
	else if (0x00020000 == chlist)	ch = 2435;
	else if (0x00040000 == chlist)	ch = 2440;
	else if (0x00080000 == chlist)	ch = 2445;
	else if (0x00100000 == chlist)	ch = 2450;
	else if (0x00200000 == chlist)	ch = 2455;
	else if (0x00400000 == chlist)	ch = 2460;
	else if (0x00800000 == chlist)	ch = 2465;
	else if (0x01000000 == chlist)	ch = 2470;
	else if (0x02000000 == chlist)	ch = 2475;
	else if (0x04000000 == chlist)	ch = 2480;
	else							ch = 0;

	sprf = pdesc[19 + nep + 9];
	if (0 == sprf)			strcpy(SPF, "NETWORK_SPECIFIC");
	else if (1 == sprf)		strcpy(SPF, "HOME_CONTROLS");
	else if (2 == sprf)		strcpy(SPF, "ZIGBEEPRO");
	else if (3 == sprf)		strcpy(SPF, "GENERIC_STAR");
	else					strcpy(SPF, "GENERIC_TREE");

	if (0 == pdesc[19 + nep + 10])			strcpy(type, "COORDINATOR");
	else if (1 == pdesc[19 + nep + 10])		strcpy(type, "ROUTER");
	else if (2 == pdesc[19 + nep + 10])		strcpy(type, "END DEVICE");
	lqi = pdesc[19 + nep + 11];
	rssi = pdesc[19 + nep + 12];

	fprintf(cgiOut, "WHOAMI_RSP, [MY]=0X%04X \n[SA]=0X%04X \n[IA]=0X%016"PRIX64" \n[PA]=0X%04X \n[PROFILE_ID]=0X%04X \n[PAN]=0X%04X \n[CH]=%04d \n[STACK_PROFILE]=%s \n[TYPE]=%s \n[LQI]=0X%02X \n[RSSI]=%d \n", 
		my, sa, ia, pa, prof, pan, ch, SPF, type, lqi, (char)rssi);
}

/*
static int PackAfExtCmd(unsigned char mode, unsigned long long int addr, unsigned short pan, unsigned short cId, unsigned char opt, unsigned char radius, unsigned char dataLen, unsigned char *pdata)
{
	unsigned char buf[320];
	unsigned char len = (int)dataLen + 19;

	if (0X00 != opt && 0X10 != opt && 0X20 != opt && 0X30 != opt)
		return 0;
	if (0 != mode && 1 != mode && 2 != mode && 3 != mode && 15 != mode)
		return 0;
	if (len > 0X93)
		return 0;
	memset(buf, 0, 320);
	srand((unsigned)time(NULL));

	buf[0] = (unsigned char)len;
	buf[1] = 0X24;
	buf[2] = 0X02;
	buf[3] = mode;					// 0:AddrNotPresent, 1:AddrGroup, 2:Addr16Bit, 3:Addr64Bit, 15:AddrBroadcast
	memcpy(&buf[4], &addr, sizeof(unsigned long long int));
	buf[12] = 103;
	memcpy(&buf[13], &pan, sizeof(unsigned short));
	buf[15] = 103;
	memcpy(&buf[16], &cId, sizeof(unsigned short));
	//buf[18] = 0;
	buf[18] = (unsigned char)(rand() % 255);
	buf[19] = opt;
	buf[20] = radius;
	buf[21] = dataLen;
	memcpy(&buf[22], pdata, (int)dataLen);
	memcpy(pdata, buf, len + 3);

	return len;
}
*/

/*
static int PackDesc(const unsigned short cId, unsigned char *pbuf)
{
	if (0 == pbuf)
		return 0;

	memcpy(&pbuf[3], &cId, sizeof(unsigned short));
	if (cId & 0X8000)
		return 0;

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
*/

static int Conn(char *domain, int port)
{
	int sock_fd;
	struct hostent *site;
	struct sockaddr_in me;
	int v;
 
	site = gethostbyname(domain);
	if (0 == site)
		fprintf(cgiOut, "(%s %d) gethostbyname() fail !\n", __FILE__, __LINE__), exit(1);
	if (0 >= site->h_length)
		fprintf(cgiOut, "(%s %d) 0 >= site->h_length \n", __FILE__, __LINE__), exit(1);

	if (0 > (sock_fd = socket(AF_INET, SOCK_STREAM, 0)))
		fprintf(cgiOut, "(%s %d) socket() fail !\n", __FILE__, __LINE__), exit(1);

	v = 1;
	if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
		fprintf(cgiOut, "(%s %d) setsockopt() fail !\n", __FILE__, __LINE__), exit(1);

	if (0 == memset(&me, 0, sizeof(struct sockaddr_in)))
		fprintf(cgiOut, "(%s %d) memset() fail !\n", __FILE__, __LINE__), exit(1);

	memcpy(&me.sin_addr, site->h_addr_list[0], site->h_length);
	me.sin_family = AF_INET;
	me.sin_port = htons(port);

	return (0 > connect(sock_fd, (struct sockaddr *)&me, sizeof(struct sockaddr))) ? -1 : sock_fd;
}

/*
static void SigInt (int signo)
{
	if (SIG_ERR == signal(signo, SIG_IGN))
		fprintf(cgiOut, "signal error");

	if (SIG_ERR == signal(signo, SigInt))
		fprintf(cgiOut, "signal error");
}
*/

static void ProcessGpioRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char P;
	unsigned char V;
	unsigned char i;

	memcpy(&my, &pdesc[5], sizeof(unsigned short));
	fprintf(cgiOut, "GPIO_RSP, [MY]=0X%04X, value=0X%02X \n", my, pdesc[8]);

	P = pdesc[7];
	if (P > 1)	P = 1;
	V = pdesc[8];
	for (i = 0; i < 8; i++)
	{
		if (V & (1 << i))
			fprintf(cgiOut, "P%d_%d=1\n", P, i);
		else
			fprintf(cgiOut, "P%d_%d=0\n", P, i);
	}
}

static void ProcessAdcRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char n = 0, i = 0;
	unsigned char *p;

	memcpy(&my, &pdesc[5], sizeof(unsigned short));
	n = pdesc[7];
	p = &pdesc[8];
	printf("ADC_RSP, [MY]=0X%04X \n", my);
	for (; i < n; i++)
	{
		short v;

		memcpy(&v, p, sizeof(unsigned short)), p += 2;
		printf("[AD%d]=%d \n", i, v);
	}
}

static void ProcessAfIncoming(unsigned short cmd, unsigned char *pbuf)
{
	unsigned short cId = 0, sa = 0XFFF0, my = 0XFFF0, gId = 0XFFF0;
	unsigned char lqi = 0X00;
	unsigned char DataLen = 0X00;
	unsigned char sn = 0X00, sec = 0;
	unsigned char br = 0X00;
	unsigned char len = 0X00;
	unsigned int ts = 0XFFF0;
	int i = 0;

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

	// TotalLen = len + 5
	// len »P DataLen ¬Û®t 11, 
	// len - DataLen = 11
	fprintf(cgiOut, "\n");
	fprintf(cgiOut, "AF_IND, [cId]=0X%04X [SA]=0X%04X [gId]=0X%04X [br]=%d [Lqi]=%d,0X%02X [sec]=%d [SN]=%u [len]=%02d [plen]=%02d [ts]=%06u\n\n", 
		cId, sa, gId, br, lqi, lqi, sec, sn, len, DataLen, ts);

	if (0X80B1 == cId)
	{
		short ad;

		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "PWM_DETECT_RSP, [MY]=0X%04X \n", my);
		memcpy(&ad, &pbuf[28], sizeof(unsigned short));
		fprintf(cgiOut, "[V0]=%d \n", ad);
		memcpy(&ad, &pbuf[30], sizeof(unsigned short));
		fprintf(cgiOut, "[V1]=%d \n", ad);
		memcpy(&ad, &pbuf[32], sizeof(unsigned short));
		fprintf(cgiOut, "[V2]=%d \n", ad);
		memcpy(&ad, &pbuf[34], sizeof(unsigned short));
		fprintf(cgiOut, "[V3]=%d \n", ad);
		if (DataLen > 15)
		{
			memcpy(&ad, &pbuf[36], sizeof(unsigned short));
			fprintf(cgiOut, "[AD0]=%d \n", ad);
			memcpy(&ad, &pbuf[38], sizeof(unsigned short));
			fprintf(cgiOut, "[AD1]=%d \n", ad);
			memcpy(&ad, &pbuf[40], sizeof(unsigned short));
			fprintf(cgiOut, "[AD2]=%d \n", ad);
			memcpy(&ad, &pbuf[42], sizeof(unsigned short));
			fprintf(cgiOut, "[AD3]=%d \n", ad);
		}
		return;
	}

	if (RESET_RSP_DESC == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "RESET_RSP, [MY]=0X%04X \n", my);
		return;
	}
	if ((ADCCFG_SET_DESC | 0X8000) == cId || (ADCCFG_REQ_DESC | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "ADCCFG_RSP, [MY]=0X%04X [VALUE]=0X%02X\n", my, pbuf[28]);
		for (i = 0; i < 8; i++)
		{
			if (pbuf[28] & (1 << i))
				fprintf(cgiOut, "AIN-%d enabled\n", i);
			else
				fprintf(cgiOut, "AIN-%d disabled\n", i);
		}
		return;
	}
	if ((PERCFG_SET_DESC | 0X8000) == cId || (PERCFG_REQ_DESC | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "PERCFG_RSP, [MY]=0X%04X [VALUE]=0X%02X\n", my, pbuf[28]);

		if (pbuf[28] & (1 << 0))	fprintf(cgiOut, "U0CFG Alt2\n");
		else						fprintf(cgiOut, "U0CFG Alt1\n");

		if (pbuf[28] & (1 << 1))	fprintf(cgiOut, "U1CFG Alt2\n");
		else						fprintf(cgiOut, "U1CFG Alt1\n");

		if (pbuf[28] & (1 << 4))	fprintf(cgiOut, "T4CFG Alt2\n");
		else						fprintf(cgiOut, "T4CFG Alt1\n");

		if (pbuf[28] & (1 << 5))	fprintf(cgiOut, "T3CFG Alt2\n");
		else						fprintf(cgiOut, "T3CFG Alt1\n");

		if (pbuf[28] & (1 << 6))	fprintf(cgiOut, "T1CFG Alt2\n");
		else						fprintf(cgiOut, "T1CFG Alt1\n");

		return;
	}

/*
	if (ADAM_4118_RSP_DESC == cId)
	{
		char szTmp[256];
		char *ps, *pn;
		char c = 0;

		memcpy(&my, &pbuf[26], sizeof(unsigned short));

		if (DataLen < 60)
			return;

		//>+04.750+0021.6+888888+888888+888888+888888+888888+888888
		if (pbuf[28] != '>')
			return;
		if (pbuf[29] != '+')
			return;

		memset(szTmp, 0, 256);
		strcpy(szTmp, (char *)&pbuf[28]);
		fprintf(cgiOut, "ADAM_RSP, [MY]=0X%04X \n[Content]=%s\n", my, szTmp);
		//fprintf(cgiOut, "ADAM_RSP, [MY]=0X%04X \n", my);
		//return;

		ps = (char *)&pbuf[30];
		for (pn = ps; ; pn++)
		{
			if (pn - ps > 7)
				return;
			if (ps - (char *)&pbuf[30] > 60)
				return;
			if (*pn == '+')
			{
				*pn = 0;

				if (0 == c)
				{
					float ma = 0.00F;
					float r = 0.00F;

					sscanf(ps, "%f", &ma);
					if (ma > 4.75)	r = (float)125.00F * (ma - (float)4.75F);
					else			r = 0;

					fprintf(cgiOut, "[CH%d]=%s -> Radition=%04.02f\n", c, ps, r);
				}
				else
					fprintf(cgiOut, "[CH%d]=%s \n", c, ps);

				ps = pn + 1;
				c++;
			}
		}

		return;
	}
*/

	if (COMPILE_DATE_RSP_DESC == cId)
	{
		unsigned char sl = 0, x = 0;
		char szDate[256];

		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		sl = pbuf[28];
		//sl = 11;
		memset(szDate, 0, sizeof(szDate));
		for (x = 0; x < sl; x++)
			szDate[x] = pbuf[29 + x];

		//strcpy
		fprintf(cgiOut, "COMPILE_DATE_RSP, [MY]=0X%04X\n", my);
		fprintf(cgiOut, "[COMPILE_DATE]=%s\n", szDate);

		return;
	}

	if (WHOAMI_RSP_DESC == cId)
	{
		ProcessWhoamiRsp(DataLen, &pbuf[21]);
		return;
	}

	if ((PWM_SET_DESC | 0X8000) == cId || (PWM_REQ_DESC | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "PWM_RSP, [MY]=0X%04X \n[Value]=%d\n", my, BUILD_UINT16(pbuf[28], pbuf[29]));
		return;
	}

	if (ADC_RSP_DESC == cId)
	{
		ProcessAdcRsp(DataLen, &pbuf[21]);
		return;
	}

	if (PHOTO_RSP_DESC == cId)
	{
		unsigned long long int ieee = 0X00124B000133B481LL;

		((unsigned char *)&ieee)[0] = pbuf[21];
		((unsigned char *)&ieee)[1] = pbuf[22];
		((unsigned char *)&ieee)[2] = pbuf[23];

		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "PHOTO_RSP, [MY]=%05u,0X%04X [IA]=0X%016"PRIX64"\n[Value]=%02d\n", my, my, ieee, BUILD_UINT16(pbuf[28], pbuf[29]));
		return;
	}

	if ((GPIO_SET_DESC | 0X8000) == cId || (GPIO_REQ_DESC | 0X8000) == cId)
	{
		ProcessGpioRsp(DataLen, &pbuf[21]);
		return;
	}

	if (TEMP_RSP_DESC == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "CHIP_TEMP_RSP, [MY]=0X%04X \n[Chip Temperature]=%02d\n", my, BUILD_UINT16(pbuf[28], pbuf[29]));
		return;
	}


	if ((UXBAUD_SET_DESC | 0X8000) == cId || (UXBAUD_REQ_DESC | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "U%dBAUD_RSP, [MY]=0X%04X \n[Value]=%d\n", pbuf[28], my, pbuf[29]);
		return;
	}
	if ((UXGCR_SET_DESC | 0X8000) == cId || (UXGCR_REQ_DESC | 0X8000) == cId)
	{
		memcpy(&my, &pbuf[26], sizeof(unsigned short));
		fprintf(cgiOut, "U%dGCR_RSP, [MY]=0X%04X \n[Value]=%d\n", pbuf[28], my, pbuf[29]);
		return;
	}

	fprintf(cgiOut, "Unprocess AF_IND, cId=0X%04X\n", cId);

	return;
}


/////////////////////////////////////////////////////////////////////////////////////////
//

int cgiMain()
{
	int res = 0;
	char temp[320];

	int v;

	int i;
	int l;
	unsigned char frame[320];

	char host[32];
	int port = 0;

	char packet[320];
	int len;

	int cmd;
	int cid;
	int sa;

	int state = 0;
	int sd = -1;
	fd_set rset;
	struct timeval tv;
	time_t now;
	time_t first;

	unsigned char ch;
	unsigned char rb[320], *pb;
	int left;
	unsigned char tl;

	cgiHeaderContentType("text/xml");

	fprintf(cgiOut, "<?xml version='1.0' encoding='Big5' ?> \n");
	fprintf(cgiOut, "<ZTOOL>\n");
	fprintf(cgiOut, "\t");
	fprintf(cgiOut, "<PACKET>");



/////////////////////////////////////////////////////////////////////////////////////////
// host, port

	memset(temp, 0, sizeof(host));
	res = cgiFormStringNoNewlines("host", host, 32);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) host not found !!", __FILE__, __LINE__);
		goto END;
	}

	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("port", temp, 16);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) packet not found !!", __FILE__, __LINE__);
		goto END;
	}
	port = atoi(temp);
	if (port < 5000 || port > 500000)
	{
		res = 1, fprintf(cgiOut, "(%s %d) port error !!", __FILE__, __LINE__);
		goto END;
	}


/////////////////////////////////////////////////////////////////////////////////////////
// 

	memset(temp, 0, sizeof(temp));
	res = cgiFormStringNoNewlines("packet", temp, 320);
	if (cgiFormNotFound == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) packet not found !!", __FILE__, __LINE__);
		goto END;
	}
	l = strlen(temp);
	if (l < 5 || l > 300)
	{
		res = 1, fprintf(cgiOut, "(%s %d) packet error !!", __FILE__, __LINE__);
		goto END;
	}
	if ((l % 3) != 2)
	{
		res = 1, fprintf(cgiOut, "(%s %d)packet error !!", __FILE__, __LINE__);
		goto END;
	}
	for (i = 0; i < l; i++)
	{
		if  (2 == (i % 3))
		{
			if (temp[i] != '-')
			{
				res = 1, fprintf(cgiOut, "(%s %d) packet error !!", __FILE__, __LINE__);
				goto END;
			}
		}
		else
		{
			if (temp[i] != '0' && temp[i] != '1' && temp[i] != '2' && temp[i] != '3' && temp[i] != '4' && temp[i] != '5' && temp[i] != '6' && temp[i] != '7' 
				&& temp[i] != '8' && temp[i] != '9' && temp[i] != 'A' && temp[i] != 'B' && temp[i] != 'C' && temp[i] != 'D' && temp[i] != 'E' && temp[i] != 'F')
			{
				res = 1, fprintf(cgiOut, "(%s %d) packet error !!", __FILE__, __LINE__);
				goto END;
			}
		}
	}

	memcpy(packet, temp, sizeof(packet));
	for (i = 0; i < sizeof(packet); i++)
	{
		if (packet[i] == '-')
			packet[i] = 0;
	}
	res = sscanf(packet + 3 * 1, "%X", &len);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
		goto END;
	}
	len &= 0XFF;
	if (0X0F != len)
	{
		res = 1, fprintf(cgiOut, "(%s %d) len error !!", __FILE__, __LINE__);
		goto END;
	}

	memset(temp, 0, sizeof(temp));
	memcpy(temp + 2 * 0, packet + 3 * 3, 2);
	memcpy(temp + 2 * 1, packet + 3 * 2, 2);
	res = sscanf(temp, "%X", &cmd);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
		goto END;
	}
	cmd &= 0XFFFF;
	if (cmd != 0X0124)
	{
		res = 1, fprintf(cgiOut, "(%s %d) cmd error, cmd=0X%04X !!", __FILE__, __LINE__, (unsigned short)cmd);
		goto END;
	}

	memset(temp, 0, sizeof(temp));
	memcpy(temp + 2 * 0, packet + 3 * 5, 2);
	memcpy(temp + 2 * 1, packet + 3 * 4, 2);
	res = sscanf(temp, "%X", &sa);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
		goto END;
	}
	sa &= 0XFFFF;

	memset(temp, 0, sizeof(temp));
	memcpy(temp + 2 * 0, packet + 3 * 9, 2);
	memcpy(temp + 2 * 1, packet + 3 * 8, 2);
	res = sscanf(temp, "%X", &cid);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
		goto END;
	}
	cid &= 0XFFFF;
	if (cid != PHOTO_REQ_DESC)
	{
		res = 1, fprintf(cgiOut, "(%s %d) cid error !!", __FILE__, __LINE__);
		goto END;
	}



/////////////////////////////////////////////////////////////////////////////////////////
// 

	if (0 > (sd = Conn(host, port)))
	{
		res = 1, fprintf(cgiOut, "(%s %d) Conn() fail !", __FILE__, __LINE__);
		goto END;
	}
	v = 1 * 1024 * 1024;
	setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v));
	setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v));

/*
	if (SIG_ERR == signal(SIGINT, SigInt))
	{
		res = 1, fprintf(cgiOut, "(%s %d) signal error !", __FILE__, __LINE__);
		goto END;
	}
	if (SIG_ERR == signal(SIGTERM, SigInt))
	{
		res = 1, fprintf(cgiOut, "(%s %d) signal error !", __FILE__, __LINE__);
		goto END;
	}
	if (SIG_ERR == signal(SIGHUP, SigInt))
	{
		res = 1, fprintf(cgiOut, "(%s %d) signal error !", __FILE__, __LINE__);
		goto END;
	}
*/
	if (1)
	{
		unsigned char hs[] = {0XFE, 0X00, 0X00, 0X04, 0X04};

		pb = hs;
		left = 5;
		for (;;)
		{
			res = write(sd, pb, left);
			if (res < 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) write() fail !\n", __FILE__, __LINE__), close(sd);
				goto END;
			}
			pb += res;
			left -= res;
			if (left <= 0)
				break;
		}
	}


/////////////////////////////////////////////////////////////////////////////////////////
// 
	//sscanf(packet + 3 * 1, "%X", &l);
	//if (-1 == res)
	//{
	//	res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__), close(sd);
	//	goto END;
	//}
	//l &= 0XFF;
	//l += 5;
	memset(frame, 0, sizeof(frame));
	for (i = 0; i < len + 5; i++)
	{
		int x;

		res = sscanf(packet + 3 * i, "%X", &x);
		if (-1 == res)
		{
			res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__), close(sd);
			goto END;
		}
		x &= 0XFF;
		frame[i] = (unsigned char)x;
	}
	pb = frame;
	left = len + 5;
	for (;;)
	{
		res = write(sd, pb, left);
		if (res < 0)
		{
			res = 1, fprintf(cgiOut, "(%s %d) write() fail !", __FILE__, __LINE__), close(sd);
			goto END;
		}
		pb += res;
		left -= res;
		if (left <= 0)
			break;
	}






#if 1// jc note, ext
	state = 0;
	time(&first);
	do
	{
		time(&now);
		if ((int)(now - first) > 3)
		{
			res = 1, fprintf(cgiOut, "(%s %d) RSP timeout !!\n", __FILE__, __LINE__), close(sd);
			goto END;
		}
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 3;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)
			continue;
		if (res < 0)
		{
			res = 1, fprintf(cgiOut, "(%s %d) select() fail !\n", __FILE__, __LINE__), close(sd);
			goto END;
		}
		if (0 == state)		// 0xfe
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd);
				goto END;
			}
			if (ch == 0XFE)
			{
				state = 1;
				rb[0] = ch;
			}
			continue;
		}
		if (1 == state)		// tok len
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd);
				goto END;
			}
			state = 2;
			rb[1] = ch;
			left = (int)ch + 3;
			pb = &rb[2];
			continue;
		}
		if (2 == state)
		{
			len = recv(sd, (char *)pb, left, 0);
			if (len <= 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd);
				goto END;
			}
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
			{
				res = 1, fprintf(cgiOut, "(%s %d) XorSum error !\n", __FILE__, __LINE__), close(sd);
				goto END;
			}
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		cmd &= 0XFFFF;
		if (cmd != 0X0164)
			continue;
		if (0 != rb[4])
		{
			fprintf(cgiOut, "[AF_DATA_REQUEST]   FAIL !!\n"), close(sd);
			goto END;
		}
		fprintf(cgiOut, "[AF_DATA_REQUEST]   SUCCESS !!\n");
		break;
	}
	while(1);
#endif //












	state = 0;
	time(&first);
	do
	{
		unsigned short addr;

		time(&now);
		if ((int)(now - first) > 6)
		{
			res = 1, fprintf(cgiOut, "(%s %d) RSP timeout !!", __FILE__, __LINE__), close(sd);
			goto END;
		}
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 3;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)
		{
			continue;
			//res = 1, fprintf(cgiOut, "(%s %d) select() timeout !", __FILE__, __LINE__), close(sd);
			//goto END;
		}
		if (res < 0)
		{
			res = 1, fprintf(cgiOut, "(%s %d) select() fail !", __FILE__, __LINE__), close(sd);
			goto END;
		}
		if (0 == state)		// 0xfe
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !", __FILE__, __LINE__), close(sd);
				goto END;
			}
			if (ch == 0XFE)
			{
				state = 1;
				rb[0] = ch;
			}
			continue;
		}
		if (1 == state)		// tok len
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !", __FILE__, __LINE__), close(sd);
				goto END;
			}
			state = 2;
			rb[1] = ch;
			left = (int)ch + 3;
			pb = &rb[2];
			continue;
		}
		if (2 == state)
		{
			len = recv(sd, (char *)pb, left, 0);
			if (len <= 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !", __FILE__, __LINE__), close(sd);
				goto END;
			}
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
			{
				res = 1, fprintf(cgiOut, "(%s %d) XorSum error !", __FILE__, __LINE__), close(sd);
				goto END;
			}
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		cmd &= 0XFFFF;
		if (cmd != 0X8144)
		{
			fprintf(cgiOut, "(%s %d) cmd=0X%04X, rb[2]=0X%02X, rb[3]=0X%02X !", __FILE__, __LINE__, cmd, rb[2], rb[3]);
			continue;
		}

		memcpy(&l, &rb[6], sizeof(unsigned short));
		l &= 0XFFFF;
		if (l != (cid | 0X8000))
		{
			fprintf(cgiOut, "(%s %d) cid=0X%04X !", __FILE__, __LINE__, cid);
			continue;
		}
		memcpy(&addr, &rb[8], sizeof(unsigned short));
		addr &= 0XFFFF;
		if (addr != sa)
			continue;
		break;
	}
	while(1);

	close(sd);

	ProcessAfIncoming(0, rb);


END:
	fprintf(cgiOut, "\nDONE\n");
	fprintf(cgiOut, "</PACKET>\n");
	fprintf(cgiOut, "</ZTOOL>\n");

	return res;
}

