
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

#define PULSE_COUNT_SET_DESC			0X0016
#define PULSE_COUNT_REQ_DESC			0X0017

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

unsigned int calccrc(unsigned char crcbuf, unsigned int crc)
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

unsigned int crc16(unsigned char *buf, unsigned short len)
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


typedef struct tm SYSTEMTIME;

void GetLocalTime(SYSTEMTIME *st)
{
	time_t now;
	struct tm *pst = NULL;

	//time_t t = time(NULL);
	time(&now);

	pst = localtime(&now);
	memcpy(st, pst, sizeof(SYSTEMTIME));
	//st->tm_year += 1900;
}


/////////////////////////////////////////////////////////////////////////////////////////
//

void ProcessWhoamiRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short pa = 0XFFF0;
	unsigned short my = 0XFFF0;
	unsigned char nep = 0;
	unsigned short pan = 0XFFF0;
	unsigned int chlist = 0XFFFFFFF0;
	char type[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char lqi = 0, rssi =0;
	unsigned char sprf;
	char SPF[32];
	unsigned short prof = 0XFFF0;
	char i = 0, lst[512], tmp[64];

	memset(lst, 0, 512);
	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	memcpy(&pa, &pdesc[17], sizeof(unsigned short)), pa &= 0XFFFF;
	nep = pdesc[19];
	memcpy(&prof, &pdesc[19 +nep + 1], sizeof(unsigned short)), prof &= 0XFFFF;;
	memcpy(&pan, &pdesc[19 +nep + 3], sizeof(unsigned short)), pan &= 0XFFFF;
	memcpy(&chlist, &pdesc[19 + nep + 5], sizeof(unsigned int));
	for (i = 0; i < 16; i++)	if ((0x00000800 << i) == (chlist & (0x00000800 << i)))	sprintf(tmp, "%d,", 2405 + 5 * i), strcat(lst, tmp);
	if (strlen(lst) > 0)						lst[strlen(lst) - 1] = 0;
	sprf = pdesc[19 + nep + 9];
	if (0 == sprf)							strcpy(SPF, "NETWORK_SPECIFIC");
	else if (1 == sprf)						strcpy(SPF, "HOME_CONTROLS");
	else if (2 == sprf)						strcpy(SPF, "ZIGBEEPRO");
	else if (3 == sprf)						strcpy(SPF, "GENERIC_STAR");
	else									strcpy(SPF, "GENERIC_TREE");
	if (0 == pdesc[19 + nep + 10])			strcpy(type, "COORDINATOR");
	else if (1 == pdesc[19 + nep + 10])		strcpy(type, "ROUTER");
	else if (2 == pdesc[19 + nep + 10])		strcpy(type, "END-DEVICE");
	lqi = pdesc[19 + nep + 11];
	rssi = pdesc[19 + nep + 12];
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	if (0 != lqi && 255 != lqi)
		printf("WHOAMI_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [PA]=0X%04X [PAN]=0X%04X [CH-LIST]=%s [TYPE]=%s [LQI]=0X%02X,%u [RSSI]=%d \n", 
			cId, my, sa, ia, pa, pan, lst, type, lqi, lqi, (char)rssi);
	else
		printf("WHOAMI_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [PA]=0X%04X [PAN]=0X%04X [CH-LIST]=%s [TYPE]=%s [LQI]=N/A [RSSI]=N/A \n", 
			cId, my, sa, ia, pa, pan, lst, type);
}

void ProcessGpioRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char P;
	unsigned char V;
	unsigned char i, j;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	P = pdesc[7];
	if (P > 1)	P = 1;
	V = pdesc[8];
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("GPIO_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64", [PORT]=0X%02X [VALUE]=0X%02X, ", cId, my, sa, ia, P, V);
	for (i = 0; i < 8; i++)
	{
		j = V & (1 << i);
		if (0 != j)	j = 1;
		if (i == 7)	printf("P%d_%d=%d\n", P, i, j);
		else			printf("P%d_%d=%d ", P, i, j);
	}
}

void ProcessModbusQueryRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short start;
	unsigned short my;
	unsigned char slave, fn, bc;
	unsigned short i, w;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	memcpy(&start, &pdesc[7], sizeof(unsigned short)), start &= 0XFFFF;
	slave = pdesc[9];
	fn = pdesc[10];
	bc = pdesc[11];
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("MODBUS_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" ", cId, my, sa, ia);
	printf("[START]=0X%04X ", start);
	printf("[SLAVE]=0X%02X ", slave);
	printf("[FN]=0X%02X ", fn);
	printf("[BC]=0X%02X ", bc);
	for (i = 0; i < bc / 2; i++)	w = (BUILD_UINT16(pdesc[13 + 2 * i], pdesc[12 + 2 * i])), printf("[R-0X%04X]=0X%04X ", start + i, w);
	printf("\n");
}

void ProcessPwmRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("PWM_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=0X%04X [VALUE]=%u\n", cId, sa, ia, my, BUILD_UINT16(pdesc[7], pdesc[8]));
	return;
}

void ProcessAdcRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char n = 0, i = 0;
	unsigned char *p;
	unsigned short v;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	n = pdesc[7];
	p = &pdesc[8];
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("ADC_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" ", cId, my, sa, ia);
	for (i = 0; i < n; i++)		memcpy(&v, p, sizeof(unsigned short)), p += 2, printf("[AD%d]=0X%04X,%05d ", i, v, v);
	printf("\n");
}

void ProcessJboxRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned short v = 0XFFF0;
	unsigned short a = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	memcpy(&v, &pdesc[7], sizeof(unsigned short));
	memcpy(&a, &pdesc[9], sizeof(unsigned short));
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("JBOX_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [VOLTAGE]=%05u [CURRENT]=%05u \n", cId, my, sa, ia, v, a);
}

void ProcessNegTempRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned short v = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	memcpy(&v, &pdesc[7], sizeof(unsigned short));
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	if (v > 80)				printf("THERM_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [THERM TEMPERATURE]=%02.1f ¢XC\n", cId, my, sa, ia, (float)v / 10);
	else						printf("THERM_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [THERM TEMPERATURE]=%02.1f ¢XC\n", cId, my, sa, ia, (float)v);
}

void ProcessPulseCountRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned int C = 0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	memcpy(&C, &pdesc[7], sizeof(unsigned int));
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("PULSE_COUNT_RSP (0X%04X), [MY]=0X%02X [SA]=0X%04X [IA]=0X%016"PRIX64" ", cId, my, sa, ia);
	printf("COUNTER=%08u\n", C);
}

void ProcessPulseOutRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("PULSEOUT_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64", [PORT]=%d, [PIN]=%d\n", cId, my, sa, ia, pdesc[6], pdesc[7]);
}

void ProcessResetRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("RESET_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64"\n", cId, my, sa, ia);
}

void ProcessModbusPresetRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	unsigned short start;
	unsigned short data;
	unsigned char slave, fn;
	slave = pdesc[7];
	fn = pdesc[8];
	start = BUILD_UINT16(pdesc[10], pdesc[9]);
	data = BUILD_UINT16(pdesc[12], pdesc[11]);

	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("MODBUS_PRESET_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" ", cId, my, sa, ia);
	printf("[SLAVE]=0X%02X ", slave);
	printf("[FN]=0X%02X ", fn);
	printf("[START]=0X%04X ", start);
	printf("[DATA]=0X%04X \n", data);
}

void ProcessRtsTimeoutRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("RTS_TIMEOUT_RSP (0X%04X) , [MY]=0X%04X [IA]=0X%016"PRIX64" [TIMEOUT]=%d \n", cId, my, ia, pdesc[7]);
}

void ProcessUxBaudRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("UX_BAUD_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64", [PORT]=%d, [VALUE]=0X%02X \n", cId, my, sa, ia, pdesc[7], pdesc[8]);
}

void ProcessUxGcrRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("UX_GCR_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64", [PORT]=%d, [VALUE]=0X%02X \n", cId, my, sa, ia, pdesc[7], pdesc[8]);
}

void ProcessTxCtrlRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("TXCTRLL_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [VALUE]=%u\n", cId, my, sa, ia, pdesc[7]);
}

void ProcessPhotoRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("PHOTO_RSP (0X%04X), [MY]=0X%02X [SA]=0X%04X [IA]=0X%016"PRIX64" [VALUE]=%02u\n", cId, my, sa, ia, BUILD_UINT16(pdesc[7], pdesc[8]));
}

void ProcessNvRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	int attr = 0;
	unsigned int value = 0;
	int i;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	memcpy(&attr, &pdesc[7], sizeof(unsigned short)), attr &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	if ((0X0081 != attr) && (1 == pdesc[9] || 2 == pdesc[9] || 4 == pdesc[9]))
	{
		memcpy(&value, &pdesc[10], pdesc[9]), value &= 0XFFFFFFFF;
		if (1 == pdesc[9])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=0X%02X\n", cId, sa, ia, my, my, attr, pdesc[9], value);
		if (2 == pdesc[9])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=0X%04X\n", cId, sa, ia, my, my, attr, pdesc[9], value);
		if (4 == pdesc[9])	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=%05u,0X%04X [ATTRIB]=0X%04X [LEN]=%u [VALUE]=0X%08X\n", cId, sa, ia, my, my, attr, pdesc[9], value);
		return;
	}

	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	if (0X0081 != attr)
	{
		printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=0X%04X [ATTRIB]=0X%04X [LEN]=%u ", cId, sa, ia, my, attr, pdesc[9]);
		for (i = 0; i < pdesc[9]; i++)			printf("DATA[%u]=0X%02X ", i, pdesc[10 + i]);
		printf("\n");
		return;
	}

	printf("NV_RSP (0X%04X), [SA]=0X%04X [IA]=0X%016"PRIX64" [MY]=0X%04X [ATTRIB]=0X%04X [LEN]=%u ", cId, sa, ia, my, attr, pdesc[9]);
	printf("ZCD_NV_USERDESC=\"%s\"\n", &pdesc[10]);
	return;
}

void ProcessAdcCfgRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char V;
	unsigned char i;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("ADC_CFG_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64", [VALUE]=0X%02X, ", cId, my, sa, ia, pdesc[8]);
	V = pdesc[7];
	for (i = 0; i < 8; i++)
	{
		if (V & (1 << i))			printf("P0_%d,AIN-ENABLED ", i);
		else						printf("P0_%d,AIN-DISABLED", i);
	}
	printf("\n");
}

void ProcessCompileDateRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char sl = 0, x = 0;
	char szDate[256];

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	sl = pdesc[7];
	memset(szDate, 0, sizeof(szDate));
	for (x = 0; x < sl; x++)			szDate[x] = pdesc[8 + x];
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("COMPILE_DATE_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" ", cId, my, sa, ia);
	printf("[COMPILE_DATE]=%s\n", szDate);
}

void ProcessPxSelRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char P = 0;
	unsigned char V;
	unsigned char i;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	P = pdesc[7];
	if (P > 1)	P = 1;
	V = pdesc[8];
	printf("PXSEL_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64", [PORT]=0X%02X [VALUE]=0X%02X ", cId, my, sa, ia, P, V);
	for (i = 0; i < 8; i++)
	{
		if (V & (1 << i))			printf("P%d_%d=PERIPHERAL ", P, i);
		else						printf("P%d_%d=GPIO ", P, i);
	}
	printf("\n");
}

void ProcessPxDirRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned char P = 0;
	unsigned char V;
	unsigned char i;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	P = pdesc[7];
	if (P > 1)	P = 1;
	V = pdesc[8];
	printf("PXDIR_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64", [PORT]=0X%02X [VALUE]=0X%02X ", cId, my, sa, ia, P, V);
	for (i = 0; i < 8; i++)
	{
		if (V & (1 << i))			printf("P%d_%d=OUTPUT ", P, i);
		else						printf("P%d_%d=INPUT ", P, i);
	}
	printf("\n");
}

void ProcessP0InpRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	int i;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("P0INP_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [VALUE]=0X%02X ", cId, my, sa, ia, pdesc[7]);
	for (i = 0; i < 8; i++)
	{
		if (pdesc[7] & (1 << i))		printf("P%d_%d=3-State ", 0, i);
		else						printf("P%d_%d=Pullup/pulldown ", 0, i);
	}
	printf("\n");
}

