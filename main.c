
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/01/19

#include "main.h"
#include "curl.h"
#include "utils.h"
#include "crypto.h"
#include "queue.h"


HINSTANCE	g_hInst = NULL;
HANDLE		g_hTerm = NULL;


//++ PluginInit
//?  Called at DLL_PROCESS_ATTACH
BOOL PluginInit( _In_ HINSTANCE hInst )
{
	if (!g_hInst) {

		TRACE( _T( "PluginInit\n" ) );

		g_hInst = hInst;
		g_hTerm = CreateEvent( NULL, TRUE, FALSE, NULL );
		assert( g_hTerm );

		UtilsInitialize();
		CurlInitialize();
		QueueInitialize();

		return TRUE;
	}
	return FALSE;
}


//++ PluginUninit
//?  Called at DLL_PROCESS_DETACH
BOOL PluginUninit()
{
	if ( g_hInst ) {

		TRACE( _T( "PluginUninit\n" ) );
		
		SetEvent( g_hTerm );

		QueueDestroy();
		CurlDestroy();
		UtilsDestroy();

		CloseHandle( g_hTerm );
		g_hInst = NULL;
		return TRUE;
	}
	return FALSE;
}


//++ UnloadCallback
//+  USAGE: extra->RegisterPluginCallback( g_hInst, UnloadCallback );
//?  By registering UnloadCallback, the framework will keep the current plugin locked in memory
//?  Otherwise, unless the caller specifies the /NOUNLOAD parameter the plugin gets unloaded when the current call returns
UINT_PTR __cdecl UnloadCallback( enum NSPIM iMessage )
{
	switch ( iMessage ) {
		case NSPIM_GUIUNLOAD: {
			TRACE( _T( "%hs( NSPIM_GUIUNLOAD )\n" ), __FUNCTION__ );
			break;
		}
		case NSPIM_UNLOAD: {
			TRACE( _T( "%hs( NSPIM_UNLOAD )\n" ), __FUNCTION__ );
			PluginUninit();
			break;
		}
	}
	return 0;
}


//++ [exported] md5 <file>
EXTERN_C __declspec(dllexport)
void __cdecl md5( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	LPTSTR psz;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );
	psz[0] = 0;

	if (popstring( psz ) == NOERROR) {
		UCHAR hash[16];
		if (FileHash( psz, hash, NULL, NULL ) == ERROR_SUCCESS) {
			MyFormatBinaryHex( hash, sizeof( hash ), psz, string_size );
		} else {
			psz[0] = 0;
		}
	}

	pushstringEx( psz );
	MyFree( psz );
}


//++ [exported] sha1 <file>
EXTERN_C __declspec(dllexport)
void __cdecl sha1( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	LPTSTR psz;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );
	psz[0] = 0;

	if (popstring( psz ) == NOERROR) {
		UCHAR hash[20];
		if (FileHash( psz, NULL, hash, NULL ) == ERROR_SUCCESS) {
			MyFormatBinaryHex( hash, sizeof( hash ), psz, string_size );
		} else {
			psz[0] = 0;
		}
	}

	pushstringEx( psz );
	MyFree( psz );
}


//++ [exported] sha256 <file>
EXTERN_C __declspec(dllexport)
void __cdecl sha256( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	LPTSTR psz;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );
	psz[0] = 0;

	if (popstring( psz ) == NOERROR) {
		UCHAR hash[32];
		if (FileHash( psz, NULL, NULL, hash ) == ERROR_SUCCESS) {
			MyFormatBinaryHex( hash, sizeof( hash ), psz, string_size );
		} else {
			psz[0] = 0;
		}
	}

	pushstringEx( psz );
	MyFree( psz );
}


//++ [exported] Echo (test)
EXTERN_C __declspec(dllexport)
void __cdecl Echo( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	LPTSTR psz, psz2;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	psz2 = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz && psz2 );

	psz[0] = 0;
	for (;;) {
		if (popstring( psz2 ) != NOERROR)
			break;

		if (*psz)
			_tcscat( psz, _T( " " ) );
		_tcscat( psz, psz2 );

		if (lstrcmpi( psz2, _T( "/END" ) ) == 0)
			break;
	}
	pushstringEx( psz );

	MyFree( psz );
	MyFree( psz2 );
}


//++ [exported] Request
EXTERN_C __declspec(dllexport)
void __cdecl Request( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	ULONG iRequestID = 0;
	LPTSTR psz = NULL;
	PCURL_REQUEST pReq = NULL;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!Request\n" ) );

	// Lock the plugin in memory
	extra->RegisterPluginCallback( g_hInst, UnloadCallback );

	// Working buffer
	psz = (LPTSTR)MyAlloc( string_size * sizeof(TCHAR) );
	assert( psz );

	// Prepare a HTTP request
	pReq = (PCURL_REQUEST)MyAlloc( sizeof( *pReq ) );
	if (pReq) {

		CurlRequestInit( pReq );
		for (;;)
		{
			if (popstring( psz ) != 0)
				break;
			if (lstrcmpi( psz, _T( "/END" ) ) == 0)
				break;

			if (!CurlParseRequestParam( psz, string_size, pReq )) {
				TRACE( _T( "  [!] Unknown parameter \"%s\"\n" ), psz );
			}
		}

		// Append to the queue
		if (QueueAdd( pReq ) == ERROR_SUCCESS) {

			// Return value
			iRequestID = pReq->Queue.iId;

		} else {
			CurlRequestDestroy( pReq );
			MyFree( pReq );
		}
	}

	pushint( iRequestID );
	MyFree( psz );
}


