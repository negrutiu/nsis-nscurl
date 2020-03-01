
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/24

#include "main.h"
#include "queue.h"

//+ Global
struct {
	CRITICAL_SECTION	Lock;
	PCURL_REQUEST		Head;
	ULONG				NextId;
	LONG				ThreadMax;
	LONG				ThreadCount;
} g_Queue = {0};


//+ Prototypes
ULONG WINAPI QueueThreadProc( _In_ LPVOID pParam );
void QueueCreateThreads( _In_ LONG iCount );


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
//x	while (TRUE) {
//x		LONG iThreadCount;
//x		QueueLock();
//x		iThreadCount = g_Queue.ThreadCount;
//x		QueueUnlock();
//x		if (iThreadCount <= 0)
//x			break;
//x		Sleep( 10 );
//x	}

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

	if (TermSignaled())
		return ERROR_SHUTDOWN_IN_PROGRESS;

	if (pReq) {
		if (!pReq->pszURL || !*pReq->pszURL)
			return ERROR_INVALID_PARAMETER;
	}

	if (pReq) {

		HANDLE hThread = NULL;

		QueueLock();
		{
			// Append to queue
			PCURL_REQUEST pTail = QueueTail();
			if (pTail) {
				pTail->Queue.pNext = pReq;
			} else {
				g_Queue.Head = pReq;
			}

			pReq->Queue.iId = ++g_Queue.NextId;
			pReq->Queue.iStatus = STATUS_WAITING;
			pReq->Queue.pNext = NULL;

			TRACE( _T( "%hs( Id:%u, Url:%hs )\n" ), __FUNCTION__, pReq->Queue.iId, pReq->pszURL );

			// New worker thread
			QueueCreateThreads( 1 );
		}
		QueueUnlock();

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


//++ QueueAbort
void QueueAbort( _In_opt_ ULONG iId )
{
	PCURL_REQUEST pReq;
	TRACE( _T( "%hs( Id:%d )\n" ), __FUNCTION__, iId );
	QueueLock();
	for (pReq = g_Queue.Head; pReq; pReq = pReq->Queue.pNext) {

		if ((iId == QUEUE_NO_ID) || (pReq->Queue.iId == iId)) {

			if (pReq->Queue.iStatus == STATUS_WAITING) {

				// Mark as Complete
				pReq->Queue.iStatus = STATUS_COMPLETE;

				// Set Win32 error
				pReq->Error.iWin32 = ERROR_CANCELLED;
				pReq->Error.pszWin32 = MyFormatError( pReq->Error.iWin32 );

				TRACE( _T( "%hs( Id:%u, Status:%hs )\n" ), __FUNCTION__, pReq->Queue.iId, "Waiting" );

			} else if (pReq->Queue.iStatus == STATUS_RUNNING) {

				// Set the Abort flag and let the transfer terminate itself
				InterlockedExchange( &pReq->Queue.iFlagAbort, TRUE );

				TRACE( _T( "%hs( Id:%u, Status:%hs )\n" ), __FUNCTION__, pReq->Queue.iId, "Running" );
			}
		}
	}
	QueueUnlock();
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
	if (iId != 0 && iId != QUEUE_NO_ID) {
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
		for (p = g_Queue.Head; p; p = p->Queue.pNext) {
			if (p->Queue.iStatus == STATUS_WAITING) {
				if (p->iDependencyId != 0) {
					PCURL_REQUEST pReqDep = QueueFind( p->iDependencyId );
					if (pReqDep) {
						if (pReqDep->Queue.iStatus == STATUS_COMPLETE) {
							return p;		/// Dependency satisfied
						} else {
							/// Continue looking for the next WAITING
							TRACE( _T( "%hs( Id:%u/Waiting <-> Id:%u/Incomplete ) = Skip\n" ), __FUNCTION__, p->Queue.iId, pReqDep->Queue.iId );
						}
					} else {
						return p;		/// Depends on inexistent/removed request
					}
				} else {
					return p;		/// Independent request
				}
			}
		}
	}
	return NULL;
}


//++ QueueCreateThreads
//!  The queue must be *locked*
void QueueCreateThreads( _In_ LONG iCount )
{
	HANDLE h;
	LONG i, n;
	n = __min( iCount, g_Queue.ThreadMax - g_Queue.ThreadCount );
	for (i = 0; i < n; i++) {
		if ((h = CreateThread( NULL, 0, QueueThreadProc, (LPVOID)++g_Queue.ThreadCount, 0, NULL )) != NULL) {
			MySetThreadName( h, L"NScurl" );
			CloseHandle( h );			//? Thread handle is no longer needed. The thread will continue to run until its procedure exits
		} else {
			g_Queue.ThreadCount--;
			TRACE( _T( "%hs::CreateThread(..) = 0x%x\n" ), __FUNCTION__, GetLastError() );
		}
	}
}


//++ QueueThreadProc
ULONG WINAPI QueueThreadProc( _In_ LPVOID pParam )
{
	ULONG e = ERROR_SUCCESS, t0;
	PCURL_REQUEST pReq = NULL;
	LONG iThreadCount = (LONG)pParam;

	TRACE( _T( "%hs( Count:%d/%d )\n" ), "CreateThread", iThreadCount, g_Queue.ThreadMax );

	while (TRUE) {

		// Check TERM event
		if (TermSignaled())
			break;

		// Select the next waiting request
		QueueLock();
		pReq = QueueFirstWaiting();
		if (pReq)
			pReq->Queue.iStatus = STATUS_RUNNING;		/// Mark as Running while locked
		QueueUnlock();
		if (!pReq)
			break;		// No more waiting requests

		//+ Transfer
		t0 = GetTickCount();
		CurlTransfer( pReq );

	#ifdef TRACE_ENABLED
		{
			TCHAR szErr[256];
			CurlRequestFormatError( pReq, szErr, ARRAYSIZE( szErr ), NULL, NULL );
			TRACE( _T( "%hs( Id:%u, Url:%hs ) = %s, %ums\n" ), "CurlTransfer", pReq->Queue.iId, pReq->pszURL, szErr, GetTickCount() - t0 );
		}
	#endif

		// Mark as Complete
		pReq->Queue.iStatus = STATUS_COMPLETE;
		MemoryBarrier();

		// Create more threads to handle multiple dependencies
		//x QueueLock();
		//x {
		//x 	PCURL_REQUEST p;
		//x 	LONG iDependCount = 0;
		//x 	for (p = g_Queue.Head; p; p = p->Queue.pNext)
		//x 		if (p->iDependencyId == pReq->Queue.iId)
		//x 			iDependCount++;
		//x 	QueueCreateThreads( iDependCount - 1 );
		//x }
		//x QueueUnlock();
	}

	// Exit
	QueueLock();
	iThreadCount = --g_Queue.ThreadCount;
	assert( g_Queue.ThreadCount >= 0 );
	QueueUnlock();

	TRACE( _T( "%hs( Count:%d/%d )\n" ), "ExitThread", iThreadCount, g_Queue.ThreadMax );

	return e;
}


//++ QueueStatistics
void QueueStatistics( _In_opt_ ULONG iId, _Out_ PQUEUE_STATS pStats )
{
	PCURL_REQUEST p;
	BOOLEAN bOK;
	ULONG iLastRunningId = QUEUE_NO_ID;

	assert( pStats );

	ZeroMemory( pStats, sizeof( *pStats ) );
	pStats->iSingleID = QUEUE_NO_ID;
	MemoryBarrier();

	for (p = g_Queue.Head; p; p = p->Queue.pNext) {

		if (iId == QUEUE_NO_ID || p->Queue.iId == iId) {

			if (p->Queue.iStatus == STATUS_WAITING)
				pStats->iWaiting++;
			else if (p->Queue.iStatus == STATUS_RUNNING)
				pStats->iRunning++, iLastRunningId = p->Queue.iId;
			else if (p->Queue.iStatus == STATUS_COMPLETE)
				pStats->iComplete++;
			else
				assert( !"Unexpected request status" );

			pStats->iDlXferred += p->Runtime.iDlXferred;
			pStats->iUlXferred += p->Runtime.iUlXferred;
			pStats->iSpeed     += (ULONG)p->Runtime.iSpeed;

			CurlRequestFormatError( p, NULL, 0, &bOK, NULL );
			if (!bOK)
				pStats->iErrors++;
		}
	}

	if (iId != QUEUE_NO_ID) {
		pStats->iSingleID = iId;
	} else if (pStats->iWaiting == 0 && pStats->iRunning == 1) {
		pStats->iSingleID = iLastRunningId;
	} else {
		pStats->iSingleID = QUEUE_NO_ID;
	}

	//x TRACE( _T( "ID:%d, SingleID:%d, Waiting:%u, Running:%u\n" ), iId, pStats->iSingleID, pStats->iWaiting, pStats->iRunning );
}


//+ [internal] QueueQueryKeywordCallback
void CALLBACK QueueQueryKeywordCallback( _Inout_ LPTSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam )
{
	QUEUE_STATS qs;

	QueueLock();
	QueueStatistics( QUEUE_NO_ID, &qs );
	QueueUnlock();

	assert( pszKeyword );
	if (lstrcmpi( pszKeyword, _T( "@TOTALCOUNT@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iWaiting + qs.iRunning + qs.iComplete );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALWAITING@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iWaiting );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALRUNNING@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iRunning );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALCOMPLETE@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iComplete );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALACTIVE@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iWaiting + qs.iRunning );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSTARTED@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iRunning + qs.iComplete );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALERRORS@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iErrors );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSPEED@" ) ) == 0) {
		MyFormatBytes( qs.iSpeed, pszKeyword, iMaxLen );
		_tcscat( pszKeyword, _T( "/s" ) );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSPEED_B@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iSpeed );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSIZE@" ) ) == 0) {
		MyFormatBytes( qs.iUlXferred + qs.iDlXferred, pszKeyword, iMaxLen );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSIZE_B@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%I64u" ), qs.iUlXferred + qs.iDlXferred );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSIZEUP@" ) ) == 0) {
		MyFormatBytes( qs.iUlXferred, pszKeyword, iMaxLen );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSIZEUP_B@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%I64u" ), qs.iUlXferred );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSIZEDOWN@" ) ) == 0) {
		MyFormatBytes( qs.iDlXferred, pszKeyword, iMaxLen );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSIZEDOWN_B@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%I64u" ), qs.iDlXferred );
	}
}


