
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/poll.h>

#if defined(MOXA)
#include <moxadevice.h>
#endif

#include <termios.h>
#include <regex.h>
#include "misc.h"
#include "cmn.h"

#if 0
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#ifdef WIN32
#define new DEBUG_NEW
#endif
#endif

#define BAUDRATE						B38400

#define FILE_MODE					(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
#define MISC_FIFO					"/tmp/misc.fifo"



//////////////////////////////////////////////////////////////////////
// macro
//////////////////////////////////////////////////////////////////////

#define BUILD_UINT16(loByte, hiByte) \
          ((unsigned short)(((loByte) & 0x00FF) + (((hiByte) & 0x00FF) << 8)))

#define BUILD_UINT32(Byte0, Byte1, Byte2, Byte3) \
          ((unsigned int)((unsigned int)((Byte0) & 0x00FF) + ((unsigned int)((Byte1) & 0x00FF) << 8) \
			+ ((unsigned int)((Byte2) & 0x00FF) << 16) + ((unsigned int)((Byte3) & 0x00FF) << 24)))

#define HI_UINT16(a) (((a) >> 8) & 0xFF)
#define LO_UINT16(a) ((a) & 0xFF)

#define BUILD_UINT8(hiByte, loByte) \
          ((unsigned char)(((loByte) & 0x0F) + (((hiByte) & 0x0F) << 4)))

#define HI_UINT8(a) (((a) >> 4) & 0x0F)
#define LO_UINT8(a) ((a) & 0x0F)

#define BREAK_UINT32(var, ByteNum) \
          (unsigned char)((unsigned int)(((var) >>((ByteNum) * 8)) & 0x00FF))



//////////////////////////////////////////////////////////////////////
// extern
//////////////////////////////////////////////////////////////////////

extern int strncmp(const char *s1, const char *s2, size_t n);
extern void *memset(void *s, int c, size_t n);
extern char *strcpy(char *dest, const char *src);
extern char *strncpy(char *dest, const char *src, size_t n);
extern size_t strlen(const char *s);
extern int strcmp(const char *s1, const char *s2);



//////////////////////////////////////////////////////////////////////
// static variable
//////////////////////////////////////////////////////////////////////

static SND_FUNC fnOnSnd = 0;
static RCV_FUNC fnOnRcv = 0;
static MSG_FUNC fnOnMsg = 0;
static TIMER_FUNC fnOnTimer = 0;
static RX_FUNC fnOnRx = 0;
static pthread_t thrdTimer;
static pthread_t thrdMuxIo;
static struct rcv_t muxList;
static int fdFifo = -1;
static struct Timer_t to[64];
static pthread_mutex_t mtxMuxList = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t mtxSndMsg = PTHREAD_MUTEX_INITIALIZER;



//////////////////////////////////////////////////////////////////////
// static
//////////////////////////////////////////////////////////////////////

static void CleanupRegx(void *param)
{
	regfree((regex_t *)param);
}

static void CleanupLock(void *param)
{
	pthread_mutex_unlock((pthread_mutex_t *)param);
}

static int DefaultTerm(struct termios *pterm)
{
	if (0 == pterm)
		return 0;

	memset(pterm, 0, sizeof(struct termios));
	pterm->c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;// | HUPCL;
	pterm->c_oflag = 0;
	pterm->c_iflag = 0;
	pterm->c_lflag = 0;
	pterm->c_cc[VMIN] = 1;
	pterm->c_cc[VTIME] = 0;
	cfsetispeed(pterm, B9600);
	cfsetospeed(pterm, B9600);

	return 1;
}

static void OnSnd(unsigned int wparam, unsigned int lparam)
{
	printf("(%s %d) OnSnd(), wparam=%d lparam=%d\n", __FILE__, __LINE__, wparam, lparam);
}

static void OnRcv(unsigned int wparam, unsigned int lparam)
{
	printf("(%s %d) OnRcv(), wparam=%d lparam=%d\n", __FILE__, __LINE__, wparam, lparam);
}

static void OnRx(unsigned int wparam, unsigned int lparam)
{
	printf("(%s %d) OnRx(), wparam=%d lparam=%d\n", __FILE__, __LINE__, wparam, lparam);
}

