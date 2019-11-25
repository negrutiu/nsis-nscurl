
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
	volatile LONG		NextId;
	LONG				ThreadMax;
	volatile LONG		ThreadCount;
} g_Queue = {0};

#define QueueThreadCount(...)	\
	(InterlockedCompareExchange( &g_Queue.ThreadCount, -1, -1 ))

//+ Prototypes
ULONG WINAPI QueueThreadProc( _In_ LPVOID pParam );


//++ QueueInitialize
void QueueInitialize()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );

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
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );

	// The global TERM event has already been set
	// Transfers are being aborted, worker threads are closing
//x	while (QueueThreadCount() > 0)
//x		Sleep( 10 );

	// Destroy the queue
	while (g_Queue.Head)
		QueueRemove( g_Queue.Head->Queue.iId );

	// Cleanup
	DeleteCriticalSection( &g_Queue.Lock );
	ZeroMemory( &g_Queue, sizeof( g_Queue ) );
}


//++ QueueLock/Unlock
void QueueLock()   { EnterCriticalSection( &g_Queue.Lock ); }
void QueueUnlock() { LeaveCriticalSection( &g_Queue.Lock ); }


//++ QueueAdd
ULONG QueueAdd( _In_ PCURL_REQUEST pReq )
{
	ULONG e = ERROR_SUCCESS;

	if (IsTermEventSet)
		return ERROR_SHUTDOWN_IN_PROGRESS;

	if (pReq) {
		if (!pReq->pszURL || !*pReq->pszURL)
			return ERROR_INVALID_PARAMETER;
	}

	if (pReq) {

		// Create a worker thread (suspended)
		HANDLE hThread = NULL;
		if (QueueThreadCount() + 1 <= g_Queue.ThreadMax) {
			if ((hThread = CreateThread( NULL, 0, QueueThreadProc, pReq, CREATE_SUSPENDED, NULL )) != NULL) {
				// NOTE: Testing/Incrementing the thread count is not atomic. It's possible that we exceed the maximum limit. We can live with that ;)
				InterlockedIncrement( &g_Queue.ThreadCount );
			} else {
				// TODO: GetLastError()
			}
		} else {
			/// No more threads are allowed
			/// The new HTTP request will wait in queue until an existing worker thread becomes available
		}

		// Add to queue
		pReq->Queue.pNext   = NULL;
		pReq->Queue.iStatus = (hThread ? STATUS_RUNNING : STATUS_WAITING);
		pReq->Queue.iId     = InterlockedIncrement( &g_Queue.NextId );

		QueueLock();
		{
			PCURL_REQUEST pTail = QueueTail();
			if (pTail) {
				pTail->Queue.pNext = pReq;
			} else {
				g_Queue.Head = pReq;
			}
		}
		QueueUnlock();

		TRACE( _T( "%hs( Id:%u, Url:%hs )\n" ), __FUNCTION__, pReq->Queue.iId, pReq->pszURL );

		// Resume the thread
		if (hThread) {
			ResumeThread( hThread );
			CloseHandle( hThread );			//? Thread handle is no longer needed. The thread will continue to run until its procedure exits
		}
	
	} else {
		e = ERROR_INVALID_PARAMETER;
	}
	return e;
}


//++ QueueRemove
//?  The queue must be *unlocked*
void QueueRemove( _In_ ULONG iId )
{
	// Find request by ID
	PCURL_REQUEST pReq;
	QueueLock();
	pReq = QueueFind( iId );
	QueueUnlock();

	if (pReq) {

		ULONG t0 = GetTickCount();

		// If running, abort the transfer now
		InterlockedExchange( &pReq->Queue.iFlagAbort, TRUE );

		// Wait for the transfer to abort
		// TODO: Polling sucks. Find something better!
		while (TRUE) {
			MemoryBarrier();
			if (pReq->Queue.iStatus != STATUS_RUNNING) {
				break;
			} else {
				Sleep( 10 );
			}
		}

		// Remove from queue
		QueueLock();
		if (g_Queue.Head == pReq) {
			g_Queue.Head = pReq->Queue.pNext;
		} else {
			PCURL_REQUEST pPrev;
			for (pPrev = g_Queue.Head; pPrev->Queue.pNext != pReq; pPrev = pPrev->Queue.pNext);
			assert( pPrev );
			pPrev->Queue.pNext = pReq->Queue.pNext;
		}
		QueueUnlock();

		TRACE( _T( "%hs( Id:%u, Url:%hs ), %ums\n" ), __FUNCTION__, pReq->Queue.iId, pReq->pszURL, GetTickCount() - t0 );

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
		for (pTail = g_Queue.Head; pTail->Queue.pNext; pTail = pTail->Queue.pNext);
		return pTail;
	}
	return NULL;
}

//++ QueueFind
PCURL_REQUEST QueueFind( _In_ ULONG iId )
{
	if (iId) {
		PCURL_REQUEST p;
		for (p = g_Queue.Head; p && (p->Queue.iId != iId); p = p->Queue.pNext);
		return p;
	}
	return NULL;
}

//++ QueueFirstWaiting
PCURL_REQUEST QueueFirstWaiting()
{
	if (g_Queue.Head) {
		PCURL_REQUEST p;
		for (p = g_Queue.Head; p && (p->Queue.iStatus != STATUS_WAITING); p = p->Queue.pNext);
		return p;
	}
	return NULL;
}


//++ QueueThreadProc
ULONG WINAPI QueueThreadProc( _In_ LPVOID pParam )
{
	ULONG e = ERROR_SUCCESS, t0;
	LONG iThreadCount;
	PCURL_REQUEST pReq = (PCURL_REQUEST)pParam;

	TRACE( _T( "%hs( Count:%d/%d )\n" ), "CreateThread", QueueThreadCount(), g_Queue.ThreadMax );

	while (TRUE) {

		// Check TERM event
		if (IsTermEventSet)
			break;

		// Select the next waiting request
		if (!pReq) {
			QueueLock();
			pReq = QueueFirstWaiting();
			if (pReq)
				pReq->Queue.iStatus = STATUS_RUNNING;
			QueueUnlock();
		}

		// No more waiting requests
		if (!pReq)
			break;

		// Transfer
		t0 = GetTickCount();
		CurlTransfer( pReq );
	#ifdef TRACE_ENABLED
		{
			TCHAR szErr[256];
			CurlRequestFormatError( pReq, szErr, ARRAYSIZE( szErr ) );
			TRACE( _T( "%hs( Id:%u, Url:%hs ) = %s, %ums\n" ), "CurlTransfer", pReq->Queue.iId, pReq->pszURL, szErr, GetTickCount() - t0 );
		}
	#endif

		pReq->Queue.iStatus = STATUS_COMPLETED;
		MemoryBarrier();

		// Cleanup
		pReq = NULL;
	}

	// Exit
	iThreadCount = InterlockedDecrement( &g_Queue.ThreadCount );
	TRACE( _T( "%hs( Count:%d/%d )\n" ), "ExitThread", iThreadCount, g_Queue.ThreadMax );

	return e;
}
