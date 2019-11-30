
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/24
//? Transfer queue

#pragma once

#include "curl.h"

#define QUEUE_NO_ID (ULONG)-1

//+ Initialization
void QueueInitialize();
void QueueDestroy();

//+ Lock/Unlock
void QueueLock();
void QueueUnlock();

//+ Add/Remove
//? The queue *must not* be locked
ULONG QueueAdd( _In_ PCURL_REQUEST pReq );		/// The queue takes ownership of pReq
void  QueueRemove( _In_ ULONG iId );

//+ Query
//? The queue *must not* be locked
//? Find and replace "keywords" in the specified string
//? Returns the length of the output string, or -1 if errors occur
LONG QueueQuery( _In_opt_ ULONG iId, _Inout_ LPTSTR pszStr, _In_ LONG iStrMaxLen );		/// ID can be QUEUE_NO_ID

//+ QueueStatistics
//! The queue must be locked by the caller
typedef struct {
	ULONG iWaiting, iRunning, iCompleted, iErrors;
	ULONG iSpeed;
	ULONG64 iDlXferred, iUlXferred;
} QUEUE_STATS, *PQUEUE_STATS;
void QueueStatistics( _Out_ PQUEUE_STATS pStats );

//+ Find
//! The queue must be locked by the caller
PCURL_REQUEST QueueHead();
PCURL_REQUEST QueueTail();
PCURL_REQUEST QueueFind( _In_ ULONG iId );
PCURL_REQUEST QueueFirstWaiting();