//++ QueueQuery
LONG QueueQuery( _In_opt_ ULONG iId, _Inout_ LPTSTR pszStr, _In_ LONG iStrMaxLen )
{
	LONG iStrLen = -1;
	if (pszStr && iStrMaxLen) {

		QueueLock();

		// Replace request-specific keywords
		iStrLen = CurlQuery( QueueFind( iId ), pszStr, iStrMaxLen );		//? pReq can be NULL

		// Replace global queue keywords
		if (iStrLen != -1)
			iStrLen = MyReplaceKeywords( pszStr, iStrMaxLen, _T( '@' ), _T( '@' ), QueueQueryKeywordCallback, NULL );

		QueueUnlock();
	}
	return iStrLen;
}


//++ QueueEnumerate
struct curl_slist* QueueEnumerate( _In_ BOOLEAN bWaiting, _In_ BOOLEAN bRunning, _In_ BOOLEAN bComplete )
{
	struct curl_slist *sl = NULL;
	PCURL_REQUEST p;

	QueueLock();

	for (p = g_Queue.Head; p; p = p->Queue.pNext) {
		if ((bWaiting && p->Queue.iStatus == STATUS_WAITING) ||
			(bRunning && p->Queue.iStatus == STATUS_RUNNING) ||
			(bComplete && p->Queue.iStatus == STATUS_COMPLETE))
		{
			CHAR sz[16];
			_snprintf( sz, ARRAYSIZE( sz ), "%u", p->Queue.iId );
			sl = curl_slist_append( sl, sz );
		}
	}

	QueueUnlock();
	return sl;
}
