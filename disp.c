
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
#include <inttypes.h>

#include <mysql/mysql.h>



/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

MYSQL_RES *result;
MYSQL_ROW row;

MYSQL mysql;
MYSQL *conn;


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

static int disp(int tty, unsigned char id, unsigned char len, char *str)
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
	res = write(tty, b, 7 + len);

	return res;
}

int main(int argc, char *argv[])
{
	fd_set rset;
	struct timeval tv;

	int len;
	unsigned char rb[128];

	struct termios term;
	int ttyfd = -1;

	int res;
	char szTmp[512];
	char szSql[512];
	char y0[16];
	char y1[16];

	double s0 =0.00F;
	double s1 =0.00F;

	double d = 0.00F;
	double w = 0.00F;
	double t = 0.00F;

	int f = 0;
	int v;


	if (2 != argc)
		printf("Usage Example: disp /dev/ttyUSB0 \n"), exit(1);

	if (0 > (ttyfd = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY)))
	{
		printf("(%s %d) open() fail !!\n", __FILE__, __LINE__);
		exit(1);
	}
	if (!isatty(ttyfd))
		printf("(%s %d) not isatty !\n", __FILE__, __LINE__), close(ttyfd), exit(1);

	memset(&term, 0, sizeof(struct termios));
	term.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
	term.c_iflag = IGNPAR;
	cfsetispeed(&term, B38400), cfsetospeed(&term, B38400);
	if (0 > tcsetattr(ttyfd, TCSANOW, &term))
		printf("(%s %d) tcsetattr fail !\n", __FILE__, __LINE__), close(ttyfd), exit(1);
	if (0 > (v = fcntl(ttyfd, F_GETFL, 0)))
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__), close(ttyfd), exit(1);
	if (0 > fcntl(ttyfd, F_SETFL, v | O_NONBLOCK))
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__), close(ttyfd), exit(1);
	if (0 > tcflush(ttyfd, TCIFLUSH))
		printf("(%s %d) TCIFLUSH fail !\n", __FILE__, __LINE__);


	if (0 == mysql_init(&mysql))
	{
		printf("(%s %d) mysql_init() fail !!\n", __FILE__, __LINE__);
		close(ttyfd);
		exit(1);
	}
	//conn = mysql_real_connect(&mysql, "211.79.161.6", "ecoadm", "ev093qer", "ECODB", 0, "", 0);
	conn = mysql_real_connect(&mysql, "127.0.0.1", "ecoadm", "ev093qer", "ECODB", 0, "", 0);
	if (0 == conn)
	{
		printf("(%s %d) mysql_real_connect() fail !!\n", __FILE__, __LINE__);
		close(ttyfd);
		exit(1);
	}


