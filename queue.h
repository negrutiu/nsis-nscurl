
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/24
//? Transfer queue

#pragma once

#include "curl.h"


//+ struct QUEUE_SELECTION
//? Identifies one/many/all requests in the queue
typedef struct {
	ULONG iId;						//? Match all request IDs  if zero
	LPCSTR pszTag;					//? Match all request tags if NULL
} QUEUE_SELECTION, *PQUEUE_SELECTION;


//+ Initialization
void QueueInitialize();
void QueueDestroy();

//+ Lock/Unlock
void QueueLock();
void QueueUnlock();

//+ Add/Remove
//? The queue *must not* be locked
ULONG QueueAdd( _In_ PCURL_REQUEST pReq );		/// The queue takes ownership of pReq
void  QueueRemove( _In_opt_ PQUEUE_SELECTION pSel );

//+ Abort
//? The queue *must not* be locked
// Sets the "Abort" flag. Does not wait for abortion to complete
// QueueAbort( NULL ) will abort all requests
void QueueAbort( _In_opt_ PQUEUE_SELECTION pSel );

//+ Query
//? The queue *must not* be locked
//? Find and replace "keywords" in the specified string
//? Returns the length of the output string, or -1 if errors occur
LONG QueueQuery( _In_opt_ PQUEUE_SELECTION pSel, _Inout_ LPTSTR pszStr, _In_ LONG iStrMaxLen );

//+ Enumerate
//? The queue *must not* be locked
// Returns a string-list of request ID-s
// The caller must curl_slist_free_all(..) it
struct curl_slist* QueueEnumerate( _In_opt_ PQUEUE_SELECTION pSel, _In_ BOOLEAN bWaiting, _In_ BOOLEAN bRunning, _In_ BOOLEAN bComplete );

//+ QueueStatistics
//! The queue must be locked by the caller
typedef struct {
	ULONG iWaiting, iRunning, iComplete, iErrors, iCancelled;
	ULONG iSpeed;
	ULONG64 iDlXferred, iUlXferred;
	ULONG iSingleId;							/// The transfer that stands out in the crowd
} QUEUE_STATS, *PQUEUE_STATS;
void QueueStatistics( _In_opt_ PQUEUE_SELECTION pSel, _Out_ PQUEUE_STATS pStats );

//+ Find
//! The queue must be locked by the caller
PCURL_REQUEST QueueHead();
PCURL_REQUEST QueueTail();
PCURL_REQUEST QueueFind( _In_ ULONG iId );
PCURL_REQUEST QueueFirstWaiting();
BOOL          QueueRequestMatch( _In_ PCURL_REQUEST pReq, _In_opt_ PQUEUE_SELECTION pSel );
