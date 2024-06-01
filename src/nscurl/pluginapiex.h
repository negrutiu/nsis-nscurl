
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/17
//? NSIS API with a bunch of additions

#pragma once
#ifndef _PLUGINAPI_EX_H_
#define _PLUGINAPI_EX_H_

#include "nsis/nsis_tchar.h"
#include "nsis/pluginapi.h"

//+ EXDLL_VALID_PARAMS
// Validate input parameters to prevent crashes
// VirusTotal "detonates" dlls by running `RunDll32.exe "<dll>",<proc>` with no parameters
// If exported functions don't validate input parameters properly they will likely crash, triggering WER to launch WerFault.exe as child process
// When this happens, our dll is labeled as a potential program launcher increasing the chances of being reported as malitious
// NOTE: Functions called from .onInit receive parent=NULL
#define EXDLL_VALID_PARAMS() \
    if ((parent && !IsWindow(parent)) || string_size == 0 || (string_size % 1024) != 0 || !variables || !stacktop || !extra) \
        return;

//+ EXDLL_INIT
//? Equivalent to the original EXDLL_INIT(), plus g_ep and g_hwndparent
#undef EXDLL_INIT
#define EXDLL_INIT() {  \
        g_stringsize=string_size; \
        g_stacktop=stacktop;      \
        g_variables=variables;    \
        g_ep=extra;               \
        g_hwndparent=parent;      \
}

//+ EXDLL_VALIDATE
//? Validate NSIS framework version
#define EXDLL_VALIDATE() \
	if (g_ep && g_ep->exec_flags && (g_ep->exec_flags->plugin_api_version < NSISPIAPIVER_CURR))  \
		return;

// Additional variables, not exported by the NSIS API
static const int INST_TEMP			= 25;
static const int INST_PLUGINSDIR	= 26;
static const int INST_EXEPATH		= 27;
static const int INST_EXEFILE		= 28;
static const int __INST_LAST_EX		= 28;
//x static const int INST_HWNDPARENT	= 29;
//xstatic const int __INST_LAST_EX		= 29;

//+ getuservariableEx
//? Equivalent to getuservariable() including the additional variables
static LPTSTR NSISCALL getuservariableEx( const int varnum )
{
	return (varnum <= __INST_LAST_EX) ? g_variables + (varnum * g_stringsize) : NULL;
}

//+ setuservariableEx
//? Equivalent to setuservariable() including the additional variables
static void NSISCALL setuservariableEx( const int varnum, LPCTSTR var )
{
	if (var && (varnum <= __INST_LAST_EX))
		lstrcpy( g_variables + (varnum * g_stringsize), var );
}

//+ pushstringEx
//? Equivalent to pushstring(), but handles NULL input
static void NSISCALL pushstringEx( LPCTSTR str )
{
	pushstring( str ? str : _T("") );
}

//+ Globals
extern extra_parameters			*g_ep;				/// main.c
extern HWND						g_hwndparent;		/// main.c

#endif	/// _PLUGINAPI_EX_H_
