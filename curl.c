
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#include "main.h"
#include "curl.h"
#include <mbedtls/ssl.h>
#include <mbedtls/sha1.h>


typedef struct {
	CRITICAL_SECTION		csLock;
	CHAR					szVersion[64];
	CHAR					szUserAgent[128];
} CURL_GLOBALS;


CURL_GLOBALS g_Curl = {0};
#define DEFAULT_HEADERS_VIRTUAL_SIZE	1024 * 128				/// 128KB
#define DEFAULT_UKNOWN_VIRTUAL_SIZE		1024 * 1024 * 200		/// 200MB


//+ CurlRequestSizes
void CurlRequestSizes( _In_ PCURL_REQUEST pReq, _Out_opt_ PULONG64 piSizeTotal, _Out_opt_ PULONG64 piSizeXferred, _Out_opt_ PBOOL pbDown )
{
	assert( pReq );

	// Uploading data goes first. Downloading server's response (remote content) goes second
	if (pReq->Runtime.iDlXferred > 0 || pReq->Runtime.iDlTotal > 0) {
		// Downloading (phase 2)
		if (pbDown)
			*pbDown = TRUE;
		if (piSizeTotal)
			*piSizeTotal = pReq->Runtime.iDlTotal;
		if (piSizeXferred)
			*piSizeXferred = pReq->Runtime.iDlXferred;
	} else {
		// Uploading (phase 1)
		if (pbDown)
			*pbDown = FALSE;
		if (piSizeTotal)
			*piSizeTotal = pReq->Runtime.iUlTotal;
		if (piSizeXferred)
			*piSizeXferred = pReq->Runtime.iUlXferred;
	}
}


//+ CurlRequestFormatError
void CurlRequestFormatError( _In_ PCURL_REQUEST pReq, _In_opt_ LPTSTR pszError, _In_opt_ ULONG iErrorLen, _Out_opt_ PBOOLEAN pbSuccess, _Out_opt_ PULONG piErrCode )
{
	if (pszError)
		pszError[0] = 0;
	if (pbSuccess)
		*pbSuccess = TRUE;
	if (piErrCode)
		*piErrCode = 0;
	if (pReq) {
		if (pReq->Error.iWin32 != ERROR_SUCCESS) {
			// Win32 error code
			if (pbSuccess) *pbSuccess = FALSE;
			if (pszError)  _sntprintf( pszError, iErrorLen, _T( "0x%x \"%s\"" ), pReq->Error.iWin32, pReq->Error.pszWin32 );
			if (piErrCode) *piErrCode = pReq->Error.iWin32;
		} else if (pReq->Error.iCurl != CURLE_OK) {
			// CURL error
			if (pbSuccess) *pbSuccess = FALSE;
			if (pszError)  _sntprintf( pszError, iErrorLen, _T( "0x%x \"%hs\"" ), pReq->Error.iCurl, pReq->Error.pszCurl );
			if (piErrCode) *piErrCode = pReq->Error.iCurl;
		} else {
			// HTTP status
			if (piErrCode) *piErrCode = pReq->Error.iHttp;
			if ((pReq->Error.iHttp == 0) || (pReq->Error.iHttp >= 200 && pReq->Error.iHttp < 300)) {
				if (pszError)  _sntprintf( pszError, iErrorLen, _T( "OK" ) );
			} else {
				if (pbSuccess) *pbSuccess = FALSE;
				if (pszError)  _sntprintf( pszError, iErrorLen, _T( "%d \"%hs\"" ), pReq->Error.iHttp, pReq->Error.pszHttp );
			}
		}
	}
}


// ____________________________________________________________________________________________________________________________________ //
//                                                                                                                                      //


//++ CurlInitialize
ULONG CurlInitialize()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );

	InitializeCriticalSection( &g_Curl.csLock );
	{
		// Plugin version
		// Default user agent
		TCHAR szBuf[MAX_PATH] = _T( "" ), szVer[MAX_PATH];
		GetModuleFileName( g_hInst, szBuf, ARRAYSIZE( szBuf ) );
		ReadVersionInfoString( szBuf, _T( "FileVersion" ), szVer, ARRAYSIZE( szVer ) );
		MyStrCopy( T2A, g_Curl.szVersion, ARRAYSIZE( g_Curl.szVersion ), szVer );
		_snprintf( g_Curl.szUserAgent, ARRAYSIZE( g_Curl.szUserAgent ), "nscurl/%s", g_Curl.szVersion );
	}

	return ERROR_SUCCESS;
}


