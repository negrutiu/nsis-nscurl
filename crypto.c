
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Compute hashes using mbedTLS cryptography engine

#include "main.h"
#include "crypto.h"

#include <mbedtls/md5.h>
#include <mbedtls/sha1.h>
#include <mbedtls/sha256.h>


//++ FileHash
ULONG FileHash( _In_ LPCTSTR pszFile, _Out_opt_ PUCHAR md5, _Out_opt_ PUCHAR sha1, _Out_opt_ PUCHAR sha256 )
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
