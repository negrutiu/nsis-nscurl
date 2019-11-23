
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#pragma once
#include "utils.h"


//+ struct CURL_REQUEST
typedef struct {
	LPCSTR		pszURL;
	LPCTSTR		pszPath;				/// Local file path. If NULL, the file will download to RAM
	LPCSTR		pszMethod;				/// can be NULL
	struct curl_slist	*pInHeaders;	/// can be NULL
	struct curl_slist	*pPostVars;		/// can be NULL
	LPVOID		pszData;				/// can be NULL. If iDataSize != 0, (LPSTR)pszData is treated as data string. If iDataSize == 0, (LPTSTR)pszData is treated as data file name
	curl_off_t	iDataSize;				/// can be 0
	LPCSTR		pszProxy;				/// can be NULL
	LPCSTR		pszProxyUser;			/// can be NULL
	LPCSTR		pszProxyPass;			/// can be NULL
	LPCSTR		pszAgent;				/// can be NULL
	LPCSTR		pszReferrer;			/// can be NULL
	BOOLEAN		bNoRedirect : 1;		/// can be 0
	BOOLEAN		bInsecure : 1;			/// can be 0
	LPCSTR		pszCacert;				/// can be NULL. Ignored if bInsecure is TRUE
	ULONG		iConnectTimeout;		/// can be 0. Connecting timeout
	ULONG		iCompleteTimeout;		/// can be 0. Complete (connect + transfer) timeout
	struct {
		CURL		*pCurl;
		HANDLE		hInFile;			/// Upload file. iDataSize represents its size
		curl_off_t	iDataPos;			/// Input data/file position
		VMEMO		OutHeaders;
		VMEMO		OutData;			/// Download to RAM (hOutFile == NULL)
		HANDLE		hOutFile;			/// Download to file
	} Runtime;
	struct {
		ULONG		iWin32;
		LPCTSTR		pszWin32;
		CURLcode	iCurl;
		LPCSTR		pszCurl;
		int			iHttp;
		LPCSTR		pszHttp;
		BOOLEAN		bStatusGrabbed : 1;
	} Error;
} CURL_REQUEST, *PCURL_REQUEST;

#define CurlRequestInit(Req) \
	ZeroMemory( &Req, sizeof( Req ) );

#define CurlRequestDestroy(Req) \
	MyFree( Req.pszURL ); \
	MyFree( Req.pszPath ); \
	MyFree( Req.pszMethod ); \
	curl_slist_free_all( Req.pInHeaders ); \
	curl_slist_free_all( Req.pPostVars ); \
	MyFree( Req.pszData ); \
	MyFree( Req.pszProxy ); \
	MyFree( Req.pszProxyUser ); \
	MyFree( Req.pszProxyPass ); \
	MyFree( Req.pszAgent ); \
	MyFree( Req.pszReferrer ); \
	MyFree( Req.pszCacert ); \
	Req.Runtime.pCurl = NULL; \
	if (VALID_HANDLE(Req.Runtime.hInFile))  CloseHandle(Req.Runtime.hInFile); \
	if (VALID_HANDLE(Req.Runtime.hOutFile)) CloseHandle(Req.Runtime.hOutFile); \
	VirtualMemoryDestroy( &Req.Runtime.OutHeaders ); \
	VirtualMemoryDestroy( &Req.Runtime.OutData ); \
	MyFree( Req.Error.pszWin32 ); \
	MyFree( Req.Error.pszCurl ); \
	MyFree( Req.Error.pszHttp ); \
	ZeroMemory( &Req, sizeof( Req ) );


//+ Initialization
ULONG CurlInitialize();
void  CurlDestroy();
ULONG CurlExtractCacert();

//+ CurlParseRequestParam
BOOL CurlParseRequestParam(
	_In_ LPTSTR pszParam,		/// Working buffer with the current parameter
	_In_ int iParamMaxLen,
	_Out_ PCURL_REQUEST pReq
);

//+ CurlTransfer
void CurlTransfer(
	_In_ PCURL_REQUEST pReq
);

//+ CurlRequestFormatError
void CurlRequestFormatError(
	_In_ PCURL_REQUEST pReq,
	_In_ LPTSTR pszError,
	_In_ ULONG iErrorLen
);