void ProcessP1InpRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	int i;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("P1INP_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [VALUE]=0X%02X ", cId, my, sa, ia, pdesc[7]);
	for (i = 0; i < 8; i++)
	{
		if (pdesc[7] & (1 << i))		printf("P%d_%d=3-State ", 0, i);
		else						printf("P%d_%d=Pullup/pulldown ", 1, i);
	}
	printf("\n");
}

void ProcessP2InpRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	int i;
	int pdup2 = 0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	printf("P2INP_RSP (0X%04X), [MY]=0X%04X [SA]=0X%04X [IA]=0X%016"PRIX64" [VALUE]=0X%02X ", cId, my, sa, ia, pdesc[7]);
	for (i = 0; i < 8; i++)
	{
		if (i != 7)					continue;
		if (pdesc[7] & (1 << i))		pdup2 = 1;
		else						pdup2 = 0;
	}
	for (i = 0; i < 8; i++)
	{
		if (i <= 4)
		{
			if (pdesc[7] & (1 << i))	printf("P%d_%d=3-State ", 2, i);
			else					printf("P%d_%d=%s ", 2, i, (0 == pdup2) ? "Pullup" : "Pulldown");
			continue;
		}
		if (pdesc[7] & (1 << i))		printf("P%d_X=Pulldown ", i - 5);
		else						printf("P%d_X=Pullup ", i - 5);
	}
	printf("\n");
}

void ProcessPwmAdcRsp(SYSTEMTIME st, unsigned short cId, unsigned short sa, unsigned long long int ia, unsigned char len, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	short ad = 0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short)), my &= 0XFFFF;
	printf("PWM_DETECT_RSP, [MY]=0X%04X [IA]=0X%016"PRIX64" ", my, ia);
	memcpy(&ad, &pdesc[7], sizeof(unsigned short));
	printf("[V0]=%d \n", ad);
	memcpy(&ad, &pdesc[9], sizeof(unsigned short));
	printf("[V1]=%d \n", ad);
	memcpy(&ad, &pdesc[11], sizeof(unsigned short));
	printf("[V2]=%d \n", ad);
	memcpy(&ad, &pdesc[13], sizeof(unsigned short));
	printf("[V3]=%d \n", ad);
	if (len > 15)
	{
		memcpy(&ad, &pdesc[15], sizeof(unsigned short));
		printf("[AD0]=%05d,0X%04X ", ad, ad);
		memcpy(&ad, &pdesc[17], sizeof(unsigned short));
		printf("[AD1]=%05d,0X%04X ", ad, ad);
		memcpy(&ad, &pdesc[19], sizeof(unsigned short));
		printf("[AD2]=%05d,0X%04X ", ad, ad);
		memcpy(&ad, &pdesc[21], sizeof(unsigned short));
		printf("[AD3]=%05d,0X%04X ", ad, ad);
	}
	printf("\n");
}