//+ [internal] GlobalQueryKeywordCallback
void CALLBACK GlobalQueryKeywordCallback( _Inout_ LPTSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam )
{
	assert( pszKeyword );
	if (lstrcmpi( pszKeyword, _T( "@@" ) ) == 0) {
		lstrcpyn( pszKeyword, _T( "@" ), iMaxLen );		// @@ -> @
	}
/*
	{ORIGINALTITLE}			| The original title text
	{ORIGINALSTATUS}		| The original status text
	{ANIMLINE}				| The classic \|/- animation
	{ANIMDOTS}				| The classic ./../... animation
*/
}


//++ [exported] Query
EXTERN_C __declspec(dllexport)
void __cdecl Query( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	ULONG iId = QUEUE_NO_ID;
	LPTSTR psz = NULL;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!Query\n" ) );

	// Working buffer
	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );
	if (psz) {

		const ULONG OPTIONAL_PARAMS = 1;
		ULONG i, e = NOERROR;
		for (i = 0; (i <= OPTIONAL_PARAMS) && (e == NOERROR); i++) {
			if ((e = popstring( psz )) == NOERROR) {
				if (lstrcmpi( psz, _T( "/ID" ) ) == 0) {
					iId = (ULONG)popintptr();
				} else {
					break;		/// Done reading optional parameters. Continue reading mandatory parameters
				}
			}
		}
		if (e == NOERROR) {

			// Replace queue keywords
			QueueQuery( iId, psz, string_size );

			// Replace global keywords
			MyReplaceKeywords( psz, string_size, _T( '@' ), _T( '@' ), GlobalQueryKeywordCallback, NULL );

			pushstringEx( psz );
		}
	}

	MyFree( psz );
}


//++ [exported] Enumerate
EXTERN_C __declspec(dllexport)
void __cdecl Enumerate( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	LPTSTR psz = NULL;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "%s!%hs\n" ), PLUGINNAME, __FUNCTION__ );

	if ((psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) )) != NULL) {

		BOOLEAN bWaiting = FALSE, bRunning = FALSE, bComplete = FALSE;
		struct curl_slist *sl = NULL;

		for (;;)
		{
			if (popstring( psz ) != NOERROR)
				break;
			if (lstrcmpi( psz, _T( "/END" ) ) == 0)
				break;

			if (lstrcmpi( psz, _T( "/STATUS" ) ) == 0) {
				if (popstring( psz ) == NOERROR) {
					if (lstrcmpi( psz, _T( "Waiting" ) ) == 0) {
						bWaiting = TRUE;
					} else if (lstrcmpi( psz, _T( "Running" ) ) == 0) {
						bRunning = TRUE;
					} else if (lstrcmpi( psz, _T( "Complete" ) ) == 0) {
						bComplete = TRUE;
					}
				}
			}
		}

		if (!bWaiting && !bRunning && !bComplete)
			bWaiting = bRunning = bComplete = TRUE;		/// All by default

		// Enumerate ID-s
		sl = QueueEnumerate( bWaiting, bRunning, bComplete );

		// Empty string marks the end of enumeration
		pushstringEx( _T( "" ) );

		// Push them on the stack in reversed order
		{
			int i, j, n;
			struct curl_slist *s;
			for (n = 0, s = sl; s; n++, s = s->next);	/// List length
			for (i = n - 1; i >= 0; i--) {
				for (j = 0, s = sl; j < i; j++, s = s->next);
				PushStringA( s->data );
			}
		}

		curl_slist_free_all( sl );
		MyFree( psz );
	}
}


//++ [exported] UrlEscape
EXTERN_C __declspec(dllexport)
void __cdecl UrlEscape( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	LPTSTR psz = NULL;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!EscapeURL\n" ) );

	psz = (LPTSTR)MyAlloc( string_size * sizeof(TCHAR) );
	assert( psz );

	if (popstring( psz ) == NOERROR) {
		LPSTR pszA = MyStrDup( eT2A, psz );
		if (pszA) {
			LPSTR pszOut = curl_easy_escape( NULL, pszA, 0 );
			if (pszOut) {
				PushStringA( pszOut );
				curl_free( pszOut );
			}
			MyFree( pszA );
		}
	}

	MyFree( psz );
}


//++ [exported] UrlUnescape
EXTERN_C __declspec(dllexport)
void __cdecl UrlUnescape( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	LPTSTR psz = NULL;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	TRACE( _T( "NSxfer!UnescapeURL\n" ) );

	psz = (LPTSTR)MyAlloc( string_size * sizeof(TCHAR) );
	assert( psz );

	if (popstring( psz ) == NOERROR) {
		LPSTR pszA = MyStrDup( eT2A, psz );
		if (pszA) {
			LPSTR pszOut = curl_easy_unescape( NULL, pszA, 0, NULL );
			if (pszOut) {
				PushStringA( pszOut );
				curl_free( pszOut );
			}
			MyFree( pszA );
		}
	}

	MyFree( psz );
}


//++ DllMain
EXTERN_C
BOOL WINAPI DllMain( HMODULE hInst, UINT iReason, LPVOID lpReserved )
{
	if ( iReason == DLL_PROCESS_ATTACH ) {
		PluginInit( hInst );
	} else if ( iReason == DLL_PROCESS_DETACH ) {
		PluginUninit();
	}
	return TRUE;
}
