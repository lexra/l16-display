
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



//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

typedef struct tm SYSTEMTIME;

void GetLocalTime(SYSTEMTIME *st)
{
	struct tm *pst = NULL;
	time_t t = time(NULL);
	pst = localtime(&t);
	memcpy(st, pst, sizeof(SYSTEMTIME));
	st->tm_year += 1900;
}

void PrintUsage(void)
{
	char file[128];
	int i = 0;

	memset(file, 0, sizeof(file));
	strcpy(file, __FILE__);
	for (; i <128; i++)
		if ('.' == file[i])	file[i] = 0;

	printf("Usage: %s -S [TTY-SERIAL-DEV] -B [BAUD] -T [TIMEOUT]\n", file);
	printf("Example: %s -S/dev/ttyUSB1 -B9600 -T24\n", file), exit(1);
}

int main(int argc, char *argv[])
{
	char ttyname[256];
	int ttyfd = -1;
	struct termios term;
	struct stat lbuf;

	int baud = 9600;
	int timeout = 24;
	time_t first;
	time_t now;
	fd_set rset;
	struct timeval tv;
	int res;
	int l;
	int probe = 2;
	SYSTEMTIME st;
	int c;
	int v;

	char ch;
	char line[1024];
	char *p = line;



//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	setbuf(stdout, 0);
	strcpy(ttyname, "/dev/ttyUSB1");
	if (argc < 3)											PrintUsage();
	while ((c = getopt(argc, argv, "s:S:b:B:t:T:")) != -1)
	{
		switch (c)
		{
		default:
			break;
		case '?':
			PrintUsage();
			break;
		case 's':
		case 'S':
			if (strlen(optarg) == 0)							PrintUsage();
			if(0 > stat(optarg, &lbuf))						PrintUsage();
			if(!S_ISCHR(lbuf.st_mode))						PrintUsage();
			strcpy(ttyname, optarg);
			break;
		case 'b':
		case 'B':
			baud = atoi(optarg);
			break;
		case 't':
		case 'T':
			timeout = atoi(optarg);
			if (timeout <= 0)								PrintUsage();
			break;
		}
	}


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

	if (0 > (ttyfd = open(ttyname, O_RDWR | O_NOCTTY | O_NDELAY)))
		printf("(%s %d) SERIAL DEVICE( \"/dev/tty[M|S|USB][0-9]\") OPEN() fail, please run as root and retry later, EXIT \n", __FILE__, __LINE__), exit(1);
	if (!isatty(ttyfd))
		printf("(%s %d) DEVICE( \"%s\"), ttyfd=%d, not ISATTY(), EXIT \n", __FILE__, __LINE__, ttyname, ttyfd), close(ttyfd), exit(1);

	memset(&term, 0, sizeof(struct termios));

  #if defined(MAC)	//MSG_NOSIGNAL
	tcgetattr(ttyfd, &term);
  #endif // MAC

	term.c_cflag |= B9600, term.c_cflag |= CLOCAL, term.c_cflag |= CREAD, term.c_cflag &= ~PARENB, term.c_cflag &= ~CSTOPB, term.c_cflag &= ~CSIZE, term.c_cflag |= CS8, term.c_iflag = IGNPAR, term.c_cc[VMIN] = 1, term.c_cc[VTIME] = 0;
	if (baud == 230400)									cfsetispeed(&term, B230400), cfsetospeed(&term, B230400);
	else if (baud == 115200)								cfsetispeed(&term, B115200), cfsetospeed(&term, B115200);
	else if (baud == 57600)									cfsetispeed(&term, B57600), cfsetospeed(&term, B57600);
	else if (baud == 38400)									cfsetispeed(&term, B38400), cfsetospeed(&term, B38400);
	else if (baud == 19200)									cfsetispeed(&term, B19200), cfsetospeed(&term, B19200);
	else if (baud == 9600)									cfsetispeed(&term, B9600), cfsetospeed(&term, B9600);
	else if (baud == 4800)									cfsetispeed(&term, B4800), cfsetospeed(&term, B4800);
	else													cfsetispeed(&term, B9600), cfsetospeed(&term, B9600);
	tcsetattr(ttyfd, TCSANOW, &term);
	if (0 > (v = fcntl(ttyfd, F_GETFL, 0)))						printf("(%s %d) FCNTL() F_GETFL(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	if (0 > fcntl(ttyfd, F_SETFL, v | O_NONBLOCK))				printf("(%s %d) FCNTL() F_SETFL,O_NONBLOCK(%d,%s) fail, EXIT \n", __FILE__, __LINE__, ttyfd, ttyname), close(ttyfd), exit(1);
	tcflush(ttyfd, TCIFLUSH);


//////////////////////////////////////////////////////////////////////
// 
//////////////////////////////////////////////////////////////////////

// 0 = A
// 1 = V
// 2 = TIMEOUT

	time(&first);
	do
	{
		time(&now);
		if ((int)(now - first) > timeout)
		{
			if (1 == probe)								printf("\n");
			printf("(%s %d) TIMEOUT !\n", __FILE__, __LINE__), close(ttyfd), exit(probe);
		}

		FD_ZERO(&rset);
		FD_SET(ttyfd, &rset);
		memset(&tv, 0, sizeof(tv)), tv.tv_sec = 1;

		if (0 == (res = select(ttyfd + 1, &rset, 0, 0, &tv)))		continue;
		if (res < 0)										printf("(%s %d) SELECT() fail !\n", __FILE__, __LINE__), close(ttyfd), exit(11);
		l = read(ttyfd, (char *)&ch, sizeof(ch));
		if (0 >= l)										printf("(%s %d) READ() FAIL !\n", __FILE__, __LINE__), close(ttyfd), exit(12);

		if (ch == '$')
		{
			p = line;
			*p++ = ch;
			continue;
		}
		if (ch != 0X0D && ch != 0X0A && ch != '$')
		{
			*p++ = ch;
			continue;
		}
		if (ch == 0X0D)
		{
			*p++ = 0;
			if (line[0] == '$' && line[1] == 'G' && line[2] == 'P' && line[3] == 'R' && line[4] == 'M' && line[5] == 'C')
			{
				if (line[18] != 'A' && line[18] != 'V')			printf("(%s %d) FORMAT ERROR !\n", __FILE__, __LINE__), close(ttyfd), exit(13);
				if (line[18] == 'A')
				{
					float y = 0.00F;
					float x = 0.00F;
					float f = 0.00F;

					GetLocalTime(&st);
					if (1 == probe)	printf("\n");
					printf("[%04d-%02d-%02d %02d:%02d:%02d %010u] ", st.tm_year, st.tm_mon + 1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec, (unsigned int)now);
					printf("%s ", line);

					y += (float)(line[20] - 0X30) * 10.00F;
					y += (float)(line[21] - 0X30) * 1.00F;
					line[29] = 0;
					res = sscanf(&line[22], "%f", &f);
					if (-1 == res)							printf("(%s %d) SSCANF ERROR !\n", __FILE__, __LINE__), close(ttyfd), exit(14);
					f = f  / 60.00F;
					y += f;
					if (line[30] != 'N')	y = 0.00F - y;

					x += (float)(line[32] - 0X30) * 100.00F;
					x += (float)(line[33] - 0X30) * 10.00F;
					x += (float)(line[34] - 0X30) * 1.00F;
					line[42] = 0;
					res = sscanf((char *)&line[35], "%f", &f);
					if (-1 == res)							printf("(%s %d) SSCANF ERROR !\n", __FILE__, __LINE__), close(ttyfd), exit(15);
					f = f  / 60.00F;
					x += f;
					if (line[43] != 'E')	x = 0.00F - x;
					printf("Y=%f,X=%f\n", y, x);
					probe = 0, close(ttyfd),exit(0);
				}

				printf(".");
				if (2 == probe)							probe = 1;
			}
			p = line;
			continue;
		}
		if (ch == 0X0A)
		{
			p = line;
			continue;
		}
	}
	while(1);

	return 0;
}