static void OnTimer(unsigned int nId)
{
	printf("(%s %d) OnTimer(), nId=%d \n", __FILE__, __LINE__, nId);
	SetTimer(0, nId, 3000, 0);
}

static void OnMsg(msg_t *pmsg)
{
	if (0 == pmsg)
		return;
	if (pmsg->type == MSG_TIMER)
		return OnTimer(pmsg->param[0]);
	if (fnOnMsg)
		(fnOnMsg)(pmsg);
}

static void *MuxIoThreadFunc(void *param)
{
	struct rcv_t *pList;
	struct list_head *pos, *q;
	struct rcv_t *rcv;

	msg_t msg;
	int fifo = 0;
	int res;
	struct timeval tv;

	int len;
	int i, j;
	int last_state, last_type;
	unsigned char buff[1024], *ptr = buff;

	if (0 == param)
		printf("(%s %d) 0 == param !!\n", __FILE__, __LINE__), 	exit(1);
	pList = (struct rcv_t *)param;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &last_state);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &last_type);

	len = sizeof(msg_t);
	for (;;)
	{
		int maxfd;
		fd_set rset;
		fd_set wset;
		int f;

		pthread_testcancel();

		FD_ZERO(&rset);
		FD_ZERO(&wset);

		maxfd = 0;
		i = 0, j = 0;

		pthread_mutex_lock(pList->pmtx);
		pthread_cleanup_push(CleanupLock, (void *)pList->pmtx);

		f = 0;
		list_for_each(pos, &pList->list)
		{
			rcv = list_entry(pos, struct rcv_t, list);
			if (-1 == rcv->fd)
			{
				printf("(%s %d) error !\n", __FILE__, __LINE__);
				exit(1);
			}
			if (0 == strncmp("CLI_SOCKET", rcv->name, 10) && rcv->txReay)
			{

#if !defined(NEXUS)
				f = 1;
#endif

				break;
			}
		}
		list_for_each(pos, &pList->list)
		{
			rcv = list_entry(pos, struct rcv_t, list);
			if (maxfd < rcv->fd)
				maxfd = rcv->fd;
			if (0 == strcmp(MISC_FIFO, rcv->name))
			{
				fifo = rcv->fd;
				FD_SET(rcv->fd, &rset);
			}
#if 0//defined(NEXUS)
			else if (0 == strncmp("/dev/ttyS", rcv->name, 9))
#else
			else if (0 == strncmp("/dev/ttyUSB", rcv->name, 11))
#endif
			{
				FD_SET(rcv->fd, &rset);
			}
			else if (0 == strncmp("CLI_SOCKET", rcv->name, 10))
			{
				if (1 == f)
					FD_SET(rcv->fd, &wset);
				else
					FD_SET(rcv->fd, &rset);
			}
			i++;
		}

		if (0 == i)
		{
			list_for_each_safe(pos, q, &pList->list)
			{
				rcv = list_entry(pos, struct rcv_t, list);
				if (0 == strncmp("CLI_SOCKET", rcv->name, 10))
				{
					close(rcv->fd);
					list_del(pos);
					free(rcv);
				}
			}
			printf("(%s %d) error !\n", __FILE__, __LINE__);
			exit(1);
		}

		pthread_cleanup_pop(1);

		memset(&tv, 0, sizeof(tv));
		tv.tv_usec = 1;			// 10 microseconds
		res = select(maxfd + 1, &rset, &wset, 0, &tv);
		if (-1 == res)
		{
			printf("(%s %d) select fail !!\n", __FILE__, __LINE__);
			exit(1);
		}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

#if defined(MOXA) || defined(NEXUS)
		if (0 == res)
		{
			pthread_mutex_lock(pList->pmtx);
			pthread_cleanup_push(CleanupLock, (void *)pList->pmtx);

			list_for_each_safe(pos, q, &pList->list)
			{
				unsigned char c;
				struct tty_t *ptty;

				rcv = list_entry(pos, struct rcv_t, list);

#if 0//defined(NEXUS)
				if (0 != strncmp("/dev/ttyS", rcv->name, 9))
#else
				if (0 != strncmp("/dev/ttyUSB", rcv->name, 11))
#endif
					continue;
				res = read(rcv->fd, &c, sizeof(unsigned char));
				if (0 > res && EAGAIN != errno)
				{
					printf("(%s %d) read return -1, errno=%d !!\n", __FILE__, __LINE__, errno);
					exit(1);
				}
				if (res > 0)
				{
					ptty = (struct tty_t *)rcv;
					if (fnOnRx)
						(fnOnRx)(ptty->port, (unsigned int)c);
					else
						OnRx(ptty->port, (unsigned int)c);
				}
			}

			pthread_cleanup_pop(1);

			continue;
		}
#else
		if (0 == res)
			continue;
#endif


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

		if (FD_ISSET(fifo, &rset))
		{
			if(ptr - buff == 0)
			{
				res = read(fifo, ptr, 1);
				if(0 == res)
				{
					printf("(%s %d) read return 0 !!\n", __FILE__, __LINE__);
					continue;
				}
				if (res < 0)
				{
					if (EWOULDBLOCK != errno)
					{
						printf("(%s %d) read fail !!\n", __FILE__, __LINE__);
						exit(1);
					}
					continue;
				}
				if(*ptr == 'E')
					ptr++;
				else
					memset((ptr = buff), 0, sizeof(buff));
			}
			else if(ptr - buff == 1)
			{
				res = read(fifo, ptr, 1);
				if(res == 0)
				{
					printf("(%s %d) read return 0 !!\n", __FILE__, __LINE__);
					continue;
				}
				if (res < 0)
				{
					if (EWOULDBLOCK != errno)
					{
						printf("(%s %d) read fail !!\n", __FILE__, __LINE__);
						exit(1);
					}
					continue;
				}
				if(*ptr == 'C')
					ptr++;
				else
					memset((ptr = buff), 0, sizeof(buff));
			}
			else if(ptr - buff == 2)
			{
				res = read(fifo, ptr, 1);
				if(res == 0)
				{
					printf("(%s %d) read return 0 !!\n", __FILE__, __LINE__);
					continue;
				}
				if (res < 0)
				{
					if (EWOULDBLOCK != errno)
					{
						printf("(%s %d) read fail !!\n", __FILE__, __LINE__);
						exit(1);
					}
					continue;
				}
				if(*ptr == 'O')
					ptr++;
				else
					memset((ptr = buff), 0, sizeof(buff));
			}
			else if(ptr - buff == 3)
			{
				res = read(fifo, ptr, 1);
				if(res == 0)
				{
					printf("(%s %d) read return 0 !!\n", __FILE__, __LINE__);
					continue;
				}
				if (res < 0)
				{
					if (EWOULDBLOCK != errno)
					{
						printf("(%s %d) read fail !!\n", __FILE__, __LINE__);
						exit(1);
					}
					continue;
				}
				if(*ptr == 'M')
					ptr++;
				else
					memset((ptr = buff), 0, sizeof(buff));
			}
			else
			{
				unsigned char *pc;

				len = sizeof(struct msg_t) - 4;
				while(len > 0)
				{
					res = read(fifo, ptr, len);
					if (res < 0)
					{
						if (EWOULDBLOCK != errno)
						{
							printf("(%s %d) read fail !!\n", __FILE__, __LINE__);
							exit(1);
						}
						continue;
					}
					ptr += res;
					len -= res;
				}

				pc = buff;
				i = 4;
				if (IsLittleEndian())
				{
					msg.type = BUILD_UINT32(pc[i + 0], pc[i + 1], pc[i + 2], pc[i + 3]); i += 4;
					msg.len = BUILD_UINT32(pc[i + 0], pc[i + 1], pc[i + 2], pc[i + 3]); i += 4;
					msg.stamp = BUILD_UINT32(pc[i + 0], pc[i + 1], pc[i + 2], pc[i + 3]); i += 4;
					for (j = 0; j < 8; j++)
					{
						msg.param[j] = BUILD_UINT32(pc[i + 0], pc[i + 1], pc[i + 2], pc[i + 3]); i += 4;
					}
				}
				else
				{
					msg.type = BUILD_UINT32(pc[i + 3], pc[i + 2], pc[i + 1], pc[i + 0]); i += 4;
					msg.len = BUILD_UINT32(pc[i + 3], pc[i + 2], pc[i + 1], pc[i + 0]); i += 4;
					msg.stamp = BUILD_UINT32(pc[i + 3], pc[i + 2], pc[i + 1], pc[i + 0]); i += 4;
					for (j = 0; j < 8; j++)
					{
						msg.param[j] = BUILD_UINT32(pc[i + 3], pc[i + 2], pc[i + 1], pc[i + 0]); i += 4;
					}
				}
				memset((ptr = buff), 0, sizeof(buff));

				OnMsg(&msg);
			}
		}


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

		pthread_mutex_lock(pList->pmtx);
		pthread_cleanup_push(CleanupLock, (void *)pList->pmtx);

		list_for_each_safe(pos, q, &pList->list)
		{
			rcv = list_entry(pos, struct rcv_t, list);

#if 0//defined(NEXUS)
			if (0 != strncmp("/dev/ttyS", rcv->name, 9))
#else
			if (0 != strncmp("/dev/ttyUSB", rcv->name, 11))
#endif
				continue;
			if (FD_ISSET(rcv->fd, &rset))
			{
				unsigned char c;
				struct tty_t *ptty;

				res = read(rcv->fd, &c, sizeof(unsigned char));
				if (0 > res && EAGAIN != errno)
				{
					printf("(%s %d) read return -1, errno=%d !!\n", __FILE__, __LINE__, errno);
					exit(1);
				}
				if (res > 0)
				{
					ptty = (struct tty_t *)rcv;
					if (fnOnRx)
						(fnOnRx)(ptty->port, (unsigned int)c);
					else
						OnRx(ptty->port, (unsigned int)c);
				}
			}
		}

		pthread_cleanup_pop(1);


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

		pthread_mutex_lock(pList->pmtx);
		pthread_cleanup_push(CleanupLock, (void *)pList->pmtx);

		list_for_each_safe(pos, q, &pList->list)
		{
			rcv = list_entry(pos, struct rcv_t, list);
			if (0 != strncmp("CLI_SOCKET", rcv->name, 10))
				continue;
			if (FD_ISSET(rcv->fd, &rset))
			{
				if (fnOnRcv)
					(fnOnRcv)((unsigned int)rcv, (unsigned int)pos);
				else
					OnRcv((unsigned int)rcv, (unsigned int)pos);
			}
		}

		pthread_cleanup_pop(1);


/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

		pthread_mutex_lock(pList->pmtx);
		pthread_cleanup_push(CleanupLock, (void *)pList->pmtx);

		list_for_each_safe(pos, q, &pList->list)
		{
			rcv = list_entry(pos, struct rcv_t, list);
			if (0 != strncmp("CLI_SOCKET", rcv->name, 10))
				continue;
			if (FD_ISSET(rcv->fd, &wset) && rcv->txReay)
			{
				rcv->txReay = 0;
				if (fnOnSnd)
					(fnOnSnd)((unsigned int)rcv, (unsigned int)pos);
				else
					OnSnd((unsigned int)rcv, (unsigned int)pos);
			}
		}

		pthread_cleanup_pop(1);
	}

	return 0;
}

