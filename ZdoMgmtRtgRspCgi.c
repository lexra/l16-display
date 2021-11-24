
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

	fprintf(cgiOut, "\nZDO_MGMT_RTG_RSP, [CMD]=0X%02X \n[Status]=%d [SA]=0X%04X [TN]=%d \n", cmd, status, sa, tn);

	p = &pdesc[10];
	for (; i < cnt; i++, p += 5)
	{
		memcpy(&da, p, sizeof(unsigned short));
		memcpy(&res, p + 2, sizeof(unsigned char));
		memcpy(&next, p + 3, sizeof(unsigned short));
		if (0 == res)
			fprintf(cgiOut, "DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "ACTIVE", next);
		else if (1 == res)
			fprintf(cgiOut, "DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "UNDERWAY", next);
		else if (2 == res)
			fprintf(cgiOut, "DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "FAIL", next);
		else if (3 == res)
			fprintf(cgiOut, "DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "INACTIVE", next);
		else
			fprintf(cgiOut, "DA[%d]=0X%04X [RES]=%s, [NEXT]=0X%04X\n", (int)start + i, da, "N/A", next);
	}
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

	int sa;

	int state = 0;
	int sd = -1;
	fd_set rset;
	struct timeval tv;
	time_t now;
	time_t first;

	unsigned char ch;
	unsigned char rb[320], *pb = 0;
	int left = 0;
	unsigned char tl;

	unsigned char hs[] = {0XFE, 0X00, 0X00, 0X04, 0XFA, 00};



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
	if (0X03 != len)
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
	if (cmd != 0X3225)
	{
		res = 1, fprintf(cgiOut, "(%s %d) cmd error, cmd=0X%04X !!", __FILE__, __LINE__, (unsigned short)cmd);
		goto END;
	}

	memset(temp, 0, sizeof(temp));
	memcpy(temp + 2 * 0, packet + 3 * 5, 2);		// 0X6F
	memcpy(temp + 2 * 1, packet + 3 * 4, 2);		// 0X79
	res = sscanf(temp, "%X", &sa);
	if (-1 == res)
	{
		res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
		goto END;
	}
	sa &= 0XFFFF;




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
		pb = hs;
		left = 5;
		for (;;)
		{
			res = write(sd, pb, left);
			if (res < 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) write() fail !\n", __FILE__, __LINE__);
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
	//	res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
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
			res = 1, fprintf(cgiOut, "(%s %d) sscanf error !!", __FILE__, __LINE__);
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
			res = 1, fprintf(cgiOut, "(%s %d) write() fail !", __FILE__, __LINE__);
			goto END;
		}
		pb += res;
		left -= res;
		if (left <= 0)
			break;
	}

	state = 0;
	time(&first);
	do
	{
		int addr;

		time(&now);
		if ((int)(now - first) > 6)
		{
			res = 1, fprintf(cgiOut, "(%s %d) RSP timeout !!", __FILE__, __LINE__);
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
			//res = 1, fprintf(cgiOut, "(%s %d) select() timeout !", __FILE__, __LINE__);
			//goto END;
		}
		if (res < 0)
		{
			res = 1, fprintf(cgiOut, "(%s %d) select() fail !", __FILE__, __LINE__);
			goto END;
		}
		if (0 == state)		// 0xfe
		{
			len = recv(sd, (char *)&ch, 1, 0);
			if (len <= 0)
			{
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !", __FILE__, __LINE__);
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
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !", __FILE__, __LINE__);
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
				res = 1, fprintf(cgiOut, "(%s %d) recv() fail !", __FILE__, __LINE__);
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
				res = 1, fprintf(cgiOut, "(%s %d) XorSum error !", __FILE__, __LINE__);
				goto END;
			}
		}
		memcpy(&cmd, &rb[2], sizeof(unsigned short));
		cmd &= 0XFFFF;
		if (cmd != 0XB245)
		{
			fprintf(cgiOut, "(%s %d) cmd=0X%04X, rb[2]=0X%02X, rb[3]=0X%02X !", __FILE__, __LINE__, cmd, rb[2], rb[3]);
			continue;
		}
		memcpy(&addr, &rb[4], sizeof(unsigned short));
		addr &= 0XFFFF;
		if (addr != sa)
			continue;
		break;
	}
	while(1);


	ProcessZdoMgmtRtgRsp(0, rb);



END:
	pb = hs;
	left = 5;
	for (;;)
	{
		res = write(sd, pb, left);
		if (res < 0)
		{
			fprintf(cgiOut, "(%s %d) write() fail !\n", __FILE__, __LINE__),	close(sd);

			fprintf(cgiOut, "\nDONE\n");
			fprintf(cgiOut, "</PACKET>\n");
			fprintf(cgiOut, "</ZTOOL>\n");
			return res;
		}
		pb += res;
		left -= res;
		if (left <= 0)
			break;
	}
	hs[0] = 0XFE, hs[1] = 0X01, hs[2] = 0X41, hs[3] = 0X00, hs[4] = 0X00, hs[5] = 0X40;
	pb = hs;
	left = 6;
	for (;;)
	{
		res = write(sd, pb, left);
		if (res < 0)
		{
			fprintf(cgiOut, "(%s %d) write() fail !\n", __FILE__, __LINE__),	close(sd);

			fprintf(cgiOut, "\nDONE\n");
			fprintf(cgiOut, "</PACKET>\n");
			fprintf(cgiOut, "</ZTOOL>\n");
			return res;
		}
		pb += res;
		left -= res;
		if (left <= 0)
			break;
	}

	close(sd);

	fprintf(cgiOut, "\nDONE\n");
	fprintf(cgiOut, "</PACKET>\n");
	fprintf(cgiOut, "</ZTOOL>\n");

	return res;
}

