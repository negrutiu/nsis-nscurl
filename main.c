
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/01/19

#include "main.h"
#include "curl.h"
#include "utils.h"
#include "crypto.h"


HINSTANCE g_hInst = NULL;


//++ PluginInit
BOOL PluginInit( _In_ HINSTANCE hInst )
{
	if (!g_hInst) {

		TRACE( _T( "PluginInit\n" ) );
		g_hInst = hInst;

		UtilsInitialize();
		CurlInitialize();

		return TRUE;
	}
	return FALSE;
}


//++ PluginUninit
BOOL PluginUninit()
{
	if ( g_hInst ) {

		TRACE( _T( "PluginUninit\n" ) );

		CurlDestroy();
		UtilsDestroy();

		g_hInst = NULL;
		return TRUE;
	}
	return FALSE;
}


//++ UnloadCallback
//+  USAGE: extra->RegisterPluginCallback( g_hInst, UnloadCallback );
//?  By registering UnloadCallback the framework will keep the current plugin locked in memory
//?  Otherwise, unless the caller specifies the /NOUNLOAD parameter, the plugin gets unloaded
UINT_PTR __cdecl UnloadCallback( enum NSPIM iMessage )
{
	switch ( iMessage ) {
		case NSPIM_UNLOAD:		TRACE( _T( "%hs( NSPIM_UNLOAD )\n" ),    __FUNCTION__ ); break;
		case NSPIM_GUIUNLOAD:	TRACE( _T( "%hs( NSPIM_GUIUNLOAD )\n" ), __FUNCTION__ ); break;
	}
	return 0;
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
//!  Should be called before downloading other files
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
			BinaryToHex( hash, sizeof( hash ), psz, string_size );
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
			BinaryToHex( hash, sizeof( hash ), psz, string_size );
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
			BinaryToHex( hash, sizeof( hash ), psz, string_size );
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


//++ [exported] DownloadCacert
EXTERN_C __declspec(dllexport)
void __cdecl DownloadCacert( HWND parent, int string_size, TCHAR *variables, stack_t **stacktop, extra_parameters *extra )
{
	EXDLL_INIT();
	EXDLL_VALIDATE();

	// Download latest cacert.pem
	pushint( UpdateCacertPem() );
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