static void *TimerThreadFunc(void *param)
{
	int last_state, last_type;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &last_state);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &last_type);
	for (;;)
	{
		pthread_testcancel();	// `pthread_testcancel` useless if the thread's cancel type is `ASYNC`
		PollTimer();
		usleep(1);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////
// routines
//////////////////////////////////////////////////////////////////////

int InitListenSocket(const struct sockaddr *pservaddr, RCV_FUNC rcvFunc, SND_FUNC sndFunc)
{
	int listenfd;
	socklen_t addrlen = 0;
	int v;

	if (0 == pservaddr)
		return -1;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == listenfd)
	{
		printf("(%s %d) socket() fail !\n", __FILE__, __LINE__);
		return -1;
	}

	v = 1;
	if (-1 == setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&v, sizeof(v)))
	{
		printf("(%s %d) setsockopt() fail !\n", __FILE__, __LINE__);
		return -1;
	}

	addrlen = sizeof(struct sockaddr_in);
	if (-1 == bind(listenfd, pservaddr, addrlen))
	{
		printf("(%s %d) bind() fail !\n", __FILE__, __LINE__);
		return -1;
	}
	if (-1 == listen(listenfd, 9))
	{
		printf("(%s %d) listen() fail !\n", __FILE__, __LINE__);
		return -1;
	}
	fnOnRcv = rcvFunc;
	fnOnSnd = sndFunc;

	return listenfd;
}

