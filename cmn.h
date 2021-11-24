
#if !defined(_CMN_0936_H__INCLUDED_)
#define _CMN_0936_H__INCLUDED_

#include <time.h>
#include <sys/timeb.h>
#include <pthread.h>

#ifndef INFINITE
#define INFINITE				0XFFFFFFFF
#endif

#define WAIT_FAILED			0XFFFFFFFFL
#define WAIT_OBJECT_0		0X00000000L
#define WAIT_TIMEOUT			ETIMEDOUT
#define WAIT_ABANDONED		0X00000080L
	
//////////////////////////////////////////////////////////////////////
// typedef
//////////////////////////////////////////////////////////////////////

typedef void *HANDLE;

typedef struct event_t 
{
 int id;
 int type;
 char name[256];
 int manual_reset;
 pthread_cond_t event;
} event_t;

//////////////////////////////////////////////////////////////////////
// routines
//////////////////////////////////////////////////////////////////////

int IsLittleEndian(void);

HANDLE CreateEvent(
   void *pAttr, int bManualReset, int bInitialState, const char *szName);
int CloseEvent(HANDLE hObject);
int SetEvent(HANDLE hEvent);
int ResetEvent(HANDLE hEvent);
int PulseEvent(HANDLE hEvent);
unsigned int WaitForSingleObject(HANDLE hEvent, unsigned int dwMilliseconds);
unsigned int WaitForMultipleObjects(unsigned int nCount, 
   const HANDLE *lpHandles, int bWaitAll, unsigned int dwMilliseconds);
 
#endif

