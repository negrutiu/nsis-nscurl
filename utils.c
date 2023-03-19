
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
VOID TraceImpl( _In_z_ _Printf_format_string_ LPCTSTR pszFormat, _In_ ... )
{
	DWORD err = ERROR_SUCCESS;
	if ( pszFormat && *pszFormat ) {

		TCHAR szStr[1024];
		int iLen1, iLen2;
		va_list args;

		va_start( args, pszFormat );

		if (pszFormat[0] == TRACE_NO_PREFIX[0]) {
			pszFormat++;
			iLen1 = 0;
		} else {
			iLen1 = _sntprintf( szStr, ARRAYSIZE( szStr ), _T( "[nscurl.th%04x] " ), GetCurrentThreadId() );
		}

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


//++ MySetThreadName
void MySetThreadName( _In_ HANDLE hThread, _In_ LPCWSTR pszName )
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


//++ MyCreateDirectory
ULONG MyCreateDirectory( _In_ LPCTSTR pszPath, _In_ BOOLEAN bHasFilename )
{
	ULONG e = ERROR_SUCCESS;
	LPTSTR pszBuf;

	if (!pszPath || !*pszPath)
		return ERROR_INVALID_PARAMETER;

	// Work with a copy of the string
	if ((pszBuf = MyStrDup( eT2T, pszPath )) != NULL) {

		TCHAR *psz, ch;
		ULONG len = lstrlen( pszBuf );
		// Strip trailing backslashes
		for (psz = pszBuf + len - 1; (psz > pszBuf) && (*psz == _T( '\\' ) || *psz == _T( '/' )); psz--)
			*psz = _T( '\0' ), len--;
		if (bHasFilename) {
			// Strip filename
			for (psz = pszBuf + len - 1; (psz > pszBuf) && (*psz != _T( '\\' )) && (*psz != _T( '/' )); psz--)
				*psz = _T( '\0' ), len--;
			// Strip trailing backslashes
			for (psz = pszBuf + len - 1; (psz > pszBuf) && (*psz == _T( '\\' ) || *psz == _T( '/' )); psz--)
				*psz = _T( '\0' ), len--;
		}
		psz = pszBuf;
		// Skip UNC server name (\\?\UNC\SERVER)
		if (CompareString( CP_ACP, NORM_IGNORECASE, psz, 8, _T( "\\\\?\\UNC\\" ), -1 ) == CSTR_EQUAL) {
			psz += 8;
			for (; *psz != _T( '\\' ) && *psz != _T( '\0' ); psz++);
		}
		// Skip \\?\ prefix
		if (CompareString( CP_ACP, NORM_IGNORECASE, psz, 4, _T( "\\\\?\\" ), -1 ) == CSTR_EQUAL)
			psz += 4;
		// Skip network server name (\\SERVER)
		if (CompareString( CP_ACP, NORM_IGNORECASE, psz, 2, _T( "\\\\" ), -1 ) == CSTR_EQUAL) {
			psz += 2;
			for (; *psz != _T( '\\' ) && *psz != _T( '\0' ); psz++);
		}
		// Create intermediate directories
		for (; (e == ERROR_SUCCESS) && *psz; ) {
			for (; *psz == _T( '\\' ); psz++);
			for (; *psz != _T( '\\' ) && *psz != _T( '\0' ); psz++);
			ch = *psz, *psz = _T( '\0' );
			e = CreateDirectory( pszBuf, NULL ) ? ERROR_SUCCESS : GetLastError();
			if (e == ERROR_ALREADY_EXISTS || e == ERROR_ACCESS_DENIED)
				e = ERROR_SUCCESS;
			*psz = ch;
		}

		MyFree( pszBuf );

	} else {
		e = ERROR_OUTOFMEMORY;
	}

	return e;
}


//++ MyStrDup
LPVOID MyStrDup( _In_ Encodings iEnc, _In_ LPCVOID pszSrc )
{
	return MyStrDupN( iEnc, pszSrc, -1 );
}


//++ MyStrDupN
LPVOID MyStrDupN( _In_ Encodings iEnc, _In_ LPCVOID pszSrc, _In_opt_ int iSrcMaxLen )
{
	LPVOID pszDest = NULL;
	int l, l2;

	// Resolve "T"-s
#ifdef _UNICODE
	if (iEnc == eA2T)
		iEnc = eA2W;
	else if (iEnc == eW2T || iEnc == eT2W || iEnc == eT2T)
		iEnc = eW2W;
	else if (iEnc == eT2A)
		iEnc = eW2A;
#else
	if (iEnc == eA2T || iEnc == eT2A || iEnc == eT2T)
		iEnc = eA2A;
	else if (iEnc == eW2T)
		iEnc = eW2A;
	else if (iEnc == eT2W)
		iEnc = eA2W;
#endif

	// Prepare input
	if (!pszSrc)
		pszSrc = L"";			/// Works for multi-byte strings as well
	if (iSrcMaxLen < 0)
		iSrcMaxLen = INT_MAX;

	// Duplicate
	if (iEnc == eW2W) {

		for (l = 0; l < iSrcMaxLen && ((LPCWSTR)pszSrc)[l]; l++);
		if ((pszDest = MyAlloc( (l + 1) * sizeof( WCHAR ) )) != NULL) {
			CopyMemory( pszDest, pszSrc, l * sizeof( WCHAR ) );
			((LPWSTR)pszDest)[l] = UNICODE_NULL;
		}

	} else if (iEnc == eA2A) {

		for (l = 0; l < iSrcMaxLen && ((LPCSTR)pszSrc)[l]; l++);
		if ((pszDest = MyAlloc( l + 1 )) != NULL) {
			CopyMemory( pszDest, pszSrc, l );
			((LPSTR)pszDest)[l] = ANSI_NULL;
		}

	} else if (iEnc == eW2A) {

		for (l = 0; l < iSrcMaxLen && ((LPCWSTR)pszSrc)[l]; l++);
		if (l > 0) {
			if ((l2 = WideCharToMultiByte( CP_UTF8, 0, (LPCWCH)pszSrc, l, NULL, 0, NULL, NULL )) > 0)
				if ((pszDest = MyAlloc( l2 + 1 )) != NULL)
					if ((l2 = WideCharToMultiByte( CP_UTF8, 0, (LPCWCH)pszSrc, l, (LPCH)pszDest, l2 + 1, NULL, NULL )) > 0)
						((LPCH)pszDest)[l2] = ANSI_NULL;
		} else {
			// NOTE: WideCharToMultiByte(..) fails with zero input length
			if ((pszDest = MyAlloc( sizeof( ANSI_NULL ) )) != NULL)
				((LPCH)pszDest)[0] = ANSI_NULL;
		}

	} else if (iEnc == eA2W) {

		for (l = 0; l < iSrcMaxLen && ((LPCSTR)pszSrc)[l]; l++);
		if (l > 0) {
			if ((l2 = MultiByteToWideChar( CP_UTF8, 0, (LPCCH)pszSrc, l, NULL, 0 )) > 0)
				if ((pszDest = MyAlloc( (l2 + 1) * sizeof(WCHAR) )) != NULL)
					if ((l2 = MultiByteToWideChar( CP_UTF8, 0, (LPCCH)pszSrc, l, (LPWCH)pszDest, l2 + 1 )) > 0)
						((LPWCH)pszDest)[l2] = UNICODE_NULL;
		} else {
			// NOTE: MultiByteToWideChar(..) fails with zero input length
			if ((pszDest = MyAlloc( sizeof( UNICODE_NULL ) )) != NULL)
				((LPWCH)pszDest)[0] = UNICODE_NULL;
		}

	} else {
		assert( !"MyStrCopy( Unexpected encoding )" );
	}

	assert( pszDest );
	return pszDest;
}


//++ MyStrCopy
LPVOID MyStrCopy( _In_ Encodings iEnc, _In_ LPVOID pszDest, _In_ ULONG iDestMaxLen, _In_ LPCVOID pszSrc )
{
	return MyStrCopyN( iEnc, pszDest, iDestMaxLen, pszSrc, -1 );
}


//++ MyStrCopyN
LPVOID MyStrCopyN( _In_ Encodings iEnc, _In_ LPVOID pszDest, _In_ ULONG iDestMaxLen, _In_ LPCVOID pszSrc, _In_opt_ int iSrcMaxLen )
{
	int iDestLen;
	if (!pszDest)
		return NULL;

	// Resolve "T"-s
#ifdef _UNICODE
	if (iEnc == eA2T)
		iEnc = eA2W;
	else if (iEnc == eW2T || iEnc == eT2W || iEnc == eT2T)
		iEnc = eW2W;
	else if (iEnc == eT2A)
		iEnc = eW2A;
#else
	if (iEnc == eA2T || iEnc == eT2A || iEnc == eT2T)
		iEnc = eA2A;
	else if (iEnc == eW2T)
		iEnc = eW2A;
	else if (iEnc == eT2W)
		iEnc = eA2W;
#endif

	// Copy
	iDestLen = (iSrcMaxLen < 0 ? INT_MAX : (iSrcMaxLen + 1));		/// Reserve room for \0
	iDestLen = __min( iDestLen, (int)iDestMaxLen );

	if (iEnc == eW2W) {
		lstrcpynW( (LPWSTR)pszDest, pszSrc ? (LPCWSTR)pszSrc : L"", iDestLen );
	} else if (iEnc == eA2A) {
		lstrcpynA( (LPSTR)pszDest, pszSrc ? (LPCSTR)pszSrc : "", iDestLen );
	} else if (iEnc == eW2A) {
		WideCharToMultiByte( CP_UTF8, 0, pszSrc ? (LPCWSTR)pszSrc : L"", iSrcMaxLen, (LPSTR)pszDest, (int)iDestMaxLen, NULL, NULL );
		((LPSTR)pszDest)[iDestLen-1] = ANSI_NULL;
	} else if (iEnc == eA2W) {
		MultiByteToWideChar( CP_UTF8, 0, pszSrc ? (LPCSTR)pszSrc : "", iSrcMaxLen, (LPWSTR)pszDest, (int)iDestMaxLen );
		((LPWSTR)pszDest)[iDestLen-1] = UNICODE_NULL;
	} else {
		assert( !"MyStrCopy( Unexpected encoding )" );
	}

	return pszDest;
}


//++ MyFormatError
LPCTSTR MyFormatError( _In_ ULONG err )
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

	return (LPCTSTR)MyStrDup( eT2T, szError );
}


//++ MyFileExistsA
BOOL MyFileExistsA( _In_ LPCSTR pszFile )
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


//++ MyFileExistsW
BOOL MyFileExistsW( _In_ LPCWSTR pszFile )
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


//++ MyReadVersionString
ULONG MyReadVersionString( _In_opt_ LPCTSTR szFile, _In_ LPCTSTR szStringName, _Out_ LPTSTR szStringValue, _In_ UINT iStringValueLen )
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


//++ MyQueryResource
ULONG MyQueryResource( _In_ HMODULE hMod, _In_ LPCTSTR pszResType, _In_ LPCTSTR pszResName, _In_ USHORT iResLang, _Out_ void **ppData, _Out_opt_ ULONG *piDataSize )
{
	ULONG e = ERROR_SUCCESS;
	HRSRC hRes = NULL;

	if (ppData)
		*ppData = NULL;
	if (piDataSize)
		*piDataSize = 0;
	if (!pszResType || !pszResName || !ppData)
		return ERROR_INVALID_PARAMETER;

	hRes = FindResourceEx( hMod, pszResType, pszResName, iResLang );
	if (hRes) {
		ULONG iResSize = SizeofResource( hMod, hRes );
		HGLOBAL hMem = LoadResource( hMod, hRes );
		if (hMem) {
			LPVOID pRes = LockResource( hMem );
			if (pRes) {
				// NOTE: The pointer returned by LockResource is valid until the module containing the resource is unloaded
				*ppData = pRes;
				if (piDataSize)
					*piDataSize = iResSize;
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


//++ MyFormatBinaryHexW
ULONG MyFormatBinaryHexW( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPWSTR pszStr, _In_ ULONG iStrLen )
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


//++ MyFormatBinaryHexA
ULONG MyFormatBinaryHexA( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPSTR pszStr, _In_ ULONG iStrLen )
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


//++ MyFormatBinaryPrintable
ULONG MyFormatBinaryPrintable( _In_ LPCVOID pData, _In_ ULONG iDataSize, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen, _In_ BOOLEAN bEscape )
{
	ULONG iStrLen = 0;
	CHAR ch;
	assert( pszStr && iStrMaxLen );
	if (pszStr && iStrMaxLen) {
		pszStr[0] = _T( '\0' );
		if (pData && iDataSize) {
			ULONG i;
			for (i = 0, iStrLen = 0; i < iDataSize && iStrLen < iStrMaxLen - 1; i++) {
				ch = ((PCCH)pData)[i];
				if (ch >= 32) {
					pszStr[iStrLen++] = ch;
				} else if (ch == '\r' || ch == '\n' || ch == '\t') {
					if (bEscape) {
						pszStr[iStrLen++] = _T( '\\' );
						switch (ch) {
							case '\r': pszStr[iStrLen++] = _T( 'r' ); break;
							case '\n': pszStr[iStrLen++] = _T( 'n' ); break;
							case '\t': pszStr[iStrLen++] = _T( 't' ); break;
						}
					} else {
						pszStr[iStrLen++] = ch;
					}
				} else {
					pszStr[iStrLen++] = _T( '.' );
				}
			}
			pszStr[iStrLen] = _T( '\0' );
		}
	}
	return iStrLen;
}


//++ MyReplaceMem
// Return delta size
LONG MyReplaceMem(
	_In_ LPCVOID pBuf, _In_ ULONG iBufSize, _In_ ULONG iBufMaxSize,
	_Inout_ PVOID pReplace, _In_ ULONG iReplaceSize,
	_In_ LPCVOID pWith, _In_ ULONG iWithSize )
{
	assert( pBuf );
	assert( iBufSize <= iBufMaxSize );
	assert( (PCH)pReplace >= (PCH)pBuf && (PCH)pReplace < (PCH)pBuf + iBufSize );

	/// Recompute sizes relative to pReplace
	iBufSize    -= (ULONG)((PCCH)pReplace - (PCCH)pBuf);
	iBufMaxSize -= (ULONG)((PCCH)pReplace - (PCCH)pBuf);

	LONG iMoveSize = (LONG)(iBufSize - iReplaceSize);
	iMoveSize = __min( iMoveSize, (LONG)(iBufMaxSize - iWithSize) );
	iMoveSize = __max( iMoveSize, 0 );
	MoveMemory( (PCH)pReplace + iWithSize, (PCH)pReplace + iReplaceSize, iMoveSize );

	ULONG iCopySize = __min( iWithSize, iBufMaxSize );
	CopyMemory( pReplace, pWith, iCopySize );

	return (LONG)(iCopySize + iMoveSize - iBufSize);
}


//++ MyReplaceKeywordsA
LONG MyReplaceKeywordsA( _Inout_ LPSTR pszStr, _In_ LONG iMaxLen, _In_ CHAR chKeywordStart, _In_ CHAR chKeywordEnd, _In_ REPLACE_KEYWORD_CALLBACK_A fnReplace, _In_opt_ LPVOID pReplaceParam )
{
	LPSTR pszBuf = NULL, psz1, psz2;
	LONG iStrLen;
	LONG l1, l2;

	if (!pszStr || !iMaxLen || !fnReplace)
		return -1;

	pszBuf = (LPSTR)malloc( iMaxLen * sizeof( CHAR ) );		/// Working buffer
	if (!pszBuf)
		return -1;

	// Compute string length, including \0
	for (iStrLen = 0; (iStrLen < iMaxLen) && pszStr[iStrLen]; iStrLen++);
	if (pszStr[iStrLen] == 0)
		iStrLen++;
	pszStr[iStrLen - 1] = 0;	/// Force \0

	for (psz2 = pszStr + iStrLen - 1; psz2 > pszStr; ) {

		for (; psz2 > pszStr && (*psz2 != chKeywordEnd); psz2--);						/// End marker
		for (psz1 = psz2 - 1; psz1 > pszStr && (*psz1 != chKeywordStart); psz1--);		/// Start marker
		if (psz2 > psz1 && psz1 >= pszStr) {

			// Include the end marker
			psz2++;

			// Replace callback function
			lstrcpynA( pszBuf, psz1, (int)(psz2 - psz1) + 1 );
			fnReplace( pszBuf, iMaxLen, pReplaceParam );

			l1 = (LONG)(psz2 - psz1);			// Old substring length
			l2 = (LONG)lstrlenA( pszBuf );		// New substring length
			if ((l1 != l2) || (CompareStringA( CP_ACP, 0, pszBuf, -1, psz1, l1 ) != CSTR_EQUAL)) {
				iStrLen += MyReplaceMem( pszStr, iStrLen * sizeof( CHAR ), iMaxLen * sizeof( CHAR ), psz1, l1 * sizeof( CHAR ), pszBuf, l2 * sizeof( CHAR ) ) / sizeof( CHAR );
				pszStr[iStrLen - 1] = 0;
			} else {
				psz1 = psz2 - 1;	// Continue the algorithm from psz2 position
			}
		}

		psz2 = psz1 - 1;
	}

	free( pszBuf );
	return iStrLen - 1;
}


//++ MyReplaceKeywordsW
LONG MyReplaceKeywordsW( _Inout_ LPWSTR pszStr, _In_ LONG iMaxLen, _In_ WCHAR chKeywordStart, _In_ WCHAR chKeywordEnd, _In_ REPLACE_KEYWORD_CALLBACK_W fnReplace, _In_opt_ LPVOID pReplaceParam )
{
	LPWSTR pszBuf = NULL, psz1, psz2;
	LONG iStrLen;
	LONG l1, l2;

	if (!pszStr || !iMaxLen || !fnReplace)
		return -1;

	pszBuf = (LPWSTR)malloc( iMaxLen * sizeof( WCHAR ) );		/// Working buffer
	if (!pszBuf)
		return -1;

	// Compute string length, including \0
	for (iStrLen = 0; (iStrLen < iMaxLen) && pszStr[iStrLen]; iStrLen++);
	if (pszStr[iStrLen] == 0)
		iStrLen++;
	pszStr[iStrLen - 1] = 0;	/// Force \0

	for (psz2 = pszStr + iStrLen - 1; psz2 > pszStr; ) {

		for (; psz2 > pszStr && (*psz2 != chKeywordEnd); psz2--);						/// End marker
		for (psz1 = psz2 - 1; psz1 > pszStr && (*psz1 != chKeywordStart); psz1--);		/// Start marker
		if (psz2 > psz1 && psz1 >= pszStr) {

			// Include the end marker
			psz2++;

			// Replace callback function
			lstrcpynW( pszBuf, psz1, (int)(psz2 - psz1) + 1 );
			fnReplace( pszBuf, iMaxLen, pReplaceParam );

			l1 = (LONG)(psz2 - psz1);			// Old substring length
			l2 = (LONG)lstrlenW( pszBuf );		// New substring length
			if ((l1 != l2) || (CompareStringW( CP_ACP, 0, pszBuf, -1, psz1, l1 ) != CSTR_EQUAL)) {
				iStrLen += MyReplaceMem( pszStr, iStrLen * sizeof( WCHAR ), iMaxLen * sizeof( WCHAR ), psz1, l1 * sizeof( WCHAR ), pszBuf, l2 * sizeof( WCHAR ) ) / sizeof( WCHAR );
				pszStr[iStrLen - 1] = 0;
			} else {
				psz1 = psz2 - 1;	// Continue the algorithm from psz2 position
			}
		}

		psz2 = psz1 - 1;
	}

	free( pszBuf );
	return iStrLen - 1;
}


//++ MyStrReplace
/// Replace all occurrences of the specified substring in a string
/// Returns the final string length, not including \0
LONG MyStrReplace(
	_Inout_ LPTSTR pszStr,
	_In_ size_t iStrMaxLen,
	_In_ LPCTSTR pszSubstr,
	_In_ LPCTSTR pszReplaceWith,
	_In_ BOOL bMatchCase
)
{
	LONG iStrLen = -1;
	if (pszStr && iStrMaxLen && pszSubstr && *pszSubstr && pszReplaceWith) {

		// Length
		LPTSTR psz;
		size_t iSubstrLen = lstrlen( pszSubstr ), iReplaceWithLen = (size_t)-1;

		// Replace all
		for (psz = pszStr; *psz; psz++) {
			if (CompareString( LOCALE_USER_DEFAULT, (bMatchCase ? 0 : LINGUISTIC_IGNORECASE), psz, (int)iSubstrLen, pszSubstr, (int)iSubstrLen ) == CSTR_EQUAL) {

				if (iStrLen == -1) {
					iStrLen = lstrlen( psz );											/// Length from the current position onwards
					iStrLen += PtrToUlong( psz ) - PtrToUlong( pszStr );				/// Length up to the current position
				}

				if (iReplaceWithLen == -1) {
					iReplaceWithLen = lstrlen( pszReplaceWith );
				}

				if ((iStrLen + iReplaceWithLen - iSubstrLen) <= iStrMaxLen) {			/// Enough room?
					size_t iSubstrIndex = psz - pszStr;
					size_t iMoveSize = (iStrLen - iSubstrIndex - iSubstrLen + 1) * sizeof(TCHAR);
					MoveMemory( psz + iReplaceWithLen, psz + iSubstrLen, iMoveSize );	/// Make room for the new substring
					CopyMemory( psz, pszReplaceWith, iReplaceWithLen * sizeof(TCHAR) );	/// Copy the new substring
					iStrLen += (iReplaceWithLen - iSubstrLen);							/// Update the overall string length
					psz = psz + iReplaceWithLen;
				} else {
					iStrLen = -1;
					break;
				}
			}
		}		/// for
	}
	return iStrLen;
}


//++ MyFormatBytes
void MyFormatBytes( _In_ ULONG64 iBytes, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen )
{
	if (pszStr) pszStr[0] = 0;
	if (pszStr && iStrMaxLen) {
		pszStr[0] = 0;
		if (iBytes >= 1024ULL * 1024 * 1024 * 1024) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%.1f TB" ), (double)iBytes / (1024ULL * 1024 * 1024 * 1024) );
		} else if (iBytes >= 1024ULL * 1024 * 1024) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%.1f GB" ), (double)iBytes / (1024ULL * 1024 * 1024) );
		} else if (iBytes >= 1024ULL * 1024) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%.1f MB" ), (double)iBytes / (1024ULL * 1024) );
		} else if (iBytes >= 1024ULL) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%.1f KB" ), (double)iBytes / 1024ULL );
		} else {
			_sntprintf( pszStr, iStrMaxLen, _T( "%I64u bytes" ), iBytes );
		}
	}
}


//++ MyFormatMilliseconds
void MyFormatMilliseconds( _In_ curl_off_t iMillis, _Out_ LPTSTR pszStr, _In_ ULONG iStrMaxLen, _In_ BOOL bUseInfinitySign )
{
	if (pszStr) pszStr[0] = 0;
	if (pszStr && iStrMaxLen) {

		ULONG iDays, iHours, iMins, iSecs;
		pszStr[0] = 0;

		iDays = (ULONG)(iMillis / 86400000);
		iMillis %= 86400000;

		iHours = (ULONG)(iMillis / 3600000);
		iMillis %= 3600000;

		iMins = (ULONG)(iMillis / 60000);
		iMillis %= 60000;

		iSecs = (ULONG)(iMillis / 1000);
		iMillis %= 1000;

		if (iMillis < 0 || iDays > 30) {
#ifdef _UNICODE
#pragma warning(suppress: 4996)
			DWORD winVer = GetVersion();
			BYTE majorVer = LOBYTE(LOWORD(winVer)), minorVer = HIBYTE(LOWORD(winVer));
			if (majorVer >= 6 && bUseInfinitySign) {
				lstrcpyn(pszStr, _T("\x221e"), (int)iStrMaxLen);  // infinity sign
			} else {
				lstrcpyn(pszStr, _T("--:--"), (int)iStrMaxLen);
			}
#else
			lstrcpyn(pszStr, _T("--:--"), (int)iStrMaxLen);
#endif
		} else if (iDays > 0) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%u.%02u:%02u:%02u" ), iDays, iHours, iMins, iSecs );
		} else if (iHours > 0) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%02u:%02u:%02u" ), iHours, iMins, iSecs );
		} else {
			_sntprintf( pszStr, iStrMaxLen, _T( "%02u:%02u" ), iMins, iSecs );
		}
	}
}