struct rcv_t *GetMuxIoList(void)
{
	return &muxList;
}

int InitTimer(TIMER_FUNC callback)
{
	int res;
	TIMER_FUNC old;

	old = fnOnTimer;
	fnOnTimer = callback;

	res = pthread_create(&thrdTimer, NULL, TimerThreadFunc, 0);
	if (0 != res)
	{
		fnOnTimer = old;

		printf("(%s %d) pthread_create fail !!\n", __FILE__, __LINE__);
		return 0;
	}

	return (int)fnOnTimer;
}

int InitMsgFifo(MSG_FUNC callback)
{
	int res;
	struct stat lbuf;
	struct rcv_t *rcv;
	pthread_mutex_t *pmtx = &mtxMuxList;

	res = mkfifo(MISC_FIFO, FILE_MODE);
	if (res == -1 && errno != EEXIST)
	{
		printf("(%s %d) mkfifo return -1 !\n", __FILE__, __LINE__);
		return 0;
	}

	if(stat(MISC_FIFO, &lbuf) < 0)
	{
		printf("(%s %d) lstat return -1 !\n", __FILE__, __LINE__);
		return 0;
	}
	if(!S_ISFIFO(lbuf.st_mode))
	{
		printf("(%s %d) not fifo !\n", __FILE__, __LINE__);
		return 0;
	}

	fdFifo = open(MISC_FIFO, O_RDWR | O_NONBLOCK);
	if (-1 == fdFifo)
	{
		printf("(%s %d) open fail !\n", __FILE__, __LINE__);
		return 0;
	}

	INIT_LIST_HEAD(&muxList.list);
	muxList.pmtx = pmtx;
	muxList.fd = fdFifo;

	rcv = malloc(sizeof(struct rcv_t));
	memset(rcv, 0, sizeof(struct rcv_t));
	rcv->fd = fdFifo;
	strcpy(rcv->name, MISC_FIFO);

	pthread_mutex_lock(pmtx);
	pthread_cleanup_push(CleanupLock, (void *)pmtx);

	list_add_tail(&rcv->list, &muxList.list);

	pthread_cleanup_pop(1);

	res = pthread_create(&thrdMuxIo, NULL, MuxIoThreadFunc, &muxList);
	if (0 != res)
	{
		printf("(%s %d) pthread_create fail !!\n", __FILE__, __LINE__);
		exit(1);
	}

	fnOnMsg = callback;
	return 1;
}


