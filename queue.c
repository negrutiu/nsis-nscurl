
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

			// Create a new (suspended) worker thread
			if (g_Queue.ThreadCount + 1 <= g_Queue.ThreadMax) {
				if ((hThread = CreateThread( NULL, 0, QueueThreadProc, pReq, CREATE_SUSPENDED, NULL )) != NULL) {
					g_Queue.ThreadCount++;
					MySetThreadName( hThread, L"NScurl" );
				} else {
					// TODO: GetLastError()
				}
			} else {
				//? The maximum number of threads are already running
				//? The new HTTP request will wait in queue until an existing thread will handle it
			}

			// Final touches
			pReq->Queue.pNext = NULL;
			pReq->Queue.iStatus = (hThread ? STATUS_RUNNING : STATUS_WAITING);
			pReq->Queue.iId = ++g_Queue.NextId;
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

	QueueLock();
	iThreadCount = g_Queue.ThreadCount;
	QueueUnlock();
	TRACE( _T( "%hs( Count:%d/%d )\n" ), "CreateThread", iThreadCount, g_Queue.ThreadMax );

	while (TRUE) {

		// Check TERM event
		if (TermSignaled())
			break;

		// Select the next waiting request
		if (!pReq) {
			QueueLock();
			pReq = QueueFirstWaiting();
			if (pReq)
				pReq->Queue.iStatus = STATUS_RUNNING;		/// Mark as Running while locked
			QueueUnlock();
		}

		// No more waiting requests
		if (!pReq)
			break;

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

		// Mark as Completed
		pReq->Queue.iStatus = STATUS_COMPLETE;
		MemoryBarrier();

		// Cleanup
		pReq = NULL;
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
void QueueStatistics( _Out_ PQUEUE_STATS pStats )
{
	PCURL_REQUEST p;
	BOOLEAN bOK;
	assert( pStats );
	ZeroMemory( pStats, sizeof( *pStats ) );
	MemoryBarrier();
	for (p = g_Queue.Head; p; p = p->Queue.pNext) {

		if (p->Queue.iStatus == STATUS_WAITING)
			pStats->iWaiting++;
		else if (p->Queue.iStatus == STATUS_RUNNING)
			pStats->iRunning++;
		else if (p->Queue.iStatus == STATUS_COMPLETE)
			pStats->iCompleted++;
		else
			assert( !"Unexpected request status" );

		pStats->iDlXferred += p->Runtime.iDlXferred;
		pStats->iUlXferred += p->Runtime.iUlXferred;
		pStats->iSpeed     += p->Runtime.iSpeed;

		CurlRequestFormatError( p, NULL, 0, &bOK, NULL );
		if (!bOK)
			pStats->iErrors++;
	}
}


//+ [internal] QueueQueryKeywordCallback
void CALLBACK QueueQueryKeywordCallback( _Inout_ LPTSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam )
{
	QUEUE_STATS qs;

	QueueLock();
	QueueStatistics( &qs );
	QueueUnlock();

	assert( pszKeyword );
	if (lstrcmpi( pszKeyword, _T( "@TOTALCOUNT@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iWaiting + qs.iRunning + qs.iCompleted );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALWAITING@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iWaiting );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALRUNNING@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iRunning );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALCOMPLETED@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iCompleted );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALACTIVE@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iWaiting + qs.iRunning );
	} else if (lstrcmpi( pszKeyword, _T( "@TOTALSTARTED@" ) ) == 0) {
		_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), qs.iRunning + qs.iCompleted );
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

		PCURL_REQUEST pReq = NULL;
		QueueLock();

		// Replace request-specific keywords
		if (iId != QUEUE_NO_ID) {
			pReq = QueueFind( iId );
		} else {
			// If a single request is running, use it for querying
			if (!pReq) {
				PCURL_REQUEST p;
				for (p = QueueHead(); p; p = p->Queue.pNext) {
					if (pReq == NULL && p->Queue.iStatus == STATUS_RUNNING) {
						pReq = p;		/// First running
					} else if (pReq != NULL && (p->Queue.iStatus == STATUS_RUNNING || p->Queue.iStatus == STATUS_WAITING)) {
						pReq = NULL;	/// Others are running/waiting as well
						break;
					}
				}
			}
			// If there's a single request, use it for querying
			if (!pReq) {
				if (QueueHead() && !QueueHead()->Queue.pNext) {
					pReq = QueueHead();
				}
			}
		}
		iStrLen = CurlQuery( pReq, pszStr, iStrMaxLen );		//? pReq can be NULL

		// Replace global queue keywords
		if (iStrLen != -1) {
			iStrLen = MyReplaceKeywords( pszStr, iStrMaxLen, _T( '@' ), _T( '@' ), QueueQueryKeywordCallback, NULL );
		}

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


//++ QueueCount
ULONG QueueCount( _In_ CHAR iStatus, _In_opt_ struct curl_slist* pIDs )
{
	ULONG n = 0;
	PCURL_REQUEST p = NULL;
	struct curl_slist *s;
	CHAR szID[16];

	for (p = g_Queue.Head; p; p = p->Queue.pNext) {
		if (p->Queue.iStatus == iStatus) {
			if (!pIDs) {
				// Any ID
				n++;
			} else {
				// Specific IDs
				_snprintf( szID, ARRAYSIZE( szID ), "%u", p->Queue.iId );
				for (s = pIDs; s; s = s->next) {
					if (lstrcmpA( s->data, szID ) == 0) {
						n++;
						break;
					}
				}
			}
		}
	}
	return n;
}
