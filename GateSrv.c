
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
#include <inttypes.h>

#include <fcntl.h>
#include <regex.h>

#include "misc.h"



//////////////////////////////////////////////////////////////////////
// define
//////////////////////////////////////////////////////////////////////

#define SOP_STATE		0X00
#define CMD_STATE1		0X01
#define CMD_STATE2		0X02
#define LEN_STATE		0X03
#define DATA_STATE		0X04
#define FCS_STATE		0X05



//////////////////////////////////////////////////////////////////////
// static
//////////////////////////////////////////////////////////////////////

static int fdUart = -1;

static unsigned char CalcFcs(unsigned char *msg_ptr, unsigned char len)
{
	unsigned char x;
	unsigned char xorResult;

	xorResult = 0;

	for (x = 0; x < len; x++, msg_ptr++)
		xorResult = xorResult ^ (*msg_ptr);

	return xorResult;
}

static void CleanupLock(void *pParam)
{
	pthread_mutex_unlock((pthread_mutex_t *)pParam);
}

static unsigned char rxPkt[256 * 8];

static void OnSnd(unsigned int wparam, unsigned int lparam)
{
	int res;
	struct rcv_t *rcv;
	unsigned char *pkt;
	int fd;
	int len = 0;
	struct list_head *pos;

	rcv = (struct rcv_t *)wparam;
	pos = (struct list_head *)lparam;

	if (0 == wparam || 0 == lparam)
	{
		printf("(%s %d) parameter == 0\n", __FILE__, __LINE__);
		return;
	}

	pkt = rcv->rxPkt;
	fd = rcv->fd;
	len = (int)pkt[1] + 5;
	res = write(fd, pkt, len);
	if (1)
	{
		int i;

		for (i = 0; i < len; i++)
		{
			if (i == len - 1)
				printf(" %02X\n", pkt[i]);
			else if (0 == i)
				printf("Snd: %02X", pkt[i]);
			else
				printf(" %02X", pkt[i]);
		}
	}
}


static void OnRcv(unsigned int wparam, unsigned int lparam)
{
	int res;
	int fd;
	int len;
	unsigned char buff[512];

	//struct rcv_t *pList;
	//struct rcv_t *tty;

	struct rcv_t *rcv = (struct rcv_t *)wparam;
	struct list_head *pos = (struct list_head *)lparam;
	if (0 == wparam || 0 == lparam)
		return;

	fd = rcv->fd;
	memset(buff, 0, sizeof(buff));
	res = read(fd, buff, 258);								// max pkt siz = 258
	//if (0 == res || (-1 == res && EWOULDBLOCK != errno && EAGAIN != errno))
	if (0 == res || (-1 == res && EAGAIN != errno))
	{
		close(fd);
		list_del(pos);
		printf("(%s %d) Close Connection, fd=%d\n", __FILE__, __LINE__, fd);
		free(rcv);
		return;
	}

/////////////////////////////////////////////////////////////////////////////////////////
//
	if (res > 0)
	{
		int i;

		for (i = 0; i < res; i++)
		{
			if (i == res - 1)
				printf(" %02X\n", buff[i]);
			else if (0 == i)
				printf("Rcv: %02X", buff[i]);
			else
				printf(" %02X", buff[i]);
		}

		write(fdUart, buff, res);
	}
	return;


	if (res > 0)
	{
		if (253 < (len = buff[1]))
			return;

		{
			int i;

			for (i = 0; i < len + 5; i++)
			{
				if (i == len + 5 - 1)
					printf(" %02X\n", buff[i]);
				else if (0 == i)
					printf("Rcv: %02X", buff[i]);
				else
					printf(" %02X", buff[i]);
			}
		}

#if 1
		if (CalcFcs(&buff[1], 3 + len) == buff[len + 4] && 0XFE == buff[0])
		{
			res = write(fdUart, buff, buff[1] + 5);
		}
#else
		if (CalcFcs(&buff[1], 3 + len) == buff[len + 4] && 0XFE == buff[0])
		{
			pList = GetMuxIoList();
			list_for_each(pos, &pList->list)
			{
				tty = list_entry(pos, struct rcv_t, list);
				if (0 != strncmp("/dev/ttyUSB", tty->name, 11))
					continue;
				write(tty->fd, buff, buff[1] + 5);
				//printf("(%s %d) Uart write, fd=%d\n", __FILE__, __LINE__, tty->fd);
			}
		}
#endif

	}
}