// `SendMsg()` now must be called out side the `MuxThread`

int SendMsg(struct msg_t *pmsg)
{
	int res;
	time_t now = 0;
	int len;
	unsigned char *pc;

	if (0 == pmsg)
	{
		printf("0 == pmsg\n");
		return 0;
	}
	if(-1 == fdFifo)
	{
		printf("SendMseMsg, `-1 == fdFifo` \n");
		return 0;
	}

	pmsg->discriminator[0] = 'E'; pmsg->discriminator[1] = 'C';
	pmsg->discriminator[2] = 'O'; pmsg->discriminator[3] = 'M';
	time(&now);
	pmsg->stamp = now;

	pthread_mutex_lock(&mtxSndMsg);
	pthread_cleanup_push(CleanupLock, &mtxSndMsg);

	pc = (unsigned char *)pmsg;
	len = sizeof(struct msg_t);
	while (len > 0)
	{
		res = write(fdFifo, pc, len);
		if (-1 == res)
		{
			if (EWOULDBLOCK != errno)
			{
				printf("(%s %d) write error, errno=%d !!\n", __FILE__, __LINE__, errno);
				exit(1);
			}
			sched_yield();
			continue;
		}
		pc += res;
		len -= res;
	}

	pthread_cleanup_pop(1);

	return (-1 == res && EWOULDBLOCK != errno) ? 0 : 1;
}

