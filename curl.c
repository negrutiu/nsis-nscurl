
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#include "main.h"
#include "curl.h"
#include "utils.h"

CURL_GLOBALS g = {0};
ULONG CurlExtractCacert();


//++ CurlInitialize
ULONG CurlInitialize()
{
	ULONG e = ERROR_SUCCESS, nThreads;
	SYSTEM_INFO si;

	if (g.hTerm)
		return ERROR_SUCCESS;

	GetSystemInfo( &si );
	nThreads = si.dwNumberOfProcessors * 2;
	nThreads = __max( nThreads, 4 );
	nThreads = __min( nThreads, 64 );

	InitializeCriticalSection( &g.Lock );
	g.hTerm = CreateEvent( NULL, TRUE, FALSE, NULL );
	g.hSema = CreateSemaphore( NULL, nThreads, nThreads, NULL );

	{
		// User agent
		TCHAR szBuf[MAX_PATH] = _T( "" );
		GetModuleFileName( g_hInst, szBuf, ARRAYSIZE( szBuf ) );
		ReadVersionInfoString( szBuf, _T( "FileVersion" ), szBuf, ARRAYSIZE( szBuf ) );
	#if _UNICODE
		_snprintf( g.szUserAgent, ARRAYSIZE( g.szUserAgent ), "nscurl/%ws", szBuf );
	#else
		_snprintf( g.szUserAgent, ARRAYSIZE( g.szUserAgent ), "nscurl/%s", szBuf );
	#endif
	}

	// Extract embedded cacert.pem to $PLUGINSDIR
	//e = CurlExtractCacert();

	{
		U8LIST l;
		InitializeU8List( &l );
		AddU8ListA( &l, "Pula1" );
		AddU8ListW( &l, L"Boi" );
		AddU8ListA( &l, "Pula2" );
		AddU8ListW( &l, L"Cavalo" );
		DestroyU8List( &l );
		l.Next = l.Next;
	}

	return e;
}

//++ CurlDestroy
void CurlDestroy()
{
	if (g.hSema)
		CloseHandle( g.hSema ), g.hSema = NULL;
	if (g.hTerm) {
		CloseHandle( g.hTerm ), g.hTerm = NULL;
		DeleteCriticalSection( &g.Lock );
	}
}


//++ CurlExtractCacert
//?  Extracts $PLUGINSDIR\cacert.pem from plugin's resource block
//?  If the file already exists the does nothing
ULONG CurlExtractCacert()
{
	ULONG e = ERROR_SUCCESS;

	assert( g_hInst != NULL );
	assert( g_variables != NULL );

	{
		TCHAR szPem[MAX_PATH];
		_sntprintf( szPem, ARRAYSIZE( szPem ), _T( "%s\\cacert.pem" ), getuservariableEx( INST_PLUGINSDIR ) );
		if (!FileExists( szPem ))
			e = ExtractResourceFile( (HMODULE)g_hInst, _T( "cacert.pem" ), MAKEINTRESOURCE( 1 ), 1033, szPem );
	}

	return e;
}
