
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
	#define assert(...)
	#define verify(expr) ((VOID)(expr))
#endif

//+ Win32Err
LPCTSTR Win32Err( _In_ ULONG err, _Out_ LPTSTR pszError, _In_ ULONG iErrorLen );

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

//+ VALID_HANDLE
#define VALID_HANDLE(h) \
	((h) != NULL) && ((h) != INVALID_HANDLE_VALUE)

//+ FileExists
BOOL FileExistsW( _In_ LPCWSTR pszFile );
BOOL FileExistsA( _In_ LPCSTR pszFile );

//+ MyTimeDiff
// Returns milliseconds
ULONG MyTimeDiff( _In_ PFILETIME pEndTime, _In_ PFILETIME pStartTime );

//+ ReadVersionInfoString
// Returns Win32 error
ULONG ReadVersionInfoString( _In_opt_ LPCTSTR szFile, _In_ LPCTSTR szStringName, _Out_ LPTSTR szStringValue, _In_ UINT iStringValueLen );

//+ ExtractResourceFile
ULONG ExtractResourceFile( _In_ HMODULE hMod, _In_ LPCTSTR pszResType, _In_ LPCTSTR pszResName, _In_ USHORT iResLang, _In_ LPCTSTR pszOutPath );

//+ BinaryToHex
// Returns number of TCHAR-s written, not including the NULL terminator
ULONG BinaryToHex( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPTSTR pszStr, _In_ ULONG iStrLen );

//+ BinaryToString
// Returns number of TCHAR-s written, not including the NULL terminator
ULONG BinaryToString( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPTSTR pszStr, _In_ ULONG iStrLen );

//+ MyStrToInt64
// Replacement for shlwapi!StrToInt64Ex introduced in "Update Rollup 1 for Windows 2000 SP4"
BOOL MyStrToInt64( _In_ LPCTSTR pszStr, _Out_ PUINT64 piNum );

//+ String List
typedef struct _STRLIST {
	LPCSTR String;													/// UTF8
	struct _STRLIST *Next;
} STRLIST;

void StrListInitialize( _Inout_ STRLIST *pList );
void StrListAddA( _Inout_ STRLIST *pList, _In_ LPCSTR pStr );		/// Add a copy of the string
void StrListAddW( _Inout_ STRLIST *pList, _In_ LPCWSTR pStr );		/// Add a copy of the string
void StrListDestroy( _Inout_ STRLIST *pList );


//+ Unicode
#ifdef _UNICODE
	#define FileExists		FileExistsW
	#define StrListAdd		StrListAddW
	#define MyStrDup		MyStrDupWW		/// LPTSTR -> LPTSTR
	#define MyStrDupA		MyStrDupAW		/// LPTSTR -> LPSTR
#else
	#define FileExists		FileExistsA
	#define StrListAdd		StrListAddA
	#define MyStrDup		MyStrDupAA		/// LPTSTR -> LPTSTR
	#define MyStrDupA		MyStrDupAA		/// LPTSTR -> LPSTR
#endif