void PollTimer(void)
{
	int i;
	int cnt = 0;
	struct timeb tb;
	unsigned int now;
	struct msg_t msg;

	cnt = sizeof(to) / sizeof(struct Timer_t);
	for (i = 0; i < cnt; i++)
	{
		if(0 == to[i].enable)
			continue;
		if(0 != to[i].expired)
		{
			ftime(&tb);
			now = tb.time * 1000 + tb.millitm;
			if(now >= to[i].expired)
			{
				to[i].expired = 0;
				if(to[i].func)
				{
					to[i].func(i);
					continue;
				}
				if (fnOnTimer)
				{
					(fnOnTimer)(i);
					continue;
				}
				memset(&msg, 0, sizeof(struct msg_t));
				msg.type = MSG_TIMER;
				msg.param[0] = i;
				SendMsg(&msg);
			}
		}
	}
}

int SetTimer(HWND hWnd, unsigned int nId, unsigned int elapse, TIMER_FUNC func)
{
	struct timeb tb;
	unsigned int et;
	int cnt;

	cnt = sizeof(to) / sizeof(struct Timer_t);
	if(nId >= cnt)
		return -1;

	ftime(&tb);
	et = tb.time * 1000 + tb.millitm + elapse;
	to[nId].func = func;
	to[nId].enable = 1;
	to[nId].expired = et;

	return nId;
}

int KillTimer(HWND hWnd, int nId)
{
	int cnt;

	cnt = sizeof(to) / sizeof(struct Timer_t);
	if(nId >= cnt)
		return -1;

	to[nId].func = 0;
	to[nId].enable = 0;
	to[nId].expired = 0;

	return nId;
}

struct tty_t *NewTtyUsb(const char *path)
{
	struct tty_t *tty;
	struct stat lbuf;

	int i;
	int res;
	regex_t regx;
	int nmatch = 10;
	regmatch_t pmatch[10];
	char tmp[32 * 32];

	if (0 == path)
	{
		printf("(%s %d) `0 == path` !\n", __FILE__, __LINE__);
		return 0;
	}
	if (-1 == fdFifo)
	{
		printf("(%s %d) `-1 == fdFifo` !\n", __FILE__, __LINE__);
		return 0;
	}
	res = stat(path, &lbuf); 
	if(0> res)
	{
		printf("(%s %d) lstat error !\n", __FILE__, __LINE__);
		return 0;
	}
	if(!S_ISCHR(lbuf.st_mode))
	{
		printf("(%s %d) not char dev !\n", __FILE__, __LINE__);
		return 0;
	}

#if 0//defined(NEXUS)
	res = regcomp(&regx, "^/dev/ttyS([0-9]{1})$", REG_EXTENDED);
#else
	res = regcomp(&regx, "^/dev/ttyUSB([0-9]{1})$", REG_EXTENDED);
#endif
	if(0 != res)
	{
		printf("(%s %d) regcomp error !\n", __FILE__, __LINE__);
		return 0;
	}
	pthread_cleanup_push(CleanupRegx, &regx);

	res = regexec(&regx, path, nmatch, pmatch, 0);
	if (0 == res)
	{
		memset(tmp, 0, sizeof(tmp));
		for (i = 0; i < nmatch && pmatch[i].rm_so >= 0; i++)
		{
			size_t len = pmatch[i].rm_eo - pmatch[i].rm_so;

			strncpy(tmp + 32 * i, path + pmatch[i].rm_so, len);
			*(tmp + 32 * i + len) = 0;
		}
	}
	pthread_cleanup_pop(1);

	if (0 != res)
	{
		printf("(%s %d) regexec error ! path='%s'\n", __FILE__, __LINE__, path);
		return 0;
	}


	tty = (struct tty_t *)malloc(sizeof(struct tty_t));
	if(0 == tty)
	{
		printf("(%s %d) malloc error !\n", __FILE__, __LINE__);
		return 0;
	}
	memset(tty, 0, sizeof(struct tty_t));
	tty->fd = -1;
	strcpy(tty->name, path);
	tty->port = atoi(&tmp[32]);
	tty->type = 0X0101;
	DefaultTerm(&tty->term);

	return tty;
}

