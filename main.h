
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/01/19

#pragma once

#ifndef _DEBUG
	#if DBG || _DEBUG
		#define _DEBUG
	#endif
#endif

#define PLUGINNAME					_T( "NScurl" )

#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0500
#define _WIN32_IE    0x0600

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
extern CHAR					g_szUserAgent[128];		/// main.c
