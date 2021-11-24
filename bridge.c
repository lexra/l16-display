
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
#include	<sys/mount.h>	// for mount
#include <fcntl.h>
#include <signal.h>
#include <netinet/tcp.h>
#include <inttypes.h>

#include "list.h"



/////////////////////////////////////////////////////////////////////////////////////////
//

struct pool_t
{
	struct list_head list;

	int sd;
	struct sockaddr_in cliaddr;

	char name[32];
	unsigned char buff[1024 * 4];
	int bufflen;

	int flag;
};

static struct pool_t pl;


static void SigInt (int signo)
{
	if (SIG_ERR == signal(signo, SIG_IGN))
		printf("signal error");

	if (SIG_ERR == signal(signo, SigInt))
		printf("signal error");
}


/////////////////////////////////////////////////////////////////////////////////////////
//

int main(int argc, char *argv[])
{
	int res, v;
	int maxfd = 0;
	int lsd[] = {-1, -1};
	int sd;
	fd_set rset;
	struct timeval tv;
	struct sockaddr_in servaddr[2];

	struct list_head *pos, *q;
	struct pool_t *a;

	if (3 != argc)
		printf("Usage: Bridge [port_0] [port_1] !\n"), exit(1);

	INIT_LIST_HEAD(&pl.list);

	lsd[0] = socket(AF_INET, SOCK_STREAM, 0);
	lsd[1] = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == lsd[0] || -1 == lsd[1])
		printf("(%s %d) socket() fail !\n", __FILE__, __LINE__), exit(1);

	v = 1;
	if (-1 == setsockopt(lsd[0], SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
		printf("(%s %d) setsockopt() fail !\n", __FILE__, __LINE__), exit(1);
	v = 1;
	if (-1 == setsockopt(lsd[1], SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
		printf("(%s %d) setsockopt() fail !\n", __FILE__, __LINE__), exit(1);
	//v = 1;
	//if (0 > setsockopt(lsd[0], SOL_TCP, TCP_NODELAY, (const void *)&v, sizeof(v)))
	//	printf("(%s %d) TCP_NODELAY fail !\n", __FILE__, __LINE__), exit(1);
	//v = 1;
	//if (0 > setsockopt(lsd[1], SOL_TCP, TCP_NODELAY, (const void *)&v, sizeof(v)))
	//	printf("(%s %d) TCP_NODELAY fail !\n", __FILE__, __LINE__), exit(1);

	if (0 == memset(&servaddr[0], 0, sizeof(servaddr)))
		printf("(%s %d) memset() fail !\n", __FILE__, __LINE__), exit(1);
	servaddr[0].sin_family = AF_INET; servaddr[0].sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr[0].sin_port = htons(atoi(argv[1]));
	if (-1 == bind(lsd[0], (struct sockaddr *)&servaddr[0], sizeof(struct sockaddr_in)))
		printf("(%s %d) bind() fail !\n", __FILE__, __LINE__), exit(1);
	if (0 == memset(&servaddr[1], 0, sizeof(servaddr)))
		printf("(%s %d) memset() fail !\n", __FILE__, __LINE__), exit(1);
	servaddr[1].sin_family = AF_INET; servaddr[1].sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr[1].sin_port = htons(atoi(argv[2]));
	if (-1 == bind(lsd[1], (struct sockaddr *)&servaddr[1], sizeof(struct sockaddr_in)))
		printf("(%s %d) bind() fail !\n", __FILE__, __LINE__), exit(1);

	if (-1 == listen(lsd[0], 12) || -1 == listen(lsd[1], 9))
		printf("(%s %d) listen() fail !\n", __FILE__, __LINE__), exit(1);

	v = fcntl(lsd[0], F_GETFL, 0);
	if (v < 0)
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__), exit(1);
	if (fcntl(lsd[0], F_SETFL, v | O_NONBLOCK) < 0)
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__), exit(1);

	v = fcntl(lsd[1], F_GETFL, 0);
	if (v < 0)
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__), exit(1);
	if (fcntl(lsd[1], F_SETFL, v | O_NONBLOCK) < 0)
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__), exit(1);

	if (SIG_ERR == signal(SIGINT, SigInt))
		printf("(%s %d) signal error", __FILE__, __LINE__), exit(1);
	if (SIG_ERR == signal(SIGTERM, SigInt))
		printf("(%s %d) signal error", __FILE__, __LINE__), exit(1);

	for (;;)
	{
		struct sockaddr_in cliaddr;
		int clilen, connfd;

		int len;
		unsigned char buff[1024 * 4];

		FD_ZERO(&rset);
		FD_SET(lsd[0], &rset);
		FD_SET(lsd[1], &rset);

		maxfd = 0;
		if (maxfd < lsd[0])
			maxfd = lsd[0];
		if (maxfd < lsd[1])
			maxfd = lsd[1];
		list_for_each(pos, &pl.list)
		{
			a = list_entry(pos, struct pool_t, list);
			if (0 == a)
				printf("(%s %d) list_entry return 0 !! \n", __FILE__, __LINE__), exit(1);

			sd = a->sd;
			FD_SET(sd, &rset);
			if (maxfd < sd)
				maxfd = sd;
		}

		if (0 == memset(&tv, 0, sizeof(tv)))
			printf("(%s %d) memset fail !! \n", __FILE__, __LINE__), exit(1);
		tv.tv_usec = 1;

		res = select(maxfd + 1, &rset, 0, 0, &tv);
		if (res < 0)
		{
			if (errno == EAGAIN)
				printf("(%s %d) select fail, errno=EAGAIN !!\n", __FILE__, __LINE__);
			else if (errno == EBADF)
				printf("(%s %d) select fail, errno=EBADF !!\n", __FILE__, __LINE__);
			else if (errno == EINTR)
				printf("(%s %d) select fail, errno=EINTR !!\n", __FILE__, __LINE__);
			else if (errno == EINVAL)
				printf("(%s %d) select fail, errno=EINVAL !!\n", __FILE__, __LINE__);
			else if (errno == ENOMEM)
				printf("(%s %d) select fail, errno=ENOMEM !!\n", __FILE__, __LINE__);
			else
				printf("(%s %d) select fail, errno=%d !!\n", __FILE__, __LINE__, errno);
			break;
		}

		if (0 == res)
		{
			list_for_each(pos, &pl.list)
			{
				a = list_entry(pos, struct pool_t, list);
				if (0 == a)
					printf("(%s %d) list_entry return 0 !! \n", __FILE__, __LINE__), exit(1);
				sd = a->sd;
				if (1 == a->flag)
				{
					a->flag = 0;
					if (a->bufflen > 0)
					{
						len = write(sd, a->buff, a->bufflen);
						if (0 > len)
						{
							if (EFAULT == errno)
							{
								printf("(%s %d) write() fail, errno=EFAULT\n", __FILE__, __LINE__);
								goto QUIT;
							}
							else if (EINTR == errno)
							{
								printf("(%s %d) write() fail, errno=EINTR\n", __FILE__, __LINE__);
								goto QUIT;
							}
							else if (EINVAL == errno)
							{
								printf("(%s %d) write() fail, errno=EINVAL\n", __FILE__, __LINE__);
								goto QUIT;
							}
							else if (ENOSPC == errno)
							{
								printf("(%s %d) write() fail, errno=ENOSPC\n", __FILE__, __LINE__);
								goto QUIT;
							}
							else if (EIO == errno)
							{
								printf("(%s %d) write() fail, errno=EIO\n", __FILE__, __LINE__);
								goto QUIT;
							}
							else if (EBADF == errno)
							{
								printf("(%s %d) write() fail, errno=EBADF\n", __FILE__, __LINE__);
								goto QUIT;
							}
						}
						a->bufflen = 0;
					}
				}
			}

			continue;
		}

		if (FD_ISSET(lsd[0], &rset))
		{
			clilen = sizeof(cliaddr);
			connfd = accept(lsd[0], (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);
			if (connfd < 0)
			{
				if (EAGAIN == errno || ECONNABORTED == errno || EPROTO == errno)
				{
					printf("(%s %d) accept() fail, EAGAIN !\n", __FILE__, __LINE__);
					break;
					//goto WOULDBLOCK_0;
				}
				else if (EINTR == errno)
					printf("(%s %d) accept fail, errno=EINTR\n", __FILE__, __LINE__);
				else if (EINVAL == errno)
					printf("(%s %d) accept fail, errno=EINVAL\n", __FILE__, __LINE__);
				else if (ENOTSOCK == errno)
					printf("(%s %d) accept fail, errno=ENOTSOCK\n", __FILE__, __LINE__);
				else if (EOPNOTSUPP == errno)
					printf("(%s %d) accept fail, errno=EOPNOTSUPP\n", __FILE__, __LINE__);
				else if (EFAULT == errno)
					printf("(%s %d) accept fail, errno=EFAULT\n", __FILE__, __LINE__);
				else if (ENOBUFS == errno || ENOMEM == errno)
					printf("(%s %d) accept fail, errno=EFAULT\n", __FILE__, __LINE__);
				else if (ENOSR == errno)
					printf("(%s %d) accept fail, errno=ENOSR\n", __FILE__, __LINE__);
				else if (ESOCKTNOSUPPORT == errno)
					printf("(%s %d) accept fail, errno=ESOCKTNOSUPPORT\n", __FILE__, __LINE__);
				else if (EPROTONOSUPPORT == errno)
					printf("(%s %d) accept fail, errno=EPROTONOSUPPORT\n", __FILE__, __LINE__);
				else if (ETIMEDOUT == errno)
					printf("(%s %d) accept fail, errno=ETIMEDOUT\n", __FILE__, __LINE__);
				else
					printf("(%s %d) accept() fail, errno=%d !\n", __FILE__, __LINE__, errno);
				break;
			}

#if 1
			{
			int keepAlive = 1;		// 開啟keepalive屬性
			int keepIdle = 60 * 5;	// 如該連接在60 * 5 秒內沒有任何數據往來,則進行探測 
			int keepInterval = 5;	// 探測時發包的時間間隔為5 秒
			int keepCount = 3;		// 探測嘗試的次數.如果第1次探測包就收到響應了,則后2次的不再發.

			if (0 > setsockopt(connfd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&keepAlive, sizeof(keepAlive)))
				printf("(%s %d) SO_KEEPALIVE !\n", __FILE__, __LINE__);
			if (0 > setsockopt(connfd, SOL_TCP, TCP_KEEPIDLE, (const void *)&keepIdle, sizeof(keepIdle)))
				printf("(%s %d) TCP_KEEPIDLE !\n", __FILE__, __LINE__);
			if (0 > setsockopt(connfd, SOL_TCP, TCP_KEEPINTVL, (const void *)&keepInterval, sizeof(keepInterval)))
				printf("(%s %d) TCP_KEEPINTVL !\n", __FILE__, __LINE__);
			if (0 > setsockopt(connfd, SOL_TCP, TCP_KEEPCNT, (const void *)&keepCount, sizeof(keepCount)))
				printf("(%s %d) TCP_KEEPCNT !\n", __FILE__, __LINE__);
			}
#endif

			v = 64 * 1024;
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__);
			v = 64 * 1024;
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__);

			v = fcntl(connfd, F_GETFL, 0);
			if (v < 0)
			{
				printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__);
				break;
			}
			if (fcntl(connfd, F_SETFL, v | O_NONBLOCK) < 0)
			{
				printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__);
				break;
			}
			a = malloc(sizeof(struct pool_t));
			if (!a)
			{
				printf("(%s %d) malloc() fail !\n", __FILE__, __LINE__);
				if (0 > close(connfd))
					printf("(%s %d) close() fail, fd=%d, errno=%d !\n", __FILE__, __LINE__, connfd, errno);
				break;
			}
			if (0 == memset(a, 0, sizeof(struct pool_t)))
				printf("(%s %d) memset fail !! \n", __FILE__, __LINE__), exit(1);
			a->sd = connfd;
			strcpy(a->name, "A0");
			memcpy(&a->cliaddr, &cliaddr, clilen);

			list_add_tail(&a->list, &pl.list);
		}