//++ MyStringToMilliseconds
UINT_PTR MyStringToMilliseconds( _In_ LPCTSTR pszStr )
{
	UINT_PTR  ms = 0;
	if (pszStr && *pszStr) {
		LPCTSTR psz;
		for (psz = pszStr; *psz && *psz >= _T( '0' ) && *psz <= _T( '9' ); psz++);		/// skip numbers
		for (; *psz == _T( ' ' ) || *psz == _T( '\t' ) || *psz == _T( '\r' ) || *psz == _T( '\n' ); psz++);		/// skip white spaces
		ms = (UINT_PTR )nsishelper_str_to_ptr( pszStr );
		if (lstrcmpi( psz, _T( "s" ) ) == 0 || lstrcmpi( psz, _T( "sec" ) ) == 0 || lstrcmpi( psz, _T( "second" ) ) == 0 || lstrcmpi( psz, _T( "seconds" ) ) == 0) {
			ms *= 1000;
		} else if (lstrcmpi( psz, _T( "m" ) ) == 0 || lstrcmpi( psz, _T( "min" ) ) == 0 || lstrcmpi( psz, _T( "minute" ) ) == 0 || lstrcmpi( psz, _T( "minutes" ) ) == 0) {
			ms *= 1000 * 60;
		} else if (lstrcmpi( psz, _T( "h" ) ) == 0 || lstrcmpi( psz, _T( "hour" ) ) == 0 || lstrcmpi( psz, _T( "hours" ) ) == 0) {
			ms *= 1000 * 60 * 60;
		}
	}
	return ms;
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

//++ VirtualMemoryReset
void VirtualMemoryReset( _Inout_ VMEMO *pMem )
{
	if (pMem) {
		if (pMem->pMem)
			VirtualFree( (LPVOID)pMem->pMem, pMem->iReserved, MEM_DECOMMIT );
		pMem->iCommitted = pMem->iSize = 0;
	}
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


//++ IDataInitialize
void IDataInitialize( _Inout_ IDATA *pData )
{
	assert( pData );
	ZeroMemory( pData, sizeof( *pData ) );
}


//++ IDataDestroy
void IDataDestroy( _Inout_ IDATA *pData )
{
	assert( pData );
	MyFree( pData->Str );
	ZeroMemory( pData, sizeof( *pData ) );
}


//++ IDataParseParam
//? Syntax: [-string|file|memory] <data>
//? Syntax: [(string|file|memory)] <data> -> still accepted for backward compatibility
BOOL IDataParseParam( _In_ LPTSTR pszParam, _In_ int iParamMaxLen, _Out_ IDATA *pData )
{
	BOOL bRet = FALSE, bDataPopped = FALSE;
	assert( pszParam && iParamMaxLen && pData );

	//? Possible combinations:
	//?   "-string" "My String"
	//?   "-file"   "C:\\MyDir\\MyFile.ext"
	//?   "-memory" 0xdeadbeef 256
	//? If the (-string|-file|-memory) hint is missing, we try to guess between `-file` and `-string`

	IDataInitialize( pData );

	// Look for the data hint
	if (lstrcmpi( pszParam, _T( "-string" ) ) == 0 || lstrcmpi( pszParam, _T( "-str" ) ) == 0 || lstrcmpi( pszParam, _T( "(string)" ) ) == 0 || lstrcmpi( pszParam, _T( "(str)" ) ) == 0) {
		pData->Type = IDATA_TYPE_STRING;
	} else if (lstrcmpi( pszParam, _T( "-file-" ) ) == 0 || lstrcmpi( pszParam, _T( "(file)" ) ) == 0) {
		pData->Type = IDATA_TYPE_FILE;
	} else if (lstrcmpi( pszParam, _T( "-memory" ) ) == 0 || lstrcmpi( pszParam, _T( "-mem" ) ) == 0 || lstrcmpi( pszParam, _T( "-buf" ) ) == 0 || lstrcmpi( pszParam, _T( "(memory)" ) ) == 0 || lstrcmpi( pszParam, _T( "(mem)" ) ) == 0 || lstrcmpi( pszParam, _T( "(buf)" ) ) == 0) {
		pData->Type = IDATA_TYPE_MEM;
	} else {
		// Try to guess data type
		if (MyFileExists( pszParam )) {
			pData->Type = IDATA_TYPE_FILE;
		} else {
			pData->Type = IDATA_TYPE_STRING;
		}
		bDataPopped = TRUE;		// pszParam already stores the data itself
	}

	// Fill IDATA in
	if (pData->Type == IDATA_TYPE_STRING) {
		if (bDataPopped || popstring( pszParam ) == NO_ERROR) {
			// Clone the string (utf8)
			if ((pData->Str = MyStrDup( eT2A, pszParam )) != NULL) {
				pData->Size = lstrlenA( pData->Str );
				bRet = TRUE;
			}
		}
	} else if (pData->Type == IDATA_TYPE_FILE) {
		// Clone the filename (TCHAR)
		if (bDataPopped || popstring( pszParam ) == NO_ERROR) {
			if ((pData->File = MyStrDup( eT2T, pszParam )) != NULL) {
				pData->Size = lstrlen( pData->File );
				bRet = TRUE;
			}
		}
	} else if (pData->Type == IDATA_TYPE_MEM) {
		// Clone the buffer (PVOID)
		LPCVOID ptr;
		size_t size;
		if ((ptr = (LPCVOID)(bDataPopped ? nsishelper_str_to_ptr( pszParam ) : popint())) != NULL) {
			if ((size = (ULONG_PTR)popintptr()) != 0) {
				if ((pData->Mem = MyAlloc( size )) != NULL) {
					CopyMemory( pData->Mem, ptr, size );
					pData->Size = size;
					bRet = TRUE;
				}
			}
		}
	} else {
		assert( !"Unexpected data type" );
	}

	if (!bRet)
		IDataDestroy( pData );
	return bRet;
}