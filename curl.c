
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
		// Default user agent
		TCHAR szBuf[MAX_PATH] = _T( "" ), szVer[MAX_PATH];
		GetModuleFileName( g_hInst, szBuf, ARRAYSIZE( szBuf ) );
		ReadVersionInfoString( szBuf, _T( "FileVersion" ), szVer, ARRAYSIZE( szVer ) );
	#if _UNICODE
		_snprintf( g.szUserAgent, ARRAYSIZE( g.szUserAgent ), "nscurl/%ws", szVer );
	#else
		_snprintf( g.szUserAgent, ARRAYSIZE( g.szUserAgent ), "nscurl/%s", szVer );
	#endif
	}

	// Extract embedded cacert.pem to $PLUGINSDIR
	//e = CurlExtractCacert();

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


//++ write_to_file_curl_callback
size_t write_to_file_curl_callback( char *ptr, size_t size, size_t nmemb, void *userdata )
{
	HANDLE h = (HANDLE)userdata;
	if (VALID_HANDLE( h )) {
		ULONG iWritten = 0;
		if (WriteFile( h, ptr, (ULONG)(size * nmemb), &iWritten, NULL ))
			return iWritten;
	}
	return 0;
}


//++ UpdateCacertPem
//?  Download the latest cacert.pem from its website
CURLcode UpdateCacertPem()
{
	CURLcode e = CURLE_OK;

	CHAR szError[CURL_ERROR_SIZE] = "";		/// Runtime error buffer
	int  iHttpStatus = 0;

	CHAR szCacert[MAX_PATH], szCacert2[MAX_PATH];
	HANDLE hFile = INVALID_HANDLE_VALUE;

#if _UNICODE
	_snprintf( szCacert,  ARRAYSIZE( szCacert ),  "%ws\\cacert.pem",  getuservariableEx( INST_PLUGINSDIR ) );
	_snprintf( szCacert2, ARRAYSIZE( szCacert2 ), "%ws\\cacert2.pem", getuservariableEx( INST_PLUGINSDIR ) );
#else
	_snprintf( szCacert,  ARRAYSIZE( szCacert ),  "%s\\cacert.pem",  getuservariableEx( INST_PLUGINSDIR ) );
	_snprintf( szCacert2, ARRAYSIZE( szCacert2 ), "%s\\cacert2.pem", getuservariableEx( INST_PLUGINSDIR ) );
#endif

	// Create destination file
	hFile = CreateFileA( szCacert2, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if (hFile != INVALID_HANDLE_VALUE) {

		// Download latest cacert.pem
		CURL *curl = curl_easy_init();
		if (curl) {

			curl_easy_setopt( curl, CURLOPT_USERAGENT, g.szUserAgent );

			curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, TRUE );			/// Follow redirects
			curl_easy_setopt( curl, CURLOPT_MAXREDIRS, 2 );

			/// SSL
			if (FileExistsA( szCacert )) {
				curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, TRUE );		/// Verify SSL certificate
				curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2 );		/// Validate host name
				curl_easy_setopt( curl, CURLOPT_CAINFO, szCacert );			/// cacert.pem path
			} else {
				curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, FALSE );
			}

			/// Error buffer
			szError[0] = ANSI_NULL;
			curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, szError );

			/// GET
			curl_easy_setopt( curl, CURLOPT_HTTPGET, TRUE );

			/// Callbacks
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_to_file_curl_callback );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, hFile );				/// Callback context (userdata)

																			/// URL
			curl_easy_setopt( curl, CURLOPT_URL, "https://curl.haxx.se/ca/cacert.pem" );

			/// Transfer
			e = curl_easy_perform( curl );

			/// Error information
			curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, (PLONG)&iHttpStatus );	/// ...might not be available
			if (!*szError)
				lstrcpynA( szError, curl_easy_strerror( e ), ARRAYSIZE( szError ) );

			// Destroy
			curl_easy_cleanup( curl );

		} else {
			e = CURLE_OUT_OF_MEMORY;
		}

		// Close and flush
		CloseHandle( hFile );

	} else {
		e = GetLastError();
		// TODO: Format error
	}

	// Replace old cacert.pem
	if (e == CURLE_OK) {
		e = DeleteFileA( szCacert ) ? ERROR_SUCCESS : GetLastError();
		if (e == ERROR_SUCCESS)
			e = MoveFileA( szCacert2, szCacert ) ? ERROR_SUCCESS : GetLastError();
		if (e != ERROR_SUCCESS) {
			// TODO: Format error
		}
	}

	// "OK"
	if (e == CURLE_OK)
		lstrcpynA( szError, "OK", ARRAYSIZE( szError ) );

	return e;
}