//WOULDBLOCK_0:


		if (FD_ISSET(lsd[1], &rset))
		{
			clilen = sizeof(cliaddr);
			connfd = accept(lsd[1], (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);
			if (connfd < 0)
			{
				if (EAGAIN == errno || ECONNABORTED == errno || EPROTO == errno)
				{
					printf("(%s %d) accept() fail, EAGAIN !\n", __FILE__, __LINE__);
					break;
					//goto WOULDBLOCK_1;
				}
				else if (EINTR == errno)
					printf("(%s %d) accept fail, errno=EINTR\n", __FILE__, __LINE__);
				else if (EINVAL == errno)
					printf("(%s %d) accept fail, errno=EINVAL\n", __FILE__, __LINE__);
				else if (ENOTSOCK == errno)
					printf("(%s %d) accept fail, errno=ENOTSOCK\n", __FILE__, __LINE__);
				else if (EOPNOTSUPP == errno)
					printf("(%s %d) accept fail, errno=EOPNOTSUPP\n", __FILE__, __LINE__);
				else if (EFAULT == errno)
					printf("(%s %d) accept fail, errno=EFAULT\n", __FILE__, __LINE__);
				else if (ENOBUFS == errno || ENOMEM == errno)
					printf("(%s %d) accept fail, errno=EFAULT\n", __FILE__, __LINE__);
				else if (ENOSR == errno)
					printf("(%s %d) accept fail, errno=ENOSR\n", __FILE__, __LINE__);
				else if (ESOCKTNOSUPPORT == errno)
					printf("(%s %d) accept fail, errno=ESOCKTNOSUPPORT\n", __FILE__, __LINE__);
				else if (EPROTONOSUPPORT == errno)
					printf("(%s %d) accept fail, errno=EPROTONOSUPPORT\n", __FILE__, __LINE__);
				else if (ETIMEDOUT == errno)
					printf("(%s %d) accept fail, errno=ETIMEDOUT\n", __FILE__, __LINE__);
				else
					printf("(%s %d) accept() fail, errno=%d !\n", __FILE__, __LINE__, errno);
				break;
			}

#if 1
			{
			int keepAlive = 1;		// 開啟keepalive屬性
			int keepIdle = 60 * 5;	// 如該連接在60秒內沒有任何數據往來,則進行探測 
			int keepInterval = 5;	// 探測時發包的時間間隔為5 秒
			int keepCount = 3;		// 探測嘗試的次數.如果第1次探測包就收到響應了,則后2次的不再發.

			if (0 > setsockopt(connfd, SOL_SOCKET, SO_KEEPALIVE, (const void *)&keepAlive, sizeof(keepAlive)))
				printf("(%s %d) SO_KEEPALIVE !\n", __FILE__, __LINE__);
			if (0 > setsockopt(connfd, SOL_TCP, TCP_KEEPIDLE, (const void *)&keepIdle, sizeof(keepIdle)))
				printf("(%s %d) TCP_KEEPIDLE !\n", __FILE__, __LINE__);
			if (0 > setsockopt(connfd, SOL_TCP, TCP_KEEPINTVL, (const void *)&keepInterval, sizeof(keepInterval)))
				printf("(%s %d) TCP_KEEPINTVL !\n", __FILE__, __LINE__);
			if (0 > setsockopt(connfd, SOL_TCP, TCP_KEEPCNT, (const void *)&keepCount, sizeof(keepCount)))
				printf("(%s %d) TCP_KEEPCNT !\n", __FILE__, __LINE__);
			}
#endif

			v = 64 * 1024;
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SO_SNDBUF fail !\n", __FILE__, __LINE__);
			v = 64 * 1024;
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
				printf("(%s %d) SO_RCVBUF fail !\n", __FILE__, __LINE__);

			v = fcntl(connfd, F_GETFL, 0);
			if (v < 0)
			{
				printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__);
				break;
			}
			if (fcntl(connfd, F_SETFL, v | O_NONBLOCK) < 0)
			{
				printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__);
				break;
			}
			a = malloc(sizeof(struct pool_t));
			if (!a)
			{
				printf("(%s %d) malloc() fail !\n", __FILE__, __LINE__);
				if (0 > close(connfd))
					printf("(%s %d) close() fail, fd=%d, errno=%d !\n", __FILE__, __LINE__, connfd, errno);
				break;
			}
			if (0 == memset(a, 0, sizeof(struct pool_t)))
				printf("(%s %d) memset fail !! \n", __FILE__, __LINE__), exit(1);
			a->sd = connfd;
			strcpy(a->name, "A1");
			memcpy(&a->cliaddr, &cliaddr, clilen);

			list_add_tail(&a->list, &pl.list);
		}