/////////////////////////////////////////////////////////////////////////////////////////
// 本年度累積發電量

	strcpy(szSql, "SELECT YEAR(NOW())");
	res = mysql_query(conn, szSql);
	if (0 != res)
	{
		printf("(%s %d) mysql_query() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	result = mysql_store_result(conn);
	if (0 == result)
	{
		printf("(%s %d) mysql_store_result() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	memset(y1, 0, sizeof(y1));
	while ((row = mysql_fetch_row(result)) != NULL)
		strcpy(y1, row[0]);
	// 2011


	strcpy(szSql, "SELECT YEAR(NOW() - INTERVAL 1 YEAR)");
	res = mysql_query(conn, szSql);
	if (0 != res)
	{
		printf("(%s %d) mysql_query() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	result = mysql_store_result(conn);
	if (0 == result)
	{
		printf("(%s %d) mysql_store_result() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	memset(y0, 0, sizeof(y0));
	while ((row = mysql_fetch_row(result)) != NULL)
		strcpy(y0, row[0]);

	sprintf(szSql, "SELECT gr_total FROM tb_generatelog WHERE YEAR(timestamp)='%s' AND zb_myid='29555' ORDER BY timestamp DESC LIMIT 1", y0);
	//printf("sql='%s' \n", szSql);
	res = mysql_query(conn, szSql);
	if (0 != res)
	{
		printf("(%s %d) mysql_query() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	result = mysql_store_result(conn);
	if (0 == result)
	{
		printf("(%s %d) mysql_store_result() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	while ((row = mysql_fetch_row(result)) != NULL)
		s0 = atof(row[0]);

	//printf("s0='%.02f' !!\n", s0);
	//return 0;


	sprintf(szSql, "SELECT gr_total FROM tb_generatelog WHERE YEAR(timestamp)='%s' AND zb_myid='29555' ORDER BY timestamp DESC LIMIT 1", y1);
	res = mysql_query(conn, szSql);
	if (0 != res)
	{
		printf("(%s %d) mysql_query() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	result = mysql_store_result(conn);
	if (0 == result)
	{
		printf("(%s %d) mysql_store_result() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	f = 0;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		f = 1;
		s1 = atof(row[0]);
	}
	if (!f)
	{
		printf("(%s %d) gr_total not found !!\n", __FILE__, __LINE__);
		mysql_close(conn);
		close(ttyfd);
		exit(2);
	}


/////////////////////////////////////////////////////////////////////////////////////////
// 即時發電功率

	sprintf(szSql, "SELECT meterbk_parm1 FROM tb_meter WHERE zb_myid='29555' AND dvc_seq='78' ORDER BY timestamp DESC LIMIT 1");
	res = mysql_query(conn, szSql);
	if (0 != res)
	{
		printf("(%s %d) mysql_query() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	result = mysql_store_result(conn);
	if (0 == result)
	{
		printf("(%s %d) mysql_store_result() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	f = 0;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		f = 1;
		w = atof(row[0]);
	}
	if (!f)
	{
		printf("(%s %d) meterbk_parm1 not found !!\n", __FILE__, __LINE__);
		mysql_close(conn);
		close(ttyfd);
		exit(2);
	}


/////////////////////////////////////////////////////////////////////////////////////////
// 溫度

	sprintf(szSql, "SELECT temp_panel FROM tb_temp WHERE zb_myid='58702' ORDER BY timestamp DESC LIMIT 1");
	res = mysql_query(conn, szSql);
	if (0 != res)
	{
		printf("(%s %d) mysql_query() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	result = mysql_store_result(conn);
	if (0 == result)
	{
		printf("(%s %d) mysql_store_result() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	f = 0;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		f = 1;
		t = atof(row[0]);
	}
	if (!f)
	{
		printf("(%s %d) temp_panel not found !!\n", __FILE__, __LINE__);
		mysql_close(conn);
		close(ttyfd);
		exit(2);
	}


/////////////////////////////////////////////////////////////////////////////////////////
// 日照

	sprintf(szSql, "SELECT photo_data FROM tb_photo WHERE zb_myid='29555' ORDER BY timestamp DESC LIMIT 1");
	res = mysql_query(conn, szSql);
	if (0 != res)
	{
		printf("(%s %d) mysql_query() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	result = mysql_store_result(conn);
	if (0 == result)
	{
		printf("(%s %d) mysql_store_result() fail '%s' !!\n", __FILE__, __LINE__, mysql_error(conn));
		mysql_close(conn);
		close(ttyfd);
		exit(1);
	}
	f = 0;
	while ((row = mysql_fetch_row(result)) != NULL)
	{
		f = 1;
		d = atof(row[0]);
	}
	if (!f)
	{
		printf("(%s %d) photo_data not found !!\n", __FILE__, __LINE__);
		mysql_close(conn);
		close(ttyfd);
		exit(2);
	}

	// calib
	if (d <= 1.00)
		w = 0.00F;

	mysql_close(conn);


/////////////////////////////////////////////////////////////////////////////////////////
// DISPLAY

	// Watt
	FD_ZERO(&rset);
	FD_SET(ttyfd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 2;

	sprintf(szTmp, "%05d", (int)w/10);
printf("watt=%05d\n", (int)w/10);
	//disp(ttyfd, 0X12, 5, szTmp);
	disp(ttyfd, 12, 5, szTmp);
	res = select(ttyfd + 1, &rset, 0, 0, &tv);
	if (res > 0)
	{
		if (FD_ISSET(ttyfd, &rset))
		{
			len = read(ttyfd, &rb, sizeof(rb));
			//if (len > 0)
			//	printf("ACK 0\n");
		}
	}

	// Temperature
	FD_ZERO(&rset);
	FD_SET(ttyfd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 2;

	sprintf(szTmp, "%02.1f", t);
printf("temp=%02.1f\n", t);

	//disp(ttyfd, 0X13, 4, szTmp);
	disp(ttyfd, 13, 4, szTmp);
	res = select(ttyfd + 1, &rset, 0, 0, &tv);
	if (res > 0)
	{
		if (FD_ISSET(ttyfd, &rset))
		{
			len = read(ttyfd, &rb, sizeof(rb));
			//if (len > 0)
			//	printf("ACK 1\n");
		}
	}

	// kWh
	FD_ZERO(&rset);
	FD_SET(ttyfd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 2;

	sprintf(szTmp, "%06d", (int)((s1-s0)/10.00));
//printf("s1=%.02f, s0=%.02f\n", s1, s0);
printf("kWh=%06d\n", (int)((s1-s0)/10.00));

	disp(ttyfd, 14, 6, szTmp);
	res = select(ttyfd + 1, &rset, 0, 0, &tv);
	if (res > 0)
	{
		if (FD_ISSET(ttyfd, &rset))
		{
			len = read(ttyfd, &rb, sizeof(rb));
			//if (len > 0)
			//	printf("ACK 2\n");
		}
	}

	// W/m^2
	FD_ZERO(&rset);
	FD_SET(ttyfd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 2;

	sprintf(szTmp, "%04d", (int)d);
printf("radiation=%04d\n", (int)d);
	disp(ttyfd, 15, 4, szTmp);
	res = select(ttyfd + 1, &rset, 0, 0, &tv);
	if (res > 0)
	{
		if (FD_ISSET(ttyfd, &rset))
		{
			len = read(ttyfd, &rb, sizeof(rb));
			//if (len > 0)
			//	printf("ACK 3\n");
		}
	}

	// CO2
	FD_ZERO(&rset);
	FD_SET(ttyfd, &rset);
	memset(&tv, 0, sizeof(tv));
	tv.tv_sec = 2;

	sprintf(szTmp, "%06d", (int)((s1-s0)/10.00*0.637));
printf("co2=%06d\n", (int)((s1-s0)/10.00*0.637));
	disp(ttyfd, 16, 6, szTmp);
	res = select(ttyfd + 1, &rset, 0, 0, &tv);
	if (res > 0)
	{
		if (FD_ISSET(ttyfd, &rset))
		{
			len = read(ttyfd, &rb, sizeof(rb));
			//if (len > 0)
			//	printf("ACK 4\n");
		}
	}


	close(ttyfd);
	return 0;
}


