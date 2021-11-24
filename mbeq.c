
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

static int PackAfCmd(unsigned short addr, unsigned short cId, unsigned char opt, unsigned char radius, unsigned char dataLen, unsigned char *pdata)
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

static int Conn(char *domain, int port)
{
	int sock_fd;
	struct hostent *site;
	struct sockaddr_in me;
	int v;
 
	site = gethostbyname(domain);
	if (0 == site)
		printf("(%s %d) gethostbyname() fail !\n", __FILE__, __LINE__), exit(1);
	if (0 >= site->h_length)
		printf("(%s %d) 0 >= site->h_length \n", __FILE__, __LINE__), exit(1);

	if (0 > (sock_fd = socket(AF_INET, SOCK_STREAM, 0)))
		printf("(%s %d) socket() fail !\n", __FILE__, __LINE__), exit(1);

	v = 1;
	if (-1 == setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
		printf("(%s %d) setsockopt() fail !\n", __FILE__, __LINE__), exit(1);

	if (0 == memset(&me, 0, sizeof(struct sockaddr_in)))
		printf("(%s %d) memset() fail !\n", __FILE__, __LINE__), exit(1);

	memcpy(&me.sin_addr, site->h_addr_list[0], site->h_length);
	me.sin_family = AF_INET;
	me.sin_port = htons(port);

	return (0 > connect(sock_fd, (struct sockaddr *)&me, sizeof(struct sockaddr))) ? -1 : sock_fd;
}

static void SigInt (int signo)
{
	if (SIG_ERR == signal(signo, SIG_IGN))
		printf("signal error");

	if (SIG_ERR == signal(signo, SigInt))
		printf("signal error");
}

static void PrintUsage(void)
{
	printf("Example: mbeq -h [host] -p [connect port] -B [baud rate] -T [RTS timeout] -I [IEEE address] -S [slave address] -F [function code] -R [start register] -N [number]\n");
	printf("Example: mbeq -hlocalhost -p9000 -B57600 -T5 -I0X00124B0001142FD0 -S1 -F3 -R0X0092 -N26\n"), exit(1);
}

static void ProcessModbusRsp(unsigned char descLen, unsigned char *pdesc)
{
	unsigned short my = 0XFFF0;
	unsigned short start = 0XFFF0;
	unsigned char slave = 0;
	unsigned char fn = 0;
	unsigned char bc = 0;

	int i = 0;
	unsigned short w = 0;

	memcpy(&my, &pdesc[5], sizeof(unsigned short));
	memcpy(&start, &pdesc[7], sizeof(unsigned short));
	slave = pdesc[9];
	fn = pdesc[10];
	bc = pdesc[11];

	if (0 != (bc % 2))
		return;

	printf("MODBUSQRY_RSP, [MY]=%05u,0X%04X [START]=0X%04X [SLAVE]=%02d [FN]=%02d [BC]=%02d\n", my, my, start, slave, fn, bc);

	// 12-26h, 2-19h
	if (0X0026 == start && 3 == fn && 2 == bc && 12 == slave)
	{
		printf("AM5H-A\n");
		printf("[Radition]=%d W/M^2\n", BUILD_UINT16(pdesc[13], pdesc[12]));
		return;
	}
	if (0X0019 == start && 3 == fn && 2 == bc && 2 == slave)
	{
		printf("AM5H-T\n");
		printf("[Temperature]=%04d C\n", BUILD_UINT16(pdesc[13], pdesc[12]));
		return;
	}

	if (0X0026 == start && 3 == fn && 4 == bc)
	{
		unsigned long rad;
		double radition;

		rad = BUILD_UINT32(pdesc[15], pdesc[14], pdesc[13], pdesc[12]);
		radition = (double)rad / 10.00;
		if (radition < 0.00)			radition = 0.00;
		else if (radition > 2000.00)	radition = 0.00;

		printf("DC5H-A\n");
		printf("[Radition]=%.02f W/M^2\n", radition);
		return;
	}

	if (0X0019 == start && 3 == fn && 4 == bc)
	{
		printf("AM5H-T\n");
		printf("[Temperature]=%04d C\n", BUILD_UINT32(pdesc[15], pdesc[14], pdesc[13], pdesc[12]));
		return;
	}

	if (0XC020 == start && 3 == fn && 48 == bc)
	{
		printf("ABLER_ENER_SOLIS INVERTER\n");
		printf("[Output power]=%d W\n", BUILD_UINT16(pdesc[13], pdesc[12]) * 10);
		printf("[AC voltage phase L1]=%d V\n", BUILD_UINT16(pdesc[15], pdesc[14]));
		printf("[AC output current L1]=%.02f A\n", BUILD_UINT16(pdesc[21], pdesc[20]) / (double)10.00);
		printf("[AC frequency]=%.02f\n", BUILD_UINT16(pdesc[25], pdesc[24]) / (double)10.00);
		printf("[DC-Bus Positive-voltage]=%d V\n", BUILD_UINT16(pdesc[27], pdesc[26]));
		printf("[Inverter internal temperature]=%d C\n", BUILD_UINT16(pdesc[31], pdesc[30]));
		printf("[Inverter Heat sink temperature]=%d C\n", BUILD_UINT16(pdesc[33], pdesc[32]));
		printf("[DC1 input voltage]=%d V\n", BUILD_UINT16(pdesc[35], pdesc[34]));
		printf("[DC2 input voltage]=%d V\n", BUILD_UINT16(pdesc[37], pdesc[36]));
		printf("[DC1 input current]=%.02f A\n", BUILD_UINT16(pdesc[39], pdesc[38]) / (double)10.00);
		printf("[DC2 input current]=%.02f A\n", BUILD_UINT16(pdesc[41], pdesc[40]) / (double)10.00);
		printf("[Input Power A]=%d W\n", BUILD_UINT16(pdesc[43], pdesc[42]) * 10);
		printf("[Input Power B]=%d W\n", BUILD_UINT16(pdesc[45], pdesc[44]) * 10);
		printf("[Total Output Power]=%ld KWH\n", (long int)(BUILD_UINT32(pdesc[49], pdesc[48], pdesc[47], pdesc[46])) );
		printf("[Battery voltage]=%.02f V\n", BUILD_UINT16(pdesc[51], pdesc[50]) / (double)10.00);
		printf("[Battery charge current]=%.02f A\n", BUILD_UINT16(pdesc[53], pdesc[52]) / (double)10.00);
		printf("[Total Charge Power]=%u KWH\n", BUILD_UINT32(pdesc[57], pdesc[56], pdesc[55], pdesc[54]));
		return;
	}

	if (0X0092 == start && 3 == fn && 52 == bc)
	{
		printf("C300-1/CT-713 AC/DC POWER TRANSDUCER\n");
		printf("[CH1 PF]=%.03f \n", BUILD_UINT16(pdesc[13], pdesc[12]) / (double)1000.00);
		printf("[CH2 PF]=%.03f \n", BUILD_UINT16(pdesc[15], pdesc[14]) / (double)1000.00);
		printf("[CH3 PF]=%.03f \n", BUILD_UINT16(pdesc[17], pdesc[16]) / (double)1000.00);
		printf("[£UPF]=%.03f \n", BUILD_UINT16(pdesc[19], pdesc[18]) / (double)1000.00);
		printf("[V1-2]=%u \n", BUILD_UINT32(pdesc[23], pdesc[22], pdesc[21], pdesc[20]));
		printf("[V2-3]=%u \n", BUILD_UINT32(pdesc[27], pdesc[26], pdesc[25], pdesc[24]));
		printf("[V3-1]=%u \n", BUILD_UINT32(pdesc[31], pdesc[30], pdesc[29], pdesc[28]));
		printf("[V1]=%u \n", BUILD_UINT32(pdesc[35], pdesc[34], pdesc[33], pdesc[32]));
		printf("[V2]=%u \n", BUILD_UINT32(pdesc[39], pdesc[38], pdesc[37], pdesc[36]));
		printf("[V3]=%u \n", BUILD_UINT32(pdesc[43], pdesc[42], pdesc[41], pdesc[40]));
		printf("[£UV]=%u \n", BUILD_UINT32(pdesc[47], pdesc[46], pdesc[45], pdesc[44]));
		printf("[A1]=%u \n", BUILD_UINT32(pdesc[51], pdesc[50], pdesc[49], pdesc[48]));
		printf("[A2]=%u \n", BUILD_UINT32(pdesc[55], pdesc[54], pdesc[53], pdesc[52]));
		printf("[A3]=%u \n", BUILD_UINT32(pdesc[59], pdesc[58], pdesc[57], pdesc[56]));
		printf("[£UA]=%u \n", BUILD_UINT32(pdesc[63], pdesc[62], pdesc[61], pdesc[60]));
		return;
	}
	else if (0X00AC == start && 3 == fn && 60 == bc)
	{
		printf("C300-1/CT-713 AC/DC POWER TRANSDUCER\n");
		printf("[W1]=%d \n", BUILD_UINT32(pdesc[15], pdesc[14], pdesc[13], pdesc[12]));
		printf("[W2]=%d \n", BUILD_UINT32(pdesc[19], pdesc[18], pdesc[17], pdesc[16]));
		printf("[W3]=%d \n", BUILD_UINT32(pdesc[23], pdesc[22], pdesc[21], pdesc[20]));
		printf("[£UW]=%d \n", BUILD_UINT32(pdesc[27], pdesc[26], pdesc[25], pdesc[24]));
		printf("[VA1]=%u \n", BUILD_UINT32(pdesc[31], pdesc[30], pdesc[29], pdesc[28]));
		printf("[VA2]=%u \n", BUILD_UINT32(pdesc[35], pdesc[34], pdesc[33], pdesc[32]));
		printf("[VA3]=%u \n", BUILD_UINT32(pdesc[39], pdesc[38], pdesc[37], pdesc[36]));
		printf("[£UVA]=%u \n", BUILD_UINT32(pdesc[43], pdesc[42], pdesc[41], pdesc[40]));
		printf("[Var1]=%u \n", BUILD_UINT32(pdesc[47], pdesc[46], pdesc[45], pdesc[44]));
		printf("[Var2]=%u \n", BUILD_UINT32(pdesc[51], pdesc[50], pdesc[49], pdesc[48]));
		printf("[Var3]=%u \n", BUILD_UINT32(pdesc[55], pdesc[54], pdesc[53], pdesc[52]));
		printf("[£UVar]=%u \n", BUILD_UINT32(pdesc[59], pdesc[58], pdesc[57], pdesc[56]));
		printf("[£UWH]=%d \n", BUILD_UINT32(pdesc[63], pdesc[62], pdesc[61], pdesc[60]));
		printf("[£U+WH]=%u \n", BUILD_UINT32(pdesc[67], pdesc[66], pdesc[65], pdesc[64]));
		printf("[£U-WH]=%u \n", BUILD_UINT32(pdesc[71], pdesc[70], pdesc[69], pdesc[68]));
		return;
	}

	if (0X0000 == start && 3 == fn && 40 == bc)
	{
		printf("CT-1700 AC TRANSDUCER, Phase-R information\n");
		printf("[Phase Voltage V(R-N)]=%.02f V\n", BUILD_UINT32(pdesc[15], pdesc[14], pdesc[13], pdesc[12]) / (double)10.00);
		printf("[Line Current R]=%.02f A\n", BUILD_UINT32(pdesc[19], pdesc[18], pdesc[17], pdesc[16]) / (double)1000.00);
		printf("[Apparent Power VA-R]=%.02f W\n", BUILD_UINT32(pdesc[23], pdesc[22], pdesc[21], pdesc[20]) / (double)10.00);
		printf("[Active Power W-R]=%.02f W\n", BUILD_UINT32(pdesc[27], pdesc[26], pdesc[25], pdesc[24]) / (double)10.00);
		printf("[Reactive Power Var-R]=%.02f W\n", BUILD_UINT32(pdesc[31], pdesc[30], pdesc[29], pdesc[28]) / (double)10.00);
		printf("[Power Factor PF-R]=%.03f \n", BUILD_UINT32(pdesc[35], pdesc[34], pdesc[33], pdesc[32]) / (double)1000.00);
		printf("[Positive Active Energy WH-R]=%.02f KWH\n", BUILD_UINT32(pdesc[39], pdesc[38], pdesc[37], pdesc[36]) / (double)10.00);
		printf("[Negative Active Energy WH-R]=%.02f KWH\n", BUILD_UINT32(pdesc[43], pdesc[42], pdesc[41], pdesc[40]) / (double)10.00);
		printf("[Positive Reactive Energy WH-R]=%.02f KWH\n", BUILD_UINT32(pdesc[47], pdesc[46], pdesc[45], pdesc[44]) / (double)10.00);
		printf("[Negative Reactive Energy WH-R]=%.02f KWH\n", BUILD_UINT32(pdesc[51], pdesc[50], pdesc[49], pdesc[48]) / (double)10.00);
		return;
	}
	else if (0X0014 == start && 3 == fn && 40 == bc)
	{
		printf("CT-1700 AC TRANSDUCER, Phase-S information\n");
		printf("[Phase Voltage V(S-N)]=%.02f V\n", BUILD_UINT32(pdesc[15], pdesc[14], pdesc[13], pdesc[12]) / (double)10.00);
		printf("[Line Current S]=%.03f A\n", BUILD_UINT32(pdesc[19], pdesc[18], pdesc[17], pdesc[16]) / (double)1000.00);
		printf("[Apparent Power VA-S]=%.02f W\n", BUILD_UINT32(pdesc[23], pdesc[22], pdesc[21], pdesc[20]) / (double)10.00);
		printf("[Active Power W-S]=%.02f W\n", BUILD_UINT32(pdesc[27], pdesc[26], pdesc[25], pdesc[24]) / (double)10.00);
		printf("[Reactive Power Var-S]=%.02f W\n", BUILD_UINT32(pdesc[31], pdesc[30], pdesc[29], pdesc[28]) / (double)10.00);
		printf("[Power Factor PF-S]=%.03f \n", BUILD_UINT32(pdesc[35], pdesc[34], pdesc[33], pdesc[32]) / (double)1000.00);
		printf("[Positive Active Energy WH-S]=%.02f KWH\n", BUILD_UINT32(pdesc[39], pdesc[38], pdesc[37], pdesc[36]) / (double)10.00);
		printf("[Negative Active Energy WH-S]=%.02f KWH\n", BUILD_UINT32(pdesc[43], pdesc[42], pdesc[41], pdesc[40]) / (double)10.00);
		printf("[Positive Reactive Energy WH-S]=%.02f KWH\n", BUILD_UINT32(pdesc[47], pdesc[46], pdesc[45], pdesc[44]) / (double)10.00);
		printf("[Negative Reactive Energy WH-S]=%.02f KWH\n", BUILD_UINT32(pdesc[51], pdesc[50], pdesc[49], pdesc[48]) / (double)10.00);
		return;
	}
	else if (0X0028 == start && 3 == fn && 40 == bc)
	{
		printf("CT-1700 AC TRANSDUCER, Phase-T information\n");
		printf("[Phase Voltage V(T-N)]=%.02f V\n", BUILD_UINT32(pdesc[15], pdesc[14], pdesc[13], pdesc[12]) / (double)10.00);
		printf("[Line Current T]=%.03f A\n", BUILD_UINT32(pdesc[19], pdesc[18], pdesc[17], pdesc[16]) / (double)1000.00);
		printf("[Apparent Power VA-T]=%.02f W\n", BUILD_UINT32(pdesc[23], pdesc[22], pdesc[21], pdesc[20]) / (double)10.00);
		printf("[Active Power W-T]=%.02f W\n", BUILD_UINT32(pdesc[27], pdesc[26], pdesc[25], pdesc[24]) / (double)10.00);
		printf("[Reactive Power Var-T]=%.02f W\n", BUILD_UINT32(pdesc[31], pdesc[30], pdesc[29], pdesc[28]) / (double)10.00);
		printf("[Power Factor PF-T]=%.03f \n", BUILD_UINT32(pdesc[35], pdesc[34], pdesc[33], pdesc[32]) / (double)1000.00);
		printf("[Positive Active Energy WH-T]=%.02f KWH\n", BUILD_UINT32(pdesc[39], pdesc[38], pdesc[37], pdesc[36]) / (double)10.00);
		printf("[Negative Active Energy WH-T]=%.02f KWH\n", BUILD_UINT32(pdesc[43], pdesc[42], pdesc[41], pdesc[40]) / (double)10.00);
		printf("[Positive Reactive Energy WH-T]=%.02f KWH\n", BUILD_UINT32(pdesc[47], pdesc[46], pdesc[45], pdesc[44]) / (double)10.00);
		printf("[Negative Reactive Energy WH-T]=%.02f KWH\n", BUILD_UINT32(pdesc[51], pdesc[50], pdesc[49], pdesc[48]) / (double)10.00);
		return;
	}
	else if (0X003C == start && 3 == fn && 40 == bc)
	{
		printf("CT-1700 AC TRANSDUCER, £Uinformation\n");
		printf("[£UV]=%.02f V\n", BUILD_UINT32(pdesc[15], pdesc[14], pdesc[13], pdesc[12]) / (double)10.00);
		printf("[£UA]=%.03f A\n", BUILD_UINT32(pdesc[19], pdesc[18], pdesc[17], pdesc[16]) / (double)1000.00);
		printf("[£UVA]=%.02f W\n", BUILD_UINT32(pdesc[23], pdesc[22], pdesc[21], pdesc[20]) / (double)10.00);
		printf("[£UW]=%.02f W\n", BUILD_UINT32(pdesc[27], pdesc[26], pdesc[25], pdesc[24]) / (double)10.00);
		printf("[£UVar]=%.02f W\n", BUILD_UINT32(pdesc[31], pdesc[30], pdesc[29], pdesc[28]) / (double)10.00);
		printf("[£UPF-T]=%.03f \n", BUILD_UINT32(pdesc[35], pdesc[34], pdesc[33], pdesc[32]) / (double)1000.00);
		printf("[Total Positive Active Energy]=%.02f KWH\n", BUILD_UINT32(pdesc[39], pdesc[38], pdesc[37], pdesc[36]) / (double)10.00);
		printf("[Total Negative Active Energy]=%.02f KWH\n", BUILD_UINT32(pdesc[43], pdesc[42], pdesc[41], pdesc[40]) / (double)10.00);
		printf("[Total Positive Reactive Energy]=%.02f KWH\n", BUILD_UINT32(pdesc[47], pdesc[46], pdesc[45], pdesc[44]) / (double)10.00);
		printf("[Total Negative Reactive Energy]=%.02f KWH\n", BUILD_UINT32(pdesc[51], pdesc[50], pdesc[49], pdesc[48]) / (double)10.00);
		return;
	}
	else
	{
		for (i = 0; i < bc / 2; i++)
		{
			w = BUILD_UINT16(pdesc[12 + 2 * i + 1], pdesc[12 + 2 * i]);
			printf("[R-0X%04X]=%05d \n", start + i, w);
		}
		//printf("\n");
	}
}

int main(int argc, char *argv[])
{
	int state = 0;
	unsigned char ch;
	int len;

	static int left;
	static unsigned char *pb = 0;

	int c, res;
	char host[128];
	int port = 9000;
	unsigned long long int ia = 0X00124B0001142FD0LLU;
	int slave = 1;
	int fc = 3;
	int start = 0X0000;
	int num = 1;
	int rtsto = 5;
	int baud = 9600;

	int v;
	int sd = -1;
	fd_set rset;
	struct timeval tv;

	unsigned char tl;
	unsigned char frame[512];
	int l;
	unsigned char rb[4096];
	unsigned short cmd, cId;
	unsigned short sa;
	time_t first;

	unsigned char UxBAUD = 0, UxGCR = 0;

	strcpy(host, "localhost");
	if (argc < 2)
		PrintUsage();

	while ((c = getopt(argc, argv, "h:p:B:T:I:S:F:R:N:")) != -1)
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
		case 'B':
			baud = atoi(optarg);
			if (9600 != baud && 19200 != baud && 38400 != baud && 57600 != baud)
				PrintUsage();
			if (9600 == baud)		UxBAUD = 59, UxGCR = 8;
			if (19200 == baud)		UxBAUD = 59, UxGCR = 9;
			if (38400 == baud)		UxBAUD = 59, UxGCR = 10;
			if (57600 == baud)		UxBAUD = 216, UxGCR = 10;
			break;
		case 'T':
			rtsto = atoi(optarg);
			if (rtsto > 100)
				PrintUsage();
			break;
		case 'I':
			if (strlen(optarg) != 18)
				PrintUsage();
			if (optarg[0] != '0')
				PrintUsage();
			if (optarg[1] != 'X' && optarg[1] != 'x')
				PrintUsage();
			res = sscanf(optarg, "%LX", &ia);
			if (-1 == res)
				PrintUsage();
			break;
		case 'S':
			slave = atoi(optarg);
			if (slave <= 0 || slave > 200)
				PrintUsage();
			break;
		case 'F':
			fc = atoi(optarg);
			if (fc <= 0 || fc > 10)
				PrintUsage();
			break;
		case 'R':
			if (strlen(optarg) != 6)
				PrintUsage();
			if (optarg[0] != '0')
				PrintUsage();
			if (optarg[1] != 'X' && optarg[1] != 'x')
				PrintUsage();
			res = sscanf(optarg, "%X", &start);
			start &= 0XFFFF;
			if (-1 == res)
				PrintUsage();
			break;
		case 'N':
			num = atoi(optarg);
			if (num <= 0 || num > 50)
				PrintUsage();
			break;
		}
	}

	if (0 > (sd = Conn(host, port)))
		printf("(%s %d) Conn() fail !\n", __FILE__, __LINE__), exit(1);

	v = 512 * 1024;
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__), close(sd), exit(1);


	if (SIG_ERR == signal(SIGINT, SigInt))
		printf("(%s %d) signal error", __FILE__, __LINE__), exit(1);
	if (SIG_ERR == signal(SIGTERM, SigInt))
		printf("(%s %d) signal error", __FILE__, __LINE__), exit(1);
	if (SIG_ERR == signal(SIGHUP, SigInt))
		printf("(%s %d) signal error", __FILE__, __LINE__), exit(1);


	// 104 ECONNRESET

