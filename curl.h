
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#pragma once

typedef struct {
	CRITICAL_SECTION		Lock;
	HANDLE					hTerm;
	HANDLE					hSema;
	CHAR					szUserAgent[128];
} CURL_GLOBALS;

extern CURL_GLOBALS g;

// Initialization
ULONG CurlInitialize();
void  CurlDestroy();