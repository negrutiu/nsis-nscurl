
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#include "main.h"
#include "curl.h"


typedef struct {
	CHAR					szUserAgent[128];
} CURL_GLOBALS;


CURL_GLOBALS g_Curl = {0};
#define DEFAULT_HEADERS_VIRTUAL_SIZE	1024 * 128				/// 128KB
#define DEFAULT_UKNOWN_VIRTUAL_SIZE		1024 * 1024 * 200		/// 200MB


//++ CurlInitialize
ULONG CurlInitialize()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );
	{
		// Default user agent
		TCHAR szBuf[MAX_PATH] = _T( "" ), szVer[MAX_PATH];
		GetModuleFileName( g_hInst, szBuf, ARRAYSIZE( szBuf ) );
		ReadVersionInfoString( szBuf, _T( "FileVersion" ), szVer, ARRAYSIZE( szVer ) );
	#if _UNICODE
		_snprintf( g_Curl.szUserAgent, ARRAYSIZE( g_Curl.szUserAgent ), "nscurl/%ws", szVer );
	#else
		_snprintf( g_Curl.szUserAgent, ARRAYSIZE( g_Curl.szUserAgent ), "nscurl/%s", szVer );
	#endif
	}

	return ERROR_SUCCESS;
}


//++ CurlDestroy
void CurlDestroy()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );
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
BOOL CurlParseRequestParam( _In_ LPTSTR pszParam, _In_ int iParamMaxLen, _Out_ PCURL_REQUEST pReq )
{
	BOOL bRet = TRUE;
	assert( iParamMaxLen && pszParam && pReq );

	if (lstrcmpi( pszParam, _T( "/URL" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszURL );
			pReq->pszURL = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/TO" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszPath );
			if (lstrcmpi( pszParam, _T( "MEMORY" ) ) != 0)
				pReq->pszPath = MyStrDup( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/METHOD" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
		//x	assert(
		//x		lstrcmpi( pszParam, _T( "GET" ) ) == 0 ||
		//x		lstrcmpi( pszParam, _T( "POST" ) ) == 0 ||
		//x		lstrcmpi( pszParam, _T( "HEAD" ) ) == 0
		//x	);
			MyFree( pReq->pszMethod );
			pReq->pszMethod = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/HEADER" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			// The string may contain multiple headers delimited by \r\n
			LPTSTR psz1, psz2;
			TCHAR ch;
			LPSTR pszA;
			UNREFERENCED_PARAMETER( pszA );
			for (psz1 = pszParam; *psz1; ) {
				for (; (*psz1 == _T('\r')) || (*psz1 == _T('\n')); psz1++);			/// Skip \r\n
				for (psz2 = psz1; (*psz2 != _T('\0')) && (*psz2 != _T('\r')) && (*psz2 != _T('\n')); psz2++);		/// Find next \r\n\0
				if (psz2 > psz1) {
					ch = *psz2, *psz2 = _T( '\0' );
				#ifdef _UNICODE
					if ((pszA = MyStrDupAW( psz1 )) != NULL) {
						pReq->pInHeaders = curl_slist_append( pReq->pInHeaders, pszA );
						MyFree( pszA );
					}
				#else
					pReq->pInHeaders = curl_slist_append( pReq->pInHeaders, psz1 );
				#endif
					*psz2 = ch;
					psz1 = psz2;
				}
			}
		}
	} else if (lstrcmpi( pszParam, _T( "/POSTVAR" ) ) == 0) {
		LPSTR pszFilename = NULL, pszType = NULL, pszName = NULL, pszData = NULL;
		/// Extract optional parameters "filename=XXX" and "type=XXX"
		int e = NOERROR;
		while (e == NOERROR) {
			if ((e = popstring( pszParam )) == NOERROR) {
				if (CompareString( CP_ACP, NORM_IGNORECASE, pszParam, 9, _T( "filename=" ), -1 ) == CSTR_EQUAL) {
					pszFilename = MyStrDupA( pszParam + 9 );
				} else if (CompareString( CP_ACP, NORM_IGNORECASE, pszParam, 5, _T( "type=" ), -1 ) == CSTR_EQUAL) {
					pszType = MyStrDupA( pszParam + 5 );
				} else {
					break;
				}
			}
		}
		/// Extract mandatory parameters "name" and "data|@datafile"
		if (e == NOERROR) {
			pszName = MyStrDupA( pszParam );
			if ((e = popstring( pszParam )) == NOERROR) {
				pszData = MyStrDupA( pszParam );

				// Store 4-tuple MIME form part
				pReq->pPostVars = curl_slist_append( pReq->pPostVars, pszFilename ? pszFilename : "" );
				pReq->pPostVars = curl_slist_append( pReq->pPostVars, pszType ? pszType : "" );
				pReq->pPostVars = curl_slist_append( pReq->pPostVars, pszName ? pszName : "" );
				pReq->pPostVars = curl_slist_append( pReq->pPostVars, pszData ? pszData : "" );
			}
		}
		MyFree( pszFilename );
		MyFree( pszType );
		MyFree( pszName );
		MyFree( pszData );
	} else if (lstrcmpi( pszParam, _T( "/DATA" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszData );
			if (pszParam[0] == _T( '@' )) {
				pReq->pszData = MyStrDup( pszParam + 1 );		/// Data file name
				pReq->iDataSize = 0;
			} else {
				pReq->pszData = MyStrDupA( pszParam );			/// Data string
				pReq->iDataSize = lstrlenA( pReq->pszData );
			}
		}
	} else if (lstrcmpi( pszParam, _T( "/CONNECTTIMEOUT" ) ) == 0 || lstrcmpi( pszParam, _T( "/TIMEOUT" ) ) == 0) {
		pReq->iConnectTimeout = popint();
	} else if (lstrcmpi( pszParam, _T( "/COMPLETETIMEOUT" ) ) == 0) {
		pReq->iCompleteTimeout = popint();
	} else if (lstrcmpi( pszParam, _T( "/PROXY" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszProxy );
			pReq->pszProxy = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/PROXYUSER" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszProxyUser );
			pReq->pszProxyUser = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/PROXYPASS" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszProxyPass );
			pReq->pszProxyPass = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/REFERER" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszReferrer );
			pReq->pszReferrer = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/USERAGENT" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszAgent );
			pReq->pszAgent = MyStrDupA( pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/NOREDIRECT" ) ) == 0) {
		pReq->bNoRedirect = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/INSECURE" ) ) == 0) {
		pReq->bInsecure = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/CACERT" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			MyFree( pReq->pszCacert );
			pReq->pszCacert = MyStrDupA( pszParam );
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
		if (!pReq->Error.bStatusGrabbed) {
			/// "HTTP/1.1 200 OK" -> "OK"
			for (; (*psz1 != ANSI_NULL) && (*psz1 != ' '); psz1++);		/// Find space #1
			for (; (*psz1 == ' '); psz1++);								/// Skip space #1
			for (; (*psz1 != ANSI_NULL) && (*psz1 != ' '); psz1++);		/// Find space #2
			for (; (*psz1 == ' '); psz1++);								/// Skip space #2
			/// Collect
			MyFree( pReq->Error.pszHttp );
			if ((pReq->Error.pszHttp = (LPCSTR)MyAlloc( (ULONG)(psz2 - psz1) + 1 )) != NULL)
				lstrcpynA( (LPSTR)pReq->Error.pszHttp, psz1, (int)(psz2 - psz1) + 1 );
		}
		pReq->Error.bStatusGrabbed = TRUE;
	} else {
		/// Empty header
		/// A new header block may be received, in which case we'll collect its status line again
		pReq->Error.bStatusGrabbed = FALSE;
	}

	// Collect headers
	return VirtualMemoryAppend( &pReq->Runtime.OutHeaders, buffer, nitems * size );
}


//++ CurlReadCallback
size_t CurlReadCallback( char *buffer, size_t size, size_t nitems, void *instream )
{
	curl_off_t l = 0;
	PCURL_REQUEST pReq = (PCURL_REQUEST)instream;

	assert( pReq && pReq->Runtime.pCurl );

	if (pReq->pszData) {	/// Either data buffer, or, file name
		if (VALID_HANDLE( pReq->Runtime.hInFile )) {
			// Read from input file
			ULONG iRead;
			if (ReadFile( pReq->Runtime.hInFile, (LPVOID)buffer, size * nitems, &iRead, NULL )) {
				l = iRead;
				pReq->Runtime.iDataPos += iRead;
			} else {
				l = CURL_READFUNC_ABORT;
			}
		} else {
			// Read from input buffer
			assert( pReq->Runtime.iDataPos <= pReq->iDataSize );
			l = __min( size * nitems, pReq->iDataSize - pReq->Runtime.iDataPos );
			CopyMemory( buffer, (PCCH)pReq->pszData + pReq->Runtime.iDataPos, (size_t)l );
			pReq->Runtime.iDataPos += l;
		}
	}

	return (size_t)l;
}


//++ CurlWriteCallback
size_t CurlWriteCallback( char *ptr, size_t size, size_t nmemb, void *userdata )
{
	PCURL_REQUEST pReq = (PCURL_REQUEST)userdata;
	assert( pReq && pReq->Runtime.pCurl );

	if (VALID_HANDLE( pReq->Runtime.hOutFile )) {
		// Write to output file
		ULONG iWritten = 0;
		if (WriteFile( pReq->Runtime.hOutFile, ptr, (ULONG)(size * nmemb), &iWritten, NULL )) {
			return iWritten;
		} else {
			pReq->Error.iWin32 = GetLastError();
			pReq->Error.pszWin32 = MyErrorStr( pReq->Error.iWin32 );
		}
	} else {
		// Initialize output virtual memory (once)
		if (!pReq->Runtime.OutData.pMem) {
			curl_off_t iMaxSize;
			if (curl_easy_getinfo( pReq->Runtime.pCurl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &iMaxSize ) != CURLE_OK)
				iMaxSize = DEFAULT_UKNOWN_VIRTUAL_SIZE;
			VirtualMemoryInitialize( &pReq->Runtime.OutData, (SIZE_T)iMaxSize );
		}
		// Write to RAM
		return VirtualMemoryAppend( &pReq->Runtime.OutData, ptr, size * nmemb );
	}

	return 0;
}


//++ CurlProgressCallback
int CurlProgressCallback( void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow )
{
	PCURL_REQUEST pReq = (PCURL_REQUEST)clientp;
	assert( pReq && pReq->Runtime.pCurl );

	if (TermSignaled())
		return CURLE_ABORTED_BY_CALLBACK;

	if (InterlockedCompareExchange(&pReq->Queue.iFlagAbort, -1, -1) != FALSE)
		return CURLE_ABORTED_BY_CALLBACK;

	return CURLE_OK;
}


//++ CurlTransfer
void CurlTransfer( _In_ PCURL_REQUEST pReq )
{
	CURL *curl;
	curl_mime *form = NULL;
	CHAR szError[CURL_ERROR_SIZE] = "";		/// Runtime error buffer
	curl_off_t iResumeFrom = 0;

	if (!pReq)
		return;
	if (!pReq->pszURL || !*pReq->pszURL) {
		pReq->Error.iWin32 = ERROR_INVALID_PARAMETER;
		pReq->Error.pszWin32 = MyErrorStr( pReq->Error.iWin32 );
		return;
	}

	// Input file
	if (pReq->pszMethod && (
		lstrcmpiA( pReq->pszMethod, "PUT" ) == 0 ||
		(lstrcmpiA( pReq->pszMethod, "POST" ) == 0 && !pReq->pPostVars)
		))
	{
		if (pReq->iDataSize == 0 && pReq->pszData && *(LPCTSTR)pReq->pszData) {
			ULONG e = ERROR_SUCCESS;
			pReq->Runtime.hInFile = CreateFile( (LPCTSTR)pReq->pszData, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL );
			if (VALID_HANDLE( pReq->Runtime.hInFile )) {
				// NOTE: kernel32!GetFileSizeEx is only available in XP+
				LARGE_INTEGER l;
				l.LowPart = GetFileSize( pReq->Runtime.hInFile, &l.HighPart );
				if (l.LowPart != INVALID_FILE_SIZE) {
					/// Store file size in iDataSize
					pReq->iDataSize = l.QuadPart;
				} else {
					e = GetLastError();
				}
			} else {
				e = GetLastError();
			}
			if (e != ERROR_SUCCESS && pReq->Error.iWin32 == ERROR_SUCCESS) {
				pReq->Error.iWin32 = e;
				pReq->Error.pszWin32 = MyErrorStr( e );
				TRACE( _T( "[!] CreateFile( DataFile:%s ) = %s\n" ), (LPCTSTR)pReq->pszData, pReq->Error.pszWin32 );
			}
		}
	}

	// Output file
	if (pReq->pszPath && *pReq->pszPath) {
		ULONG e = ERROR_SUCCESS;
		pReq->Runtime.hOutFile = CreateFile( pReq->pszPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if (VALID_HANDLE( pReq->Runtime.hOutFile )) {
			LARGE_INTEGER l;
			l.LowPart = GetFileSize( pReq->Runtime.hOutFile, &l.HighPart );
			if (l.LowPart != INVALID_FILE_SIZE) {
				iResumeFrom = l.QuadPart;
				if (SetFilePointer( pReq->Runtime.hOutFile, 0, NULL, FILE_END ) == INVALID_SET_FILE_POINTER) {
					e = GetLastError();
				}
			} else {
				e = GetLastError();
			}
		} else {
			e = GetLastError();
		}
		if (e != ERROR_SUCCESS && pReq->Error.iWin32 == ERROR_SUCCESS) {
			pReq->Error.iWin32 = e;
			pReq->Error.pszWin32 = MyErrorStr( e );
			TRACE( _T( "[!] CreateFile( OutFile:%s ) = %s\n" ), pReq->pszPath, pReq->Error.pszWin32 );
		}
	}

	if (pReq->Error.iWin32 == ERROR_SUCCESS) {

		// Transfer
		curl = curl_easy_init();	// TODO: Cache
		if (curl) {

			/// Error buffer
			szError[0] = ANSI_NULL;
			curl_easy_setopt( curl, CURLOPT_ERRORBUFFER, szError );

			curl_easy_setopt( curl, CURLOPT_USERAGENT, pReq->pszAgent ? pReq->pszAgent : g_Curl.szUserAgent );
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

			/// Request method
			if (!pReq->pszMethod || !*pReq->pszMethod ||
				lstrcmpiA( pReq->pszMethod, "GET" ) == 0)
			{

				// GET
				curl_easy_setopt( curl, CURLOPT_HTTPGET, TRUE );

			} else if (lstrcmpiA( pReq->pszMethod, "POST" ) == 0) {

				// POST
				curl_easy_setopt( curl, CURLOPT_POST, TRUE );
				if (pReq->pPostVars) {
					/// Send variables as multi-part MIME form (CURLOPT_MIMEPOST, "multipart/form-data")
					form = curl_mime_init( curl );
					if (form) {
						struct curl_slist *p;
						for (p = pReq->pPostVars; p; p = p->next) {
							curl_mimepart *part = curl_mime_addpart( form );
							if (part) {
								/// String 1
								if (p && p->data && *p->data)
									curl_mime_filename( part, p->data );
								/// String 2
								p = p->next;
								assert( p && p->data );
								if (p && p->data && *p->data)
									curl_mime_type( part, p->data );
								/// String 3
								p = p->next;
								assert( p && p->data );
								if (p && p->data && *p->data)
									curl_mime_name( part, p->data );
								/// String 4
								p = p->next;
								assert( p && p->data );
								if (p && p->data && *p->data) {
									if (p->data[0] == '@') {
										curl_mime_filedata( part, p->data + 1 );				/// Data file
									} else {
										curl_mime_data( part, p->data, CURL_ZERO_TERMINATED );	/// Data string
									}
								}
							}
						}
					}
					curl_easy_setopt( curl, CURLOPT_MIMEPOST, form );
				} else {
					/// Send input data as raw form (CURLOPT_POSTFIELDS, "application/x-www-form-urlencoded")
					//! The caller is responsible to format/escape the input data
					curl_easy_setopt( curl, CURLOPT_POSTFIELDSIZE_LARGE, pReq->iDataSize );
				}

			} else if (lstrcmpiA( pReq->pszMethod, "HEAD" ) == 0) {

				// HEAD
				curl_easy_setopt( curl, CURLOPT_NOBODY, TRUE );

			} else if (lstrcmpiA( pReq->pszMethod, "PUT" ) == 0) {

				// PUT
				curl_easy_setopt( curl, CURLOPT_PUT, TRUE );
				curl_easy_setopt( curl, CURLOPT_INFILESIZE_LARGE, pReq->iDataSize );		/// "Content-Length: <filesize>" header is mandatory in HTTP/1.x

			} else {

				// DELETE, OPTIONS, TRACE, etc.
				curl_easy_setopt( curl, CURLOPT_CUSTOMREQUEST, pReq->pszMethod );
			}

			/// Request Headers
			if (pReq->pInHeaders)
				curl_easy_setopt( curl, CURLOPT_HTTPHEADER, pReq->pInHeaders );

			// TODO: PROXY

			/// Resume
			curl_easy_setopt( curl, CURLOPT_RESUME_FROM_LARGE, iResumeFrom );

			/// Callbacks
			VirtualMemoryInitialize( &pReq->Runtime.OutHeaders, DEFAULT_HEADERS_VIRTUAL_SIZE );
			curl_easy_setopt( curl, CURLOPT_HEADERFUNCTION, CurlHeaderCallback );
			curl_easy_setopt( curl, CURLOPT_HEADERDATA, pReq );
			curl_easy_setopt( curl, CURLOPT_READFUNCTION, CurlReadCallback );
			curl_easy_setopt( curl, CURLOPT_READDATA, pReq );
			curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback );
			curl_easy_setopt( curl, CURLOPT_WRITEDATA, pReq );
			curl_easy_setopt( curl, CURLOPT_XFERINFOFUNCTION, CurlProgressCallback );
			curl_easy_setopt( curl, CURLOPT_XFERINFODATA, pReq );
			curl_easy_setopt( curl, CURLOPT_NOPROGRESS, FALSE );	/// Activate progress callback function

			/// URL
			curl_easy_setopt( curl, CURLOPT_URL, pReq->pszURL );

			/// Transfer
			pReq->Runtime.pCurl = curl;
			pReq->Error.iCurl = curl_easy_perform( curl );

			/// Error information
			pReq->Error.pszCurl = MyStrDupAA( *szError ? szError : curl_easy_strerror( pReq->Error.iCurl ) );
			curl_easy_getinfo( curl, CURLINFO_RESPONSE_CODE, (PLONG)&pReq->Error.iHttp );	/// ...might not be available

			// Cleanup
			curl_easy_reset( curl );
			curl_easy_cleanup( curl );		// TODO: Return to cache
		}

		curl_mime_free( form );
	}

	// Cleanup
	if (VALID_HANDLE( pReq->Runtime.hInFile ))
		CloseHandle( pReq->Runtime.hInFile ), pReq->Runtime.hInFile = NULL;
	if (VALID_HANDLE( pReq->Runtime.hOutFile ))
		CloseHandle( pReq->Runtime.hOutFile ), pReq->Runtime.hOutFile = NULL;
}


//++ CurlRequestFormatError
void CurlRequestFormatError( _In_ PCURL_REQUEST pReq, _In_ LPTSTR pszError, _In_ ULONG iErrorLen )
{
	if (pszError)
		pszError[0] = 0;
	if (pReq) {
		if (pReq->Error.iWin32 != ERROR_SUCCESS) {
			_sntprintf( pszError, iErrorLen, _T( "0x%x \"%s\"" ), pReq->Error.iWin32, pReq->Error.pszWin32 );
		} else if (pReq->Error.iCurl != CURLE_OK) {
			_sntprintf( pszError, iErrorLen, _T( "0x%x \"%hs\"" ), pReq->Error.iCurl, pReq->Error.pszCurl );
		} else {
			if (pReq->Error.iHttp >= 200 && pReq->Error.iHttp < 300) {
				_sntprintf( pszError, iErrorLen, _T( "OK" ) );
			} else {
				_sntprintf( pszError, iErrorLen, _T( "%d \"%hs\"" ), pReq->Error.iHttp, pReq->Error.pszHttp );
			}
		}
	}
}