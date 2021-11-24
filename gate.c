
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <inttypes.h>

#include "list.h"


#if defined(MOXA)
 #include <moxadevice.h>
#endif // MOXA, MAC


#if !defined(MSG_NOSIGNAL)
 #define MSG_NOSIGNAL									0
#endif // MSG_NOSIGNAL




/////////////////////////////////////////////////////////////////////////////////////////
//

#define CONCURRENT_CLIENT_NUMBER						(36)
#define SOCKET_BUFFER_SIZE								(512 * 1024)
#define RW_LEN											(32 * 1024)
#define BUFFER_PRINT_OUT									0
#define RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT		0
#define SELECT_EXPIRY_SECONDS							(300)
#define OPEN_FILE_MAX									(1000)	//	/proc/sys/fs/file-max




struct pool_t
{
	struct list_head list;
	int sd;
	struct sockaddr_in cliaddr;
	char name[32];
};

static struct pool_t pl;


static void SigInt (int signo)
{
	if (SIG_ERR == signal(signo, SIG_IGN))	printf("signal error\n");

	if (SIGINT == signo)					printf("SIGINT\n");
	else if (SIGTERM == signo)				printf("SIGTERM\n");
	else if (SIGHUP == signo)				printf("SIGHUP\n");
	else if (SIGPIPE == signo)				printf("SIGPIPE\n");
	else if (SIGABRT == signo)				printf("SIGABRT\n");
	else									printf("MISC SIGNO=%d\n", signo);

	if (SIG_ERR == signal(signo, SigInt))		printf("signal error\n");
}



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

int main(int argc, char *argv[])
{
	int lsd = -1;
	int ttyfd = -1;
	struct stat lbuf;
	struct termios term;
	int sd = -1;

	int res, v, maxfd = 0;
	fd_set rset;
	struct timeval tv;
	struct sockaddr_in servaddr;

	struct list_head *pos, *q;
	struct pool_t *a;

	SYSTEMTIME st;
	int l;

	if (4 != argc)
		printf("Usage: gate /dev/tty[M|S|USB][0-9] B38400 34000\nCompile Date: %s\n", __DATE__), exit(1);
	if (!BUFFER_PRINT_OUT)
		setbuf(stdout, 0);

	GetLocalTime(&st);

	if (0 > (lsd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) SOCKET() fail, EXIT \n", __FILE__, __LINE__), exit(1);
	}

	v = 1;
	if (0 > setsockopt(lsd, SOL_SOCKET, SO_REUSEADDR, (const void *)&v, sizeof(v)))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) SETSOCKOPT() fail, SO_REUSEADDR, EXIT \n", __FILE__, __LINE__), exit(1);
	}

	if (0 == memset(&servaddr, 0, sizeof(servaddr)))	printf("(%s %d) MEMSET() fail, EXIT \n", __FILE__, __LINE__), exit(1);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[3]));
	if (0 > bind(lsd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr_in)))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) tcp port (%s %d) BIND() fail, system need restart or retry later, EXIT \n", __FILE__, __LINE__, argv[3], atoi(argv[3])), close(lsd), exit(13);
	}
	if (0 > listen(lsd, 12))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) tcp port (%s %d) LISTEN() fail, EXIT \n", __FILE__, __LINE__, argv[3], atoi(argv[3])), close(lsd), exit(1);
	}

	if (0 > (v = fcntl(lsd, F_GETFL, 0)))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() fail, F_GETFL, EXIT \n", __FILE__, __LINE__), close(lsd), exit(1);
	}
	if (0 > fcntl(lsd, F_SETFL, v | O_NONBLOCK))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_SETFL fail, O_NONBLOCK, EXIT \n", __FILE__, __LINE__), close(lsd), exit(1);
	}

	if(0 > stat(argv[1], &lbuf))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) STAT() fail, SERIAL DEVICE( \"/dev/tty[M|S|USB][0-9]\") not found, EXIT \n", __FILE__, __LINE__), close(lsd), exit(1);
	}
	if(!S_ISCHR(lbuf.st_mode))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) not S_ISCHR(), EXIT \n", __FILE__, __LINE__), close(lsd), exit(1);
	}


	if (0 > (ttyfd = open(argv[1], O_RDWR | O_NOCTTY | O_NDELAY)))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) SERIAL DEVICE( \"/dev/tty[M|S|USB][0-9]\") OPEN() fail, please run as root and retry later, EXIT \n", __FILE__, __LINE__), close(lsd), exit(1);
	}
	if (!isatty(ttyfd))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) DEVICE( \"%s\"), ttyfd=%d, not ISATTY(), EXIT \n", __FILE__, __LINE__, argv[1], ttyfd), close(ttyfd), close(lsd), exit(1);
	}


	if (0 == memset(&term, 0, sizeof(struct termios)))
		printf("(%s %d) MEMSET() fail, EXIT \n", __FILE__, __LINE__), exit(1);


