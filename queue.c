
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/24

#include "main.h"
#include "queue.h"

#define STATUS_WAITING		0
#define STATUS_RUNNING		'r'
#define STATUS_COMPLETED	'c'

//+ Global
struct {
	CRITICAL_SECTION	Lock;
	PCURL_REQUEST		Head;
	ULONG				NextId;
	LONG				ThreadMax;
	volatile LONG		ThreadCount;
	volatile LONG		FlagTermAll;
} g_Queue = {0};


//+ Prototypes
ULONG QueueStartTransfer( _In_ PCURL_REQUEST pReq );
ULONG QueueStopTransfer( _In_ PCURL_REQUEST pReq );


//++ QueueInitialize
void QueueInitialize()
{
	InitializeCriticalSection( &g_Queue.Lock );
	g_Queue.Head = NULL;
	g_Queue.NextId = 0;

	{
		SYSTEM_INFO si;
		GetSystemInfo( &si );
		g_Queue.ThreadMax = si.dwNumberOfProcessors * 2;
		g_Queue.ThreadMax = __max( g_Queue.ThreadMax, 4 );
		g_Queue.ThreadMax = __min( g_Queue.ThreadMax, 64 );
	}
	g_Queue.ThreadCount = 0;
}


//++ QueueDestroy
void QueueDestroy()
{
	while (g_Queue.Head)
		QueueRemove( g_Queue.Head );
	DeleteCriticalSection( &g_Queue.Lock );
}


//++ QueueLock/Unlock
void QueueLock()   { EnterCriticalSection( &g_Queue.Lock ); }
void QueueUnlock() { LeaveCriticalSection( &g_Queue.Lock ); }


//++ QueueAdd
ULONG QueueAdd( _In_ PCURL_REQUEST pReq )
{
	ULONG e = ERROR_SUCCESS;
	if (pReq) {
		PCURL_REQUEST pTail = QueueTail();
		if (pTail) {
			pTail->pNext = pReq;
		} else {
			g_Queue.Head = pReq;
		}
		pReq->pNext = NULL;
		pReq->iStatus = STATUS_WAITING;
		pReq->iId = ++g_Queue.NextId;
		// Start transfer
		e = QueueStartTransfer( pReq );
		if (e == ERROR_IO_PENDING)
			e = ERROR_SUCCESS;
	} else {
		e = ERROR_INVALID_PARAMETER;
	}
	return e;
}


//++ QueueRemove
void QueueRemove( _In_ PCURL_REQUEST pReq )
{
	if (pReq) {
		// Stop transfer (if in progress...)
		QueueStopTransfer( pReq );
		// Remove from queue
		if (g_Queue.Head == pReq) {
			g_Queue.Head = pReq->pNext;
		} else {
			PCURL_REQUEST pPrev;
			for (pPrev = g_Queue.Head; pPrev->pNext != pReq; pPrev = pPrev->pNext);
			assert( pPrev );
			pPrev->pNext = pReq->pNext;
		}
		// Free
		CurlRequestDestroy( pReq );
		MyFree( pReq );
	}
}


//++ QueueHead
PCURL_REQUEST QueueHead()
{
	return g_Queue.Head;
}


//++ QueueTail
PCURL_REQUEST QueueTail()
{
	if (g_Queue.Head) {
		PCURL_REQUEST pTail;
		for (pTail = g_Queue.Head; pTail->pNext; pTail = pTail->pNext);
		return pTail;
	}
	return NULL;
}


//++ QueueFirstWaiting
PCURL_REQUEST QueueFirstWaiting()
{
	if (g_Queue.Head) {
		PCURL_REQUEST p;
		for (p = g_Queue.Head; p && (p->iStatus != STATUS_WAITING); p = p->pNext);
		return p;
	}
	return NULL;
}


//++ QueueThreadProc
ULONG WINAPI QueueThreadProc( _In_ LPVOID pParam )
{
	ULONG e = ERROR_SUCCESS;
	PCURL_REQUEST pReq = (PCURL_REQUEST)pParam;

	while (TRUE) {

		// Check TERM-ALL flag
		if (InterlockedCompareExchange( &g_Queue.FlagTermAll, 1, 1 ) != 0)
			break;

		// Select the next waiting request, or, the one received as parameter...
		if (!pReq) {
			QueueLock();
			pReq = QueueFirstWaiting();
			QueueUnlock();
		}

		// No more waiting requests
		if (!pReq)
			break;

		// Transfer
		CurlTransfer( pReq );

		// Cleanup
		pReq = NULL;
	}

	// Exit
	InterlockedDecrement( &g_Queue.ThreadCount );
	return e;
}


//++ QueueStartTransfer
ULONG QueueStartTransfer( _In_ PCURL_REQUEST pReq )
{
	ULONG e = ERROR_SUCCESS;
	if (!pReq)
		return ERROR_INVALID_PARAMETER;
	if (pReq->iStatus == STATUS_WAITING) {
		if (g_Queue.ThreadCount < g_Queue.ThreadMax) {
			// TODO: Start thread
			HANDLE hThread = CreateThread( NULL, 0, QueueThreadProc, pReq, 0, NULL );
			if (hThread) {
				/// Close thread handle. The thread will continue to run until its procedure exits
				CloseHandle( hThread );
				g_Queue.ThreadCount++;
			} else {
				e = GetLastError();
			}
		} else {
			/// The maximum number of worker threads are already running
			/// This request will wait in the queue until a thread becomes available
			e = ERROR_IO_PENDING;	//? This is a success error code
		}
	} else {
		/// This request is either Running or Completed
		e = ERROR_ALREADY_EXISTS;
	}
	return e;
}


//++ QueueStopTransfer
ULONG QueueStopTransfer( _In_ PCURL_REQUEST pReq )
{
	ULONG e = ERROR_SUCCESS;
	if (!pReq)
		return ERROR_INVALID_PARAMETER;
	if (pReq->iStatus == STATUS_RUNNING) {
		// TODO: Signal termination
		// TODO: Wait to stop
	}
	return e;
}
