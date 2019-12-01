
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

static LPVOID MyAlloc( _In_ ULONG iSize ) {
	LPVOID p = GlobalLock( GlobalAlloc( GMEM_FIXED, iSize ) );
	g_MemStats.AllocBytes += iSize;
	g_MemStats.AllocCalls++;
	return p;
}

#define MyFree(_ptr) { \
	if ( _ptr ) { \
		g_MemStats.FreeBytes += GlobalSize((HGLOBAL)(_ptr)); \
		g_MemStats.FreeCalls++; \
		GlobalFree((HGLOBAL)(_ptr)); \
		(_ptr) = NULL; \
	}}


LPSTR  MyStrDupAA( _In_ LPCSTR pStr );
LPSTR  MyStrDupAW( _In_ LPCWSTR pStr );
LPWSTR MyStrDupWA( _In_ LPCSTR pStr );
LPWSTR MyStrDupWW( _In_ LPCWSTR pStr );


// ANSI (multi byte) <-> Unicode (wide char)
typedef enum { A2A, A2W, A2T, W2A, W2W, W2T, T2A, T2W, T2T } SrcDestEncoding;

//+ MyStrCopy
LPVOID MyStrCopy( _In_ SrcDestEncoding iEnc, _In_ LPVOID pszDest, _In_ ULONG iDestMaxLen, _In_ LPCVOID pszSrc );


//? The caller must MyFree(..) the string
LPCTSTR MyErrorStr( _In_ ULONG err );

//+ VALID_HANDLE
#define VALID_HANDLE(h) \
	((h) != NULL) && ((h) != INVALID_HANDLE_VALUE)

//+ FileExists
BOOL FileExistsW( _In_ LPCWSTR pszFile );
BOOL FileExistsA( _In_ LPCSTR pszFile );

//+ MyTimeDiff
// Returns milliseconds
ULONG MyTimeDiff( _In_ PFILETIME pEndTime, _In_ PFILETIME pStartTime );

//+ SetThreadName
// Available in Win10+
void SetThreadName( _In_ HANDLE hThread, _In_ LPCWSTR pszName );

//+ ReadVersionInfoString
// Returns Win32 error
ULONG ReadVersionInfoString( _In_opt_ LPCTSTR szFile, _In_ LPCTSTR szStringName, _Out_ LPTSTR szStringValue, _In_ UINT iStringValueLen );

//+ ExtractResourceFile
ULONG ExtractResourceFile( _In_ HMODULE hMod, _In_ LPCTSTR pszResType, _In_ LPCTSTR pszResName, _In_ USHORT iResLang, _In_ LPCTSTR pszOutPath );

//+ BinaryToHex
// Returns number of TCHAR-s written, not including the NULL terminator
ULONG BinaryToHexA( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPSTR pszStr, _In_ ULONG iStrLen );
ULONG BinaryToHexW( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPWSTR pszStr, _In_ ULONG iStrLen );

//+ BinaryToString
// Returns number of TCHAR-s written, not including the NULL terminator
ULONG BinaryToString( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPTSTR pszStr, _In_ ULONG iStrLen );

//+ ReplaceKeywordsA
// Replaces "keywords" in a string
// A callback function is called for each keyword
// Returns the lstrlen(..) of the output string, or -1 if errors occurred
typedef void (CALLBACK *REPLACE_KEYWORD_CALLBACK_A)(_Inout_ LPSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam);
typedef void (CALLBACK *REPLACE_KEYWORD_CALLBACK_W)(_Inout_ LPWSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam);
LONG ReplaceKeywordsA(
	_Inout_ LPSTR pszStr, _In_ LONG iMaxLen,
	_In_ CHAR chKeywordStart, _In_ CHAR chKeywordEnd,
	_In_ REPLACE_KEYWORD_CALLBACK_A fnReplace, _In_ LPVOID pReplaceParam
);
LONG ReplaceKeywordsW(
	_Inout_ LPWSTR pszStr, _In_ LONG iMaxLen,
	_In_ WCHAR chKeywordStart, _In_ WCHAR chKeywordEnd,
	_In_ REPLACE_KEYWORD_CALLBACK_W fnReplace, _In_ LPVOID pReplaceParam
);

//+ MyStrReplace
// Returns the lstrlen(..) of the output string, or -1 if errors occurred
LONG MyStrReplace(
	_Inout_ LPTSTR pszStr, _In_ size_t iStrMaxLen,
	_In_ LPCTSTR pszSubstr, _In_ LPCTSTR pszReplaceWith,
	_In_ BOOL bMatchCase
);

//+ MyStrToInt64
// Replacement for shlwapi!StrToInt64Ex introduced in "Update Rollup 1 for Windows 2000 SP4"
BOOL MyStrToInt64( _In_ LPCTSTR pszStr, _Out_ PUINT64 piNum );

//+ MyFormatBytes
void MyFormatBytes( _In_ ULONG64 iBytes, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen );

//+ MyFormatMilliseconds
void MyFormatMilliseconds( _In_ ULONG64 iMillis, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen );

//+ Virtual Memory
typedef struct {
	PCCH   pMem;		/// Memory buffer
	SIZE_T iSize;		/// Memory size
	SIZE_T iReserved, iCommitted;
} VMEMO;
ULONG  VirtualMemoryInitialize( _Inout_ VMEMO *pMem, _In_ SIZE_T iMaxSize );			/// Return Win32 error
SIZE_T VirtualMemoryAppend( _Inout_ VMEMO *pMem, _In_ PVOID mem, _In_ SIZE_T size );	/// Return bytes written
void   VirtualMemoryReset( _Inout_ VMEMO *pMem );
void   VirtualMemoryDestroy( _Inout_ VMEMO *pMem );

//+ Unicode
#ifdef _UNICODE
	#define FileExists		FileExistsW
	#define StrListAdd		StrListAddW
	#define BinaryToHex		BinaryToHexW
	#define MyStrDup		MyStrDupWW		/// LPTSTR -> LPTSTR
	#define MyStrDupA		MyStrDupAW		/// LPTSTR -> LPSTR
	#define ReplaceKeywords	ReplaceKeywordsW
#else
	#define FileExists		FileExistsA
	#define StrListAdd		StrListAddA
	#define BinaryToHex		BinaryToHexA
	#define MyStrDup		MyStrDupAA		/// LPTSTR -> LPTSTR
	#define MyStrDupA		MyStrDupAA		/// LPTSTR -> LPSTR
	#define ReplaceKeywords	ReplaceKeywordsA
#endif
