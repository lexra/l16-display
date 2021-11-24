
#if !defined(AFX_MISC_H__A3151524_6E3A_44C9_BA50_39E1A15FA74F__INCLUDED_)
#define AFX_MISC_H__A3151524_6E3A_44C9_BA50_39E1A15FA74F__INCLUDED_

#include <time.h>
#include <pthread.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

#include "list.h"
#if 0
#pragma once
#endif


typedef struct msg_t
{
	unsigned char discriminator[4];
	unsigned int type;
	unsigned int len;
	time_t stamp;
	unsigned int param[8];
} msg_t;

typedef void *HWND;
typedef void (*TIMER_FUNC)(unsigned int nId);
typedef void (*MSG_FUNC)(msg_t *pmsg);
typedef void (*RX_FUNC)(unsigned int wparam, unsigned int lparam);
typedef void (*RCV_FUNC)(unsigned int wparam, unsigned int lparam);
typedef void (*SND_FUNC)(unsigned int wparam, unsigned int lparam);

typedef struct rcv_t
{
	pthread_mutex_t *pmtx;
	struct list_head list;
	unsigned int type;
	int fd;
	char name[32];

	int txReay;
	unsigned char rxPkt[512];
} rcv_t;

typedef struct tty_t
{
	pthread_mutex_t *pmtx;
	struct list_head list;
	unsigned int type;
	int fd;
	char name[32];

	int txReay;
	unsigned char rxPkt[512];

	struct termios term;
	unsigned int port;
} tty_t;

#define MSG_START			0X0001
#define MSG_TIMER			0X0002

typedef struct Timer_t
{
	unsigned int expired;
	TIMER_FUNC func;
	unsigned int enable;
} Timer_t;


#if defined(__cplusplus) || defined(__CPLUSPLUS__)
extern "C" {
#endif


struct rcv_t *GetMuxIoList(void);
int InitListenSocket(const struct sockaddr *pservaddr, RCV_FUNC rcvFunc, SND_FUNC sndFunc);

int InitMsgFifo(MSG_FUNC callback);
int SendMsg(struct msg_t *pmsg);

int InitTimer(TIMER_FUNC callback);
void PollTimer(void);
int SetTimer(HWND hWnd, unsigned int nId, unsigned int elapse, TIMER_FUNC func);
int KillTimer(HWND hWnd, int nId);

struct tty_t *NewTtyUsb(const char *path);
int TtyUsbConnect(struct tty_t *tty, RX_FUNC func);
int TtyUsbDisconnect(struct tty_t *tty);


#if defined(__cplusplus) || defined(__CPLUSPLUS__)
}
#endif


#endif // !defined(AFX_MISC_H__A3151524_6E3A_44C9_BA50_39E1A15FA74F__INCLUDED_)



