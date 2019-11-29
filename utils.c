
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/02/02

#include "main.h"
#include "utils.h"

MEMORY_STATS g_MemStats = { 0 };


//++ UtilsInitialize
VOID UtilsInitialize()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );
}


//++ UtilsDestroy
VOID UtilsDestroy()
{
	TRACE( _T( "%hs( Alloc: #%u/%I64u bytes, Free: #%u/%I64u bytes )\n" ), __FUNCTION__, g_MemStats.AllocCalls, g_MemStats.AllocBytes, g_MemStats.FreeCalls, g_MemStats.FreeBytes );
	assert( g_MemStats.AllocBytes == g_MemStats.FreeBytes );
}


//++ TraceImpl
#if defined (TRACE_ENABLED)
VOID TraceImpl( _In_ LPCTSTR pszFormat, _In_ ... )
{
	DWORD err = ERROR_SUCCESS;
	if ( pszFormat && *pszFormat ) {

		TCHAR szStr[1024];
		int iLen1, iLen2;
		va_list args;

		iLen1 = _sntprintf( szStr, ARRAYSIZE(szStr), _T( "[nscurl.th%04x] " ), GetCurrentThreadId() );

		va_start( args, pszFormat );
		iLen2 = _vsntprintf( szStr + iLen1, (int)ARRAYSIZE( szStr ) - iLen1, pszFormat, args );
		if ( iLen2 > 0 ) {
			if ( iLen1 + iLen2 < ARRAYSIZE( szStr ) )
				szStr[iLen1 + iLen2] = 0;	/// The string is not guaranteed to be null terminated
		} else {
			szStr[ARRAYSIZE( szStr ) - 1] = 0;
		}
		OutputDebugString( szStr );
		va_end( args );
	}
}
#endif


//++ SetThreadName
void SetThreadName( _In_ HANDLE hThread, _In_ LPCWSTR pszName )
{
	typedef HRESULT( WINAPI *TfnSetThreadDescription )(_In_ HANDLE hThread, _In_ PCWSTR lpThreadDescription);
	#define NO_INIT (TfnSetThreadDescription)1

	// One-time initialization
	static TfnSetThreadDescription fnSetThreadDescription = NO_INIT;
	if (fnSetThreadDescription == NO_INIT)
		fnSetThreadDescription = (TfnSetThreadDescription)GetProcAddress( GetModuleHandle( _T("kernel32") ), "SetThreadDescription" );

	if (fnSetThreadDescription > NO_INIT)
		fnSetThreadDescription( hThread, pszName );
	#undef NO_INIT
}


//++ MyStrDupAA
LPSTR MyStrDupAA( _In_ LPCSTR pStr )
{
	if (pStr) {
		ULONG l = lstrlenA( pStr );
		LPSTR psz = (LPSTR)MyAlloc( l + 1 );
		if (psz) {
			CopyMemory( psz, pStr, l + 1 );
			return psz;
		}
	}
	return NULL;
}


//++ MyStrDupAW
LPSTR MyStrDupAW( _In_ LPCWSTR pStr )
{
	if (pStr) {
		int l = WideCharToMultiByte( CP_UTF8, 0, pStr, -1, NULL, 0, NULL, 0 );		/// Returns length including \0
		if (l > 0) {
			LPSTR psz = (LPSTR)MyAlloc( l );
			if (psz && (l = WideCharToMultiByte( CP_UTF8, 0, pStr, -1, psz, l, NULL, 0 )) > 0)
				return psz;
		}
	}
	return NULL;
}


//++ MyStrDupWA
LPWSTR MyStrDupWA( _In_ LPCSTR pStr )
{
	if (pStr) {
		int l = MultiByteToWideChar( CP_UTF8, 0, pStr, -1, NULL, 0 );		/// Returns length including \0
		if (l > 0) {
			LPWSTR psz = (LPWSTR)MyAlloc( l * sizeof(WCHAR) );
			if (psz && (l = MultiByteToWideChar( CP_UTF8, 0, pStr, -1, psz, l )) > 0)
				return psz;
		}
	}
	return NULL;
}


//++ MyStrDupWW
LPWSTR MyStrDupWW( _In_ LPCWSTR pStr )
{
	if (pStr) {
		ULONG l = (lstrlenW( pStr ) + 1) * sizeof(WCHAR);
		LPWSTR psz = (LPWSTR)MyAlloc( l );
		if (psz) {
			CopyMemory( psz, pStr, l );
			return psz;
		}
	}
	return NULL;
}


