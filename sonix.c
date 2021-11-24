
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <errno.h>

#include "list.h"



/////////////////////////////////////////////////////////////////////////////////////////
//

typedef void *HWND;
typedef void (*TIMER_FUNC)(unsigned int nId);

typedef struct Timer_t
{
	unsigned int expired;
	TIMER_FUNC func;
	unsigned int enable;
} Timer_t;


/////////////////////////////////////////////////////////////////////////////////////////
//

extern void exit(int status);

void PollTimer(void);
int SetTimer(HWND hWnd, unsigned int nId, unsigned int elapse, TIMER_FUNC func);
int KillTimer(HWND hWnd, int nId);


/////////////////////////////////////////////////////////////////////////////////////////
//

static struct Timer_t to[64];


void PollTimer(void)
{
	int i;
	int cnt = 0;
	struct timeb tb;
	unsigned int now;

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
				}
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

static void *TimerThreadFunc(void *param)
{
	for (;;)
	{
		PollTimer();
		usleep(1);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t thrdTimer;
	int res;
	int fdDev;

	if (2 != argc)
		printf("Usage: sonix [/home/sample.jpg] \n"), exit(1);

	fdDev = open(argv[1], O_RDONLY);
	if (-1 == fdDev)
		printf("(%s %d) open fail !!\n", __FILE__, __LINE__), exit(1);

	res = pthread_create(&thrdTimer, NULL, TimerThreadFunc, 0);
	if (0 != res)
		printf("(%s %d) pthread_create fail !!\n", __FILE__, __LINE__), exit(1);




	pause();

	return 0;
}







