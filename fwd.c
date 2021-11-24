
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


#include <netinet/tcp.h>
#include <netdb.h>

#include <fcntl.h>
#include <inttypes.h>


#if !defined(MSG_NOSIGNAL)
 #define MSG_NOSIGNAL									0
#endif // MSG_NOSIGNAL




/////////////////////////////////////////////////////////////////////////////////////////
//

#define SOCKET_BUFFER_SIZE								(512 * 1024)
#define RW_LEN											(32 * 1024)
#define BUFFER_PRINT_OUT									0
#define RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT		0
#define SELECT_EXPIRY_SECONDS							(300)
#define OPEN_FILE_MAX									(1000)	//	/proc/sys/fs/file-max




/////////////////////////////////////////////////////////////////////////////////////////
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


/////////////////////////////////////////////////////////////////////////////////////////
//

static void SigInt (int signo)
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

static int fwdConnect(char *domain, int port)
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
	int res, val, len;
	int sd[] = {-1, -1};

	int v;
	int l;
	SYSTEMTIME st;

	unsigned char rxBuff[RW_LEN * 2];

	if (5 != argc)
		printf("Usage: fwd [localHost] [localPort] [remoteHost] [remotePort] \nCompile Date: %s\n", __DATE__), exit(1);
	if (!BUFFER_PRINT_OUT)
		setbuf(stdout, 0);

	if (0 == memset(rxBuff, 0, sizeof(rxBuff)))
		printf("(%s %d) MEMSET() fail !\n", __FILE__, __LINE__), exit(1);

	GetLocalTime(&st);

	if (0 > (sd[1] = fwdConnect(argv[3], atoi(argv[4]))))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) CONNECT() sd[1]=(%s %d) fail !\n", __FILE__, __LINE__, argv[3],  atoi(argv[4])), exit(1);
	}
	if (-1 == (val = fcntl(sd[1], F_GETFL, 0)))			printf("(%s %d) sd[1]=%d F_GETFL fail !\n", __FILE__, __LINE__, sd[1]), close(sd[1]), exit(1);
	if (-1 == fcntl(sd[1], F_SETFL, val | O_NONBLOCK))	printf("(%s %d) sd[1]=%d F_SETFL fail !\n", __FILE__, __LINE__, sd[1]), close(sd[1]), exit(1);


	if (-1 == (sd[0] = fwdConnect(argv[1], atoi(argv[2]))))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) connect sd[0]=(%s %d) fail !\n", __FILE__, __LINE__, argv[1],  atoi(argv[2])), close(sd[1]), exit(1);
	}
	val = fcntl(sd[0], F_GETFL, 0);
	if (-1 == val)									printf("(%s %d) sd[0]=%d F_GETFL fail !\n", __FILE__, __LINE__, sd[0]), close(sd[1]), close(sd[0]), exit(1);
	if (-1 == fcntl(sd[0], F_SETFL, val | O_NONBLOCK))	printf("(%s %d) sd[0]=%d F_SETFL fail !\n", __FILE__, __LINE__, sd[0]), close(sd[1]), close(sd[0]), exit(1);


	if (sd[1] > OPEN_FILE_MAX)		printf("(%s %d) sd[1] > OPEN_FILE_MAX, QUIT \n", __FILE__, __LINE__), close(sd[1]), close(sd[1]), close(sd[0]), exit(1);
	if (sd[0] > OPEN_FILE_MAX)		printf("(%s %d) sd[0] > OPEN_FILE_MAX, QUIT \n", __FILE__, __LINE__), close(sd[1]), close(sd[1]), close(sd[0]), exit(1);


	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) NOW CONNCT TO sd[1]=(%d, %s %d) \n", __FILE__, __LINE__, sd[1], argv[3], atoi(argv[4]));
	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) NOW CONNCT TO sd[0]=(%d, %s %d) \n", __FILE__, __LINE__, sd[0], argv[1], atoi(argv[2]));


	v = SOCKET_BUFFER_SIZE;
	if (0 > setsockopt(sd[0], SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) sd[0]=%d SO_SNDBUF fail, EXIT \n", __FILE__, __LINE__, sd[0]), close(sd[1]), close(sd[0]), exit(1);
	if (0 > setsockopt(sd[0], SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) sd[0]=%d SO_RCVBUF fail, EXIT \n", __FILE__, __LINE__, sd[0]), close(sd[1]), close(sd[0]), exit(1);

	if (0 > setsockopt(sd[1], SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) sd[1]=%d SO_SNDBUF fail, EXIT \n", __FILE__, __LINE__, sd[1]), close(sd[1]), close(sd[0]), exit(1);
	if (0 > setsockopt(sd[1], SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
		printf("(%s %d) sd[1]=%d SO_RCVBUF fail, EXIT \n", __FILE__, __LINE__, sd[1]), close(sd[1]), close(sd[0]), exit(1);


	if (SIG_ERR == signal(SIGINT, SigInt))
		printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(sd[1]), close(sd[0]), exit(1);
	if (SIG_ERR == signal(SIGTERM, SigInt))
		printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(sd[1]), close(sd[0]), exit(1);
	if (SIG_ERR == signal(SIGHUP, SigInt))
		printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(sd[1]), close(sd[0]), exit(1);
	if (SIG_ERR == signal(SIGPIPE, SIG_IGN))
		printf("(%s %d) SIGPIPE error, EXIT \n", __FILE__, __LINE__), close(sd[1]), close(sd[0]), exit(1);



	for (;;)
	{
		int maxfd = 0;
		fd_set rset;
		fd_set wset;
		struct timeval tv;

		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_SET(sd[0], &rset);
		FD_SET(sd[1], &rset);

		if (maxfd < sd[0])			maxfd = sd[0];
		if (maxfd < sd[1])			maxfd = sd[1];

		if (0 == memset(&tv, 0, sizeof(tv)))	printf("(%s %d) memset() fail, EXIT \n", __FILE__, __LINE__), close(sd[1]), close(sd[0]), exit(1);
		tv.tv_sec = SELECT_EXPIRY_SECONDS;

		if (SELECT_EXPIRY_SECONDS)
			res = select(maxfd + 1, &rset, 0, 0, &tv);
		else
			res = select(maxfd + 1, &rset, 0, 0, 0);
		if (0 > res)
		{
			GetLocalTime(&st);
			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

			if (errno == EAGAIN)
				printf("(%s %d) SELECT() fail, EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EWOULDBLOCK)
				printf("(%s %d) SELECT() fail, EWOULDBLOCK, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EBADF)
				printf("(%s %d) SELECT() fail, EBADF, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EINTR)
				printf("(%s %d) SELECT() fail, EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EINVAL)
				printf("(%s %d) SELECT() fail, EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == ENOMEM)
				printf("(%s %d) SELECT() fail, ENOMEM, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else
				printf("(%s %d) SELECT() fail, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			goto QUIT;
		}


		GetLocalTime(&st);
		if (0 == res)
		{
			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
			printf("(%s %d) SELECT() 300-SECONDS TIMEOUT, NO-DATA TRANSFER, QUIT \n", __FILE__, __LINE__);
			goto QUIT;
		}


		if (FD_ISSET(sd[0], &rset))
		{
			len = read(sd[0], &rxBuff[0], RW_LEN);
			if (0 > len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

				if (EAGAIN == errno)
				{
					if (RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT)
					{
						printf("(%s %d) socket[0]=%d READ() fail, EAGAIN errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}

					printf("(%s %d) socket[0]=%d READ() fail, EAGAIN errno=%d, CONTINUE \n", __FILE__, __LINE__, sd[0], errno);
					goto NEXT_ISSET;
				}
				if (EWOULDBLOCK == errno)
				{
					if (RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT)
					{
						printf("(%s %d) socket[0]=%d READ() fail, EAGAIN errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}

					printf("(%s %d) socket[0]=%d READ() fail, EWOULDBLOCK errno=%d, CONTINUE \n", __FILE__, __LINE__, sd[0], errno);
					goto NEXT_ISSET;
				}
				if (EINTR == errno)
				{
					printf("(%s %d) socket[0]=%d READ() fail, EINTR errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
					goto QUIT;
				}

				if (ECONNRESET == errno)
				{
					printf("(%s %d) socket[0]=%d READ() fail, ECONNRESET errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
					goto QUIT;
				}

				printf("(%s %d) socket[0]=%d READ() fail, MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
				goto QUIT;
			}
			if (0 == len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) socket[0]=%d READ() EOF, QUIT \n", __FILE__, __LINE__, sd[0]);
				goto QUIT;
			}
			if (len > 0)
			{
				if (0 > (l = sendto(sd[1], &rxBuff[0], len, MSG_NOSIGNAL, NULL, 0)))
				{
					printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

					if (EPIPE == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EPIPE, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					if (EAGAIN == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					if (EWOULDBLOCK == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EWOULDBLOCK, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					if (ECONNRESET == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, ECONNRESET, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}

					if (EFAULT == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EFAULT, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					else if (EINTR == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					else if (EINVAL == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					else if (ENOSPC == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, ENOSPC, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					else if (EIO == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EIO, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					else if (EBADF == errno)
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, EBADF, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
					else
					{
						printf("(%s %d) socket[1]=%d SENDTO() fail, MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}
				}

				if (0 == l)
				{
					printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
					printf("(%s %d) socket[1]=%d SENDTO() ZERO, QUIT \n", __FILE__, __LINE__, sd[1]);
					goto QUIT;
				}
			}
		}



NEXT_ISSET:
		if (FD_ISSET(sd[1], &rset))
		{
			len = read(sd[1], &rxBuff[RW_LEN], RW_LEN);
			if (0 > len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

				if (EAGAIN == errno)
				{
					if (RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT)
					{
						printf("(%s %d) socket[0]=%d READ() fail, EAGAIN errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}

					printf("(%s %d) socket[1]=%d READ() fail, EAGAIN errno=%d, CONTINUE \n", __FILE__, __LINE__, sd[1], errno);
					continue;
				}
				if (EWOULDBLOCK == errno)
				{
					if (RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT)
					{
						printf("(%s %d) socket[0]=%d READ() fail, EAGAIN errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
						goto QUIT;
					}

					printf("(%s %d) socket[1]=%d READ() fail, EWOULDBLOCK errno=%d, CONTINUE \n", __FILE__, __LINE__, sd[1], errno);
					continue;
				}
				if (EINTR == errno)
				{
					printf("(%s %d) socket[1]=%d READ() fail, EINTR errno=%d, CONTINUE \n", __FILE__, __LINE__, sd[1], errno);
					goto QUIT;
				}

				if (ECONNRESET == errno)
				{
					printf("(%s %d) socket[1]=%d READ() fail, ECONNRESET errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
					goto QUIT;
				}

				printf("(%s %d) socket[1]=%d READ() fail, MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd[1], errno);
				goto QUIT;
			}
			if (0 == len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) socket[1]=%d READ() EOF, QUIT \n", __FILE__, __LINE__, sd[0]);
				goto QUIT;
			}
			if (len > 0)
			{
				if (0 > (l = sendto(sd[0], &rxBuff[RW_LEN], len, MSG_NOSIGNAL, NULL, 0)))
				{
					printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

					if (EPIPE == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					if (EAGAIN == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					if (EWOULDBLOCK == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EWOULDBLOCK, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					if (ECONNRESET == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, ECONNRESET, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}

					if (EFAULT == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EFAULT, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					else if (EINTR == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					else if (EINVAL == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					else if (ENOSPC == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, ENOSPC, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					else if (EIO == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EIO, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					else if (EBADF == errno)
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, EBADF, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
					else
					{
						printf("(%s %d) socket[0]=%d SENDTO() fail, MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd[0], errno);
						goto QUIT;
					}
				}

				if (0 == l)
				{
					printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
					printf("(%s %d) socket[0]=%d SENDTO() ZERO, QUIT \n", __FILE__, __LINE__, sd[0]);
					goto QUIT;
				}
			}
		}
	}


QUIT:
	if (0 > close(sd[0]))
		printf("(%s %d) sd[0]=%d close() fail, errno=%d !\n", __FILE__, __LINE__, sd[0], errno);
	if (0 > close(sd[1]))
		printf("(%s %d) sd[1]=%d close() fail, errno=%d !\n", __FILE__, __LINE__, sd[1], errno);

	sleep(1);
	return 1;
}




