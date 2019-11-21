
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Make HTTP/S requests using libcurl

#pragma once
#include "utils.h"


//+ struct CURL_REQUEST
typedef struct {
	LPCSTR  pszURL;
	LPCTSTR pszPath;				/// Local file path. If NULL, the file will download to RAM
	LPCSTR  pszMethod;				/// can be NULL
	LPCSTR  pszHeaders;				/// can be NULL
	LPVOID  pData;					/// can be NULL
	ULONG   iDataSize;				/// can be 0
	LPCSTR  pszProxy;				/// can be NULL
	LPCSTR  pszProxyUser;			/// can be NULL
	LPCSTR  pszProxyPass;			/// can be NULL
	LPCSTR  pszAgent;				/// can be NULL
	LPCSTR  pszReferrer;			/// can be NULL
	LPCSTR  pszCacert;				/// can be NULL
	ULONG   iTimeout;				/// can be 0
	struct {
		CURL		*pCurl;
		CURLcode	iCurlError;
		LPCSTR		pszCurlError;
		int			iHttpStatus;
		LPCSTR		pszHttpStatus;
		BOOL		bHttpStatus;
		HANDLE		hFile;
		VMEMO		OutHeaders;
		VMEMO		OutData;		/// Used downloading to RAM
	} Runtime;
} CURL_REQUEST, *PCURL_REQUEST;

#define CurlRequestInit(Req) \
	ZeroMemory( &Req, sizeof( Req ) );

#define CurlRequestDestroy(Req) \
	MyFree( Req.pszURL ); \
	MyFree( Req.pszPath ); \
	MyFree( Req.pszMethod ); \
	MyFree( Req.pszHeaders ); \
	MyFree( Req.pData ); \
	MyFree( Req.pszProxy ); \
	MyFree( Req.pszProxyUser ); \
	MyFree( Req.pszProxyPass ); \
	MyFree( Req.pszAgent ); \
	MyFree( Req.pszReferrer ); \
	MyFree( Req.pszCacert ); \
	UNREFERENCED_PARAMETER( Req.Runtime.pCurl ); \
	if (VALID_HANDLE(Req.Runtime.hFile)) CloseHandle(Req.Runtime.hFile); \
	MyFree( Req.Runtime.pszCurlError ); \
	MyFree( Req.Runtime.pszHttpStatus ); \
	VirtualMemoryDestroy( &Req.Runtime.OutHeaders ); \
	VirtualMemoryDestroy( &Req.Runtime.OutData ); \
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
