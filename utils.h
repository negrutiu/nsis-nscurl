
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/02/02

#pragma once

//+ Initialization
VOID UtilsInitialize(void);
VOID UtilsDestroy(void);


//+ TRACE
#if defined(DBG) || defined(_DEBUG)
	#define TRACE_ENABLED
#endif
#define TRACE_NO_PREFIX _T("\x0001")

#if defined (TRACE_ENABLED)
	#define TRACE TraceImpl
    #define TRACE2 TraceImpl	// More verbosity
	VOID TraceImpl( _In_z_ _Printf_format_string_ LPCTSTR pszFormat, _In_ ... );
#else
	#define TRACE(...)
	#define TRACE2(...)
#endif


//+ assert
#if defined(DBG) || defined(_DEBUG)
	#define assert(expr) \
		if ( !(expr)) { \
			TCHAR szMsg[512]; \
			TRACE( _T("  [!] %s, %s:%ld\n"), _T(#expr), _T( __FILE__ ), __LINE__ ); \
			_sntprintf( szMsg, (int)ARRAYSIZE( szMsg ), _T("%s\n%s : %ld\n"), _T(#expr), _T( __FILE__ ), __LINE__ ); \
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
	HGLOBAL h = GlobalAlloc( GMEM_FIXED, iSize );
	if (h) {
		LPVOID p = GlobalLock( h );
		if (p) {
			g_MemStats.AllocBytes += iSize;
			g_MemStats.AllocCalls++;
		#ifdef _DEBUG
			FillMemory( p, iSize, 0xcd );
		#endif
			return p;
		}
	}
	return NULL;
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

/// \brief Check if a character is a valid path separator (backslash or forward slash).
#define IsPathSeparator(ch) \
    ((ch) == _T('\\') || (ch) == _T('/'))

// ANSI (multi byte) <-> Unicode (wide char)
typedef enum { eA2A, eA2W, eA2T, eW2A, eW2W, eW2T, eT2A, eT2W, eT2T } Encodings;

/// \brief Convert string to number. \n
/// Syntax: \c "[+|-][0x][0]digits"
/// - Sign \c "+|-" is optional
/// - \c "0x" and \c "0X" prefixes indicate a hexadecimal number
/// - \c "0" prefix indicates an octal number
/// \param s Input string
/// \param nextChar Receives pointer to the next character
/// \param skipSpaces If \c TRUE, leading and trailing whitespaces are ignored (i.e. "  -10  " returns -10)
/// \return If the function fails, the return value is zero and \c nextChar is equal to \c s.
INT_PTR MyAtoi(LPCTSTR s, LPCTSTR* nextChar, BOOL skipSpaces);

//+ MyStrDup
// If the input string is NULL, the function allocates and returns an empty string (length zero)
// The caller must MyFree(..) the string
LPVOID MyStrDup ( _In_ Encodings iEnc, _In_ LPCVOID pszSrc );
LPVOID MyStrDupN( _In_ Encodings iEnc, _In_ LPCVOID pszSrc, _In_opt_ int iSrcMaxLen );		/// If iSrcMaxLen < 0 the string is assumed to be NULL terminated


//+ MyStrCopy
LPVOID MyStrCopy ( _In_ Encodings iEnc, _In_ LPVOID pszDest, _In_ ULONG iDestMaxLen, _In_ LPCVOID pszSrc );
LPVOID MyStrCopyN( _In_ Encodings iEnc, _In_ LPVOID pszDest, _In_ ULONG iDestMaxLen, _In_ LPCVOID pszSrc, _In_opt_ int iSrcMaxLen );		/// If iSrcMaxLen < 0 the string is assumed to be NULL terminated


//+ MyFormatError
// The caller must MyFree(..) the string
LPCTSTR MyFormatError( _In_ ULONG err );

/// \brief Canonicalize path (see \c GetFullPathName).
/// \param pszPath Absolute or relative path.
/// \return New buffer with the canonical path. \n
///         In case of errors, a copy of the original path is returned. \n
///         The caller must call \c MyFree to release it.
LPTSTR MyCanonicalizePath(_In_ LPCTSTR pszPath);

//+ MyFileExists
BOOL MyFileExistsW( _In_ LPCWSTR pszFile );
BOOL MyFileExistsA( _In_ LPCSTR pszFile );


//+ MyCreateDirectory
// Create a directory tree, including intermediate sub-directories
// bHasFilename indicates that the specified path is a full file path. The file name will be stripped away before creating directories
ULONG MyCreateDirectory( _In_ LPCTSTR pszPath, _In_ BOOLEAN bHasFilename );


//+ MySetThreadName
// Available in Win10+
void MySetThreadName( _In_ HANDLE hThread, _In_ LPCWSTR pszName );


//+ MyReadVersionString
// Reads a named string (e.g. "FileVersion", "CompanyName", "LegalCopyright", etc.) from an executable's Version Information block
// Returns Win32 error
ULONG MyReadVersionString( _In_opt_ LPCTSTR szFile, _In_ LPCTSTR szStringName, _Out_ LPTSTR szStringValue, _In_ UINT iStringValueLen );


//+ MyQueryResource
// Retrieve a const pointer to PE resource
ULONG MyQueryResource( _In_ HMODULE hMod, _In_ LPCTSTR pszResType, _In_ LPCTSTR pszResName, _In_ USHORT iResLang, _Out_ void **ppData, _Out_opt_ ULONG *piDataSize );


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
typedef void (CALLBACK *REPLACE_KEYWORD_CALLBACK_A)(_Inout_ LPSTR pszKeyword, _In_ ULONG iMaxLen, _In_opt_ PVOID pParam);
typedef void (CALLBACK *REPLACE_KEYWORD_CALLBACK_W)(_Inout_ LPWSTR pszKeyword, _In_ ULONG iMaxLen, _In_opt_ PVOID pParam);
LONG MyReplaceKeywordsA(
	_Inout_ LPSTR pszStr, _In_ LONG iMaxLen,
	_In_ CHAR chKeywordStart, _In_ CHAR chKeywordEnd,
	_In_ REPLACE_KEYWORD_CALLBACK_A fnReplace, _In_opt_ LPVOID pReplaceParam
);
LONG MyReplaceKeywordsW(
	_Inout_ LPWSTR pszStr, _In_ LONG iMaxLen,
	_In_ WCHAR chKeywordStart, _In_ WCHAR chKeywordEnd,
	_In_ REPLACE_KEYWORD_CALLBACK_W fnReplace, _In_opt_ LPVOID pReplaceParam
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
void MyFormatMilliseconds( _In_ curl_off_t iMillis, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen, _In_ BOOL bUseInfinitySign );


//+ MyStringToMilliseconds
// Convert strings like "30s", "15m", "24h" to milliseconds
// Input strings without any suffix is assumed to be milliseconds (e.g. "2500" will return 2500)
UINT_PTR MyStringToMilliseconds( _In_ LPCTSTR pszStr );


//+ Input Data
#define IDATA_TYPE_STRING		's'
#define IDATA_TYPE_FILE			'f'
#define IDATA_TYPE_MEM			'm'
typedef struct {
	char Type;					/// IDATA_TYPE_XXX
	union {
		LPCSTR  Str;			/// utf8
		LPCTSTR File;			/// TCHAR
		PUCHAR  Mem;			/// LPBYTE
	};
	curl_off_t Size;
} IDATA;
void IDataInitialize( _Inout_ IDATA *pData );
void IDataDestroy( _Inout_ IDATA *pData );
ULONG IDataParseParam( _In_ LPTSTR pszParam, _In_ int iParamMaxLen, _Out_ IDATA *pData );	/// Return Win32 error


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