static unsigned char state = SOP_STATE;
static unsigned char LEN_Token = 0;
static unsigned char FSC_Token = 0;
static unsigned char tempDataLen = 0;

static void OnRx(unsigned int wparam, unsigned int lparam)
{
	unsigned int port = 0;
	unsigned char ch = 0;

	ch = (unsigned char)(lparam & 0X000000FF);
	port = wparam;

	switch (state)
	{
	default:
		state = SOP_STATE;
		break;

	case SOP_STATE:
		if (ch == 0XFE)
		{
			rxPkt[0] = ch;
			state = LEN_STATE;
		}
		break;

	case LEN_STATE:
		if (ch > 253)
		{
			state = SOP_STATE;
			break;
		}
		LEN_Token = ch;
		tempDataLen = 0;
		rxPkt[1] = LEN_Token;
		state = CMD_STATE1;
		break;

	case CMD_STATE1:
		rxPkt[2] = ch;
		state = CMD_STATE2;
		break;

	case CMD_STATE2:
		rxPkt[3] = ch;
		if (LEN_Token > 0)
			state = DATA_STATE;
		else
			state = FCS_STATE;
		break;

	case DATA_STATE:
		rxPkt[4 + tempDataLen++] = ch;
		if (tempDataLen >= LEN_Token)
			state = FCS_STATE;
		break;

	case FCS_STATE:
		rxPkt[4 + rxPkt[1]] = ch;
		FSC_Token = ch;
		if (253 >= rxPkt[1] && CalcFcs(&rxPkt[1], 3 + LEN_Token) == FSC_Token)
		{
			struct list_head *pos;
			struct rcv_t *pList;
			pthread_mutex_t *pmtx;
			struct rcv_t *rcv;

			pList = GetMuxIoList();
			pmtx = pList->pmtx;
			list_for_each(pos, &pList->list)
			{
				rcv = list_entry(pos, struct rcv_t, list);
				if (0 == strncmp("CLI_SOCKET", rcv->name, 10))
				{

#if !defined(MOXA) && !defined(NEXUS)
					if (-1 != rcv->fd && 0 == rcv->txReay)
					{
						memcpy(rcv->rxPkt, rxPkt, rxPkt[1] + 5);
						rcv->txReay = 1;
					}
#else
					if (-1 != rcv->fd && 0 == rcv->txReay)
					{
						memcpy(rcv->rxPkt, rxPkt, rxPkt[1] + 5);
						rcv->txReay = 1;
						OnSnd((unsigned int)rcv, (unsigned int)pos);
						rcv->txReay = 0;
					}
#endif

				}
			}
		}
		state = SOP_STATE;
		break;
	}

	return;
}

static void OnTimer(unsigned int nId)
{
	printf("(%s %d) OnTimer(), nId=%d\n", __FILE__, __LINE__, nId);
	SetTimer(0, nId, 2000, 0);
}

static void OnMsg(msg_t *pmsg)
{
	printf("(%s %d) OnMsg()\n", __FILE__, __LINE__);
}