int TtyUsbConnect(struct tty_t *tty, RX_FUNC func)
{
	int fd;
	struct stat lbuf;
	struct termios oldterm;
	int val;

	pthread_mutex_t *pmtx = &mtxMuxList;

	if (0 == tty)
	{
		printf("(%s %d) `0 == tty` !\n", __FILE__, __LINE__);
		return 0;
	}
	if (-1 == fdFifo)
	{
		printf("(%s %d) `-1 == fdFifo` !\n", __FILE__, __LINE__);
		return 0;
	}
	if(stat(tty->name, &lbuf) < 0)
	{
		printf("(%s %d) lstat error !\n", __FILE__, __LINE__);
		return 0;
	}
	if(!S_ISCHR(lbuf.st_mode))
	{
		printf("(%s %d) not S_ISCHR !\n", __FILE__, __LINE__);
		return 0;
	}
	if (-1 != tty->fd)
	{
		printf("(%s %d) `-1 != tty->fd` !\n", __FILE__, __LINE__);
		return tty->fd;
	}
	if (12 < strlen(tty->name))
	{
		printf("(%s %d) strlen > 12, name=`%s` !\n", __FILE__, __LINE__, tty->name);
		return 0;
	}

	fd = open(tty->name, O_RDWR | O_NOCTTY | O_NDELAY);
	if (-1 == fd)
	{
		printf("(%s %d) open fail !\n", __FILE__, __LINE__);
		return 0;
	}

#if defined(MOXA)
	val = 0;
	if (ioctl(fd, MOXA_SET_OP_MODE, &val) < 0)
	{
		printf("(%s %d) MOXA_SET_OP_MODE fail !\n", __FILE__, __LINE__);
		return 0;
	}
	val = fcntl(fd, F_GETFL, 0);
	if (val < 0)
	{
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__);
		return 0;
	}
	if (fcntl(fd, F_SETFL, val | FNDELAY) < 0)
	{
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__);
		return 0;
	}
#else
	val = fcntl(fd, F_GETFL, 0);
	if (val < 0)
	{
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__);
		return 0;
	}
	if (fcntl(fd, F_SETFL, val | O_NONBLOCK) < 0)
	{
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__);
		return 0;
	}
#endif

	if (0 == isatty(fd))
	{
		printf("(%s %d) not isatty !\n", __FILE__, __LINE__);

		close(fd);
		return 0;
	}
	if (tcgetattr(fd, &oldterm) < 0)
	{
		printf("(%s %d) tcgetattr error!\n", __FILE__, __LINE__);
		close(fd);
		return 0;
	}
	if (tcflush(fd, TCIFLUSH) < 0)
	{
		printf("(%s %d) tcflush fail !\n", __FILE__, __LINE__);

		tcsetattr(fd, TCSANOW, &oldterm);

		printf("(%s %d) tcflush error !\n", __FILE__, __LINE__);
		close(fd);
		return 0;
	}
	if (tcsetattr(fd, TCSANOW, &tty->term) < 0)
	{
		printf("(%s %d) tcsetattr error !\n", __FILE__, __LINE__);
		close(fd);
		return 0;
	}
	tty->fd = fd;

	pthread_mutex_lock(pmtx);
	pthread_cleanup_push(CleanupLock, pmtx);
	list_add_tail(&tty->list, &muxList.list);
	pthread_cleanup_pop(1);

	fnOnRx = func;
	return tty->fd;
}

int TtyUsbDisconnect(struct tty_t *tty)
{
	struct list_head *pos, *q;
	struct rcv_t *pList;
	struct rcv_t *rcv = 0;
	char szTmp[256];

	if (9 < tty->port || -1 == tty->fd)
		return 0;

#if 0//defined(NEXUS)
	sprintf(szTmp, "/dev/ttyS%d", (int)tty->port);
#else
	sprintf(szTmp, "/dev/ttyUSB%d", (int)tty->port);
#endif
	pList = GetMuxIoList();

	pthread_mutex_lock(pList->pmtx);
	pthread_cleanup_push(CleanupLock, pList->pmtx);

	list_for_each_safe(pos, q, &pList->list)
	{
		rcv = list_entry(pos, struct rcv_t, list);
		if (0 == strcmp(szTmp, rcv->name))
			break;
	}
	if (rcv)
		list_del(pos);

	pthread_cleanup_pop(1);

	return (int)rcv;
}


