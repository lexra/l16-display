
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

#include "list.h"


/////////////////////////////////////////////////////////////////////////////////////////
//

#define SOCKET_BUFFER_SIZE								(512 * 1024)
#define RW_LEN											(32 * 1024)
#define BUFFER_PRINT_OUT									0
#define OPEN_FILE_MAX									(1000)	//	/proc/sys/fs/file-max



/////////////////////////////////////////////////////////////////////////////////////////
//
typedef struct tm SYSTEMTIME;

struct pool_t
{
	struct list_head list;
	int sd;
	int dir;
	int received;
	struct sockaddr_in cliaddr;

	struct pool_t *peer;
};

static struct pool_t pl;




/////////////////////////////////////////////////////////////////////////////////////////
//
extern char *inet_ntoa(struct in_addr in);


void GetLocalTime(SYSTEMTIME *st)
{
	struct tm *pst = NULL;
	time_t t = time(NULL);
	pst = localtime(&t);
	memcpy(st, pst, sizeof(SYSTEMTIME));
	st->tm_year += 1900;
}

void SigInt (int signo)
{
	if (SIG_ERR == signal(signo, SIG_IGN))
		printf("signal error");

	if (SIGINT == signo)
		printf("SIGINT\n");
	else if (SIGTERM == signo)
		printf("SIGTERM\n");
	else if (SIGHUP == signo)
		printf("SIGHUP\n");
	else if (SIGPIPE == signo)
		printf("SIGPIPE\n");
	else if (SIGABRT == signo)
		printf("SIGABRT\n");
	else
		printf("MISC SIGNO=%d\n", signo);

	if (SIG_ERR == signal(signo, SigInt))
		printf("signal error");
}

void PrintUsage(void)
{
	char filename[128];
	int i = 0;

	memset(filename, 0, sizeof(filename));
	strcpy(filename, __FILE__);
	for (; i <128; i++)
		if ('.' == filename[i])	filename[i] = 0;

	printf("Usage: %s [SSH_FWD_PORT] [SSH_CONN_PORT_1]\n", filename);
	printf("%s 14000 34000\n", filename), exit(1);
}


