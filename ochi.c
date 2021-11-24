
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
#include <signal.h>
#include <time.h>
#include <inttypes.h>

#include <netinet/tcp.h>
#include <netdb.h>

#include <fcntl.h>



///////////////////////
//
#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((unsigned int)((unsigned int)((Byte0) & 0x00FF) + ((unsigned int)((Byte1) & 0x00FF) << 8) \
			+ ((unsigned int)((Byte2) & 0x00FF) << 16) + ((unsigned int)((Byte3) & 0x00FF) << 24)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)



///////////////////////
//
typedef struct tm SYSTEMTIME;

static void GetLocalTime(SYSTEMTIME *st)
{
	struct tm *pst = NULL;
	time_t t = time(NULL);
	pst = localtime(&t);
	memcpy(st, pst, sizeof(SYSTEMTIME));
	st->tm_year += 1900;
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

static unsigned int crc16(unsigned char *buf, unsigned short len)
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


static int mq(int s, unsigned char slaveAddr, unsigned char fnCode, unsigned short reg, unsigned short num)
{
	int res;
	unsigned char query[8];

	memset(query, 0, 8);
	query[0] = slaveAddr;
	query[1] = fnCode;
	query[2] = (unsigned char)(reg >> 8);
	query[3] = (unsigned char)(reg & 0XFF);
	query[4] = (unsigned char)(num >> 8);
	query[5] = (unsigned char)(num & 0XFF);
	query[6] = (unsigned char)(crc16(query, 6) >> 8);
	query[7] = (unsigned char)(crc16(query, 6) & 0XFF);

	res = write(s, query, 8);

	return res;
}

static void ReverseBytes(unsigned char *pData, unsigned char len)
{
        unsigned char i, j;
        unsigned char temp;

        for (i = 0, j = len - 1; len > 1; len -= 2)
        {
                temp = pData[i];
                pData[i++] = pData[j];
                pData[j--] = temp;
        }
}

static int ds(int s, unsigned char id, unsigned char len, char *str)
{
	int res;
	unsigned char sum = 0, i = 0;
	unsigned char b[512];

	if (0 == str || 0 == len)
		return -1;

	memset(b, 0, sizeof(b));
	b[0] = 0X02;

	b[1] = 0X4E;
	b[2] = id;
	b[3] = 0X41;
	b[4] = len;
	memcpy(&b[5], str, (size_t)len);

	ReverseBytes(&b[5], len);

	for (i = 0; i < len + 4; i++)
		sum = sum ^ b[1 + i];
	b[5 + len] = sum;
	b[6 + len] = 0X03;
	res = write(s, b, 7 + len);

	return res;
}

static int CONN(char *domain, int port)
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

int main(int argc, char *argv[])
{
	int res;
	fd_set rset;
	struct timeval tv;
	int sd = -1;
	int len;
	unsigned char rb[1024*128], *p = rb;
	char sz[512];
	char buf[512];
	unsigned int w;
	unsigned int wh = 0, owh = 0;
	struct stat ls;
	int v;

	SYSTEMTIME st;
	FILE * pFile;
	int val;
	int i = 0;


	if (0 > (sd = CONN("127.0.0.1", 34000)))
		printf("(%s %d) CONN fail !\n", __FILE__, __LINE__), exit(1);
	if (-1 == (val = fcntl(sd, F_GETFL, 0)))
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (-1 == fcntl(sd, F_SETFL, val | O_NONBLOCK))
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	v = 16 * 1024;
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail, EXIT \n", __FILE__, __LINE__), exit(1);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail, EXIT \n", __FILE__, __LINE__), exit(1);
	setbuf(stdout, 0);


///////////////////////
//
	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	if ((res = mq(sd, 1, 3, 0X0092, 26)) <= 0)			printf("(%s %d) MODBUS QUERY 1-0X0092-26 fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if ((res = select(sd + 1, &rset, 0, 0, &tv)) <= 0)		printf("(%s %d) select timeout or fail 1, No ACK !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 < res)
	{
		if ((len = read(sd, rb, sizeof(rb))) < 0)			printf("(%s %d) read fail 1 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (len == 0)								printf("(%s %d) connection reset by remote peers 1 !\n", __FILE__, __LINE__), close(sd), exit(1);
		GetLocalTime(&st);
		printf("[%04d-%02d-%02d] (%s %d) MODBUS 1-0X0092_26 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
	}
	for (i = 0; i < 12; i++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_usec = 1000;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)	printf("(%s %d) select fail 2 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, rb, sizeof(rb))) < 0)	printf("(%s %d) read fail 2 !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (len == 0)							printf("(%s %d) connection reset by remote peers 2 !\n", __FILE__, __LINE__), close(sd), exit(1);
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d] (%s %d) MODBUS 1-0X0092_26 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
		}
	}


///////////////////////
//
	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	if ((res = mq(sd, 1, 3, 0X00AC, 30)) <= 0)			printf("(%s %d) MODBUS QUERY 1-0X00AC-30 fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if ((res = select(sd + 1, &rset, 0, 0, &tv)) <= 0)		printf("(%s %d) select timeout or fail 3, No ACK !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 < res)
	{
		if ((len = read(sd, rb, sizeof(rb))) < 0)		printf("(%s %d) read fail 3 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (len == 0)								printf("(%s %d) connection reset by remote peers 3 !\n", __FILE__, __LINE__), close(sd), exit(1);
		GetLocalTime(&st);
		printf("[%04d-%02d-%02d] (%s %d) MODBUS 1-0X00AC_30 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
	}
	for (i = 0; i < 12; i++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_usec = 1000;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)	printf("(%s %d) select fail 4 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, rb, sizeof(rb))) < 0)	printf("(%s %d) read fail 4 !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (len == 0)							printf("(%s %d) connection reset by remote peers 4 !\n", __FILE__, __LINE__), close(sd), exit(1);
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d] (%s %d) MODBUS 1-0X00AC_30 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
		}
	}


///////////////////////
//
	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	if ((res = mq(sd, 2, 3, 0XC020, 24)) <= 0)			printf("(%s %d) MODBUS QUERY 2-0XC020-24 fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if ((res = select(sd + 1, &rset, 0, 0, &tv)) <= 0)		printf("(%s %d) select timeout or fail 5, No ACK !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 < res)
	{
		if ((len = read(sd, (p = rb), 1024)) < 0)		printf("(%s %d) read fail 5 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (len == 0)								printf("(%s %d) connection reset by remote peers 5 !\n", __FILE__, __LINE__), close(sd), exit(1);
		GetLocalTime(&st);
		printf("[%04d-%02d-%02d] (%s %d) MODBUS 2-0XC020_24 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
		p += len;
	}
	for (i = 0; i < 12; i++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_usec = 1000;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)	printf("(%s %d) select fail 6 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, p, 1024)) < 0)			printf("(%s %d) read fail 6 !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (len == 0)							printf("(%s %d) connection reset by remote peers 6 !\n", __FILE__, __LINE__), close(sd), exit(1);
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d] (%s %d) MODBUS 2-0XC020_24 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
			p += len;
		}
	}


///////////////////////
//
	w = (unsigned int)(BUILD_UINT16(rb[4], rb[3])) * 10;				// W
	wh = (unsigned int)(BUILD_UINT32(rb[40], rb[39], rb[38], rb[37]));		// KWH
	GetLocalTime(&st);
	printf("[%04d-%02d-%02d] Inverter reply: %u Watt and %u kWatt-Hour\n", st.tm_year, st.tm_mon + 1, st.tm_mday, w, wh);


	res = stat("/usr/local/etc/ochi.conf", &ls);
	if (0 == res)
	{
		//pFile = fopen("/usr/local/etc/ochi.conf","w+");
		pFile = fopen("/usr/local/etc/ochi.conf","rw");
		if (pFile == NULL)							printf("(%s %d) fopen w+ fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		setbuf(pFile, 0);
		fseek(pFile, 0, SEEK_SET);
		sprintf(sz, "[%04d-%02d-%02d] %u\n", st.tm_year, st.tm_mon + 1, st.tm_mday, wh);
		fputs(sz, pFile);
		fclose (pFile);
		printf("[%04d-%02d-%02d] Create initialized kWatt-Hour: %u\n", st.tm_year, st.tm_mon + 1, st.tm_mday, wh);
	}
	else
	{
		pFile = fopen("/usr/local/etc/ochi.conf","rw");
		if (pFile == NULL)							printf("(%s %d) fopen rw fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		setbuf(pFile, 0);
		fseek(pFile, 0, SEEK_SET);
		if (NULL == fgets(buf, sizeof(buf) , pFile))		printf("(%s %d) fgets fail !\n", __FILE__, __LINE__), close(sd), fclose(pFile), unlink("/usr/local/etc/ochi.conf"), exit(1);
		if (2 != sscanf (buf, "[%s] %u", sz, &owh))		printf("(%s %d) scanf fail !\n", __FILE__, __LINE__), close(sd), fclose(pFile), unlink("/usr/local/etc/ochi.conf"), exit(1);
		sprintf(sz, "[%04d-%02d-%02d] %u\n", st.tm_year, st.tm_mon + 1, st.tm_mday, wh);
		if (0 != memcmp(sz, buf, 13))
		{
			fseek(pFile , 0, SEEK_SET);
			sprintf(sz, "[%04d-%02d-%02d] %u\n", st.tm_year, st.tm_mon + 1, st.tm_mday, wh);
			fputs(sz, pFile);
			printf("[%04d-%02d-%02d] Re-write initialized kWatt-Hour: %u\n", st.tm_year, st.tm_mon + 1, st.tm_mday, wh);
		}
		else
			printf("[%04d-%02d-%02d] Read initialized kWatt-Hour: %u\n", st.tm_year, st.tm_mon + 1, st.tm_mday, wh);
		fclose (pFile);
	}


///////////////////////
//
	// KW
	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	GetLocalTime(&st);
	sprintf(sz, "%01.02f", (double)w / 1000.00);		printf("[%04d-%02d-%02d] POWER=%s KW\n", st.tm_year, st.tm_mon + 1, st.tm_mday, sz);
	if ((res = ds(sd, 1, 4, sz)) <= 0)					printf("(%s %d) disp POWER (KW) fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)		printf("(%s %d) select fail 7 !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 == res)									printf("(%s %d) select timeout 7, No ACK !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 < res)
	{
		if ((len = read(sd, rb, sizeof(rb))) < 0)		printf("(%s %d) read fail 7 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (len == 0)								printf("(%s %d) connection reset by remote peers 7 !\n", __FILE__, __LINE__), close(sd), exit(1);
		GetLocalTime(&st);
		printf("[%04d-%02d-%02d] (%s %d) 1-DISP KW ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
	}
	for (i = 0; i < 12; i++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_usec = 1000;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)	printf("(%s %d) select fail 8 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, rb, sizeof(rb))) < 0)	printf("(%s %d) read fail 8 !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (len == 0)							printf("(%s %d) connection reset by remote peers 8 !\n", __FILE__, __LINE__), close(sd), exit(1);
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d] (%s %d) 1-DISP KW ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
		}
	}


///////////////////////
//
	// YEAR KWH
	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	sprintf(sz, "%05u", wh - owh);					printf("[%04d-%02d-%02d] YEAR-KWH=%s KWH\n", st.tm_year, st.tm_mon + 1, st.tm_mday, sz);
	if ((res = ds(sd, 2, 5, sz)) <= 0)					printf("(%s %d) disp YEAR TOTAL (KWH) fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)		printf("(%s %d) select timeout or fail 9 !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 == res)									printf("(%s %d) select timeout 9, No ACK !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 < res)
	{
		if ((len = read(sd, rb, sizeof(rb))) < 0)		printf("(%s %d) read fail 9 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (len == 0)								printf("(%s %d) connection reset by remote peers 9 !\n", __FILE__, __LINE__), close(sd), exit(1);
		GetLocalTime(&st);
		printf("[%04d-%02d-%02d] (%s %d) 2-DISP YEAR-KWH ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
	}
	for (i = 0; i < 12; i++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_usec = 1000;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)	printf("(%s %d) select fail 10 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, rb, sizeof(rb))) < 0)	printf("(%s %d) read fail 10 !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (len == 0)							printf("(%s %d) connection reset by remote peers 10 !\n", __FILE__, __LINE__), close(sd), exit(1);
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d] (%s %d) 2-DISP YEAR-KWH ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
		}
	}


///////////////////////
//
	// CO2 (KG)
	FD_ZERO(&rset);
	FD_SET(sd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 1;
	sprintf(sz, "%04.01f", (double)(wh-owh) * 0.637);	printf("[%04d-%02d-%02d] YEAR-CO2=%s KG\n", st.tm_year, st.tm_mon + 1, st.tm_mday, sz);
	if ((res = ds(sd, 3, 6, sz)) <= 0)					printf("(%s %d) disp CO2 (KG) fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)		printf("(%s %d) select timeout or fail 11 !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 == res)									printf("(%s %d) select timeout 11, No ACK !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (0 < res)
	{
		if ((len = read(sd, rb, sizeof(rb))) < 0)		printf("(%s %d) read fail 11 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (len == 0)								printf("(%s %d) connection reset by remote peers 11 !\n", __FILE__, __LINE__), close(sd), exit(1);
		GetLocalTime(&st);
		printf("[%04d-%02d-%02d] (%s %d) 3-DISP CO2 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
	}
	for (i = 0; i < 12; i++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_usec = 1000;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)	printf("(%s %d) select fail 12 !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, rb, sizeof(rb))) < 0)	printf("(%s %d) read fail 12 !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (len == 0)							printf("(%s %d) connection reset by remote peers 12 !\n", __FILE__, __LINE__), close(sd), exit(1);
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d] (%s %d) 3-DISP CO2 ACK !\n", st.tm_year, st.tm_mon + 1, st.tm_mday, __FILE__, __LINE__);
		}
	}

	close(sd);
	return 0;
}