//////////////////////////////////////////////////////////////////////
// main
//////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	int res;
	struct tty_t *pttyS;

	int listenfd;
	struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;

	int val;
	struct rcv_t *pList;
	pthread_mutex_t *pmtx;

	int i, j;
	regex_t regx;
	int nmatch = 10;
	regmatch_t pmatch[10];
	int portNo;

	if (3 != argc)
	{
		printf("Usage: GateSrv [/dev/ttyUsb0] [ListenPort] !\n");
		return 1;
	}
	res = regcomp(&regx, "^([0-9]{5})$", REG_EXTENDED);
	if(0 != res)
	{
		printf("(%s %d) regcomp error !\n", __FILE__, __LINE__);
		return 1;
	}
	res = regexec(&regx, argv[2], nmatch, pmatch, 0);
	if (0 != res)
	{
		printf("Usage: GateSrv [ttyUsb] [ListenPort] !\n");
		regfree(&regx);
		return 0;
	}
	j = 0;
	for (i = 1; i < nmatch && pmatch[i].rm_so >= 0; i++)
		j++;
	regfree(&regx);
	if (0 == j)
	{
		printf("Usage: GateSrv [ttyUsb] [ListenPort] !\n");
		return 1;
	}
	portNo = atoi(argv[2]);
	if(portNo < 10000)
	{
		printf("Usage: GateSrv [ttyUsb] [ListenPort] !\n");
		printf("[ListenPort] must >= 10000 !\n");
		return 1;
	}

	res = InitMsgFifo(OnMsg);
	if (0 == res)
	{
		printf("(%s %d) InitMsgFifo() return 0 !\n", __FILE__, __LINE__);
		return 1;
	}
	res = InitTimer(OnTimer);
	if (0 == res)
	{
		printf("(%s %d) InitTimer() return 0 !\n", __FILE__, __LINE__);
		return 1;
	}

	pttyS = NewTtyUsb(argv[1]);
	if (0 == pttyS)
	{
		printf("(%s %d) NewTtyUsb() return 0 !\n", __FILE__, __LINE__);
		return 1;
	}
	cfsetispeed(&pttyS->term, B38400);
	res = TtyUsbConnect(pttyS, OnRx);
	if (0 >= res)
	{
		printf("(%s %d) TtyUsbConnect() return 0 !\n", __FILE__, __LINE__);
		return 1;
	}
	fdUart = res;

	//SetTimer(0, 1, 500, 0);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(portNo);
	listenfd = InitListenSocket((const struct sockaddr *)&servaddr, OnRcv, OnSnd);
	if (-1 == listenfd)
	{
		printf("(%s %d) InitListenSocket() fail !\n", __FILE__, __LINE__);
		return 1;
	}
	val = fcntl(listenfd, F_GETFL, 0);
	if (val < 0)
	{
		printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__);
		return 1;
	}
	if (fcntl(listenfd, F_SETFL, val | O_NONBLOCK) < 0)
	{
		printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__);
		return 1;
	}

	pList = GetMuxIoList();
	pmtx = pList->pmtx;
	for (;;)
	{
		int maxfd;
		fd_set rset;
		struct rcv_t *rcv;

		pthread_testcancel();

		maxfd = 0;
		FD_ZERO(&rset);
		FD_SET(listenfd, &rset);
		if (maxfd < listenfd)
			maxfd = listenfd;
		res = select(maxfd + 1, &rset, 0, 0, 0);
		if (-1 == res)
		{
			printf("(%s %d) select fail !! \n", __FILE__, __LINE__);
			exit(1);
		}
		if (0 == res)
			continue;

		if (FD_ISSET(listenfd, &rset))
		{
			int clilen, connfd;

			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *)&cliaddr, (socklen_t *)&clilen);
			if (connfd < 0)
			{
				if (EWOULDBLOCK == errno || ECONNABORTED == errno || EPROTO == errno || EINTR == errno)
					continue;
				printf("(%s %d) accept() fail !\n", __FILE__, __LINE__);
				close(listenfd);
				return 1;
			}
			val = fcntl(connfd, F_GETFL, 0);
			if (val < 0)
			{
				printf("(%s %d) F_GETFL fail !\n", __FILE__, __LINE__);
				return 1;
			}
			if (fcntl(connfd, F_SETFL, val | O_NONBLOCK) < 0)
			{
				printf("(%s %d) F_SETFL fail !\n", __FILE__, __LINE__);
				return 1;
			}

			rcv = malloc(sizeof(struct rcv_t));
			memset(rcv, 0, sizeof(struct rcv_t));
			rcv->fd = connfd;
			rcv->type = 0X0103;
			strcpy(rcv->name, "CLI_SOCKET");

			pthread_mutex_lock(pmtx);
			pthread_cleanup_push(CleanupLock, (void *)pmtx);

			list_add_tail(&rcv->list, &pList->list);
			printf("(%s %d) New Connection, fd=%d\n", __FILE__, __LINE__, connfd);

			pthread_cleanup_pop(1);

			continue;
		}

	}

	TtyUsbDisconnect(pttyS);
	free(pttyS);

	return 0;
}



