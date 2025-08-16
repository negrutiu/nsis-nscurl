
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#pragma once
#include <curl/curl.h>
#include "utils.h"
#include "vmemo.h"

#define STATUS_WAITING		0
#define STATUS_RUNNING		'r'
#define STATUS_COMPLETE		'c'

/// \brief Filename reserved for in-memory transfers.
#define FILENAME_MEMORY		_T("Memory")

#define CACERT_BUILTIN      NULL
#define CACERT_NONE         ((LPCSTR)1)

//+ struct CURL_REQUEST
typedef struct _CURL_REQUEST {
	LPCSTR		pszURL;
	LPCTSTR		pszPath;				/// Local file path. If NULL, the file will download to RAM
	LPCSTR		pszMethod;				/// can be NULL
	LPCSTR		pszProxy;				/// can be NULL
	LPCSTR		pszProxyUser;			/// can be NULL
	LPCSTR		pszProxyPass;			/// can be NULL
	int			iAuthType;				/// can be 0
	LPCSTR		pszUser;				/// can be NULL
	LPCSTR		pszPass;				/// can be NULL. Represents OAuth 2.0 token if iAuthType is CURLAUTH_BEARER
	LPCSTR		pszTlsUser;				/// can be NULL
	LPCSTR		pszTlsPass;				/// can be NULL
	struct curl_slist	*pOutHeaders;	/// can be NULL
	struct curl_slist	*pPostVars;		/// can be NULL
	IDATA		Data;					/// can be empty. Input string/buffer/file
	LPCSTR		pszAgent;				/// can be NULL
	LPCSTR		pszReferrer;			/// can be NULL
	BOOLEAN		bResume       : 1;
	BOOLEAN		bInsist       : 1;
	BOOLEAN		bNoRedirect   : 1;
	BOOLEAN		bNoDebugData  : 1;
	BOOLEAN		bMarkOfTheWeb : 1;
	BOOLEAN     bHttp11       : 1;
	BOOLEAN     bHttp3        : 1;
	BOOLEAN     bEncoding     : 1;
	BOOLEAN     bWeakSecurity : 1;		/// weak crypto, weak protocols (tls 1.0+), unsafe renegociation
	BOOLEAN     bCastore      : 1;      /// Use native CA store (CURLSSLOPT_NATIVE_CA)
	LPCSTR		pszCacert;				/// can be CACERT_BUILTIN(NULL), CACERT_NONE, or a file path
	struct curl_slist *pCertList;		/// List of sha1 certificate thumprints. can be NULL
	struct curl_slist *pPemList;		/// List of pem blobs. can be NULL
	uint64_t	opensslSetFlags;		// set additional SSL_CTX flags during SSL handshake (see SSL_CTX_set_options)
	uint64_t	opensslClearFlags;		// clear SSL_CTX flags during SSL handshake (see SSL_CTX_set_options)
	LPCTSTR		pszDebugFile;			/// can be NULL
	ULONG		iConnectTimeout;		/// can be 0. Connecting timeout
	ULONG		iCompleteTimeout;		/// can be 0. Complete (connect + transfer) timeout
	ULONG		iLowSpeedLimit;			/// can be 0. Low speed limit (bps)
	ULONG		iLowSpeedTime;			/// can be 0. Low speed time (sec)
	ULONG		iSpeedCap;				/// can be 0. Speed cap (ms)
	ULONG		iDependencyId;			/// can be 0. This request will not be carried out until its dependency completes
	LPCSTR		pszTag;					/// can be NULL
	LPCSTR		pszDOH;					/// can be NULL. DNS-over-HTTPS server (e.g. "https://1.1.1.1/dns-query")
	LPCSTR		pszCookieFile;
	struct {
		struct _CURL_REQUEST* pNext;	/// Singly linked list
		ULONG			iId;			/// Unique ID
		CHAR			iStatus;		/// '\0' = Waiting, 'r' = Running, 'c' = Complete
		volatile LONG	iFlagAbort;		/// If TRUE, the transfer will abort. iStatus will be set to 'c'
	} Queue;
	struct {
		CURL		*pCurl;
		ULONG		iTimeStart;			/// Milliseconds (tick counts)
		HANDLE		hInFile;			/// Upload file. iDataSize represents its size
		curl_off_t	iDataPos;			/// Input data/file position
		VMEMO		InHeaders;
		VMEMO		OutHeaders;
		VMEMO		OutData;			/// Download to RAM (hOutFile == NULL)
		HANDLE		hOutFile;			/// Download to file
		BOOLEAN		bTrustedCert : 1;	/// Used only when validating against /CERT certificate thumbprints
		ULONG		iRootCertFlags;		/// Original root certificate validation flags
		HANDLE		hDebugFile;			/// Debug connection
		LPCSTR		pszFinalURL;		/// The final URL, after following all redirections
		LONG		iServerPort;
		LPCSTR		pszServerIP;		/// Can be IPv6
		curl_off_t	iResumeFrom;		/// Bytes already downloaded
		curl_off_t	iTimeElapsed;		/// Milliseconds
		curl_off_t	iTimeRemaining;		/// Milliseconds
		curl_off_t	iDlXferred, iDlTotal;
		curl_off_t	iUlXferred, iUlTotal;
		struct {
			ULONG measureStartTime;
			curl_off_t measureStartSize;
			curl_off_t current;			// Current speed (measured by us)
    		curl_off_t average;			// Average speed (reported by curl)
		} Speed;
	} Runtime;
	struct {
		ULONG		iWin32;
		LPCTSTR		pszWin32;
		CURLcode	iCurl;
		LPCSTR		pszCurl;
		int			iX509;
		LPCSTR		pszX509;
		int			iHttp;
		LPCSTR		pszHttp;
	} Error;
} CURL_REQUEST, *PCURL_REQUEST;