//WOULDBLOCK_1:


		list_for_each_safe(pos, q, &pl.list)
		{
			a = list_entry(pos, struct pool_t, list);
			if (0 == a)
			{
				printf("(%s %d) list_entry return 0 !! \n", __FILE__, __LINE__);
				goto QUIT;
			}
			sd = a->sd;
			if (!FD_ISSET(sd, &rset))
				continue;

			len = read(sd, buff, sizeof(buff));
			if (0 >= len)
			{
				if (0 > close(sd))
					printf("(%s %d) close() fail, fd=%d, errno=%d !\n", __FILE__, __LINE__, sd, errno);
				list_del(pos);
				free(a);
				continue;
			}

			if (0 == strcmp("A0", a->name))
			{
				list_for_each(pos, &pl.list)
				{
					a = list_entry(pos, struct pool_t, list);
					if (0 == a)
						printf("(%s %d) list_entry return 0 !! \n", __FILE__, __LINE__), exit(1);
					if (0 == strcmp("A1", a->name))
					{
						memcpy(a->buff, buff, len);
						a->flag = 1;
						a->bufflen = len;
					}
				}
			}
			else if (0 == strcmp("A1", a->name))
			{
				list_for_each(pos, &pl.list)
				{
					a = list_entry(pos, struct pool_t, list);
					if (0 == a)
						printf("(%s %d) list_entry return 0 !! \n", __FILE__, __LINE__), exit(1);
					if (0 == strcmp("A0", a->name))
					{
						memcpy(a->buff, buff, len);
						a->flag = 1;
						a->bufflen = len;
					}
				}
			}
		}

	}