int main(int argc, char *argv[])
{
	int lsd[2] = {-1, -1};
	SYSTEMTIME st;
	int v = 0;
	struct sockaddr_in servaddr[2];
	fd_set rset;
	int maxfd = 0;
	struct list_head *pos, *q;
	struct list_head *pos1, *q1;
	struct pool_t *a;
	int sd;
	int pair;

	int res;
	struct pool_t *a1;

	if (!BUFFER_PRINT_OUT)										setbuf(stdout, 0);
	if (3 != argc)													PrintUsage();

	GetLocalTime(&st);
	lsd[0] = socket(AF_INET, SOCK_STREAM, 0);
	lsd[1] = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == lsd[0] || -1 == lsd[1])
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) SOCKET() fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}
	v = 1;
	if (-1 == setsockopt(lsd[0], SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) SETSOCKOPT() SO_REUSEADDR fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}
	v = 1;
	if (-1 == setsockopt(lsd[1], SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) SETSOCKOPT() SO_REUSEADDR fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}

	if (0 == memset(&servaddr[0], 0, sizeof(struct sockaddr_in)))			printf("(%s %d) MEMSET() fail, EXIT \n", __FILE__, __LINE__), exit(1);
	servaddr[0].sin_family = AF_INET; servaddr[0].sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr[0].sin_port = htons(atoi(argv[1]));
	if (0 > bind(lsd[0], (struct sockaddr *)&servaddr[0], sizeof(struct sockaddr_in)))
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) tcp port 0 (%s %d) BIND() fail, EXIT \n", __FILE__, __LINE__, argv[1], atoi(argv[1])), exit(1);
	}

	if (0 == memset(&servaddr[1], 0, sizeof(struct sockaddr_in)))			printf("(%s %d) MEMSET() fail, EXIT \n", __FILE__, __LINE__), exit(1);
	servaddr[1].sin_family = AF_INET; servaddr[1].sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr[1].sin_port = htons(atoi(argv[2]));
	if (-1 == bind(lsd[1], (struct sockaddr *)&servaddr[1], sizeof(struct sockaddr_in)))
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) tcp port 1 (%s %d) BIND() fail, EXIT \n", __FILE__, __LINE__, argv[2], atoi(argv[2])), exit(1);
	}

	if (-1 == listen(lsd[0], 12) || -1 == listen(lsd[1], 12))
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) LISTEN() fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}

	v = fcntl(lsd[0], F_GETFL, 0);
	if (0 > v)
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_GETFL fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}
	if (0 > fcntl(lsd[0], F_SETFL, v | O_NONBLOCK))
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_SETFL|O_NONBLOCK fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}
	v = fcntl(lsd[1], F_GETFL, 0);
	if (0 > v)
	{
		if (lsd[0] != -1)	close(lsd[0]);
		if (lsd[1] != -1)	close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_GETFL fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}
	if (0 > fcntl(lsd[1], F_SETFL, v | O_NONBLOCK))
	{
		if (lsd[0] != -1)											close(lsd[0]);
		if (lsd[1] != -1)											close(lsd[1]);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_SETFL|O_NONBLOCK fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}

	if (SIG_ERR == signal(SIGINT, SigInt))							printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(lsd[0]), close(lsd[1]), exit(1);
	if (SIG_ERR == signal(SIGTERM, SigInt))							printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(lsd[0]), close(lsd[1]), exit(1);
	if (SIG_ERR == signal(SIGHUP, SigInt))							printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(lsd[0]), close(lsd[1]), exit(1);
	if (SIG_ERR == signal(SIGPIPE, SIG_IGN))							printf("(%s %d) SIGPIPE error, EXIT \n", __FILE__, __LINE__), close(lsd[0]), close(lsd[1]), exit(1);

	INIT_LIST_HEAD(&pl.list);
	for (;;)
	{
		struct sockaddr_in cliaddr;
		int clilen, connfd;
		int l, f, len;
		//unsigned char buff[RW_LEN * 2];
		unsigned char buff[RW_LEN];

		FD_ZERO(&rset);
		FD_SET(lsd[0], &rset);
		FD_SET(lsd[1], &rset);
		maxfd = 0;
		if (maxfd < lsd[0])											maxfd = lsd[0];
		if (maxfd < lsd[1])											maxfd = lsd[1];
		list_for_each(pos, &pl.list)
		{
			a = list_entry(pos, struct pool_t, list);
			sd = a->sd;
			FD_SET(sd, &rset);
			if (maxfd < sd)										maxfd = sd;
		}
		if (0 > (res = select(maxfd + 1, &rset, 0, 0, 0)))
		{
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
			if (errno == EAGAIN)									printf("(%s %d) SELECT() fail, EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EBADF)								printf("(%s %d) SELECT() fail, EBADF, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EINTR)								printf("(%s %d) SELECT() fail, EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EINVAL)								printf("(%s %d) SELECT() fail, EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == ENOMEM)								printf("(%s %d) SELECT() fail, ENOMEM, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else													printf("(%s %d) SELECT() fail, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			goto QUIT;
		}

		if (FD_ISSET(lsd[0], &rset))
		{
			clilen = sizeof(cliaddr);
			connfd = accept(lsd[0], (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);
			if (connfd < 0)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

				if (EAGAIN == errno || ECONNABORTED == errno || EPROTO == errno)
				{
					printf("(%s %d) lsd[0] ACCEPT() fail, /ECONNABORTED/EPROTO, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET1;
				}
				else if (ECONNRESET == errno)
				{
					printf("(%s %d) lsd[0] ACCEPT() fail, ECONNRESET, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET1;
				}
				else if (EINTR == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EINVAL == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOTSOCK == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, ENOTSOCK, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EOPNOTSUPP == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, EOPNOTSUPP, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EFAULT == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, EFAULT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOBUFS == errno || ENOMEM == errno)
					printf("(%s %d) lsd[0]  ACCEPT() fail, ENOMEM, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOSR == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, ENOSR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ESOCKTNOSUPPORT == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, ESOCKTNOSUPPORT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EPROTONOSUPPORT == errno)
					printf("(%s %d) lsd[0] ACCEPT() fail, EPROTONOSUPPORT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ETIMEDOUT == errno)
				{
					printf("(%s %d) lsd[0] ACCEPT() fail, ETIMEDOUT, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET1;
				}
				else												printf("(%s %d) lsd[0] ACCEPT() fail, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				goto QUIT;
			}
			if (connfd > OPEN_FILE_MAX)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) connfd=%d > OPEN_FILE_MAX, QUIT \n", __FILE__, __LINE__, connfd), close(connfd);
				goto QUIT;
			}

			v = SOCKET_BUFFER_SIZE;
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SETSOCKOPT() SO_SNDBUF fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SETSOCKOPT() SO_RCVBUF fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 > (v = fcntl(connfd, F_GETFL, 0)))
				printf("(%s %d) FCNTL() F_GETFL fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 > fcntl(connfd, F_SETFL, v | O_NONBLOCK))
				printf("(%s %d) FCNTL() F_SETFL|O_NONBLOCK fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);

			if (0 == (a = (struct pool_t *)malloc(sizeof(struct pool_t))))	printf("(%s %d) MALLOC() fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 == memset(a, 0, sizeof(struct pool_t)))				printf("(%s %d) MEMSET() fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			a->sd = connfd, a->dir = 0, a->peer = 0,  memcpy(&a->cliaddr, &cliaddr, clilen);
			list_for_each(pos1, &pl.list)
			{
				a1 = list_entry(pos1, struct pool_t, list);
				if (1 != a1->dir || 0 != a1->peer)					continue;
				a->peer = a1, a1->peer = a;
				break;
			}
			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
			printf("(%d,%d) CLIENT(%s) CONNECTED ON PORT(%d) \n", connfd, a->peer ? a->peer->sd : -1, (char *)inet_ntoa(a->cliaddr.sin_addr), ntohs(servaddr[0].sin_port));
			list_add_tail(&a->list, &pl.list);
		}

NEXT_ISSET1:
		if (FD_ISSET(lsd[1], &rset))
		{
			clilen = sizeof(cliaddr);
			connfd = accept(lsd[1], (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);
			if (connfd < 0)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

				if (EAGAIN == errno || ECONNABORTED == errno || EPROTO == errno)
				{
					printf("(%s %d) lsd[1] ACCEPT() fail, /ECONNABORTED/EPROTO, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET2;
				}
				else if (ECONNRESET == errno)
				{
					printf("(%s %d) lsd[1] ACCEPT() fail, ECONNRESET, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET2;
				}
				else if (EINTR == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EINVAL == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOTSOCK == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, ENOTSOCK, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EOPNOTSUPP == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, EOPNOTSUPP, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EFAULT == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, EFAULT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOBUFS == errno || ENOMEM == errno)
					printf("(%s %d) lsd[1]  ACCEPT() fail, ENOMEM, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOSR == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, ENOSR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ESOCKTNOSUPPORT == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, ESOCKTNOSUPPORT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EPROTONOSUPPORT == errno)
					printf("(%s %d) lsd[1] ACCEPT() fail, EPROTONOSUPPORT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ETIMEDOUT == errno)
				{
					printf("(%s %d) lsd[1] ACCEPT() fail, ETIMEDOUT, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET2;
				}
				else												printf("(%s %d) lsd[1] ACCEPT() fail, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				goto QUIT;
			}
			if (connfd > OPEN_FILE_MAX)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) connfd=%d > OPEN_FILE_MAX, QUIT \n", __FILE__, __LINE__, connfd), close(connfd);
				goto QUIT;
			}

			v = SOCKET_BUFFER_SIZE;
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SETSOCKOPT() SO_SNDBUF fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SETSOCKOPT() SO_RCVBUF fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 > (v = fcntl(connfd, F_GETFL, 0)))
				printf("(%s %d) FCNTL() F_GETFL fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 > fcntl(connfd, F_SETFL, v | O_NONBLOCK))
				printf("(%s %d) FCNTL() F_SETFL|O_NONBLOCK fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);

			if (0 == (a = (struct pool_t *)malloc(sizeof(struct pool_t))))	printf("(%s %d) MALLOC() fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			if (0 == memset(a, 0, sizeof(struct pool_t)))				printf("(%s %d) MEMSET() fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			a->sd = connfd, a->dir = 1, a->peer = 0,  memcpy(&a->cliaddr, &cliaddr, clilen);
			list_for_each(pos1, &pl.list)
			{
				a1 = list_entry(pos1, struct pool_t, list);
				if (0 != a1->dir || 0 != a1->peer)					continue;
				a->peer = a1, a1->peer = a;
				break;
			}
			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
			printf("(%d,%d) CLIENT(%s) CONNECTED ON PORT(%d) \n", connfd, a->peer ? a->peer->sd : -1, (char *)inet_ntoa(a->cliaddr.sin_addr), ntohs(servaddr[1].sin_port));
			list_add_tail(&a->list, &pl.list);
		}

NEXT_ISSET2:
		list_for_each_safe(pos, q, &pl.list)
		{
			a = list_entry(pos, struct pool_t, list);
			sd = a->sd;
			if (a->peer)											pair = a->peer->sd;
			else													pair = -1;

			if (!FD_ISSET(sd, &rset))								continue;
			len = read(sd, buff, sizeof(buff));
			if (0 > len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

				if (EWOULDBLOCK == errno)
				{
					printf("(%s %d) socket=%d READ() fail, EWOULDBLOCK, errno=%d, CONTINUE \n", __FILE__, __LINE__, sd, errno);
					continue;
				}
				if (EAGAIN == errno)
				{
					printf("(%s %d) socket=%d READ() fail, EAGAIN, errno=%d, CONTINUE \n", __FILE__, __LINE__, sd, errno);
					continue;
				}
				if (EINTR == errno)
				{
					printf("(%s %d) socket=%d READ() fail, EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
					goto QUIT;
				}

				if (EBADF == errno)
				{
					printf("(%s %d) socket=%d READ() fail, EBADF, errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
					goto QUIT;

				}
				if (ECONNRESET == errno)
				{
					printf("(%s %d) socket=%d READ() fail, ECONNRESET, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)),  close(sd);
					if ((f = (a->peer) ? 1 : 0))						a->peer->peer = 0, a->peer = 0;
					list_del(pos), free(a);
					if (f && -1 != pair)
					{
						list_for_each_safe(pos1, q1, &pl.list)
						{
							a1 = list_entry(pos1, struct pool_t, list);
							if (pair != a1->sd)						continue;
							list_del(pos1), free(a1);
							close(pair);
							break;
						}
					}
					goto NEXT_ISSET2;
				}
				printf("(%s %d) socket=%d READ() fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
				goto QUIT;
			}
			if (0 == len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) socket=%d READ() EOF, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
				if ((f = (a->peer) ? 1 : 0))							a->peer->peer = 0, a->peer = 0;
				list_del(pos), free(a);
				if (f && -1 != pair)
				{
					list_for_each_safe(pos1, q1, &pl.list)
					{
						a1 = list_entry(pos1, struct pool_t, list);
						if (pair != a1->sd)							continue;
						list_del(pos1), free(a1);
						close(pair);
						break;
					}
				}
				goto NEXT_ISSET2;
			}

			if (0 == a->received)
			{
				a->received = 1;
				if (buff[8] == 'O' && buff[9] == 'p' && buff[10] == 'e' && buff[11] == 'n' && buff[12] == 'S' && buff[13] == 'S' && buff[14] == 'H')
				{
					if (-1 == pair)
					{
						list_del(pos), free(a);
						close(sd);
						goto NEXT_ISSET2;
					}
				}
			}
			if (-1 == pair)											goto NEXT_ISSET2;
			if (0 > (l = sendto(pair, buff, len, MSG_NOSIGNAL, NULL, 0)))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				if (EPIPE == errno)
				{
					printf("EPIPE\n");
					goto QUIT;
				}
				if (EAGAIN == errno)
				{
					printf("EAGAIN\n");
					goto QUIT;
				}
				if (EWOULDBLOCK == errno)
				{
					printf("EWOULDBLOCK\n");
					goto QUIT;

				}
				if (EBADF == errno)
				{
					printf("EBADF\n");
					goto QUIT;
				}
				if (ECONNRESET == errno)
				{
					printf("(%s %d) socket=%d SENDTO() fail, ECONNRESET, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr));
					close(sd);
					if ((f = (a->peer) ? 1 : 0))						a->peer->peer = 0, a->peer = 0;
					list_del(pos), free(a);
					if (f && -1 != pair)
					{
						list_for_each_safe(pos1, q1, &pl.list)
						{
							a1 = list_entry(pos1, struct pool_t, list);
							if (pair != a1->sd)						continue;
							list_del(pos1), free(a1);
							close(pair);
							break;
						}
					}
					goto NEXT_ISSET2;
				}
				printf("(%s %d) socket=%d SENDTO() fail, MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
				goto QUIT;
			}
			if (0 == l)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) socket=%d SENDTO() ZERO, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
				if ((f = (a->peer) ? 1 : 0))							a->peer->peer = 0, a->peer = 0;
				list_del(pos), free(a);
				if (f && -1 != pair)
				{
					list_for_each_safe(pos1, q1, &pl.list)
					{
						a1 = list_entry(pos1, struct pool_t, list);
						if (pair != a1->sd)							continue;
						list_del(pos1), free(a1);
						close(pair);
						break;
					}
				}
				goto NEXT_ISSET2;
			}
		}
	}

QUIT:
	GetLocalTime(&st);
	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) PROGRAM IS GOING DOWN !!\n", __FILE__, __LINE__);

	list_for_each_safe(pos, q, &pl.list)
	{
		a = list_entry(pos, struct pool_t, list);
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) socket=%d CLOSE(%s) \n", __FILE__, __LINE__, a->sd, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(a->sd);
		list_del(pos), free(a);
	}
	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) lsd[0]=%d CLOSE LISTEN PORT(%d) \n", __FILE__, __LINE__, lsd[0], ntohs(servaddr[0].sin_port)), close(lsd[0]);
	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) lsd[1]=%d CLOSE LISTEN PORT(%d) \n", __FILE__, __LINE__, lsd[1], ntohs(servaddr[1].sin_port)), close(lsd[1]);

	return 0;
}



