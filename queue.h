
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/24
//? Transfer queue

#pragma once

#include "curl.h"


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

//+ Find
//! The queue must be locked by the caller
PCURL_REQUEST QueueHead();
PCURL_REQUEST QueueTail();
PCURL_REQUEST QueueFind( _In_ ULONG iId );
PCURL_REQUEST QueueFirstWaiting();
