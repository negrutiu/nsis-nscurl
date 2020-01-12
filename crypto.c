
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Compute hashes using mbedTLS cryptography engine

#include "main.h"
#include "crypto.h"

#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>


//++ Hash
ULONG Hash( _In_ IDATA *pData, _Out_opt_ PUCHAR md5, _Out_opt_ PUCHAR sha1, _Out_opt_ PUCHAR sha256 )
{
	if (!pData)
		return ERROR_INVALID_PARAMETER;

	if (pData->Type == IDATA_TYPE_FILE) {
		return HashFile( pData->File, md5, sha1, sha256 );
	} else if (pData->Type == IDATA_TYPE_MEM) {
		return HashMem( pData->Mem, (size_t)pData->Size, md5, sha1, sha256 );
	} else if (pData->Type == IDATA_TYPE_STRING) {
		assert( lstrlenA( pData->Str ) == pData->Size );
		return HashMem( pData->Str, (size_t)pData->Size, md5, sha1, sha256 );
	}

	return ERROR_NOT_SUPPORTED;
}


//++ HashFile
ULONG HashFile( _In_ LPCTSTR pszFile, _Out_opt_ PUCHAR md5, _Out_opt_ PUCHAR sha1, _Out_opt_ PUCHAR sha256 )
{
	ULONG e = ERROR_SUCCESS;
	HANDLE h;

	if (!pszFile || !*pszFile || (!md5 && !sha1 && !sha256))
		return ERROR_INVALID_PARAMETER;

	h = CreateFile( pszFile, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL );
	if (h != INVALID_HANDLE_VALUE) {
		ULONG bufsize = 1024 * 1024;
		PUCHAR buf = (PUCHAR)HeapAlloc( GetProcessHeap(), 0, bufsize );
		if (buf) {

			mbedtls_md5_context		md5ctx;
			mbedtls_sha1_context	sha1ctx;
			mbedtls_sha256_context	sha2ctx;
			ULONG l;

			mbedtls_md5_init( &md5ctx );
			mbedtls_sha1_init( &sha1ctx );
			mbedtls_sha256_init( &sha2ctx );

			if (md5) mbedtls_md5_starts( &md5ctx );
			if (sha1) mbedtls_sha1_starts( &sha1ctx );
			if (sha256) mbedtls_sha256_starts( &sha2ctx, FALSE );

			while ((e = ReadFile( h, buf, bufsize, &l, NULL ) ? ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS && (l > 0)) {
				if (md5) mbedtls_md5_update( &md5ctx, buf, l );
				if (sha1) mbedtls_sha1_update( &sha1ctx, buf, l );
				if (sha256) mbedtls_sha256_update( &sha2ctx, buf, l );
			}

			if (md5) mbedtls_md5_finish( &md5ctx, md5 );
			if (sha1) mbedtls_sha1_finish( &sha1ctx, sha1 );
			if (sha256) mbedtls_sha256_finish( &sha2ctx, sha256 );

			mbedtls_md5_free( &md5ctx );
			mbedtls_sha1_free( &sha1ctx );
			mbedtls_sha256_free( &sha2ctx );

			HeapFree( GetProcessHeap(), 0, buf );

		} else {
			e = ERROR_OUTOFMEMORY;
		}
		CloseHandle( h );
	} else {
		e = GetLastError();
	}

	return e;
}


//++ HashMem
ULONG HashMem( _In_ LPCVOID pPtr, _In_ size_t iSize, _Out_opt_ PUCHAR md5, _Out_opt_ PUCHAR sha1, _Out_opt_ PUCHAR sha256 )
{
	ULONG e = ERROR_SUCCESS;

	if (!pPtr || !iSize || (!md5 && !sha1 && !sha256))
		return ERROR_INVALID_PARAMETER;

	if (md5) {
		mbedtls_md5_context ctx;
		mbedtls_md5_init( &ctx );
		mbedtls_md5_starts( &ctx );
		mbedtls_md5_update( &ctx, pPtr, iSize );
		mbedtls_md5_finish( &ctx, md5 );
		mbedtls_md5_free( &ctx );
	}

	if (sha1) {
		mbedtls_sha1_context ctx;
		mbedtls_sha1_init( &ctx );
		mbedtls_sha1_starts( &ctx );
		mbedtls_sha1_update( &ctx, pPtr, iSize );
		mbedtls_sha1_finish( &ctx, sha1 );
		mbedtls_sha1_free( &ctx );
	}

	if (sha256) {
		mbedtls_sha256_context ctx;
		mbedtls_sha256_init( &ctx );
		mbedtls_sha256_starts( &ctx, FALSE );
		mbedtls_sha256_update( &ctx, pPtr, iSize );
		mbedtls_sha256_finish( &ctx, sha256 );
		mbedtls_sha256_free( &ctx );
	}

	return e;
}


//++ EncBase64
LPSTR EncBase64( _In_ LPCVOID pPtr, _In_ size_t iSize )
{
	LPSTR pszBase64 = NULL;
	int e2;
	size_t l;
	
	assert( pPtr && iSize );

	// Compute base64 length
	e2 = mbedtls_base64_encode( NULL, 0, &l, pPtr, iSize );
	if (e2 != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
		return NULL;

	pszBase64 = (LPSTR)MyAlloc( l );
	if (pszBase64) {
		e2 = mbedtls_base64_encode( pszBase64, l, &l, pPtr, iSize );
		if (e2 == 0) {
			// OK
		} else {
			MyFree( pszBase64 );
		}
	}

	return pszBase64;
}


//++ DecBase64
PVOID DecBase64( _In_ LPCSTR pszBase64, _Out_opt_ size_t *piSize )
{
	PVOID pPtr = NULL;
	int e2;
	size_t l = 0;

	if (piSize)
		*piSize = 0;
	assert( pszBase64 );

	// Compute decoded length
	e2 = mbedtls_base64_decode( NULL, 0, &l, pszBase64, lstrlenA( pszBase64 ) );
	if (e2 != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL)
		return NULL;

	pPtr = MyAlloc( l );
	if (pPtr) {
		e2 = mbedtls_base64_decode( pPtr, l, &l, pszBase64, lstrlenA( pszBase64 ) );
		if (e2 == 0) {
			// OK
			if (piSize)
				*piSize = l;
		} else {
			MyFree( pPtr );
		}
	}

	return pPtr;
}