void ProcessAfIncoming(unsigned short cmd, unsigned char *pbuf)
{
	unsigned long long int ia = 0X00124B000133B481LL;
	unsigned short cId = 0;
	unsigned short sa = 0XFFF0;
	SYSTEMTIME st;

	GetLocalTime(&st);
	memcpy(&cId, &pbuf[6], sizeof(unsigned short)), cId &= 0XFFFF;
	memcpy(&sa, &pbuf[8], sizeof(unsigned short)), sa &= 0XFFFF;
	((unsigned char *)&ia)[0] = pbuf[21];
	((unsigned char *)&ia)[1] = pbuf[22];
	((unsigned char *)&ia)[2] = pbuf[23];


	if (WHOAMI_RSP_DESC == cId)
		return ProcessWhoamiRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((GPIO_SET_DESC | 0X8000) == cId || (GPIO_REQ_DESC | 0X8000) == cId)
		return ProcessGpioRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (MODBUSQRY_RSP_DESC == cId)
		return ProcessModbusQueryRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((PWM_SET_DESC | 0X8000) == cId || (PWM_REQ_DESC | 0X8000) == cId)
		return ProcessPwmRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (ADC_RSP_DESC == cId)
		return ProcessAdcRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (JBOX_RSP_DESC == cId)
		return ProcessJboxRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (NTEMP_RSP_DESC == cId)
		return ProcessNegTempRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (0X8055 == cId)
		return ProcessPulseOutRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((PULSE_COUNT_REQ_DESC | 0X8000) == cId || (PULSE_COUNT_SET_DESC | 0X8000) == cId)
		return ProcessPulseCountRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (COMPILE_DATE_RSP_DESC == cId)
		return ProcessCompileDateRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (RESET_RSP_DESC == cId)
		return ProcessResetRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (0X80BA == cId)
		return ProcessRtsTimeoutRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((ADCCFG_SET_DESC | 0X8000) == cId || (ADCCFG_REQ_DESC | 0X8000) == cId)
		return ProcessAdcCfgRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (0X80B8 == cId)
		return ProcessModbusPresetRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (0X802F == cId || 0X8030 == cId)
		return ProcessUxGcrRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (0X802D == cId || 0X802E == cId)
		return ProcessUxBaudRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((0X0012 | 0X8000) == cId || (0X0013 | 0X8000) == cId)
		return ProcessNvRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if (PHOTO_RSP_DESC == cId)
		return ProcessPhotoRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((0X001B | 0X8000) == cId || (0X001A | 0X8000) == cId)
		return ProcessTxCtrlRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((PXSEL_SET_DESC | 0X8000) == cId || (PXSEL_REQ_DESC | 0X8000) == cId)
		return ProcessPxSelRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((PXDIR_SET_DESC | 0X8000) == cId || (PXDIR_REQ_DESC | 0X8000) == cId)
		return ProcessPxDirRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((0X00B3 | 0X8000) == cId || (0X00B2 | 0X8000) == cId)
		return ProcessP0InpRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((0X00B5 | 0X8000) == cId || (0X00B4 | 0X8000) == cId)
		return ProcessP1InpRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((0X00B6 | 0X8000) == cId || (0X00B7 | 0X8000) == cId)
		return ProcessP2InpRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);
	if ((0X00B1 | 0X8000) == cId)
		return ProcessPwmAdcRsp(st, cId, sa, ia, pbuf[20], &pbuf[21]);

	return;
}