#if defined(MOXA)
	term.c_cflag = B38400 | CS8 | CLOCAL | CREAD;
	term.c_iflag = IGNPAR;
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;

	if (0 == strcmp("B230400", argv[2]))		cfsetispeed(&term, B230400), cfsetospeed(&term, B230400), printf("B230400\n");
	else if (0 == strcmp("B115200", argv[2]))	cfsetispeed(&term, B115200), cfsetospeed(&term, B115200), printf("B115200\n");
	else if (0 == strcmp("B57600", argv[2]))	cfsetispeed(&term, B57600), cfsetospeed(&term, B57600), printf("B57600\n");
	else if (0 == strcmp("B38400", argv[2]))	cfsetispeed(&term, B38400), cfsetospeed(&term, B38400), printf("B38400\n");
	else if (0 == strcmp("B19200", argv[2]))	cfsetispeed(&term, B19200), cfsetospeed(&term, B19200), printf("B19200\n");
	else if (0 == strcmp("B9600", argv[2]))	cfsetispeed(&term, B9600), cfsetospeed(&term, B9600), printf("B9600\n");
	else if (0 == strcmp("B4800", argv[2]))	cfsetispeed(&term, B4800), cfsetospeed(&term, B4800), printf("B4800\n");
	else if (0 == strcmp("B2400", argv[2]))	cfsetispeed(&term, B2400), cfsetospeed(&term, B2400), printf("B2400\n");
	else if (0 == strcmp("B1800", argv[2]))	cfsetispeed(&term, B1800), cfsetospeed(&term, B1800), printf("B1800\n");
	else if (0 == strcmp("B1200", argv[2]))	cfsetispeed(&term, B1200), cfsetospeed(&term, B1200), printf("B1200\n");
	else if (0 == strcmp("B600", argv[2]))		cfsetispeed(&term, B600), cfsetospeed(&term, B600), printf("B600\n");
	else if (0 == strcmp("B300", argv[2]))		cfsetispeed(&term, B300), cfsetospeed(&term, B300), printf("B300\n");
	else if (0 == strcmp("B200", argv[2]))		cfsetispeed(&term, B200), cfsetospeed(&term, B200), printf("B200\n");
	else if (0 == strcmp("B150", argv[2]))		cfsetispeed(&term, B150), cfsetospeed(&term, B150), printf("B150\n");
	else if (0 == strcmp("B134", argv[2]))		cfsetispeed(&term, B134), cfsetospeed(&term, B134), printf("B134\n");
	else if (0 == strcmp("B110", argv[2]))		cfsetispeed(&term, B110), cfsetospeed(&term, B110), printf("B110\n");
	else if (0 == strcmp("B75", argv[2]))		cfsetispeed(&term, B75), cfsetospeed(&term, B75), printf("B75\n");
	else if (0 == strcmp("B50", argv[2]))		cfsetispeed(&term, B50), cfsetospeed(&term, B50), printf("B50\n");
	else if (0 == strcmp("B0", argv[2]))		cfsetispeed(&term, B0), cfsetospeed(&term, B0), printf("B0\n");
	else									cfsetispeed(&term, B38400), cfsetospeed(&term, B38400), printf("B38400\n");

	if (0 > tcsetattr(ttyfd, TCSANOW, &term))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) TCSETATTR() fail, EXIT \n", __FILE__, __LINE__), close(ttyfd), close(lsd), exit(1);
	}

	v = RS232_MODE;
	if (0 == strcmp(argv[1], "/dev/ttyO0"))
		v = RS232_MODE, printf("RS232_MODE\n");
	if (0 == strcmp(argv[1], "/dev/ttyO1"))
		v = RS485_2WIRE_MODE, printf("RS485_2WIRE_MODE\n");
	if (0 == strcmp(argv[1], "/dev/ttyO2"))
		v = RS422_MODE, printf("RS422_MODE\n");
	if (0 == strcmp(argv[1], "/dev/ttyO3"))
		v = RS485_4WIRE_MODE, printf("RS485_4WIRE_MODE\n");

	if (0 > ioctl(ttyfd, MOXA_SET_OP_MODE, &v))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) IOCTL() fail (%d,%s), MOXA_SET_SPECIAL_BAUD_RATE, EXIT \n", __FILE__, __LINE__, v, argv[1]), close(ttyfd), close(lsd), exit(1);
	}

	if (0 > (v = fcntl(ttyfd, F_GETFL, 0)))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_GETFL(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, argv[1]), close(ttyfd), close(lsd), exit(1);
	}
	if (fcntl(ttyfd, F_SETFL, v | FNDELAY) < 0)
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_SETFL | FNDELAY(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, argv[1]), close(ttyfd), close(lsd), exit(1);
	}
