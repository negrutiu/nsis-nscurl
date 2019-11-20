
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/02/02

#include "main.h"
#include "utils.h"

MEMORY_STATS g_MemStats = { 0 };


//++ UtilsInitialize
VOID UtilsInitialize()
{
	TRACE( _T( "  UtilsInitialize()\n" ) );
}


//++ UtilsDestroy
VOID UtilsDestroy()
{
	TRACE( _T( "  MyAlloc: %u calls, %I64u bytes\n" ), g_MemStats.AllocCalls, g_MemStats.AllocBytes );
	TRACE( _T( "  MyFree:  %u calls, %I64u bytes\n" ), g_MemStats.FreeCalls, g_MemStats.FreeBytes );
	TRACE( _T( "  UtilsDestroy()\n" ) );
}


//++ TraceImpl
#if defined (TRACE_ENABLED)
VOID TraceImpl( __in LPCTSTR pszFormat, ... )
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


//++ AllocErrorStr
VOID AllocErrorStr( _In_ DWORD dwErrCode, _Out_ TCHAR **ppszErrText )
{
	if ( ppszErrText ) {

		DWORD iLen, iFlags = 0;
		TCHAR szError[512];
		HMODULE hModule = NULL;

	/*	if ( dwErrCode >= INTERNET_ERROR_BASE && dwErrCode <= INTERNET_ERROR_LAST ) {
			hModule = GetModuleHandle( _T( "wininet.dll" ) );
			iFlags = FORMAT_MESSAGE_FROM_HMODULE;
		} else */ {
			iFlags = FORMAT_MESSAGE_FROM_SYSTEM;
		}

		szError[0] = 0;
		iLen = FormatMessage( FORMAT_MESSAGE_IGNORE_INSERTS | iFlags, hModule, dwErrCode, 0, szError, ARRAYSIZE( szError ), NULL );
		if ( iLen > 0 ) {
			//x StrTrim( szError, _T( ". \r\n" ) );
			for (iLen--; (iLen > 0) && (szError[iLen] == _T('.') || szError[iLen] == _T(' ') || szError[iLen] == _T('\r') || szError[iLen] == _T('\n')); iLen--)
				szError[iLen] = _T('\0');
			iLen++;
			MyStrDup( *ppszErrText, szError );
		}
	}
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
ULONG MyTimeDiff( __in PFILETIME pEndTime, __in PFILETIME pStartTime )
{
	if (pStartTime && pEndTime) {
		ULONGLONG iDiff = (((PULARGE_INTEGER)pEndTime)->QuadPart - ((PULARGE_INTEGER)pStartTime)->QuadPart) / 10000;
		return (iDiff < UINT_MAX) ? (ULONG)iDiff : UINT_MAX;	/// UINT_MAX == ~49 days
	}
	return 0;
}


//++ ReadVersionInfoString
ULONG ReadVersionInfoString( _In_opt_ LPCTSTR szFile, _In_ LPCTSTR szStringName, _Out_ LPTSTR szStringValue, _In_  UINT iStringValueLen )
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
TCHAR Dec2Hex( _In_ UCHAR dec )
{
	if (dec < 10) {
		return dec + _T( '0' );
	} else if (dec < 16) {
		return dec - 10 + _T( 'a' );
	}
	return _T( '.' );
}


//++ BinaryToHex
ULONG BinaryToHex( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPTSTR pszStr, _In_ ULONG iStrLen )
{
	ULONG i, j, n;

	if (pszStr && iStrLen)
		pszStr[0] = 0;

	n = __min( iDataSize, (iStrLen - 1) / 2 );		/// Total input bytes that fit in the output buffer, not including \0
	for (i = 0, j = 0; i < n; i++, j = i << 1) {
		pszStr[j] = Dec2Hex( ((PUCHAR)pData)[i] >> 4 );
		pszStr[j + 1] = Dec2Hex( ((PUCHAR)pData)[i] & 0xf );
	}

	pszStr[j] = _T( '\0' );
	return j;										/// Characters written, not including \0
}


//++ BinaryToString
ULONG BinaryToString(
	__in LPVOID pData, __in ULONG iDataSize,
	__out LPTSTR pszStr, __in ULONG iStrLen
	)
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