/////////////////////////////////////////////////////////////////////////////////////////
// ZDO_NWK_ADDR_REQ

	frame[0] = 0X0A;
	frame[1] = 0X25;
	frame[2] = 0X00;
	memcpy(&frame[3], &ia, sizeof(unsigned long long int));
	frame[11] = 0;
	frame[12] = 0;
	l = PackFrame(frame);
	res = write(sd, frame, l);
	if (res <= 0)
		printf("(%s %d) write() fail !\n", __FILE__, __LINE__), close(sd), exit(1);

	state = 0;
	time(&first);
	do
	{
		unsigned long long int ieee;
		time_t now;

		time(&now);
		if ((int)(now - first) > 6)
			printf("(%s %d) ZDO_NWK_ADDR_RSP timeout !\n", __FILE__, __LINE__), close(sd), exit(1);

		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 3;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)
		{
			continue;
			//printf("(%s %d) select() timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		if (res < 0)
			printf("(%s %d) select() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)		// 0xfe
		{
			len = read(sd, &ch, 1);
			if (len <= 0)
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (ch == 0XFE)
			{
				state = 1;
				rb[0] = ch;
			}
			else
			{
				//printf("state=%d, but ch!=0XFE\n", state);
			}
			continue;
		}
		if (1 == state)		// tok len
		{
			len = read(sd, &ch, 1);
			if (len <= 0)
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
				printf("(%s %d) XorSum error !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		if (cmd != 0X8045)
			continue;
		memcpy(&ieee, &rb[5], sizeof(unsigned long long int));
		if (ieee != ia)
			continue;
		memcpy(&sa, &rb[13], sizeof(unsigned short));
		break;
	}
	while(1);


/////////////////////////////////////////////////////////////////////////////////////////
// RTS_TIMEOUT_SET
	if (0 == (l = PackDesc(RTS_TIMEOUT_SET_DESC, frame)))
		PrintUsage();
	frame[5] = (unsigned char)rtsto;
	if (0 == PackAfCmd(sa, RTS_TIMEOUT_SET_DESC, 0X30, 15, (unsigned char)l, frame))
		PrintUsage();
	l = PackFrame(frame);
	res = write(sd, frame, l);
	if (res <= 0)
		printf("(%s %d) write() fail !\n", __FILE__, __LINE__), close(sd), exit(1);

	state = 0;
	time(&first);
	do
	{
		time_t now;
		unsigned short addr;

		time(&now);
		if ((int)(now - first) > 6)
			printf("(%s %d) RTS_TIMEOUT_RSP timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 1;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)
		{
			continue;
			//printf("(%s %d) select() timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		if (res < 0)
			printf("(%s %d) select() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)		// 0xfe
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) RECV() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
				printf("(%s %d) XorSum error !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		if (cmd != 0X8144)
			continue;
		memcpy(&cId, &rb[6], sizeof(unsigned short));
		if (cId != RTS_TIMEOUT_RSP_DESC)
			continue;
		memcpy(&addr, &rb[8], sizeof(unsigned short));
		if (addr != sa)
			continue;
		break;
	}
	while(1);


/////////////////////////////////////////////////////////////////////////////////////////
// UXBAUD_SET
	if (0 == (l = PackDesc(UXBAUD_SET_DESC, frame)))
		PrintUsage();
	frame[5] = 0;
	frame[6] = UxBAUD;
	if (0 == PackAfCmd(sa, UXBAUD_SET_DESC, 0X30, 15, (unsigned char)l, frame))
		PrintUsage();
	l = PackFrame(frame);
	res = write(sd, frame, l);
	if (res <= 0)
		printf("(%s %d) write() fail !\n", __FILE__, __LINE__), close(sd), exit(1);

	state = 0;
	time(&first);
	do
	{
		time_t now;
		unsigned short addr;

		time(&now);
		if ((int)(now - first) > 6)
			printf("(%s %d) UXBAUD_RSP timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 3;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)
		{
			continue;
			printf("(%s %d) select() timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		if (res < 0)
			printf("(%s %d) select() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)		// 0xfe
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
				printf("(%s %d) XorSum error !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		if (cmd != 0X8144)
			continue;
		memcpy(&cId, &rb[6], sizeof(unsigned short));
		if (cId != 0X802D && cId != 0X802E)
			continue;
		memcpy(&addr, &rb[8], sizeof(unsigned short));
		if (addr != sa)
			continue;
		break;
	}
	while(1);


/////////////////////////////////////////////////////////////////////////////////////////
// UXGCR_SET
	if (0 == (l = PackDesc(UXGCR_SET_DESC, frame)))
		PrintUsage();
	frame[5] = 0;
	frame[6] = UxGCR;
	if (0 == PackAfCmd(sa, UXGCR_SET_DESC, 0X30, 15, (unsigned char)l, frame))
		PrintUsage();
	l = PackFrame(frame);
	res = write(sd, frame, l);
	if (res <= 0)
		printf("(%s %d) write() fail !\n", __FILE__, __LINE__), close(sd), exit(1);

	state = 0;
	time(&first);
	do
	{
		time_t now;
		unsigned short addr;

		time(&now);
		if ((int)(now - first) > 6)
			printf("(%s %d) UXGCR_RSP_DESC timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 3;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)
		{
			continue;
			//printf("(%s %d) select() timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		if (res < 0)
			printf("(%s %d) select() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)		// 0xfe
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
				printf("(%s %d) XorSum error !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		if (cmd != 0X8144)
			continue;
		memcpy(&cId, &rb[6], sizeof(unsigned short));
		if (cId != 0X802F && cId != 0X8030)
			continue;
		memcpy(&addr, &rb[8], sizeof(unsigned short));
		if (addr != sa)
			continue;
		break;
	}
	while(1);


/////////////////////////////////////////////////////////////////////////////////////////
// MODBUSQRY_REQ
	if (0 == (l = PackDesc(MODBUSQRY_REQ_DESC, frame)))
		PrintUsage();
	frame[5] = (unsigned char)slave;
	frame[6] = (unsigned char)fc;
	frame[7] = HI_UINT16(start);
	frame[8] = LO_UINT16(start);		// lo
	frame[9] = HI_UINT16(num);		// hi
	frame[10] = LO_UINT16(num);		// lo
	if (0 == PackAfCmd(sa, MODBUSQRY_REQ_DESC, 0X30, 15, (unsigned char)l, frame))
		PrintUsage();
	l = PackFrame(frame);
	res = write(sd, frame, l);
	if (res <= 0)
		printf("(%s %d) write() fail !\n", __FILE__, __LINE__), close(sd), exit(1);

	state = 0;
	time(&first);
	do
	{
		time_t now;
		unsigned short addr;

		time(&now);
		if ((int)(now - first) > 6)
			printf("(%s %d) UXBAUD_RSP timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 3;
		res = select(sd + 1, &rset, 0, 0, &tv);
		if (res == 0)
		{
			continue;
			//printf("(%s %d) select() timeout !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		if (res < 0)
			printf("(%s %d) select() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 == state)		// 0xfe
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
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
				printf("(%s %d) recv() fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			pb += len;
			left -= len;
			if (left > 0)
				continue;
			state = 0;
			tl = rb[1];
			if (rb[tl + 4] != XorSum(&rb[1], rb[1] + 3))
				printf("(%s %d) XorSum error !\n", __FILE__, __LINE__), close(sd), exit(1);
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		if (cmd != 0X8144)
			continue;
		memcpy(&cId, &rb[6], sizeof(unsigned short));
		if (cId != MODBUSQRY_RSP_DESC)
			continue;
		memcpy(&addr, &rb[8], sizeof(unsigned short));
		if (addr != sa)
			continue;

		ProcessModbusRsp(rb[20], &rb[21]);
		break;
	}
	while(1);


	close(sd);
	return 0;
}


