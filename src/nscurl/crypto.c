
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Compute hashes using our embedded cryptographic engine

#include "main.h"
#include "crypto.h"
#include "openssl/evp.h"
#include "crypto/evp.h"


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

			ULONG l;
			EVP_MD_CTX *md5ctx  = EVP_MD_CTX_new();
			EVP_MD_CTX *sha1ctx = EVP_MD_CTX_new();
			EVP_MD_CTX *sha2ctx = EVP_MD_CTX_new();
			if (md5ctx && sha1ctx && sha2ctx) {
				if (md5) EVP_DigestInit( md5ctx, EVP_md5() );
				if (sha1) EVP_DigestInit( sha1ctx, EVP_sha1() );
				if (sha256) EVP_DigestInit( sha2ctx, EVP_sha256() );
				while ((e = ReadFile( h, buf, bufsize, &l, NULL ) ? ERROR_SUCCESS : GetLastError()) == ERROR_SUCCESS && (l > 0)) {
					if (md5) EVP_DigestUpdate( md5ctx, buf, l );
					if (sha1) EVP_DigestUpdate( sha1ctx, buf, l );
					if (sha256) EVP_DigestUpdate( sha2ctx, buf, l );
				}
				if (md5) EVP_DigestFinal( md5ctx, md5, NULL );
				if (sha1) EVP_DigestFinal( sha1ctx, sha1, NULL );
				if (sha256) EVP_DigestFinal( sha2ctx, sha256, NULL );
			}
			if (md5ctx)  EVP_MD_CTX_free( md5ctx );
			if (sha1ctx) EVP_MD_CTX_free( sha1ctx );
			if (sha2ctx) EVP_MD_CTX_free( sha2ctx );

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
	if (!pPtr || !iSize || (!md5 && !sha1 && !sha256))
		return ERROR_INVALID_PARAMETER;

	if (md5) {
		EVP_MD_CTX *ctx = EVP_MD_CTX_new();
		if (ctx) {
			EVP_DigestInit( ctx, EVP_md5() );
			EVP_DigestUpdate( ctx, pPtr, iSize );
			EVP_DigestFinal( ctx, md5, NULL );
			EVP_MD_CTX_free( ctx );
		}
	}

	if (sha1) {
		EVP_MD_CTX *ctx = EVP_MD_CTX_new();
		if (ctx) {
			EVP_DigestInit( ctx, EVP_sha1() );
			EVP_DigestUpdate( ctx, pPtr, iSize );
			EVP_DigestFinal( ctx, sha1, NULL );
			EVP_MD_CTX_free( ctx );
		}
	}

	if (sha256) {
		EVP_MD_CTX *ctx = EVP_MD_CTX_new();
		if (ctx) {
			EVP_DigestInit( ctx, EVP_sha256() );
			EVP_DigestUpdate( ctx, pPtr, iSize );
			EVP_DigestFinal( ctx, sha256, NULL );
			EVP_MD_CTX_free( ctx );
		}
	}

	return ERROR_SUCCESS;
}


//++ EncBase64
LPSTR EncBase64( _In_ LPCVOID pPtr, _In_ size_t iSize )
{
	LPSTR pszBase64 = NULL;
	EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
	if (ctx) {
		int lOut = 4 * ((iSize + 2) / 3) + 1;
		if ((pszBase64 = (LPSTR)MyAlloc( lOut )) != NULL) {
			unsigned char* psz = (unsigned char*)pszBase64;
			int len;
			EVP_EncodeInit( ctx );
			evp_encode_ctx_set_flags( ctx, EVP_ENCODE_CTX_NO_NEWLINES );
			EVP_EncodeUpdate( ctx, psz, &len, pPtr, (int)iSize );
			EVP_EncodeFinal( ctx, (psz += len), &len );
		}
		EVP_ENCODE_CTX_free( ctx );
	}
	return pszBase64;
}


//++ DecBase64
PVOID DecBase64( _In_ LPCSTR pszBase64, _Out_opt_ size_t *piSize )
{
	LPSTR pPtr = NULL;
	EVP_ENCODE_CTX *ctx = EVP_ENCODE_CTX_new();
	if (piSize) *piSize = 0;
	if (ctx) {
		int lIn = lstrlenA( pszBase64 );
		int lOut = (3 * lIn) / 4;
		if ((pPtr = (LPSTR)MyAlloc( lOut )) != NULL) {
			unsigned char* psz = (unsigned char*)pPtr;
			int len1 = 0, len2 = 0;
			EVP_DecodeInit( ctx );
			if (EVP_DecodeUpdate( ctx, psz, &len1, (const unsigned char*)pszBase64, lIn ) != -1 &&
				EVP_DecodeFinal( ctx, (psz += len1), &len2 ) != -1)
			{
				if (piSize) {
				    *piSize += len1 + len2;
				}
			} else {
				MyFree( pPtr );
			}
		}
		EVP_ENCODE_CTX_free( ctx );
	}

	return (PVOID)pPtr;
}