QUIT:
	v = fcntl(lsd[0], F_GETFL, 0);
	if (-1 != v)
	{
		v &= ~O_NONBLOCK;
		fcntl(lsd[0], F_SETFL, v);
	}
	v = fcntl(lsd[1], F_GETFL, 0);
	if (-1 != v)
	{
		v &= ~O_NONBLOCK;
		fcntl(lsd[1], F_SETFL, v);
	}

	if (0 > close(lsd[0]))
		printf("(%s %d) close() fail, fd=%d, errno=%d !\n", __FILE__, __LINE__, lsd[0], errno);
	if (0 > close(lsd[1]))
		printf("(%s %d) close() fail, fd=%d, errno=%d !\n", __FILE__, __LINE__, lsd[1], errno);

	list_for_each_safe(pos, q, &pl.list)
	{
		a = list_entry(pos, struct pool_t, list);
		if (0 == a)
			continue;
		sd = a->sd;

		v = fcntl(sd, F_GETFL, 0);
		if (-1 != v)
		{
			v &= ~O_NONBLOCK;
			fcntl(sd, F_SETFL, v);
		}

		if (0 > close(sd))
			printf("(%s %d) close() fail, fd=%d, errno=%d !\n", __FILE__, __LINE__, sd, errno);
		list_del(pos);
		free(a);
	}
	return 1;
}