#else

  #if defined(MAC)	//MSG_NOSIGNAL
	if (0 > tcgetattr(ttyfd, &term))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) TCGETATTR() fail, EXIT \n", __FILE__, __LINE__), close(ttyfd), close(lsd), exit(1);
  	}
  #endif // MAC

	term.c_cflag |= B38400;
	term.c_cflag |= CLOCAL;
	term.c_cflag |= CREAD;
	term.c_cflag &= ~PARENB;
	term.c_cflag &= ~CSTOPB;
	term.c_cflag &= ~CSIZE;
	term.c_cflag |= CS8;
 	term.c_iflag = IGNPAR;
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
 
	if (0 == strcmp("B230400", argv[2]))		cfsetispeed(&term, B230400), cfsetospeed(&term, B230400), printf("B230400\n");
	else if (0 == strcmp("B115200", argv[2]))	cfsetispeed(&term, B115200), cfsetospeed(&term, B115200), printf("B115200\n");
	else if (0 == strcmp("B57600", argv[2]))	cfsetispeed(&term, B57600), cfsetospeed(&term, B57600), printf("B57600\n");
	else if (0 == strcmp("B38400", argv[2]))	cfsetispeed(&term, B38400), cfsetospeed(&term, B38400), printf("B38400\n");
	else if (0 == strcmp("B19200", argv[2]))	cfsetispeed(&term, B19200), cfsetospeed(&term, B19200), printf("B19200\n");
	else if (0 == strcmp("B9600", argv[2]))	cfsetispeed(&term, B9600), cfsetospeed(&term, B9600), printf("B9600\n");
	else if (0 == strcmp("B4800", argv[2]))	cfsetispeed(&term, B4800), cfsetospeed(&term, B4800), printf("B4800\n");
	else if (0 == strcmp("B2400", argv[2]))	cfsetispeed(&term, B2400), cfsetospeed(&term, B2400), printf("B2400\n");
	else if (0 == strcmp("B1800", argv[2]))	cfsetispeed(&term, B1800), cfsetospeed(&term, B1800), printf("B1800\n");
	else if (0 == strcmp("B1200", argv[2]))	cfsetispeed(&term, B1200), cfsetospeed(&term, B1200), printf("B1200\n");
	else if (0 == strcmp("B600", argv[2]))		cfsetispeed(&term, B600), cfsetospeed(&term, B600), printf("B600\n");
	else if (0 == strcmp("B300", argv[2]))		cfsetispeed(&term, B300), cfsetospeed(&term, B300), printf("B300\n");
	else if (0 == strcmp("B200", argv[2]))		cfsetispeed(&term, B200), cfsetospeed(&term, B200), printf("B200\n");
	else if (0 == strcmp("B150", argv[2]))		cfsetispeed(&term, B150), cfsetospeed(&term, B150), printf("B150\n");
	else if (0 == strcmp("B134", argv[2]))		cfsetispeed(&term, B134), cfsetospeed(&term, B134), printf("B134\n");
	else if (0 == strcmp("B110", argv[2]))		cfsetispeed(&term, B110), cfsetospeed(&term, B110), printf("B110\n");
	else if (0 == strcmp("B75", argv[2]))		cfsetispeed(&term, B75), cfsetospeed(&term, B75), printf("B75\n");
	else if (0 == strcmp("B50", argv[2]))		cfsetispeed(&term, B50), cfsetospeed(&term, B50), printf("B50\n");
	else if (0 == strcmp("B0", argv[2]))		cfsetispeed(&term, B0), cfsetospeed(&term, B0), printf("B0\n");
	else									cfsetispeed(&term, B38400), cfsetospeed(&term, B38400), printf("B38400\n");

	if (0 > tcsetattr(ttyfd, TCSANOW, &term))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) TCSETATTR() fail, EXIT \n", __FILE__, __LINE__), close(ttyfd), close(lsd), exit(1);
	}

	if (0 > (v = fcntl(ttyfd, F_GETFL, 0)))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_GETFL(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, argv[1]), close(ttyfd), close(lsd), exit(1);
	}
	if (0 > fcntl(ttyfd, F_SETFL, v | O_NONBLOCK))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) FCNTL() F_SETFL,O_NONBLOCK(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, argv[1]), close(ttyfd), close(lsd), exit(1);
	}