// 0xfe Len Cmd0 Cmd1 Gid0 Gid1 Cid0 Cid1 Smode Ie0 Ie1 Ie2 Ie3 Ie4 Ie5 Ie6 Ie7 Sep Span0 Span1 Dep Brocast Lqi Security Stamp0 Stamp1 Stamp2 Stamp3 Seq DataLen Data0 Data1 Data2 Data3 Data4 My0 My1 ...
void ProcessAfExtIncoming(unsigned short cmd, unsigned char *pbuf)
{
	unsigned long long int ia = 0X00124B000133B481LL;
	unsigned short cId = 0;
	unsigned short sa;
	SYSTEMTIME st;

	//if (pbuf[8] != 2)		return;
	//printf("ProcessAfExtIncoming(), 0X%02X\n", pbuf[8]);

	GetLocalTime(&st);
	memcpy(&ia, &pbuf[9], sizeof(unsigned long long int));
	sa = (ia & 0XFFFF);
	memcpy(&cId, &pbuf[6], sizeof(unsigned short)), cId &= 0XFFFF;
	ia = 0X00124B000133B481LL;
	((unsigned char *)&ia)[0] = pbuf[21 + 9];
	((unsigned char *)&ia)[1] = pbuf[22 + 9];
	((unsigned char *)&ia)[2] = pbuf[23 + 9];

	if (MODBUSQRY_RSP_DESC == cId)
		return ProcessModbusQueryRsp(st, cId, sa, ia, pbuf[20 + 9], &pbuf[21 + 9]);
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

int PackDesc(const unsigned short cId, unsigned char *pbuf)
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

	int sd = -1;
	int v;
	int len;
	int ack = 0;

	int res;
	int state = 0;
	fd_set rset;
	struct timeval tv;
	int c;
	unsigned char ch;
	unsigned char rb[4096];
	//unsigned char tl = 0;
	unsigned short cmd;
	static int left = 0;
	static unsigned char *p = 0;
	SYSTEMTIME st;
	unsigned int count = 0;


	//setbuf(stdout, 0);
	strcpy(host, "localhost");
	if (argc < 3)									PrintUsage();

	while ((c = getopt(argc, argv, "H:h:P:p:")) != -1)
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
		}
	}
	if (0 > (sd = CONNECT(host, port)))				printf("TCP CONNECT TO (%s %d) fail !\n", host, port), exit(1);
	v = 16 * 1024 * 1024;
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > (v = fcntl(sd, F_GETFL, 0)))
		printf("(%s %d) FCNTL() fail, F_GETFL, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > fcntl(sd, F_SETFL, v | O_NONBLOCK))
		printf("(%s %d) FCNTL() F_SETFL fail, O_NONBLOCK, EXIT \n", __FILE__, __LINE__), close(sd), exit(1);

	state = 0;
	do
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv)), tv.tv_sec = 300;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)								break;
		if (res < 0)								printf("(%s %d) SELECT() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)							printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (0XFE == ch)
			{
				state = 1;
				rb[0] = ch;
				continue;
			}
			state = 0;
			printf("X");
			continue;
		}
		if (1 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)							printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (0XFE == ch)
			{
				state = 2;
				rb[1] = ch;
				continue;
			}
			state = 0;
			printf("x");
			continue;
		}
		if (2 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)							printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			state = 3;
			rb[2] = ch;
			continue;
		}
		if (3 == state)
		{
			len = read(sd, &ch, 1);
			if (len <= 0)							printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			state = 4;
			rb[3] = ch;
			left = 6 + BUILD_UINT16(rb[3], rb[2]);
			if (left > 64)
			{
				state = 0;
				printf("#");
				continue;
			}
			p = &rb[4];
			continue;
		}

		if (4 == state)
		{
			unsigned int crc0 = 0;
			unsigned int crc1 = 0;

			len = read(sd, (char *)p, left);
			if (len <= 0)							printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(sd), exit(1);
			p += len;
			left -= len;
			if (left > 0)							continue;
			state = 0;
			len = BUILD_UINT16(rb[3], rb[2]);
			crc0 = BUILD_UINT16(rb[len + 9], rb[len + 8]);
			crc1 = crc16(rb, len + 8);
			if (crc0 != crc1)
			{
				printf("\n");
				printf("(%s %d) CRC16 ERROR !\n", __FILE__, __LINE__);
				continue;
			}
		}

		ack = rb[5];
		if (0 == ack)
			continue;
		GetLocalTime(&st);
		len = BUILD_UINT16(rb[3], rb[2]);
		cmd = BUILD_UINT16(rb[7], rb[6]);
		if (cmd == 0X0000)
		{
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("HELLO_RSP\n");
			continue;
		}

		if (cmd == 0X0010)
		{
			if (2 != len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("KEY_NOTIFY, number=%d, status=%d\n", rb[8], rb[9]);
			continue;
		}

		if (cmd == 0X0011)
		{
			if (0 == len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("WHR_NOTIFY, value=%d\n", rb[8]);
			continue;
		}
		if (cmd == 0X0012)
		{
			if (0 == len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("CHR_NOTIFY, value=%d\n", rb[8]);
			continue;
		}

		if (cmd == 0X0016)
		{
			if (0 == len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			if (1 == len)
				printf("BIKE_RPM_NOTIFY_8, value=%d\n", rb[8]);
			else
				printf("BIKE_RPM_NOTIFY_16, value=%d\n", BUILD_UINT16(rb[9], rb[8]));
			continue;
		}

		if (cmd == 0X0018)
		{
			if (0 == len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("BUZZER_RSP, value=%d\n", rb[8]);
			continue;
		}

		if (cmd == 0X0074)
		{
			if (1 != len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("CG6_PEDAL_RPM_RSP, value=%d\n", rb[8]);
			continue;
		}
		if (cmd == 0X0078)
		{
			if (1 != len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("CG6_WEIGHT_LIFT_RSP, left=%d right=%d\n", rb[8] & 0X0F, rb[8] >> 4);
			continue;
		}
		if (cmd == 0X0079)
		{
			if (1 != len)
				continue;
			printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
			printf("CG6_LOW_RPM_NOTIFY\n");
			continue;
		}

		printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
		printf("UNKNOWN, did=0X%04X\n", cmd);

	}
	while(1);

	GetLocalTime(&st);
	printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year + 1900, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)mktime(&st));
	if (count > 60000)
		close(sd), printf("EXIT 0\n"), exit(0);
	close(sd), printf("EXIT 1\n");
	return 1;
}




