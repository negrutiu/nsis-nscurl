
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/02/02

#include "main.h"
#include "utils.h"

MEMORY_STATS g_MemStats = { 0 };


//++ UtilsInitialize
VOID UtilsInitialize(void)
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );
}


//++ UtilsDestroy
VOID UtilsDestroy(void)
{
	TRACE( _T( "%hs( Alloc: #%u/%I64u bytes, Free: #%u/%I64u bytes )\n" ), __FUNCTION__, g_MemStats.AllocCalls, g_MemStats.AllocBytes, g_MemStats.FreeCalls, g_MemStats.FreeBytes );
	assert( g_MemStats.AllocBytes == g_MemStats.FreeBytes );
}


//++ TraceImpl
#if defined (TRACE_ENABLED)
VOID TraceImpl( _In_z_ _Printf_format_string_ LPCTSTR pszFormat, _In_ ... )
{
	if (pszFormat && *pszFormat)
	{
		const int bufsize = 1024 * 16;
		LPTSTR buf = malloc(bufsize * sizeof(TCHAR));
		assert(buf);
		if (buf) 
		{
			int iLen1, iLen2;
			va_list args;

			va_start(args, pszFormat);

			if (pszFormat[0] == TRACE_NO_PREFIX[0]) {
				pszFormat++;
				iLen1 = 0;
			} else {
				iLen1 = _sntprintf(buf, bufsize, _T("[nscurl.th%04lx] "), GetCurrentThreadId());
			}

			iLen2 = _vsntprintf(buf + iLen1, bufsize - iLen1, pszFormat, args);
			if (iLen2 > 0) {
				if (iLen1 + iLen2 < bufsize)
					buf[iLen1 + iLen2] = 0;	// The string is not guaranteed to be null terminated
			} else {
				buf[bufsize - 1] = 0;
			}
			OutputDebugString(buf);

			va_end(args);
			free(buf);
		}
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
		fnSetThreadDescription = (TfnSetThreadDescription)(void*)GetProcAddress( GetModuleHandle( _T("kernel32") ), "SetThreadDescription" );

	if (fnSetThreadDescription != NO_INIT && fnSetThreadDescription != NULL)
		fnSetThreadDescription( hThread, pszName );
	#undef NO_INIT
}


LPTSTR MyCanonicalizePath(_In_ LPCTSTR pszPath)
{
	LPTSTR canonicalPath = NULL;
	if (pszPath && pszPath[0]) {
		const ULONG bufLen = 32768;
		LPTSTR bufPtr = MyAlloc(bufLen * sizeof(TCHAR));
		if (bufPtr) {
			ULONG len;
			if (!(pszPath[0] == _T('\\') && pszPath[1] == _T('\\') && !IsPathSeparator(pszPath[2]))) {
				for (; IsPathSeparator(*pszPath); pszPath++);	// strip leading separators for !network paths
			}
			len = GetFullPathName(pszPath, bufLen, bufPtr, NULL);
			if (len != 0 && len < bufLen) {
				canonicalPath = MyStrDup(eT2T, bufPtr);
			}
			MyFree(bufPtr);
		}
	}
	if (!canonicalPath) {
		canonicalPath = MyStrDup(eT2T, pszPath ? pszPath : _T(""));
	}
	return canonicalPath;
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
		for (psz = pszBuf + len - 1; (psz >= pszBuf) && IsPathSeparator(*psz); psz--)
			*psz = _T( '\0' ), len--;
		if (bHasFilename) {
			// Strip filename
			for (psz = pszBuf + len - 1; (psz >= pszBuf) && !IsPathSeparator(*psz); psz--)
				*psz = _T( '\0' ), len--;
			// Strip trailing backslashes
			for (psz = pszBuf + len - 1; (psz >= pszBuf) && IsPathSeparator(*psz); psz--)
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
			for (; IsPathSeparator(*psz); psz++);
			for (; !IsPathSeparator(*psz) && *psz != _T( '\0' ); psz++);
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

INT_PTR MyAtoi(LPCTSTR s, LPCTSTR* nextChar, BOOL skipSpaces)
{
	if (skipSpaces) { while (*s == _T(' ') || *s == _T('\t')) { s++; }}

	int sign = 0;
	if (*s == _T('+')) s++;
	if (*s == _T('-')) sign++, s++;

	INT_PTR v = 0;
	if (*s == _T('0') && (s[1] == _T('x') || s[1] == _T('X'))) {
		s++;
		for (;;) {
			int c = *(++s);
			if (c >= _T('0') && c <= _T('9')) c -= _T('0');
			else if (c >= _T('a') && c <= _T('f')) c -= _T('a') - 10;
			else if (c >= _T('A') && c <= _T('F')) c -= _T('A') - 10;
			else break;
			v <<= 4;
			v += c;
		}
	} else if (*s == _T('0') && s[1] <= _T('7') && s[1] >= _T('0')) {
		for (;;) {
			int c = *(++s);
			if (c >= _T('0') && c <= _T('7')) c -= _T('0');
			else break;
			v <<= 3;
			v += c;
		}
	} else {
		for (s--;;) {
			int c = *(++s) - _T('0');
			if (c < 0 || c > 9) break;
			v *= 10;
			v += c;
		}
	}

    if (skipSpaces) { while (*s == _T(' ') || *s == _T('\t')) { s++; }}
	if (nextChar) *nextChar = s;
	if (sign) v = -v;
	return v;
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
	iDestLen = min( iDestLen, (int)iDestMaxLen );

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

	n = min( iDataSize, (iStrLen - 1) / 2 );		/// Total input bytes that fit in the output buffer, not including \0
	for (i = 0, j = 0; i < n; i++, j = i << 1) {
		pszStr[j] = (WCHAR)Dec2Hex( ((PUCHAR)pData)[i] >> 4 );
		pszStr[j + 1] = (WCHAR)Dec2Hex( ((PUCHAR)pData)[i] & 0xf );
	}

	pszStr[j] = UNICODE_NULL;
	return j;										/// Characters written, not including \0
}

ULONG MyWriteDataToFile(_In_ const void* pData, _In_ ULONG64 iSize, _In_ LPCTSTR pszOutFile)
{
	ULONG err = ERROR_SUCCESS;
	if (pData && pszOutFile && pszOutFile[0]) {
		HANDLE hOutFile = CreateFile(pszOutFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (MyValidHandle(hOutFile)) {
			ULONG iWritten;
			while (err == ERROR_SUCCESS && iSize > 0) {
				err = WriteFile(hOutFile, pData, (ULONG)min(iSize, 1024 * 1024ULL), &iWritten, NULL) ? ERROR_SUCCESS : GetLastError();
				if (err == ERROR_SUCCESS) {
					pData = (const char*)pData + iWritten;
					iSize -= iWritten;
				}
			}
			CloseHandle(hOutFile);
		} else {
			err = GetLastError();
		}
	} else {
		err = ERROR_INVALID_PARAMETER;
	}
	return err;
}

ULONG MyWriteFileToFile(_In_ HANDLE hInFile, _In_ ULONG64 iOffset, _In_ ULONG64 iSize, LPCTSTR pszOutFile)
{
	ULONG err = ERROR_SUCCESS;
	if (MyValidHandle(hInFile) && pszOutFile && pszOutFile[0]) {
		err = SetFilePointer(hInFile, (LONG)((PLARGE_INTEGER)&iOffset)->LowPart, &((PLARGE_INTEGER)&iOffset)->HighPart, FILE_BEGIN) != INVALID_SET_FILE_POINTER ? ERROR_SUCCESS : GetLastError();
		if (err == ERROR_SUCCESS) {
			HANDLE hOutFile = CreateFile(pszOutFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (MyValidHandle(hOutFile)) {
				const ULONG iBufSize = 1024 * 1024;
				char* pBuf = (char*)MyAlloc(iBufSize);
				if (pBuf) {
					ULONG iRead, iWritten;
					while (err == ERROR_SUCCESS && iSize > 0) {
						err = ReadFile(hInFile, pBuf, (ULONG)min(iSize, iBufSize), &iRead, NULL) ? ERROR_SUCCESS : GetLastError();
						if (err == ERROR_SUCCESS) {
							if (iRead == 0)
								break;
							err = WriteFile(hOutFile, pBuf, iRead, &iWritten, NULL) ? ERROR_SUCCESS : GetLastError();
							if (err == ERROR_SUCCESS) {
								if (iWritten == iRead) {
									iSize -= iWritten;
								} else {
									err = ERROR_WRITE_FAULT;
								}
							}
						}
					}
					MyFree(pBuf);
				} else {
					err = ERROR_OUTOFMEMORY;
				}
				CloseHandle(hOutFile);
			} else {
				err = GetLastError();
			}
		}
	} else {
		err = GetLastError();
	}
	return err;
}


//++ MyFormatBinaryHexA
ULONG MyFormatBinaryHexA( _In_ LPVOID pData, _In_ ULONG iDataSize, _Out_ LPSTR pszStr, _In_ ULONG iStrLen )
{
	ULONG i, j, n;

	if (pszStr && iStrLen)
		pszStr[0] = 0;

	n = min( iDataSize, (iStrLen - 1) / 2 );		/// Total input bytes that fit in the output buffer, not including \0
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
	iMoveSize = min( iMoveSize, (LONG)(iBufMaxSize - iWithSize) );
	iMoveSize = max( iMoveSize, 0 );
	MoveMemory( (PCH)pReplace + iWithSize, (PCH)pReplace + iReplaceSize, iMoveSize );

	ULONG iCopySize = min( iWithSize, iBufMaxSize );
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


BOOL MySplitKeyword(LPCTSTR input, Keyword* data)
{
#define IS_WHITESPACE(ch) ((ch) == _T(' ') || (ch) == _T('\t') || (ch) == _T('\r') || (ch) == _T('\n'))
#define TRIM_WHITESPACES(begin, end) \
    while ((begin) < (end) && IS_WHITESPACE((begin)[0])) { (begin)++; }; \
    while ((end) > (begin) && IS_WHITESPACE((end)[-1])) { (end)--; }

	if (input && input[0] && data) {
		int l = lstrlen(input);
		if (l > 2 && input[0] == _T('@') && input[l - 1] == _T('@')) {
			LPCTSTR psz = input + 1;

			for (data->keywordBegin = psz++; psz[0] != _T('\0') && psz[0] != _T(':') && psz[0] != _T('>') && psz[0] != _T('@'); psz++) { }
			data->keywordEnd = psz;
			TRIM_WHITESPACES(data->keywordBegin, data->keywordEnd);

			if (psz[0] == _T(':')) {
				for (data->paramsBegin = ++psz; psz[0] != _T('\0') && psz[0] != _T('>') && psz[0] != _T('@'); psz++) { }
				data->paramsEnd = psz;
				TRIM_WHITESPACES(data->paramsBegin, data->paramsEnd);
			}

			if (psz[0] == _T('>')) {
				for (data->pathBegin = ++psz; psz[0] != _T('\0') && psz[0] != _T('@'); psz++) { }
				data->pathEnd = psz;
				TRIM_WHITESPACES(data->pathBegin, data->pathEnd);
			}

			if (psz[0] != _T('@') || psz[1] != _T('\0'))
				return FALSE;
		}
	}

	return data &&
		data->keywordBegin && data->keywordEnd && (data->keywordEnd > data->keywordBegin) &&    // keyword is mandatory
		(!data->paramsBegin || (data->paramsEnd > data->paramsBegin)) &&    // null or valid, but not empty
		(!data->pathBegin || (data->pathEnd > data->pathBegin));            // null or valid, but not empty

#undef IS_WHITESPACE
#undef TRIM_WHITESPACES
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
					iStrLen += (LONG)(iReplaceWithLen - iSubstrLen);					/// Update the overall string length
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
			BYTE majorVer = LOBYTE(LOWORD(winVer));
			if (majorVer >= 6 && bUseInfinitySign) {
				lstrcpyn(pszStr, _T("\x221e"), (int)iStrMaxLen);  // infinity sign
			} else {
				lstrcpyn(pszStr, _T("--:--"), (int)iStrMaxLen);
			}
#else
			lstrcpyn(pszStr, _T("--:--"), (int)iStrMaxLen);
#endif
		} else if (iDays > 0) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%lu.%02lu:%02lu:%02lu" ), iDays, iHours, iMins, iSecs );
		} else if (iHours > 0) {
			_sntprintf( pszStr, iStrMaxLen, _T( "%02lu:%02lu:%02lu" ), iHours, iMins, iSecs );
		} else {
			_sntprintf( pszStr, iStrMaxLen, _T( "%02lu:%02lu" ), iMins, iSecs );
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
		ms = (UINT_PTR )_tstoi64( pszStr );
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
//? Syntax: [-string|-file|-memory] <data>
//? Syntax: [(string)|(file)|(memory)] <data> -> still accepted for backward compatibility
ULONG IDataParseParam( _In_ LPTSTR pszParam, _In_ int iParamMaxLen, _Out_ IDATA *pData )
{
	ULONG err = ERROR_SUCCESS;
	BOOL bDataPopped = FALSE;
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
	} else if (lstrcmpi( pszParam, _T( "-file" ) ) == 0 || lstrcmpi( pszParam, _T( "(file)" ) ) == 0) {
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
			} else {
				err = ERROR_OUTOFMEMORY;
			}
		} else {
			err = ERROR_BAD_ARGUMENTS;
		}
	} else if (pData->Type == IDATA_TYPE_FILE) {
		// Clone the filename (TCHAR)
		if (bDataPopped || popstring( pszParam ) == NO_ERROR) {
		    if ((pData->File = MyCanonicalizePath(pszParam)) != NULL) {
				if (MyFileExists(pData->File)) {
					pData->Size = lstrlen( pData->File );
				} else {
					err = ERROR_FILE_NOT_FOUND;
				}
			} else {
				err = ERROR_INVALID_NAME;
			}
		} else {
			err = ERROR_BAD_ARGUMENTS;
		}
	} else if (pData->Type == IDATA_TYPE_MEM) {
		// Clone the buffer (PVOID)
		LPCVOID ptr;
		if ((ptr = (LPCVOID)(bDataPopped ? (INT_PTR)_tstoi64( pszParam ) : popintptr())) != NULL) {
			size_t size;
			if ((size = (ULONG_PTR)popintptr()) != 0) {
				if ((pData->Mem = MyAlloc( (ULONG)size )) != NULL) {
					CopyMemory( pData->Mem, ptr, size );
					pData->Size = size;
				} else {
					err = ERROR_OUTOFMEMORY;
				}
			}
		} else {
			err = ERROR_BAD_ARGUMENTS;
		}
	} else {
		assert( !"Unexpected data type" );
		err = ERROR_INVALID_DATATYPE;
	}

	if (err != ERROR_SUCCESS)
		IDataDestroy( pData );
	return err;
}