#endif // MOXA



	if (0 > tcflush(ttyfd, TCIFLUSH))
	{
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) TCFLUSH() TCIFLUSH(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, argv[1]), close(ttyfd), close(lsd), exit(1);
	}


	if (SIG_ERR == signal(SIGINT, SigInt))
		printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(ttyfd), close(lsd), exit(1);
	if (SIG_ERR == signal(SIGTERM, SigInt))
		printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(ttyfd), close(lsd), exit(1);
	if (SIG_ERR == signal(SIGHUP, SigInt))
		printf("(%s %d) signal error, EXIT \n", __FILE__, __LINE__), close(ttyfd), close(lsd), exit(1);
	if (SIG_ERR == signal(SIGPIPE, SIG_IGN))
		printf("(%s %d) SIGPIPE error, EXIT \n", __FILE__, __LINE__), close(ttyfd), close(lsd), exit(1);


	INIT_LIST_HEAD(&pl.list);
	for (;;)
	{
		struct sockaddr_in cliaddr;
		int clilen, connfd;
		int len;
		unsigned char buff[RW_LEN * 2];

		FD_ZERO(&rset);
		FD_SET(lsd, &rset);
		FD_SET(ttyfd, &rset);
		maxfd = 0;
		if (maxfd < lsd)
			maxfd = lsd;
		if (maxfd < ttyfd)
			maxfd = ttyfd;
		list_for_each(pos, &pl.list)
		{
			if (0 == (a = list_entry(pos, struct pool_t, list)))				printf("(%s %d) LIST_ENTRY() return 0, EXIT \n", __FILE__, __LINE__), exit(1);
			sd = a->sd;
			FD_SET(sd, &rset);
			if (maxfd < sd)
				maxfd = sd;
		}
		if (0 == memset(&tv, 0, sizeof(tv)))							printf("(%s %d) MEMSET() fail, EXIT  \n", __FILE__, __LINE__), exit(1);
		tv.tv_sec = SELECT_EXPIRY_SECONDS;
		if (SELECT_EXPIRY_SECONDS)								res = select(maxfd + 1, &rset, 0, 0, &tv);
		else														res = select(maxfd + 1, &rset, 0, 0, 0);
		if (0 > res)
		{
			GetLocalTime(&st);

			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

			if (errno == EAGAIN)									printf("(%s %d) SELECT() fail, errno=EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EBADF)								printf("(%s %d) SELECT() fail, errno=EBADF, errno=%d , QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EINTR)								printf("(%s %d) SELECT() fail, errno=EINTR, errno=%d , QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EINVAL)								printf("(%s %d) SELECT() fail, errno=EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == ENOMEM)								printf("(%s %d) SELECT() fail, errno=ENOMEM, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else if (errno == EBADF)								printf("(%s %d) SELECT() fail, errno=EBADF, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			else													printf("(%s %d) SELECT() fail, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
			goto QUIT;
		}


		GetLocalTime(&st);
		if (0 == res)
		{
			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
			printf("(%s %d) SELECT() 300-SECONDS TIMEOUT, NO-DATA TRANSFER, QUIT \n", __FILE__, __LINE__);
			goto QUIT;
		}


		if (FD_ISSET(lsd, &rset))
		{
			int lc = 0;

			list_for_each(pos, &pl.list)
			{
				if (0 == (a = list_entry(pos, struct pool_t, list)))	printf("(%s %d) LIST_ENTRY() return 0, EXIT \n", __FILE__, __LINE__), exit(1);
				lc++;
			}
			if(lc > CONCURRENT_CLIENT_NUMBER)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) total available socket client full (%d), QUIT \n", __FILE__, __LINE__, lc);
				goto QUIT;
			}

			clilen = sizeof(cliaddr);
			if (0 > (connfd = accept(lsd, (struct sockaddr *)&cliaddr, (socklen_t *)&clilen)))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				if (EAGAIN == errno)
				{
					if (RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT)
					{
						printf("(%s %d) ACCEPT() EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
						goto QUIT;
					}

					printf("(%s %d) ACCEPT() EAGAIN, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET;
				}
				else if (ECONNABORTED == errno)
				{
					printf("(%s %d) ACCEPT() ECONNABORTED, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET;
				}
				else if (EPROTO == errno)
				{
					printf("(%s %d) ACCEPT() EPROTO, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET;
				}
				else if (ECONNRESET == errno)
				{
					printf("(%s %d) ACCEPT() ECONNRESET, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET;
				}
				else if (EINTR == errno)
					printf("(%s %d) ACCEPT() EINTR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EINVAL == errno)
					printf("(%s %d) ACCEPT() EINVAL, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOTSOCK == errno)
					printf("(%s %d) ACCEPT() ENOTSOCK, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EOPNOTSUPP == errno)
					printf("(%s %d) ACCEPT() EOPNOTSUPP, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EFAULT == errno)
					printf("(%s %d) ACCEPT() EFAULT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOBUFS == errno || ENOMEM == errno)
					printf("(%s %d) ACCEPT() ENOMEM, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ENOSR == errno)
					printf("(%s %d) ACCEPT() ENOSR, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ESOCKTNOSUPPORT == errno)
					printf("(%s %d) ACCEPT() ESOCKTNOSUPPORT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (EPROTONOSUPPORT == errno)
					printf("(%s %d) ACCEPT() EPROTONOSUPPORT, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				else if (ETIMEDOUT == errno)
				{
					printf("(%s %d) ACCEPT() ETIMEDOUT, errno=%d, CONTINUE \n", __FILE__, __LINE__, errno);
					goto NEXT_ISSET;
				}
				else
					printf("(%s %d) ACCEPT() MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, errno);
				goto QUIT;
			}

			if (connfd > OPEN_FILE_MAX)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) connfd=%d > OPEN_FILE_MAX, QUIT \n", __FILE__, __LINE__, connfd), close(connfd);
				goto QUIT;
			}

			if (0 > (v = fcntl(connfd, F_GETFL, 0)))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) FCNTL() F_GETFL(%d) fail, EXIT \n", __FILE__, __LINE__, connfd), close(connfd), exit(1);
			}
			if (0 > fcntl(connfd, F_SETFL, v | O_NONBLOCK))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) FCNTL() F_SETFL|O_NONBLOCK(%d) fail, EXIT \n", __FILE__, __LINE__, connfd), close(connfd), exit(1);
			}

			v = SOCKET_BUFFER_SIZE;
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, (const void *)&v, sizeof(v)))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) SETSOCKOPT() fail, SO_SNDBUF(%d), EXIT \n", __FILE__, __LINE__, connfd), close(connfd), exit(1);
			}
			if (0 > setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, (const void *)&v, sizeof(v)))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) SETSOCKOPT() fail, SO_RCVBUF(%d), EXIT \n", __FILE__, __LINE__, connfd), close(connfd), exit(1);
			}

			if (0 == (a = (struct pool_t *)malloc(sizeof(struct pool_t))))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) MALLOC() fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			}
			if (0 == memset(a, 0, sizeof(struct pool_t)))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) MEMSET() fail, EXIT \n", __FILE__, __LINE__), close(connfd), exit(1);
			}
			a->sd = connfd;
			strcpy(a->name, "A");
			memcpy(&a->cliaddr, &cliaddr, clilen);

			printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
			printf("CLIENT(%s) CONNECTED ON PORT(%d) \n", (char *)inet_ntoa(a->cliaddr.sin_addr), ntohs(servaddr.sin_port));

			list_add_tail(&a->list, &pl.list);
		}


