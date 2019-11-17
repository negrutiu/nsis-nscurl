
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/01/19

#pragma once

#ifndef _DEBUG
	#if DBG || _DEBUG
		#define _DEBUG
	#endif
#endif

#define PLUGINNAME					_T( "NScurl" )

#define COBJMACROS

#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0500
#define _WIN32_IE    0x0600
#include <windows.h>
#include <commctrl.h>
//#include <Shobjidl.h>				/// ITaskbarList
#include <stdio.h>


extern HINSTANCE g_hInst;			/// Defined in main.c
// NSIS API
#include "pluginapiex.h"

