
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#include "main.h"
#include "curl.h"


typedef struct {
	CRITICAL_SECTION		Lock;
	HANDLE					hTerm;
	HANDLE					hSema;
	CHAR					szUserAgent[128];
} CURL_GLOBALS;


CURL_GLOBALS g = {0};
#define DEFAULT_HEADERS_VIRTUAL_SIZE	1024 * 128				/// 128KB
#define DEFAULT_UKNOWN_VIRTUAL_SIZE		1024 * 1024 * 200		/// 200MB


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


//++ CurlParseRequestParam
BOOL CurlParseRequestParam( _In_ LPTSTR pszParam, _In_ int iParamMaxLen, _Out_ PCURL_REQUEST pParam )
{
	BOOL bRet = TRUE;
	assert( iParamMaxLen && pszParam && pParam );

	if (lstrcmpi( pszParam, _T( "/URL" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszURL );
			pParam->pszURL = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/TO" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszPath );
			if (lstrcmpi( pszParam, _T( "MEMORY" ) ) != 0)
				pParam->pszPath = MyStrDup( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/METHOD" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
		//x	assert(
		//x		lstrcmpi( pszParam, _T( "GET" ) ) == 0 ||
		//x		lstrcmpi( pszParam, _T( "POST" ) ) == 0 ||
		//x		lstrcmpi( pszParam, _T( "HEAD" ) ) == 0
		//x	);
			MyFree( pParam->pszMethod );
			pParam->pszMethod = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/HEADERS" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszHeaders );
			pParam->pszHeaders = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/DATA" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pData );
			pParam->pData = MyStrDup( pszParam );
			pParam->iDataSize = lstrlenA( (LPCSTR)pParam->pData );
		}
	} else if (lstrcmpi( pszParam, _T( "/DATAFILE" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			HANDLE hFile = CreateFile( pszParam, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
			if (hFile != INVALID_HANDLE_VALUE) {
				ULONG iFileSize = GetFileSize( hFile, NULL );
				if (iFileSize != INVALID_FILE_SIZE || GetLastError() == ERROR_SUCCESS) {
					MyFree( pParam->pData );
					pParam->iDataSize = 0;
					pParam->pData = MyAlloc( iFileSize );
					if (pParam->pData) {
						if (!ReadFile( hFile, pParam->pData, iFileSize, &pParam->iDataSize, NULL )) {
							MyFree( pParam->pData );
							pParam->iDataSize = 0;
							assert( !"/DATAFILE: Failed to read" );
						}
					} else {
						assert( !"/DATAFILE: Failed to allocate memory" );
					}
				} else {
					assert( !"/DATAFILE: Failed to get size" );
				}
				CloseHandle( hFile );
			} else {
				assert( !"/DATAFILE: Failed to open" );
			}
		}
	} else if (lstrcmpi( pszParam, _T( "/CONNECTTIMEOUT" ) ) == 0) {
		pParam->iConnectTimeout = popint();
	} else if (lstrcmpi( pszParam, _T( "/COMPLETETIMEOUT" ) ) == 0) {
		pParam->iCompleteTimeout = popint();
	} else if (lstrcmpi( pszParam, _T( "/PROXY" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszProxy );
			pParam->pszProxy = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/PROXYUSER" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszProxyUser );
			pParam->pszProxyUser = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/PROXYPASS" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszProxyPass );
			pParam->pszProxyPass = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/REFERER" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszReferrer );
			pParam->pszReferrer = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/USERAGENT" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszAgent );
			pParam->pszAgent = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/NOREDIRECT" ) ) == 0) {
		pParam->bNoRedirect = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/INSECURE" ) ) == 0) {
		pParam->bInsecure = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/CACERT" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pParam->pszCacert );
			pParam->pszCacert = MyStrDupA( pszParam );
		}
	} else {
		bRet = FALSE;	/// This parameter is not valid for Request
	}

	return bRet;
}


//++ CurlHeaderCallback
size_t CurlHeaderCallback( char *buffer, size_t size, size_t nitems, void *userdata )
{
	PCURL_REQUEST pReq = (PCURL_REQUEST)userdata;

	// Collect status line
	LPCSTR psz1, psz2;
	for (psz1 = psz2 = buffer; (*psz2 != '\0') && (*psz2 != '\r') && (*psz2 != '\n'); psz2++);
	if (psz2 > psz1) {
		if (!pReq->Runtime.bHttpStatus) {
			/// "HTTP/1.1 200 OK" -> "OK"
			for (; (*psz1 != ANSI_NULL) && (*psz1 != ' '); psz1++);		/// Find space #1
			for (; (*psz1 == ' '); psz1++);								/// Skip space #1
			for (; (*psz1 != ANSI_NULL) && (*psz1 != ' '); psz1++);		/// Find space #2
			for (; (*psz1 == ' '); psz1++);								/// Skip space #2
			/// Collect
			MyFree( pReq->Runtime.pszHttpStatus );
			if ((pReq->Runtime.pszHttpStatus = (LPCSTR)MyAlloc( (ULONG)(psz2 - psz1) + 1 )) != NULL)
				lstrcpynA( (LPSTR)pReq->Runtime.pszHttpStatus, psz1, (int)(psz2 - psz1) + 1 );
		}
		pReq->Runtime.bHttpStatus = TRUE;
	} else {
		/// Empty header
		/// A new header block may be received, in which case we'll collect its status line again
		pReq->Runtime.bHttpStatus = FALSE;
	}

	// Collect headers
	return VirtualMemoryWrite( &pReq->Runtime.OutHeaders, buffer, nitems * size );
}


//++ CurlWriteCallback
size_t CurlWriteCallback( char *ptr, size_t size, size_t nmemb, void *userdata )
{
	PCURL_REQUEST pReq = (PCURL_REQUEST)userdata;
	assert( pReq && pReq->Runtime.pCurl );

	if (VALID_HANDLE( pReq->Runtime.hFile )) {
		// Write to output file
		ULONG iWritten = 0;
		if (WriteFile( pReq->Runtime.hFile, ptr, (ULONG)(size * nmemb), &iWritten, NULL ))
			return iWritten;
	} else {
		// Initialize output virtual memory (once)
		if (!pReq->Runtime.OutData.pMem) {
			curl_off_t iMaxSize;
			if (curl_easy_getinfo( pReq->Runtime.pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &iMaxSize ) != CURLE_OK)
				iMaxSize = DEFAULT_UKNOWN_VIRTUAL_SIZE;
			VirtualMemoryInitialize( &pReq->Runtime.OutData, (SIZE_T)iMaxSize );
		}
		// Write to RAM
		return VirtualMemoryWrite( &pReq->Runtime.OutData, ptr, size * nmemb );
	}

	return 0;
}


//++ CurlTransfer
void CurlTransfer( _In_ PCURL_REQUEST pReq )
{
	CURL *curl;
	CHAR szError[CURL_ERROR_SIZE] = "";		/// Runtime error buffer
	curl_off_t iResumeFrom = 0;

	if (!pReq || !pReq->pszURL || !*pReq->pszURL)
		return;

	// Create destination file
	if (pReq->pszPath && *pReq->pszPath) {
		pReq->Runtime.hFile = CreateFile( pReq->pszPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if (VALID_HANDLE( pReq->Runtime.hFile )) {
			LARGE_INTEGER l;
			if (GetFileSizeEx( pReq->Runtime.hFile, &l )) {
				iResumeFrom = l.QuadPart;
				if (SetFilePointer( pReq->Runtime.hFile, 0, NULL, FILE_END ) == INVALID_SET_FILE_POINTER) {
					ULONG e = GetLastError();	// TODO: Store
				}
			} else {
				ULONG e = GetLastError();	// TODO: Store
			}
		}
	}

	// Transfer
	curl = curl_easy_init();	// TODO: Cache
	if (curl) {

		/// Error buffer
		szError[0] = ANSI_NULL;
		curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, szError );

		curl_easy_setopt( curl, CURLOPT_USERAGENT, pReq->pszAgent ? pReq->pszAgent : g.szUserAgent );
		if (pReq->pszReferrer)
			curl_easy_setopt( curl, CURLOPT_REFERER, pReq->pszReferrer );

		if (pReq->bNoRedirect) {
			curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, FALSE );
		} else {
			curl_easy_setopt( curl, CURLOPT_FOLLOWLOCATION, TRUE );			/// Follow redirects
			curl_easy_setopt( curl, CURLOPT_MAXREDIRS, 10 );
		}

		if (pReq->iConnectTimeout > 0)
			curl_easy_setopt( curl, CURLOPT_CONNECTTIMEOUT_MS, pReq->iConnectTimeout );
		if (pReq->iCompleteTimeout > 0)
			curl_easy_setopt( curl, CURLOPT_TIMEOUT_MS, pReq->iCompleteTimeout );

		/// SSL
		if (!pReq->bInsecure) {
			CHAR szCacert[MAX_PATH];
			if (pReq->pszCacert) {
				lstrcpynA( szCacert, pReq->pszCacert, ARRAYSIZE( szCacert ) );
			} else {
			#if _UNICODE
				_snprintf( szCacert, ARRAYSIZE( szCacert ), "%ws\\cacert.pem", getuservariableEx( INST_PLUGINSDIR ) );
			#else
				_snprintf( szCacert, ARRAYSIZE( szCacert ), "%s\\cacert.pem", getuservariableEx( INST_PLUGINSDIR ) );
			#endif
			}
			if (FileExistsA( szCacert )) {
				curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, TRUE );		/// Verify SSL certificate
				curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2 );		/// Validate host name
				curl_easy_setopt( curl, CURLOPT_CAINFO, szCacert );			/// cacert.pem path
			} else {
				curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, FALSE );
			}
		} else {
			curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, FALSE );
		}

		/// GET
		curl_easy_setopt( curl, CURLOPT_HTTPGET, TRUE );

		/// Resume
		curl_easy_setopt( curl, CURLOPT_RESUME_FROM_LARGE, iResumeFrom );

		/// Callbacks
		VirtualMemoryInitialize( &pReq->Runtime.OutHeaders, DEFAULT_HEADERS_VIRTUAL_SIZE );
		curl_easy_setopt( curl, CURLOPT_HEADERFUNCTION, CurlHeaderCallback );
		curl_easy_setopt( curl, CURLOPT_HEADERDATA, pReq );
		curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback );
		curl_easy_setopt( curl, CURLOPT_WRITEDATA, pReq );

		/// URL
		curl_easy_setopt( curl, CURLOPT_URL, pReq->pszURL );

		/// Transfer
		pReq->Runtime.pCurl = curl;
		pReq->Runtime.iCurlError = curl_easy_perform( curl );

		/// Error information
		pReq->Runtime.pszCurlError = MyStrDupAA( *szError ? szError : curl_easy_strerror( pReq->Runtime.iCurlError ) );
		curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, (PLONG)&pReq->Runtime.iHttpStatus );	/// ...might not be available

		// Destroy
		if (VALID_HANDLE( pReq->Runtime.hFile ))
			CloseHandle( pReq->Runtime.hFile ), pReq->Runtime.hFile = NULL;
		curl_easy_cleanup( curl );		// TODO: Clear all options + Return to cache
	}
}