//++ CurlDestroy
void CurlDestroy()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );
	DeleteCriticalSection( &g_Curl.csLock );
}


//++ CurlExtractCacert
//?  Extract the built-in certificate storage to "$PLUGINSDIR\cacert.pem"
//?  The function does nothing if the file already exists
ULONG CurlExtractCacert()
{
	ULONG e = ERROR_SUCCESS;

	assert( g_hInst != NULL );
	assert( g_variables != NULL );

	{
		TCHAR szPem[MAX_PATH];
		_sntprintf( szPem, ARRAYSIZE( szPem ), _T( "%s\\cacert.pem" ), getuservariableEx( INST_PLUGINSDIR ) );
		if (!FileExists( szPem )) {

			EnterCriticalSection( &g_Curl.csLock );
			
			if (!FileExists( szPem )) 		/// Verify again
				e = ExtractResourceFile( (HMODULE)g_hInst, _T( "cacert.pem" ), MAKEINTRESOURCE( 1 ), 1033, szPem );
			
			LeaveCriticalSection( &g_Curl.csLock );
		}
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
	} else if (lstrcmpi( pszParam, _T( "/OUT" ) ) == 0) {
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
	} else if (lstrcmpi( pszParam, _T( "/RESUME" ) ) == 0) {
		pReq->bResume = TRUE;
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
	} else if (lstrcmpi( pszParam, _T( "/CERT" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR && *pszParam) {
			/// Validate SHA1 hash
			if (lstrlen( pszParam ) == 40) {
				int i;
				for (i = 0; isxdigit( pszParam[i] ); i++);
				if (i == 40) {
					LPSTR psz = MyStrDupA( pszParam );
					if (psz) {
						pReq->pCertList = curl_slist_append( pReq->pCertList, psz );
						MyFree( psz );
					}
				}
			}
		}
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



//++ MbedtlsCertVerify
int MbedtlsCertVerify(void *pParam, mbedtls_x509_crt *crt, int iChainIndex, uint32_t *piVerifyFlags)
{
	//? This function gets called during SSL negotiation to analyze server's certificate chain
	//? It'll be called sequentially for each certificate in the chain
	//? That usually happens three times per connection: 2(root certificate), 1(intermediate certificate), 0(user certificate)

	//? Our logic:
	//? * If all certificates in the chain are UNTRUSTED, we'll set the MBEDTLS_X509_BADCERT_NOT_TRUSTED verification flag when returning from the last call (index 0)
	//? * If we TRUST at least one certificate we simply do nothing (return all zeros)

	PCURL_REQUEST pReq = (PCURL_REQUEST)pParam;
	assert( !pReq->bInsecure && pReq->pCertList );

	if (!crt)
		return 1;

	// Reset all verification flags pre-calculated by libcurl
	// When validating against explicit thumbprints we don't care about the status of the certificate (expired, invalid, whatever...)
	// If the caller TRUSTs it, the connection is allowed to continue
	*piVerifyFlags = 0;

	// Don't verify any more certificates if we already found one that we TRUST
	if (!pReq->Runtime.bTrustedCert) {

		UCHAR Thumbprint[20];
		CHAR szThumbprint[41];

		// Compute certificate's thumbprint
		mbedtls_sha1_context sha1c;
		mbedtls_sha1_init( &sha1c );
		mbedtls_sha1_starts( &sha1c );
		mbedtls_sha1_update( &sha1c, crt->raw.p, crt->raw.len );
		mbedtls_sha1_finish( &sha1c, Thumbprint );
		mbedtls_sha1_free( &sha1c );
		BinaryToHexA( Thumbprint, sizeof( Thumbprint ), szThumbprint, sizeof( szThumbprint ) );

		// Verify against our list of accepted certificates
		{
			struct curl_slist *p;
			for (p = pReq->pCertList; p; p = p->next) {
				if (lstrcmpiA( p->data, szThumbprint ) == 0) {

					pReq->Runtime.bTrustedCert = TRUE;
					break;
				}
			}
			TRACE( _T( "Certificate( \"%hs\" ) = %hs\n" ), szThumbprint, p ? "TRUSTED" : "UNTRUSTED" );
		}
	}

	// Final verdict
	if (iChainIndex == 0)
		if (!pReq->Runtime.bTrustedCert)
			(*piVerifyFlags) |= MBEDTLS_X509_BADCERT_NOT_TRUSTED;

	return 0;	/// Success always
}


//++ CurlSSLCallback
//? This callback function gets called by libcurl just before the initialization of an SSL connection
CURLcode CurlSSLCallback( CURL *curl, void *ssl_ctx, void *userptr )
{
	PCURL_REQUEST pReq = (PCURL_REQUEST)userptr;
	mbedtls_ssl_config *pssl = (mbedtls_ssl_config*)ssl_ctx;

	assert( pReq && pReq->Runtime.pCurl == curl);
	assert( !pReq->bInsecure && pReq->pCertList );

	//? Setup an mbedTLS low-level callback function to analyze server's certificate chain
	pssl->f_vrfy = MbedtlsCertVerify;
	pssl->p_vrfy = pReq;

	return CURLE_OK;
}


//++ CurlHeaderCallback
size_t CurlHeaderCallback( char *buffer, size_t size, size_t nitems, void *userdata )
{
	PCURL_REQUEST pReq = (PCURL_REQUEST)userdata;
	LPCSTR psz1, psz2;

	assert( pReq && pReq->Runtime.pCurl );

	// Collect connection info
	if (!pReq->Runtime.pszServerIP) {
		/// Server IP address
		curl_easy_getinfo( pReq->Runtime.pCurl, CURLINFO_PRIMARY_IP, &psz1 );
		pReq->Runtime.pszServerIP = MyStrDupAA( psz1 );
		/// Server port
		curl_easy_getinfo( pReq->Runtime.pCurl, CURLINFO_PRIMARY_PORT, &pReq->Runtime.iServerPort );
	}

	// Collect status line
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
	curl_off_t iTotal, iXferred, iSpeed;

	assert( pReq && pReq->Runtime.pCurl );

	if (TermSignaled())
		return CURLE_ABORTED_BY_CALLBACK;

	if (InterlockedCompareExchange(&pReq->Queue.iFlagAbort, -1, -1) != FALSE)
		return CURLE_ABORTED_BY_CALLBACK;

//x	TRACE( _T( "%hs( DL:%I64u/%I64u, UL:%I64u/%I64u )\n" ), __FUNCTION__, dlnow, dltotal, ulnow, ultotal );

	if (dlnow > 0) {
		/// Downloading (phase 2)
		iTotal = dltotal, iXferred = dlnow;
		curl_easy_getinfo( pReq->Runtime.pCurl, CURLINFO_SPEED_DOWNLOAD_T, &iSpeed );
	} else {
		/// Uploading (phase 1)
		iTotal = ultotal, iXferred = ulnow;
		curl_easy_getinfo( pReq->Runtime.pCurl, CURLINFO_SPEED_UPLOAD_T, &iSpeed );
	}

	curl_easy_getinfo( pReq->Runtime.pCurl, CURLINFO_TOTAL_TIME_T, &pReq->Runtime.iTimeElapsed );
	if (iTotal == 0) {
		/// Unknown size. Unknown ETA
		pReq->Runtime.iPercent = -1;
		pReq->Runtime.iTimeRemaining = 0;
	} else {
		pReq->Runtime.iPercent = (short)((iXferred * 100) / iTotal);
		if (pReq->Runtime.iPercent >= 1) {
			pReq->Runtime.iTimeRemaining = (iTotal * pReq->Runtime.iTimeElapsed) / iXferred - pReq->Runtime.iTimeElapsed;
		} else {
			pReq->Runtime.iTimeRemaining = 0;
		}
	}

	pReq->Runtime.iDlTotal = dltotal;
	pReq->Runtime.iDlXferred = dlnow;
	pReq->Runtime.iUlTotal = ultotal;
	pReq->Runtime.iUlXferred = ulnow;
	pReq->Runtime.iSpeed = (ULONG)iSpeed;
	MemoryBarrier();

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

	// Extract "$PLUGINSDIR\cacert.pem" once
	if (!pReq->bInsecure && !pReq->pCertList && !pReq->pszCacert)
		CurlExtractCacert();

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
		pReq->Runtime.hOutFile = CreateFile( pReq->pszPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, (pReq->bResume ? OPEN_ALWAYS : CREATE_ALWAYS), FILE_ATTRIBUTE_NORMAL, NULL );
		if (VALID_HANDLE( pReq->Runtime.hOutFile )) {
			/// Resume?
			if (pReq->bResume) {
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
			if (pReq->bInsecure) {
				// Don't validate server certificate
				curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, FALSE );
			} else {
				if (pReq->pCertList) {
					// Verify against /CERT thumbprints
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, TRUE );		/// Verify SSL certificate
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 0 );		/// No host name
					curl_easy_setopt( curl, CURLOPT_CAINFO, NULL );				/// No cacert.pem
					// SSL callback
					curl_easy_setopt( curl, CURLOPT_SSL_CTX_FUNCTION, CurlSSLCallback );
					curl_easy_setopt( curl, CURLOPT_SSL_CTX_DATA, pReq );
				} else if (pReq->pszCacert) {
					// Verify against custom "cacert.pem"
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, TRUE );		/// Verify SSL certificate
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2 );		/// Validate host name
					curl_easy_setopt( curl, CURLOPT_CAINFO, pReq->pszCacert );	/// cacert.pem path
				} else {
					// Verify against the built-in "cacert.pem"
					// It's extracted at runtime in $PLUGINSDIR\cacert.pem
					CHAR szCacert[MAX_PATH];
					#if _UNICODE
						_snprintf( szCacert, ARRAYSIZE( szCacert ), "%ws\\cacert.pem", getuservariableEx( INST_PLUGINSDIR ) );
					#else
						_snprintf( szCacert, ARRAYSIZE( szCacert ), "%s\\cacert.pem", getuservariableEx( INST_PLUGINSDIR ) );
					#endif
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYPEER, TRUE );		/// Verify SSL certificate
					curl_easy_setopt( curl, CURLOPT_SSL_VERIFYHOST, 2 );		/// Validate host name
					curl_easy_setopt( curl, CURLOPT_CAINFO, szCacert );			/// cacert.pem path
				}
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
			if (iResumeFrom > 0)
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


//+ [internal] CurlQueryKeywordCallback
void CALLBACK CurlQueryKeywordCallback(_Inout_ LPTSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam)
{
	// NOTE: pReq may be NULL
	PCURL_REQUEST pReq = (PCURL_REQUEST)pParam;
	assert( pszKeyword );

	if (lstrcmpi( pszKeyword, _T( "@PLUGINNAME@" ) ) == 0) {
		MyStrCopy( T2T, pszKeyword, iMaxLen, PLUGINNAME );
	} else if (lstrcmpi( pszKeyword, _T( "@PLUGINVERSION@" ) ) == 0) {
		MyStrCopy( A2T, pszKeyword, iMaxLen, g_Curl.szVersion );
	} else if (lstrcmpi( pszKeyword, _T( "@CURLVERSION@" ) ) == 0) {
		curl_version_info_data *ver = curl_version_info( CURLVERSION_NOW );
		MyStrCopy( A2T, pszKeyword, iMaxLen, ver->version );
	} else if (lstrcmpi( pszKeyword, _T( "@MBEDTLSVERSION@" ) ) == 0) {
		//? "mbedTLS/x.y.z" -> "x.y.z"
		LPCSTR psz;
		curl_version_info_data *ver = curl_version_info( CURLVERSION_NOW );
		for (psz = ver->ssl_version; *psz && *psz != '/'; psz++);
		if (*psz)
			psz++;
		MyStrCopy( A2T, pszKeyword, iMaxLen, psz );
	} else if (lstrcmpi( pszKeyword, _T( "@USERAGENT@" ) ) == 0) {
		MyStrCopy( A2T, pszKeyword, iMaxLen, g_Curl.szUserAgent );
	} else if (pReq) {

		if (lstrcmpi( pszKeyword, _T( "@ID@" ) ) == 0) {
			_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), pReq->Queue.iId );
		} else if (lstrcmpi( pszKeyword, _T( "@STATUS@" ) ) == 0) {
			switch (pReq->Queue.iStatus) {
				case STATUS_WAITING:  lstrcpyn( pszKeyword, _T( "Waiting" ), iMaxLen ); break;
				case STATUS_RUNNING:  lstrcpyn( pszKeyword, _T( "Running" ), iMaxLen ); break;
				case STATUS_COMPLETE: lstrcpyn( pszKeyword, _T( "Complete" ), iMaxLen ); break;
				default: assert( !"Unexpected request status" );
			}
		} else if (lstrcmpi( pszKeyword, _T( "@STATUS@" ) ) == 0) {
			MyStrCopy( A2T, pszKeyword, iMaxLen, pReq->pszMethod ? pReq->pszMethod : "GET" );
		} else if (lstrcmpi( pszKeyword, _T( "@URL@" ) ) == 0) {
			MyStrCopy( A2T, pszKeyword, iMaxLen, pReq->pszURL );
		} else if (lstrcmpi( pszKeyword, _T( "@OUT@" ) ) == 0) {
			MyStrCopy( T2T, pszKeyword, iMaxLen, pReq->pszPath ? pReq->pszPath : _T( "Memory" ) );
		} else if (lstrcmpi( pszKeyword, _T( "@OUTFILE@" ) ) == 0) {
			if (pReq->pszPath) {
				LPCTSTR psz, pszLastSep = NULL;
				for (psz = pReq->pszPath; *psz; psz++)
					if (*psz == '\\')
						pszLastSep = psz;		/// Last '\\'
				if (pszLastSep) {
					MyStrCopy( T2T, pszKeyword, iMaxLen, pszLastSep + 1 );
				} else {
					MyStrCopy( T2T, pszKeyword, iMaxLen, pReq->pszPath );
				}
			} else {
				MyStrCopy( T2T, pszKeyword, iMaxLen, _T( "Memory" ) );
			}
		} else if (lstrcmpi( pszKeyword, _T( "@OUTDIR@" ) ) == 0) {
			if (pReq->pszPath) {
				LPCTSTR psz, pszLastSep = NULL;
				for (psz = pReq->pszPath; *psz; psz++)
					if (*psz == '\\')
						pszLastSep = psz;		/// Last '\\'
				if (pszLastSep) {
					for (; pszLastSep > pReq->pszPath && *pszLastSep == '\\'; pszLastSep--);	/// Move before '\\'
					lstrcpyn( pszKeyword, pReq->pszPath, (int)__min( iMaxLen, (ULONG)(pszLastSep - pReq->pszPath) + 2 ) );
				} else {
					MyStrCopy( T2T, pszKeyword, iMaxLen, pReq->pszPath );
				}
			} else {
				MyStrCopy( T2T, pszKeyword, iMaxLen, _T( "Memory" ) );
			}
		} else if (lstrcmpi( pszKeyword, _T( "@SERVERIP@" ) ) == 0) {
			MyStrCopy( A2T, pszKeyword, iMaxLen, pReq->Runtime.pszServerIP );
		} else if (lstrcmpi( pszKeyword, _T( "@SERVERPORT@" ) ) == 0) {
			_sntprintf( pszKeyword, iMaxLen, _T( "%d" ), pReq->Runtime.iServerPort );
		} else if (lstrcmpi( pszKeyword, _T( "@FILESIZE@" ) ) == 0) {
			ULONG64 iTotal;
			CurlRequestSizes( pReq, &iTotal, NULL, NULL );
			MyFormatBytes( iTotal, pszKeyword, iMaxLen );
		} else if (lstrcmpi( pszKeyword, _T( "@FILESIZE_B@" ) ) == 0) {
			ULONG64 iTotal;
			CurlRequestSizes( pReq, &iTotal, NULL, NULL );
			_sntprintf( pszKeyword, iMaxLen, _T( "%I64u" ), iTotal );
		} else if (lstrcmpi( pszKeyword, _T( "@XFERSIZE@" ) ) == 0) {
			ULONG64 iXferred;
			CurlRequestSizes( pReq, NULL, &iXferred, NULL );
			MyFormatBytes( iXferred, pszKeyword, iMaxLen );
		} else if (lstrcmpi( pszKeyword, _T( "@XFERSIZE_B@" ) ) == 0) {
			ULONG64 iXferred;
			CurlRequestSizes( pReq, NULL, &iXferred, NULL );
			_sntprintf( pszKeyword, iMaxLen, _T( "%I64u" ), iXferred );
		} else if (lstrcmpi( pszKeyword, _T( "@PERCENT@" ) ) == 0) {
			_sntprintf( pszKeyword, iMaxLen, _T( "%hd" ), pReq->Runtime.iPercent );	/// Can be -1
		} else if (lstrcmpi( pszKeyword, _T( "@SPEED@" ) ) == 0) {
			if (pReq->Runtime.iSpeed > 0) {
				MyFormatBytes( pReq->Runtime.iSpeed, pszKeyword, iMaxLen );
				_tcscat( pszKeyword, _T( "/s" ) );
			} else {
				lstrcpyn( pszKeyword, _T( "-" ), iMaxLen );
			}
		} else if (lstrcmpi( pszKeyword, _T( "@SPEED_B@" ) ) == 0) {
			_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), pReq->Runtime.iSpeed );
		} else if (lstrcmpi( pszKeyword, _T( "@TIMEELAPSED@" ) ) == 0) {
			if (pReq->Runtime.iTimeElapsed > 0) {
				MyFormatMilliseconds( pReq->Runtime.iTimeElapsed / 1000, pszKeyword, iMaxLen );
			} else {
				lstrcpyn( pszKeyword, _T( "-" ), iMaxLen );
			}
		} else if (lstrcmpi( pszKeyword, _T( "@TIMEELAPSED_MS@" ) ) == 0) {
			_sntprintf( pszKeyword, iMaxLen, _T( "%I64u" ), pReq->Runtime.iTimeElapsed / 1000 );
		} else if (lstrcmpi( pszKeyword, _T( "@TIMEREMAINING@" ) ) == 0) {
			if (pReq->Runtime.iTimeRemaining > 0) {
				MyFormatMilliseconds( pReq->Runtime.iTimeRemaining / 1000, pszKeyword, iMaxLen );
			} else {
				lstrcpyn( pszKeyword, _T( "-" ), iMaxLen );
			}
		} else if (lstrcmpi( pszKeyword, _T( "@TIMEREMAINING_MS@" ) ) == 0) {
			_sntprintf( pszKeyword, iMaxLen, _T( "%I64u" ), pReq->Runtime.iTimeRemaining / 1000 );
		} else if (lstrcmpi( pszKeyword, _T( "@ERROR@" ) ) == 0) {
			CurlRequestFormatError( pReq, pszKeyword, iMaxLen, NULL, NULL );
		} else if (lstrcmpi( pszKeyword, _T( "@ERRORCODE@" ) ) == 0) {
			ULONG e;
			CurlRequestFormatError( pReq, NULL, 0, NULL, &e );
			_sntprintf( pszKeyword, iMaxLen, _T( "%u" ), e );
		}
	}
/*
	{PROXY}

	/SENTHEADERS
	/RECVHEADERS
	/CONTENT
	/DATA

	{SSL/TLS info}
	{EffectiveURL}
	{HTTPAuth available}
*/
}


//++ CurlQuery
LONG CurlQuery( _In_ PCURL_REQUEST pReq, _Inout_ LPTSTR pszStr, _In_ LONG iStrMaxLen )
{
	if (!pszStr || !iStrMaxLen)
		return -1;

	return ReplaceKeywords( pszStr, iStrMaxLen, _T( '@' ), _T( '@' ), CurlQueryKeywordCallback, pReq );
}
