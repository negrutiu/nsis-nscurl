
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/02/02

#pragma once

//+ Initialization
VOID UtilsInitialize();
VOID UtilsDestroy();


//+ TRACE
//#if DBG || _DEBUG
	#define TRACE_ENABLED
//#endif

#if defined (TRACE_ENABLED)
	#define TRACE TraceImpl
	#define TRACE2(...)			/// More verbose tracing
	VOID TraceImpl( __in LPCTSTR pszFormat, ... );
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

//+ Error
LPCTSTR E32( _In_ ULONG err, _Out_ LPTSTR pszError, _In_ ULONG iErrorLen );

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

#define MyZeroMemory(_ptr, _cnt) { \
	LPBYTE p; \
	for ( p = (LPBYTE)(_ptr) + (_cnt) - 1; p >= (LPBYTE)(_ptr); p-- ) *p = 0; \
}

//+ VALID_HANDLE
#define VALID_HANDLE(h) \
	((h) != NULL) && ((h) != INVALID_HANDLE_VALUE)

//+ FileExists
BOOL FileExistsW( _In_ LPCWSTR pszFile );
BOOL FileExistsA( _In_ LPCSTR pszFile );
#ifdef _UNICODE
#define FileExists		FileExistsW
#else
#define FileExists		FileExistsA
#endif

//+ MyTimeDiff
// Returns milliseconds
ULONG MyTimeDiff( __in PFILETIME pEndTime, __in PFILETIME pStartTime );

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
ULONG BinaryToString( __in LPVOID pData, __in ULONG iDataSize, __out LPTSTR pszStr, __in ULONG iStrLen );

//+ MyStrToInt64
// Replacement for shlwapi!StrToInt64Ex introduced in "Update Rollup 1 for Windows 2000 SP4"
BOOL MyStrToInt64( _In_ LPCTSTR pszStr, _Out_ PUINT64 piNum );

//++ UTF-8 String List
typedef struct _UTF8LIST {
	LPCSTR String;
	struct _UTF8LIST *Next;
} U8LIST;

void InitializeU8List( _Inout_ U8LIST *pList );
void AddU8ListPtr( _Inout_ U8LIST *pList, _In_ LPCSTR pStr );		/// Add directly
void AddU8ListA( _Inout_ U8LIST *pList, _In_ LPCSTR pStr );			/// Add clone
void AddU8ListW( _Inout_ U8LIST *pList, _In_ LPCWSTR pStr );		/// Add clone
void DestroyU8List( _Inout_ U8LIST *pList );
#ifdef _UNICODE
	#define AddUtf8List		AddU8ListW
#else
	#define AddUtf8List		AddU8ListA
#endif