NEXT_ISSET:
		if (FD_ISSET(ttyfd, &rset))
		{
 			if (0 >= (len = read(ttyfd, &buff[0], RW_LEN)))
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

				if (0 == len)	printf("(%s %d) ttyfd=%d READ() EOF, QUIT \n", __FILE__, __LINE__, ttyfd);
				else
					printf("(%s %d) ttyfd=%d READ() fail, errno=%d, QUIT \n", __FILE__, __LINE__, ttyfd, errno);
				goto QUIT;
			}

			list_for_each_safe(pos, q, &pl.list)
			{
				if (0 == (a = list_entry(pos, struct pool_t, list)))			printf("(%s %d) LIST_ENTRY() return 0, EXIT \n", __FILE__, __LINE__), exit(1);
				sd = a->sd;
				if (0 > (l = sendto(sd, &buff[0], len, MSG_NOSIGNAL, NULL, 0)))
				{
					printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

					if (EPIPE == errno)
					{
						printf("(%s %d) socket=%d SENDTO() EPIPE, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
						list_del(pos), free(a);
						continue;
					}

					if (EAGAIN == errno)
					{
						printf("(%s %d) socket=%d SENDTO() EAGAIN, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
						list_del(pos), free(a);
						continue;
					}

					if (ECONNRESET == errno)
					{
						printf("(%s %d) socket=%d SENDTO() ECONNRESET, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
						list_del(pos), free(a);
						continue;
					}

					if (EBADF == errno)
					{
						printf("(%s %d) socket=%d SENDTO() EBADF, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
						list_del(pos), free(a);
						continue;
					}

					if (EINTR == errno)
					{
						printf("(%s %d) socket=%d SENDTO() fail, EINTR errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
						goto QUIT;
					}

					printf("(%s %d) socket=%d SENDTO() MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
					goto QUIT;
				}

				if (0 == l)
				{
					printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
					printf("(%s %d) socket=%d SENDTO() ==0, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
					list_del(pos), free(a);
					continue;
				}
			}
		}


		list_for_each_safe(pos, q, &pl.list)
		{
			if (0 == (a = list_entry(pos, struct pool_t, list)))
				printf("(%s %d) list_entry return 0, EXIT \n", __FILE__, __LINE__), exit(1);
			sd = a->sd;
			if (!FD_ISSET(sd, &rset))
				continue;
			len = read(sd, &buff[RW_LEN], RW_LEN);
			if (0 > len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);

				if (EWOULDBLOCK == errno)
				{
					if (RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT)
					{
						printf("(%s %d) socket=%d READ() EWOULDBLOCK errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
						list_del(pos), free(a);
						continue;
					}

					printf("(%s %d) socket=%d READ() EWOULDBLOCK, errno=%d, CONTINUE \n", __FILE__, __LINE__, sd, errno);
					continue;
				}
				if (EAGAIN == errno)
				{
					if (RESOURCE_TEMPORARILY_UNAVAILABLE_THEN_QUIT)
					{
						printf("(%s %d) socket=%d READ() EAGAIN errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
						list_del(pos), free(a);
						continue;
					}

					printf("(%s %d) socket=%d READ() EAGAIN, errno=%d, CONTINUE \n", __FILE__, __LINE__, sd, errno);
					continue;
				}

				if (EBADF == errno)
				{
					printf("(%s %d) socket=%d READ() EBADF, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
					list_del(pos), free(a);
					continue;
				}
				if (ECONNRESET == errno)
				{
					printf("(%s %d) socket=%d READ() ECONNRESET, errno=%d, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, errno, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
					list_del(pos), free(a);
					continue;
				}

				if (EINTR == errno)
				{
					printf("(%s %d) socket=%d READ() fail, EINTR errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
					goto QUIT;
				}

				printf("(%s %d) socket=%d READ() MISC fail, errno=%d, QUIT \n", __FILE__, __LINE__, sd, errno);
				goto QUIT;
			}
			if (0 == len)
			{
				printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
				printf("(%s %d) socket=%d READ() EOF, CLOSE(%s) & CONTINUE \n", __FILE__, __LINE__, sd, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(sd);
				list_del(pos), free(a);
			}

			if (len > 0)
			{
				if (0 > write(ttyfd, &buff[RW_LEN], len))
				{
					if (EAGAIN == errno)
					{
						printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
						printf("(%s %d) ttyfd=%d WRITE() EAGAIN, errno=%d, QUIT \n", __FILE__, __LINE__, ttyfd, errno);
						goto QUIT;
					}

					printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
					printf("(%s %d) ttyfd=%d WRITE() fail, errno=%d, QUIT \n", __FILE__, __LINE__, ttyfd, errno);
					goto QUIT;
				}
			}
		}
	}


QUIT:
	GetLocalTime(&st);
	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) PROGRAM IS GOING DOWN !!\n", __FILE__, __LINE__);

	list_for_each_safe(pos, q, &pl.list)
	{
		if (0 == (a = list_entry(pos, struct pool_t, list))) {}
		printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
		printf("(%s %d) socket=%d CLOSE(%s) \n", __FILE__, __LINE__, a->sd, (char *)inet_ntoa(a->cliaddr.sin_addr)), close(a->sd);
		list_del(pos), free(a);
	}
	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) lsd=%d CLOSE LISTEN PORT(%d) \n", __FILE__, __LINE__, lsd, ntohs(servaddr.sin_port)), close(lsd);
	printf("[%04d-%02d-%02d %02d:%02d:%02d] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
	printf("(%s %d) ttyfd=%d CLOSE(%s) \n", __FILE__, __LINE__, ttyfd, argv[1]), close(ttyfd);

	sleep(1);
	return 0;
}

