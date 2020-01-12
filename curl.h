
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#pragma once
#include "utils.h"

#define STATUS_WAITING		0
#define STATUS_RUNNING		'r'
#define STATUS_COMPLETE		'c'


//+ struct CURL_REQUEST
typedef struct _CURL_REQUEST {
	LPCSTR		pszURL;
	LPCTSTR		pszPath;				/// Local file path. If NULL, the file will download to RAM
	LPCSTR		pszMethod;				/// can be NULL
	LPCSTR		pszProxy;				/// can be NULL
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
	BOOLEAN		bNoRedirect   : 1;
	LPCSTR		pszCacert;				/// can be NULL. If valid and empty ("") no cacert.pem is used
	struct curl_slist *pCertList;		/// can be NULL. If pszCacert=="" and pCertList==NULL, the SSL validation is turned off
	LPCTSTR		pszDebugFile;			/// can be NULL
	ULONG		iConnectTimeout;		/// can be 0. Connecting timeout
	ULONG		iCompleteTimeout;		/// can be 0. Complete (connect + transfer) timeout
	struct {
		struct _CURL_REQUEST* pNext;	/// Singly linked list
		ULONG			iId;			/// Unique ID
		CHAR			iStatus;		/// '\0' = Waiting, 'r' = Running, 'c' = Complete
		volatile LONG	iFlagAbort;		/// If TRUE, the transfer will abort. iStatus will be set to 'c'
	} Queue;
	struct {
		CURL		*pCurl;
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
		curl_off_t	iTimeElapsed;		/// Microseconds
		curl_off_t	iTimeRemaining;		/// Microseconds
		ULONG64		iDlXferred, iDlTotal;
		ULONG64		iUlXferred, iUlTotal;
		ULONG		iSpeed;
		short		iPercent;			/// -1 if the total size is unknown
	} Runtime;
	struct {
		ULONG		iWin32;
		LPCTSTR		pszWin32;
		CURLcode	iCurl;
		LPCSTR		pszCurl;
		int			iHttp;
		LPCSTR		pszHttp;
	} Error;
} CURL_REQUEST, *PCURL_REQUEST;

//+ CurlRequestInit
static void CurlRequestInit( _Inout_ PCURL_REQUEST pReq ) {
	if (!pReq) return;
	ZeroMemory( pReq, sizeof( *pReq ) );
	pReq->Runtime.iPercent = -1;		/// Unknown size
	pReq->Runtime.iRootCertFlags = -1;	/// Uninitialized
}

//+ CurlRequestDestroy
static void CurlRequestDestroy( _Inout_ PCURL_REQUEST pReq ) {
	if (!pReq) return;
	MyFree( pReq->pszURL );
	MyFree( pReq->pszPath );
	MyFree( pReq->pszMethod );
	MyFree( pReq->pszProxy );
	MyFree( pReq->pszUser );
	MyFree( pReq->pszPass );
	MyFree( pReq->pszTlsUser );
	MyFree( pReq->pszTlsPass );
	curl_slist_free_all( pReq->pOutHeaders );
	curl_slist_free_all( pReq->pPostVars );
	IDataDestroy( &pReq->Data );
	MyFree( pReq->pszAgent );
	MyFree( pReq->pszReferrer );
	curl_slist_free_all( pReq->pCertList );
	MyFree( pReq->pszCacert );
	MyFree( pReq->pszDebugFile );
	pReq->Runtime.pCurl = NULL;
	if (MyValidHandle( pReq->Runtime.hInFile ))
		CloseHandle( pReq->Runtime.hInFile );
	if (MyValidHandle( pReq->Runtime.hOutFile ))
		CloseHandle( pReq->Runtime.hOutFile );
	if (MyValidHandle( pReq->Runtime.hDebugFile ))
		CloseHandle( pReq->Runtime.hDebugFile );
	VirtualMemoryDestroy( &pReq->Runtime.InHeaders );
	VirtualMemoryDestroy( &pReq->Runtime.OutHeaders );
	VirtualMemoryDestroy( &pReq->Runtime.OutData );
	MyFree( pReq->Runtime.pszFinalURL );
	MyFree( pReq->Runtime.pszServerIP );
	MyFree( pReq->Error.pszWin32 );
	MyFree( pReq->Error.pszCurl );
	MyFree( pReq->Error.pszHttp );
	ZeroMemory( pReq, sizeof( *pReq ) );
}

//+ CurlRequestSizes
void CurlRequestSizes( _In_ PCURL_REQUEST pReq, _Out_opt_ PULONG64 piSizeTotal, _Out_opt_ PULONG64 piSizeXferred, _Out_opt_ PBOOL pbDown );

//+ CurlRequestFormatError
void CurlRequestFormatError( _In_ PCURL_REQUEST pReq, _In_ LPTSTR pszError, _In_ ULONG iErrorLen, _Out_opt_ PBOOLEAN pbSuccess, _Out_opt_ PULONG piErrCode  );

// ____________________________________________________________________________________________________________________________________ //
//                                                                                                                                      //

//+ Initialization
ULONG CurlInitialize();
void  CurlDestroy();
ULONG CurlExtractCacert();		/// Called automatically

//+ CurlParseRequestParam
BOOL CurlParseRequestParam(
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