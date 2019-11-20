
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/11/20
//? Compute hashes using mbedTLS cryptography engine

#pragma once

//++ FileHash
ULONG FileHash(
	_In_ LPCTSTR pszFile,
	_Out_opt_ PUCHAR md5,					/// 16 bytes / 128 bits
	_Out_opt_ PUCHAR sha1,					/// 20 bytes / 160 bits
	_Out_opt_ PUCHAR sha256					/// 32 bytes / 256 bits
);