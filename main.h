
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/01/19

#pragma once

#ifndef _DEBUG
	#if DBG || _DEBUG
		#define _DEBUG
	#endif
#endif

#define PLUGINNAME			_T( "NScurl" )

#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT		0x0400
//#define _WIN32_IE			0x0600

#include <winsock2.h>
#include <windows.h>

// NSIS API
#include "pluginapiex.h"

// CURL
#define CURL_STATICLIB
#ifdef _DEBUG
	#define DEBUGBUILD
#endif
#include <curl/curl.h>

// Globals
extern HINSTANCE			g_hInst;				/// main.c
extern HANDLE				g_hTerm;				/// main.c

#define TermSignaled(...)	(g_hTerm && (WaitForSingleObject(g_hTerm, 0) != WAIT_TIMEOUT))


//+ MainQuery
//? Find and replace "keywords" in the specified string
//? Returns the length of the output string, or -1 if errors occur
LONG MainQuery( _In_opt_ ULONG iId, _Inout_ LPTSTR pszStr, _In_ LONG iStrMaxLen );		/// ID can be QUEUE_NO_ID
