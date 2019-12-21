
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/02/02

#pragma once

//+ Initialization
VOID UtilsInitialize();
VOID UtilsDestroy();


//+ TRACE
#if DBG || _DEBUG
	#define TRACE_ENABLED
#endif

#if defined (TRACE_ENABLED)
	#define TRACE TraceImpl
	#define TRACE2(...)			/// More verbose tracing
	VOID TraceImpl( _In_ LPCTSTR pszFormat, _In_ ... );
#else
	#define TRACE(...)
	#define TRACE2(...)
#endif


//+ assert
#if DBG || _DEBUG
	#define assert(expr) \
		if ( !(expr)) { \
			TCHAR szMsg[512]; \
			TRACE( _T("  [!] %s, %s:%u\n"), _T(#expr), _T( __FILE__ ), __LINE__ ); \
			_sntprintf( szMsg, (int)ARRAYSIZE( szMsg ), _T("%s\n%s : %u\n"), _T(#expr), _T( __FILE__ ), __LINE__ ); \
			if (MessageBox( NULL, szMsg, _T("ASSERT"), MB_ICONERROR|MB_OKCANCEL ) != IDOK) \
				ExitProcess( 666 ); \
		}
	#define verify assert
#else
	#define assert(...)  ((VOID)0)
	#define verify(expr) ((VOID)(expr))
#endif

//+ Memory
/// We'll use global memory, to be compatible with NSIS API
typedef struct {
	UINT64 AllocBytes, FreeBytes;
	UINT   AllocCalls, FreeCalls;
} MEMORY_STATS;
extern MEMORY_STATS g_MemStats;


//+ MyAlloc
static LPVOID MyAlloc( _In_ ULONG iSize ) {
	LPVOID p = GlobalLock( GlobalAlloc( GMEM_FIXED, iSize ) );
	g_MemStats.AllocBytes += iSize;
	g_MemStats.AllocCalls++;
#ifdef _DEBUG
	if (p)
		FillMemory( p, iSize, 0xcd );
#endif
	return p;
}


//+ MyFree
#define MyFree(_ptr) { \
	if ( _ptr ) { \
		g_MemStats.FreeBytes += GlobalSize((HGLOBAL)(_ptr)); \
		g_MemStats.FreeCalls++; \
		GlobalFree((HGLOBAL)(_ptr)); \
		(_ptr) = NULL; \
	}}


//+ MyValidHandle
#define MyValidHandle(h) \
	((h) != NULL) && ((h) != INVALID_HANDLE_VALUE)


// ANSI (multi byte) <-> Unicode (wide char)
typedef enum { eA2A, eA2W, eA2T, eW2A, eW2W, eW2T, eT2A, eT2W, eT2T } Encodings;


//+ MyStrDup
// If the input string is NULL, the function allocates and returns an empty string (length zero)
// The caller must MyFree(..) the string
LPVOID MyStrDup ( _In_ Encodings iEnc, _In_ LPCVOID pszSrc );
LPVOID MyStrDupN( _In_ Encodings iEnc, _In_ LPCVOID pszSrc, _In_opt_ int iSrcMaxLen );		/// If iSrcMaxLen < 0 the string is assumed to be NULL terminated


//+ MyStrCopy
LPVOID MyStrCopy( _In_ Encodings iEnc, _In_ LPVOID pszDest, _In_ ULONG iDestMaxLen, _In_ LPCVOID pszSrc );


//+ MyFormatError
// The caller must MyFree(..) the string
LPCTSTR MyFormatError( _In_ ULONG err );


//+ MyFileExists
BOOL MyFileExistsW( _In_ LPCWSTR pszFile );
BOOL MyFileExistsA( _In_ LPCSTR pszFile );


//+ MyCreateDirectory
ULONG MyCreateDirectory( _In_ LPCTSTR pszPath, _In_ BOOLEAN bHasFilename );


//+ MySetThreadName
// Available in Win10+
void MySetThreadName( _In_ HANDLE hThread, _In_ LPCWSTR pszName );


//+ MyReadVersionString
// Returns Win32 error
ULONG MyReadVersionString( _In_opt_ LPCTSTR szFile, _In_ LPCTSTR szStringName, _Out_ LPTSTR szStringValue, _In_ UINT iStringValueLen );


//+ MySaveResource
// Extract PE resource to file
ULONG MySaveResource( _In_ HMODULE hMod, _In_ LPCTSTR pszResType, _In_ LPCTSTR pszResName, _In_ USHORT iResLang, _In_ LPCTSTR pszOutPath );


//+ MyFormatBinaryHex
// Returns the length of the output string, without \0
ULONG MyFormatBinaryHexA( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPSTR pszStr, _In_ ULONG iStrLen );
ULONG MyFormatBinaryHexW( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPWSTR pszStr, _In_ ULONG iStrLen );


//+ MyFormatBinaryPrintable
// Returns the length of the output string, without \0
ULONG MyFormatBinaryPrintable( _In_ LPCVOID pData, _In_ ULONG iDataSize, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen, _In_ BOOLEAN bEscape );


//+ MyReplaceKeywordsA
// A callback function is called for each keyword
// Returns the length of the output string, without \0. Returns -1 if errors occur
typedef void (CALLBACK *REPLACE_KEYWORD_CALLBACK_A)(_Inout_ LPSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam);
typedef void (CALLBACK *REPLACE_KEYWORD_CALLBACK_W)(_Inout_ LPWSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam);
LONG MyReplaceKeywordsA(
	_Inout_ LPSTR pszStr, _In_ LONG iMaxLen,
	_In_ CHAR chKeywordStart, _In_ CHAR chKeywordEnd,
	_In_ REPLACE_KEYWORD_CALLBACK_A fnReplace, _In_ LPVOID pReplaceParam
);
LONG MyReplaceKeywordsW(
	_Inout_ LPWSTR pszStr, _In_ LONG iMaxLen,
	_In_ WCHAR chKeywordStart, _In_ WCHAR chKeywordEnd,
	_In_ REPLACE_KEYWORD_CALLBACK_W fnReplace, _In_ LPVOID pReplaceParam
);


//+ MyStrReplace
// Returns the length of the output string, without \0. Returns -1 if errors occur
LONG MyStrReplace(
	_Inout_ LPTSTR pszStr, _In_ size_t iStrMaxLen,
	_In_ LPCTSTR pszSubstr, _In_ LPCTSTR pszReplaceWith,
	_In_ BOOL bMatchCase
);


//+ MyFormatBytes
void MyFormatBytes( _In_ ULONG64 iBytes, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen );


//+ MyFormatMilliseconds
void MyFormatMilliseconds( _In_ ULONG64 iMillis, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen );


//+ Virtual Memory
// Auto-growing virtual memory buffer
typedef struct {
	PCCH   pMem;		/// Memory buffer
	SIZE_T iSize;		/// Memory size
	SIZE_T iReserved, iCommitted;
} VMEMO;
ULONG  VirtualMemoryInitialize( _Inout_ VMEMO *pMem, _In_ SIZE_T iMaxSize );			/// Return Win32 error
SIZE_T VirtualMemoryAppend( _Inout_ VMEMO *pMem, _In_ PVOID mem, _In_ SIZE_T size );	/// Return bytes written
void   VirtualMemoryReset( _Inout_ VMEMO *pMem );			/// Reset to 0-bytes
void   VirtualMemoryDestroy( _Inout_ VMEMO *pMem );			/// Free everything


//+ Unicode
#ifdef _UNICODE
	#define MyFileExists		MyFileExistsW
	#define MyFormatBinaryHex	MyFormatBinaryHexW
	#define MyReplaceKeywords	MyReplaceKeywordsW
#else
	#define MyFileExists		MyFileExistsA
	#define MyFormatBinaryHex	MyFormatBinaryHexA
	#define MyReplaceKeywords	MyReplaceKeywordsA
#endif