//++ MyErrorStr
LPCTSTR MyErrorStr( _In_ ULONG err )
{
	TCHAR szError[512];
	DWORD iLen, iFlags = 0;
	HMODULE hModule = NULL;

/*	if ( err >= INTERNET_ERROR_BASE && dwErrCode <= INTERNET_ERROR_LAST ) {
		hModule = GetModuleHandle( _T( "wininet.dll" ) );
		iFlags = FORMAT_MESSAGE_FROM_HMODULE;
	} else */ {
		iFlags = FORMAT_MESSAGE_FROM_SYSTEM;
	}

	szError[0] = 0;
	iLen = FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | iFlags, hModule, err, 0, szError, ARRAYSIZE(szError), NULL );
	if ( iLen > 0 ) {
		//x StrTrim( szError, _T( ". \r\n" ) );
		for (iLen--; (iLen > 0) && (szError[iLen] == _T('.') || szError[iLen] == _T(' ') || szError[iLen] == _T('\r') || szError[iLen] == _T('\n')); iLen--)
			szError[iLen] = _T('\0');
		iLen++;
	}

	return MyStrDup( szError );
}


//++ FileExistsA
BOOL FileExistsA( _In_ LPCSTR pszFile )
{
	if (pszFile && *pszFile) {
		WIN32_FIND_DATAA fd;
		HANDLE hFind = FindFirstFileA( pszFile, &fd );
		if (hFind != INVALID_HANDLE_VALUE) {
			FindClose( hFind );
			return TRUE;
		}
	}
	return FALSE;
}


//++ FileExistsW
BOOL FileExistsW( _In_ LPCWSTR pszFile )
{
	if (pszFile && *pszFile) {
		WIN32_FIND_DATAW fd;
		HANDLE hFind = FindFirstFileW( pszFile, &fd );
		if (hFind != INVALID_HANDLE_VALUE) {
			FindClose( hFind );
			return TRUE;
		}
	}
	return FALSE;
}


//++ MyTimeDiff
ULONG MyTimeDiff( _In_ PFILETIME pEndTime, _In_ PFILETIME pStartTime )
{
	if (pStartTime && pEndTime) {
		ULONGLONG iDiff = (((PULARGE_INTEGER)pEndTime)->QuadPart - ((PULARGE_INTEGER)pStartTime)->QuadPart) / 10000;
		return (iDiff < UINT_MAX) ? (ULONG)iDiff : UINT_MAX;	/// UINT_MAX == ~49 days
	}
	return 0;
}


//++ ReadVersionInfoString
ULONG ReadVersionInfoString( _In_opt_ LPCTSTR szFile, _In_ LPCTSTR szStringName, _Out_ LPTSTR szStringValue, _In_ UINT iStringValueLen )
{
	ULONG e = ERROR_SUCCESS;

	// Validate parameters
	if (szStringName && *szStringName && szStringValue && (iStringValueLen > 0)) {

		ULONG iVerInfoSize;
		TCHAR szExeFile[MAX_PATH];
		szStringValue[0] = 0;

		if (szFile && *szFile) {
			lstrcpyn( szExeFile, szFile, ARRAYSIZE( szExeFile ) );
		} else {
			GetModuleFileName( NULL, szExeFile, ARRAYSIZE( szExeFile ) );	/// Current executable
		}

		iVerInfoSize = GetFileVersionInfoSize( szExeFile, NULL );
		if (iVerInfoSize > 0) {
			HANDLE hMem = GlobalAlloc( GMEM_MOVEABLE, iVerInfoSize );
			if (hMem) {
				LPBYTE pMem = GlobalLock( hMem );
				if (pMem) {
					if (GetFileVersionInfo( szExeFile, 0, iVerInfoSize, pMem )) {
						typedef struct _LANGANDCODEPAGE { WORD wLanguage; WORD wCodePage; } LANGANDCODEPAGE;
						LANGANDCODEPAGE *pCodePage;
						UINT iCodePageSize = sizeof( *pCodePage );
						/// Code page
						if (VerQueryValue( pMem, _T( "\\VarFileInfo\\Translation" ), (LPVOID*)&pCodePage, &iCodePageSize )) {
							TCHAR szTemp[255];
							LPCTSTR szValue = NULL;
							UINT iValueLen = 0;
							/// Read version string
							_sntprintf( szTemp, ARRAYSIZE( szTemp ), _T( "\\StringFileInfo\\%04x%04x\\%s" ), pCodePage->wLanguage, pCodePage->wCodePage, szStringName );
							if (VerQueryValue( pMem, szTemp, (LPVOID*)&szValue, &iValueLen )) {
								/// Output
								if (*szValue) {
									lstrcpyn( szStringValue, szValue, iStringValueLen );
									if (iValueLen > iStringValueLen) {
										/// The output buffer is not large enough
										/// We'll return the truncated string, and ERROR_BUFFER_OVERFLOW error code
										e = ERROR_BUFFER_OVERFLOW;
									}
								} else {
									e = ERROR_NOT_FOUND;
								}
							} else {
								e = ERROR_NOT_FOUND;
							}
						} else {
							e = ERROR_NOT_FOUND;
						}
					} else {
						e = GetLastError();
					}
					GlobalUnlock( hMem );
				} else {
					e = GetLastError();
				}
				GlobalFree( hMem );
			} else {
				e = GetLastError();
			}
		} else {
			e = GetLastError();
		}
	} else {
		e = ERROR_INVALID_PARAMETER;
	}
	return e;
}