//+ CurlRequestInit
void CurlRequestInit( _Inout_ PCURL_REQUEST pReq );

//+ CurlRequestDestroy
void CurlRequestDestroy( _Inout_ PCURL_REQUEST pReq );

//+ CurlRequestSetAbortFlag
#define CurlRequestSetAbortFlag( pReq ) \
	InterlockedExchange( &(pReq)->Queue.iFlagAbort, TRUE )

//+ CurlRequestGetAbortFlag
#define CurlRequestGetAbortFlag( pReq ) \
	InterlockedCompareExchange( &(pReq)->Queue.iFlagAbort, -1, -1 )

//+ CurlRequestSizes
void CurlRequestComputeNumbers( _In_ PCURL_REQUEST pReq, _Out_opt_ PULONG64 piSizeTotal, _Out_opt_ PULONG64 piSizeXferred, _Out_opt_ PSHORT piPercent, _Out_opt_ PBOOL pbDown );

//+ CurlRequestFormatError
void CurlRequestFormatError( _In_ PCURL_REQUEST pReq, _In_opt_ LPTSTR pszError, _In_opt_ ULONG iErrorLen, _Out_opt_ PBOOLEAN pbSuccess, _Out_opt_ PULONG piErrorCode );
LPCSTR CurlRequestErrorType( _In_ PCURL_REQUEST pReq );		//? Returns: "win32", "curl", "http" or ""
ULONG  CurlRequestErrorCode( _In_ PCURL_REQUEST pReq );		//? e.g. 404

// ____________________________________________________________________________________________________________________________________ //
//                                                                                                                                      //

//+ Initialization
ULONG CurlInitialize(void);
void  CurlDestroy(void);

//+ CurlParseRequestParam
// Returns Win32 error code (ERROR_NOT_SUPPORTED for unknown parameters)
ULONG CurlParseRequestParam(
	_In_ ULONG iParamIndex,
	_In_ LPTSTR pszParam,		/// Working buffer with the current parameter
	_In_ int iParamMaxLen,
	_Out_ PCURL_REQUEST pReq
);

//+ CurlTransfer
void CurlTransfer(
	_In_ PCURL_REQUEST pReq
);

//+ CurlQuery
//? Find and replace "keywords" in the specified string
//? Returns the length of the output string, or -1 if errors occur
LONG CurlQuery(
	_In_ PCURL_REQUEST pReq,
	_Inout_ LPTSTR pszStr, _In_ LONG iStrMaxLen
);