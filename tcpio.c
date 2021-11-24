
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



static int conn(char *domain, int port)
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
	//char host[128];
	int i, j;
	int l;

	int sd = -1;
	int val, v;
	fd_set rset;
	struct timeval tv;
	int res;
	unsigned char buf[256];
	char hex[256 * 3];
	int len;
	char rb[128 * 1024];

	unsigned int n;


	if (4 != argc)
		printf("Usage:\t\ttcpio [host_ip] [conn_port] [hex_data] \n"), \
			printf("example:\ttcpio 192.168.2.126 34000 FE-0F-24-01-00-00-67-67-01-00-94-20-1E-05-A0-3A-0E-01-00-11 \n"), exit(1);

	if (15 < (l = strlen(argv[1])))		printf("[host_ip] format error 0\n"), exit(1);
	if (0 == l)					printf("[host_ip] format error 1\n"), exit(1);
	if (argv[1][0] == '.')			printf("[host_ip] format error 2\n"), exit(1);
	for (i = 0; i < l; i++)
		if (!('.' == argv[1][i] || (argv[1][i] >= '0' && argv[1][i] <= '9')))		printf("[host_ip] format error 3\n"), exit(1);


	if (6 < (l = strlen(argv[2])))		printf("[conn_port] format error\n"), exit(1);
	if (0 == l)					printf("[conn_port] format error\n"), exit(1);
	for (i = 0; i < l; i++)
	{
		if (argv[2][i] < '0')			printf("[conn_port] format error\n"), exit(1);
		if (argv[2][i] > '9')			printf("[conn_port] format error\n"), exit(1);
	}
	if (0 >= atoi(argv[2]))			printf("[conn_port] format error\n"), exit(1);

	if (2 > (l = strlen(argv[3])))		printf("[hex_data] format error\n"), exit(1);
	if (2 != (l % 3))				printf("[hex_data] format error\n"), exit(1);
	for (i = 0; i < l; i++)
	{
		if (2 == (i % 3))
		{
			if (argv[3][i] != '-')	printf("[hex_data] format error\n"), exit(1);
		}
		else
		{
			if (argv[3][i] != '0' && argv[3][i] != '1' && argv[3][i] != '2' && argv[3][i] != '3' && argv[3][i] != '4' && argv[3][i] != '5' && argv[3][i] != '6' && argv[3][i] != '7' && argv[3][i] != '8' && argv[3][i] != '9' && argv[3][i] != 'A' && argv[3][i] != 'B' && argv[3][i] != 'C' && argv[3][i] != 'D' && argv[3][i] != 'E' && argv[3][i] != 'F')
				printf("[hex_data] format error\n"), exit(1);
		}
	}
	strcpy(hex, argv[3]);
	for (i = 0; i < l; i++)
		if ('-' == hex[i])	hex[i] = 0;

	l = (l + 1) / 3;
	for (i = 0; i < l; i++)
	{
		sscanf(&hex[3 * i], "%2X", &n);
		buf[i] = (unsigned char)(n & 0XFF);
	}



	if (0 > (sd = conn(argv[1], atoi(argv[2]))))
		printf("(%s %d) conn() fail !\n", __FILE__, __LINE__), exit(1);
	if (-1 == (val = fcntl(sd, F_GETFL, 0)))
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	if (-1 == fcntl(sd, F_SETFL, val | O_NONBLOCK))
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__), close(sd), exit(1);
	v = 256 * 1024;
	if (0 > setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_SNDBUF fail, EXIT \n", __FILE__, __LINE__), exit(1);
	if (0 > setsockopt(sd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) SO_RCVBUF fail, EXIT \n", __FILE__, __LINE__), exit(1);
	setbuf(stdout, 0);


	for (j = 0; j < 9; j++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 1;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)		printf("(%s %d) select fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, rb, sizeof(rb))) < 0)			printf("(%s %d) read fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (0 == len)
			{
				printf("(%s %d) read EOF !\n", __FILE__, __LINE__), close(sd), sd = -1;
				goto SUCCESS;
			}
			for (i = 0; i < len; i++)	printf(".");
			printf("\n");
		}
	}

	len = sendto(sd, buf, l, 0, NULL, 0);
	//len = write(sd, &buf[0], l);
	if (0 > len)									printf("(%s %d) write fail !\n", __FILE__, __LINE__);
	if (0 == len)
	{
		printf("(%s %d) write ZERO !\n", __FILE__, __LINE__), close(sd), sd = -1;
		goto SUCCESS;
	}
	for (i = 0; i < len; i++)		printf("+");
	printf("\n");


	for (j = 0; j < 9; j++)
	{
		FD_ZERO(&rset);
		FD_SET(sd, &rset);
		memset(&tv, 0, sizeof(tv));
		tv.tv_sec = 1;
		if ((res = select(sd + 1, &rset, 0, 0, &tv)) < 0)		printf("(%s %d) select fail !\n", __FILE__, __LINE__), close(sd), exit(1);
		if (0 < res)
		{
			if ((len = read(sd, rb, sizeof(rb))) < 0)			printf("(%s %d) read fail !\n", __FILE__, __LINE__), close(sd), exit(1);
			if (0 == len)
			{
				printf("(%s %d) read EOF !\n", __FILE__, __LINE__), close(sd), sd = -1;
				goto SUCCESS;
			}
			for (i = 0; i < len; i++)
				printf(".");
			printf("\n");
		}
	}


SUCCESS:
	if (-1 != sd)	close(sd);
	printf("tcpio test success \n");
	return 0;
}