//++ ExtractResourceFile
ULONG ExtractResourceFile( _In_ HMODULE hMod, _In_ LPCTSTR pszResType, _In_ LPCTSTR pszResName, _In_ USHORT iResLang, _In_ LPCTSTR pszOutPath )
{
	ULONG e = ERROR_SUCCESS;
	HRSRC hRes = NULL;

	if (!pszResType || !pszResName || !pszOutPath || !*pszOutPath)
		return ERROR_INVALID_PARAMETER;
	
	hRes = FindResourceEx( hMod, pszResType, pszResName, iResLang );
	if (hRes) {
		ULONG iResSize = SizeofResource( hMod, hRes );
		HGLOBAL hMem = LoadResource( hMod, hRes );
		if (hMem) {
			LPVOID pRes = LockResource( hMem );
			if (pRes) {
				HANDLE h = CreateFile( pszOutPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
				if (h != INVALID_HANDLE_VALUE) {
					ULONG iWritten;
					if (WriteFile( h, pRes, iResSize, &iWritten, NULL )) {
						// OK
					} else {
						e = GetLastError();
					}
					CloseHandle( h );
				}
			} else {
				e = ERROR_INVALID_DATA;
			}
		} else {
			e = GetLastError();
		}
	} else {
		e = GetLastError();
	}

	return e;
}


//++ Dec2Hex
UCHAR Dec2Hex( _In_ UCHAR dec )
{
	if (dec < 10) {
		return dec + '0';
	} else if (dec < 16) {
		return dec - 10 + 'a';
	}
	return '.';
}


//++ BinaryToHexW
ULONG BinaryToHexW( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPWSTR pszStr, _In_ ULONG iStrLen )
{
	ULONG i, j, n;

	if (pszStr && iStrLen)
		pszStr[0] = 0;

	n = __min( iDataSize, (iStrLen - 1) / 2 );		/// Total input bytes that fit in the output buffer, not including \0
	for (i = 0, j = 0; i < n; i++, j = i << 1) {
		pszStr[j] = (WCHAR)Dec2Hex( ((PUCHAR)pData)[i] >> 4 );
		pszStr[j + 1] = (WCHAR)Dec2Hex( ((PUCHAR)pData)[i] & 0xf );
	}

	pszStr[j] = UNICODE_NULL;
	return j;										/// Characters written, not including \0
}


//++ BinaryToHexA
ULONG BinaryToHexA( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPSTR pszStr, _In_ ULONG iStrLen )
{
	ULONG i, j, n;

	if (pszStr && iStrLen)
		pszStr[0] = 0;

	n = __min( iDataSize, (iStrLen - 1) / 2 );		/// Total input bytes that fit in the output buffer, not including \0
	for (i = 0, j = 0; i < n; i++, j = i << 1) {
		pszStr[j] = (CHAR)Dec2Hex( ((PUCHAR)pData)[i] >> 4 );
		pszStr[j + 1] = (CHAR)Dec2Hex( ((PUCHAR)pData)[i] & 0xf );
	}

	pszStr[j] = ANSI_NULL;
	return j;										/// Characters written, not including \0
}


//++ BinaryToString
ULONG BinaryToString( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPTSTR pszStr, _In_ ULONG iStrLen )
{
	ULONG iLen = 0;
	if (pszStr && iStrLen) {
		pszStr[0] = _T( '\0' );
		if (pData) {
			ULONG i, n;
			CHAR ch;
			for (i = 0, n = (ULONG)__min( iDataSize, iStrLen - 1 ); i < n; i++) {
				ch = ((PCH)pData)[i];
				if ((ch >= 32 /*&& ch < 127*/) || ch == '\r' || ch == '\n') {
					pszStr[i] = ch;
				} else {
					pszStr[i] = _T( '.' );
				}
			}
			pszStr[i] = _T( '\0' );
			iLen += i;		/// Not including NULL terminator
		}
	}
	return iLen;
}


//++ ReplaceKeywordsA
LONG ReplaceKeywordsA( _Inout_ LPSTR pszStr, _In_ LONG iMaxLen, _In_ CHAR chKeywordStart, _In_ CHAR chKeywordEnd, _In_ REPLACE_KEYWORD_CALLBACK_A fnReplace, _In_ LPVOID pReplaceParam )
{
	LPSTR pszBuf = NULL, psz1, psz2;
	LONG iStrLen;
	LONG l1, l2;

	if (!pszStr || !iMaxLen || !fnReplace)
		return -1;

	pszBuf = (LPSTR)malloc( iMaxLen * sizeof( CHAR ) );		/// Working buffer
	if (!pszBuf)
		return -1;

	iStrLen = lstrlenA( pszStr );
	for (psz1 = pszStr; *psz1; ) {

		// Find the next keyword start
		for (; *psz1 && (*psz1 != chKeywordStart); psz1++);
		if (!*psz1)
			break;

		// Find the next keyword end
		for (psz2 = psz1 + 1; *psz2 && (*psz2 != chKeywordEnd); psz2++);
		if (!*psz2++)
			break;

		// Replace callback function
		lstrcpynA( pszBuf, psz1, (int)(psz2 - psz1) + 1 );
		fnReplace( pszBuf, iMaxLen, pReplaceParam );

		l1 = (LONG)(psz2 - psz1);			// Old substring length
		l2 = (LONG)lstrlenA( pszBuf );		// New substring length
		if ((l1 != l2) || (CompareStringA( CP_ACP, 0, pszBuf, -1, psz1, l1 ) != CSTR_EQUAL)) {

			if ((iStrLen + l2 - l1) < iMaxLen) {
				// Replace keyword <-> value
				size_t iSubstrIndex = psz1 - pszStr;
				size_t iMoveSize = (iStrLen - iSubstrIndex - l1 + 1) * sizeof( CHAR );
				MoveMemory( psz1 + l2, psz1 + l1, iMoveSize );
				CopyMemory( psz1, pszBuf, l2 * sizeof( CHAR ) );
				iStrLen += (l2 - l1);
				psz1 += l2;
			} else {
				//x ERROR_INSUFFICIENT_BUFFER;
				iStrLen = -1;
				break;
			}

		} else {
			// The keyword/substring is unmodified
			psz1++;
		}
	}	/// for

	free( pszBuf );
	return iStrLen;
}


//++ ReplaceKeywordsW
LONG ReplaceKeywordsW( _Inout_ LPWSTR pszStr, _In_ LONG iMaxLen, _In_ WCHAR chKeywordStart, _In_ WCHAR chKeywordEnd, _In_ REPLACE_KEYWORD_CALLBACK_W fnReplace, _In_ LPVOID pReplaceParam )
{
	LPWSTR pszBuf = NULL, psz1, psz2;
	LONG iStrLen;
	LONG l1, l2;

	if (!pszStr || !iMaxLen || !fnReplace)
		return -1;

	pszBuf = (LPWSTR)malloc( iMaxLen * sizeof( WCHAR ) );		/// Working buffer
	if (!pszBuf)
		return -1;

	iStrLen = lstrlenW( pszStr );
	for (psz1 = pszStr; *psz1; ) {

		// Find the next keyword start
		for (; *psz1 && (*psz1 != chKeywordStart); psz1++);
		if (!*psz1)
			break;

		// Find the next keyword end
		for (psz2 = psz1 + 1; *psz2 && (*psz2 != chKeywordEnd); psz2++);
		if (!*psz2++)
			break;

		// Replace callback function
		lstrcpynW( pszBuf, psz1, (int)(psz2 - psz1) + 1 );
		fnReplace( pszBuf, iMaxLen, pReplaceParam );

		l1 = (LONG)(psz2 - psz1);			// Old substring length
		l2 = (LONG)lstrlenW( pszBuf );		// New substring length
		if ((l1 != l2) || (CompareStringW( CP_ACP, 0, pszBuf, -1, psz1, l1 ) != CSTR_EQUAL)) {

			if ((iStrLen + l2 - l1) < iMaxLen) {
				// Replace keyword <-> value
				size_t iSubstrIndex = psz1 - pszStr;
				size_t iMoveSize = (iStrLen - iSubstrIndex - l1 + 1) * sizeof( WCHAR );
				MoveMemory( psz1 + l2, psz1 + l1, iMoveSize );
				CopyMemory( psz1, pszBuf, l2 * sizeof( WCHAR ) );
				iStrLen += (l2 - l1);
				psz1 += l2;
			} else {
				//x ERROR_INSUFFICIENT_BUFFER;
				iStrLen = -1;
				break;
			}

		} else {
			// The keyword/substring is unmodified
			psz1++;
		}
	}	/// for

	free( pszBuf );
	return iStrLen;
}


//++ MyStrToInt64
BOOL MyStrToInt64( _In_ LPCTSTR pszStr, _Out_ PUINT64 piNum )
{
	BOOL bRet = FALSE;
	int ch, i;
	UINT64 n;
	if (pszStr && piNum) {
		for (*piNum = 0;;) {

			ch = *(pszStr++) - _T( '0' );
			if (ch < 0 || ch > 9)
				break;

			/// *piNum *= 10;
			for (i = 0, n = *piNum; i < 9; i++)
				*piNum += n;

			*piNum += ch;
		}
		bRet = TRUE;
	}
	return bRet;
}


//++ VirtualMemoryInitialize
ULONG VirtualMemoryInitialize( _Inout_ VMEMO *pMem, _In_ SIZE_T iMaxSize )
{
	if (pMem) {
		ZeroMemory( pMem, sizeof( *pMem ) );
		pMem->pMem = (PCCH)VirtualAlloc( NULL, iMaxSize, MEM_RESERVE, PAGE_READWRITE );
		if (pMem->pMem) {
			MEMORY_BASIC_INFORMATION mbi = {0};
			pMem->iReserved = (VirtualQuery( pMem->pMem, &mbi, sizeof( mbi ) ) > 0) ? mbi.RegionSize : iMaxSize;
		} else {
			return GetLastError();
		}
	}
	return ERROR_SUCCESS;
}

//++ VirtualMemoryAppend
SIZE_T VirtualMemoryAppend( _Inout_ VMEMO *pMem, _In_ PVOID buf, _In_ SIZE_T size )
{
	ULONG e = ERROR_SUCCESS;
	if (pMem && pMem->pMem && buf && size) {

		/// Commit more virtual memory as needed
		if (pMem->iSize + size > pMem->iCommitted) {
			SYSTEM_INFO si;
			GetSystemInfo( &si );
			SIZE_T n = ((size + si.dwPageSize) / si.dwPageSize) * si.dwPageSize;
			pMem->iCommitted = __min( pMem->iCommitted + n, pMem->iReserved );
			e = VirtualAlloc( (LPVOID)pMem->pMem, pMem->iCommitted, MEM_COMMIT, PAGE_READWRITE ) ? ERROR_SUCCESS : GetLastError();
		}

		/// Write
		if (e == ERROR_SUCCESS) {
			SIZE_T n = __min( size, pMem->iReserved - pMem->iSize );
			if (n > 0) {
				CopyMemory( (LPVOID)(pMem->pMem + pMem->iSize), buf, n );
				pMem->iSize += n;
			}
			return n;
		}
	}
	return 0;
}

//++ VirtualMemoryDestroy
void VirtualMemoryDestroy( _Inout_ VMEMO *pMem )
{
	if (pMem) {
		if (pMem->pMem)
			VirtualFree( (LPVOID)pMem->pMem, 0, MEM_RELEASE );
		ZeroMemory( pMem, sizeof( *pMem ) );
	}
}
