
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
VOID AllocErrorStr( _In_ DWORD dwErrCode, _Out_ TCHAR **ppszErrText );		/// Call MyFree(ppszErrText) when no longer needed

//+ Memory
/// We'll use global memory, to be compatible with NSIS API
typedef struct {
	UINT64 AllocBytes, FreeBytes;
	UINT   AllocCalls, FreeCalls;
} MEMORY_STATS;
extern MEMORY_STATS g_MemStats;

#define MyAlloc(_size)		GlobalLock(GlobalAlloc(GMEM_FIXED, _size)); g_MemStats.AllocBytes += (_size); g_MemStats.AllocCalls++
#define MyAllocStr(_len)	(LPTSTR)MyAlloc( ((_len) + 1) * sizeof(TCHAR))
#define MyFree(_ptr)		if ( _ptr ) { g_MemStats.FreeBytes += GlobalSize((HGLOBAL)(_ptr)); g_MemStats.FreeCalls++; GlobalFree((HGLOBAL)(_ptr)); (_ptr) = NULL; }

#define MyDataDup(_dst, _src, _size) { \
	(_dst) = MyAlloc( _size ); \
	if (_dst) { \
		LPBYTE pDst = (LPBYTE)(_dst); \
		LPBYTE pSrc = (LPBYTE)(_src); \
		ULONG i; \
		for ( i = 0; i < (_size); i++, pDst++, pSrc++ ) \
			*pDst = *pSrc; \
	} \
}

#define MyStrDup(_dst, _src) { \
	(_dst) = MyAllocStr( lstrlen( _src )); \
	if (_dst) \
		lstrcpy( (LPTSTR)(_dst), (LPTSTR)(_src) ); \
}

#define MyZeroMemory(_ptr, _cnt) { \
	LPBYTE p; \
	for ( p = (LPBYTE)(_ptr) + (_cnt) - 1; p >= (LPBYTE)(_ptr); p-- ) *p = 0; \
}

//+ VALID_HANDLE
#define VALID_HANDLE(h) \
	((h) != NULL) && ((h) != INVALID_HANDLE_VALUE)

//+ FileExists
BOOL FileExists( _In_ LPCTSTR pszFile );

//+ MyTimeDiff
// Returns milliseconds
ULONG MyTimeDiff( __in PFILETIME pEndTime, __in PFILETIME pStartTime );

//+ ReadVersionInfoString
// Returns Win32 error
DWORD ReadVersionInfoString( __in_opt LPCTSTR szFile, __in LPCTSTR szStringName, __out LPTSTR szStringValue, __in UINT iStringValueLen );

//+ ExtractResourceFile
ULONG ExtractResourceFile( _In_ HMODULE hMod, _In_ LPCTSTR pszResType, _In_ LPCTSTR pszResName, _In_ USHORT iResLang, _In_ LPCTSTR pszOutPath );

//+ BinaryToString
// Returns number of TCHAR-s written, not including the NULL terminator
ULONG BinaryToString( __in LPVOID pData, __in ULONG iDataSize, __out LPTSTR pszStr, __in ULONG iStrLen );

//+ MyStrToInt64
// Replacement for shlwapi!StrToInt64Ex introduced in "Update Rollup 1 for Windows 2000 SP4"
BOOL MyStrToInt64( _In_ LPCTSTR pszStr, _Out_ PUINT64 piNum );