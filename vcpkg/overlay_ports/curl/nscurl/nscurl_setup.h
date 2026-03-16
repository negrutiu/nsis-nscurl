#include "curl_setup.h"
#include <assert.h>

#ifdef _WIN32

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Called by \c Curl_win32_init to perform global initializations.
static void nscurl_init(long flags)
{
    UNREFERENCED_PARAMETER(flags);
}

/// \brief Called by \c Curl_win32_cleanup to perform global cleanup.
static void nscurl_cleanup(long init_flags)
{
    UNREFERENCED_PARAMETER(init_flags);
}

#if (_WIN32_WINNT < 0x0500)

/// \brief Reimplementation of \c MultiByteToWideChar that handles \c CP_UTF8 in NT4.
static int WINAPI nscurl_MultiByteToWideChar(
    UINT codepage, const DWORD flags,
    LPCCH narrowstr, const int narrowlen,
    LPWCH widestr, const int widelen)
{
    typedef int(WINAPI * TypeMultiByteToWideChar)(UINT, DWORD, LPCCH, int, LPWCH, int);
    static TypeMultiByteToWideChar fnMultiByteToWideChar = (TypeMultiByteToWideChar)-1;

    if (fnMultiByteToWideChar == (TypeMultiByteToWideChar)-1) {
        fnMultiByteToWideChar = (TypeMultiByteToWideChar)GetProcAddress(GetModuleHandle(_T("kernel32")), "MultiByteToWideChar");
    }

    if (fnMultiByteToWideChar) {
        // NT4 doesn't support UTF-8 code page
        if (codepage == CP_UTF8) {
            BYTE major = LOBYTE(LOWORD(GetVersion()));
            if (major < 5) {
                codepage = CP_ACP;
            }
        }
        return fnMultiByteToWideChar(codepage, flags, narrowstr, narrowlen, widestr, widelen);
    } else {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }
}

/// \brief Reimplementation of \c WideCharToMultiByte that handles \c CP_UTF8 in NT4.
static int WINAPI nscurl_WideCharToMultiByte(
    UINT codepage, DWORD flags,
    LPCWCH widestr, const int widelen,
    LPSTR narrowstr, const int narrowlen,
    LPCCH defaultchar, LPBOOL defaultused)
{
    typedef int(WINAPI * TypeWideCharToMultiByte)(UINT, DWORD, LPCWCH, int, LPSTR, int, LPCCH, LPBOOL);
    static TypeWideCharToMultiByte fnWideCharToMultiByte = (TypeWideCharToMultiByte)-1;

    if (fnWideCharToMultiByte == (TypeWideCharToMultiByte)-1) {
        fnWideCharToMultiByte = (TypeWideCharToMultiByte)GetProcAddress(GetModuleHandle(_T("kernel32")), "WideCharToMultiByte");
    }

    if (fnWideCharToMultiByte) {
        // NT4 doesn't support UTF-8 code page
        if (codepage == CP_UTF8) {
            BYTE major = LOBYTE(LOWORD(GetVersion()));
            if (major < 5) {
                codepage = CP_ACP;
            }
        }
        return fnWideCharToMultiByte(codepage, flags, widestr, widelen, narrowstr, narrowlen, defaultchar, defaultused);
    } else {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return 0;
    }
}

#define MultiByteToWideChar       nscurl_MultiByteToWideChar
#define WideCharToMultiByte       nscurl_WideCharToMultiByte

#endif  // (_WIN32_WINNT < 0x0500)


#if __MINGW32__

/// \brief Reimplementation of \c wcstombs_s compatible with older Windows versions.
/// \details \c msvcrt!wcstombs_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_wcstombs_s(size_t* outLen, char* mbstr, size_t mbstrMaxLen, wchar_t const* wcstr, size_t wcstrLen)
{
    int count = WideCharToMultiByte(CP_UTF8, 0, wcstr, (int)(ptrdiff_t)wcstrLen, mbstr, (int)(ptrdiff_t)mbstrMaxLen, NULL, NULL);
    if (count >= 0) {
        if (outLen)
            *outLen = (size_t)(ptrdiff_t)count;
        return 0;
    } else {
        if (outLen)
            *outLen = 0U;
        return EILSEQ;
    }
}

/// \brief Reimplementation of \c _sopen_s compatible with older Windows versions.
/// \details \c msvcrt!_sopen_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_sopen_s(int* pfh, const char* filename, int oflag, int shflag, int pmode)
{
    int fd = _sopen(filename, oflag, shflag, pmode);    // NOLINT(clang-diagnostic-deprecated-declarations)
    if (fd == -1) {
        if (pfh)
            *pfh = -1;
        return errno;
    } else {
        if (pfh)
            *pfh = fd;
        return 0;
    }
}

/// \brief Reimplementation of \c _wsopen_s compatible with older Windows versions.
/// \details \c msvcrt!_wsopen_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_wsopen_s(int* pfh, const wchar_t* filename, int oflag, int shflag, int pmode)
{
    int fd = _wsopen(filename, oflag, shflag, pmode);
    if (fd == -1) {
        if (pfh)
            *pfh = -1;
        return errno;
    } else {
        if (pfh)
            *pfh = fd;
        return 0;
    }
}

/// \brief Reimplementation of \c freopen_s compatible with older Windows versions.
/// \details \c msvcrt!freopen_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_freopen_s(FILE** newStream, const char* path, const char* mode, FILE* oldStream)
{
    if (!newStream) {
        assert("pointer to return value is NULL" && 0);
        return EINVAL;
    }
    *newStream = freopen(path, mode, oldStream);    // NOLINT(clang-diagnostic-deprecated-declarations)
    return *newStream ? 0 : EINVAL;
}

/// \brief Reimplementation of \c _wfreopen_s compatible with older Windows versions.
/// \details \c msvcrt!_wfreopen_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_wfreopen_s(FILE** newStream, const wchar_t* path, const wchar_t* mode, FILE* oldStream)
{
    if (!newStream) {
        assert("pointer to return value is NULL" && 0);
        return EINVAL;
    }
    *newStream = _wfreopen(path, mode, oldStream);    // NOLINT(clang-diagnostic-deprecated-declarations)
    return *newStream ? 0 : EINVAL;
}

/// \brief Reimplementation of \c strcpy_s compatible with older Windows versions.
/// \details \c msvcrt! strcpy_sis available starting with Windows Vista.
static errno_t __cdecl nscurl_strcpy_s(char* dest, rsize_t destMaxLen, const char* src)
{
    char* result = lstrcpynA(dest, src, (int)(ptrdiff_t)destMaxLen);
    if (!result) {
        if (dest)
            dest[0] = '\0';
        return EINVAL;
    }
    if (src[strlen(result)] != '\0') {
        assert("buffer is too small" && 0);
        return STRUNCATE;
    }
    return 0;
}

/// \brief Reimplementation of \c wcscpy_s compatible with older Windows versions.
/// \details \c msvcrt!wcscpy_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_wcscpy_s(wchar_t* dest, rsize_t destMaxLen, const wchar_t* src)
{
    wchar_t* result = lstrcpynW(dest, src, (int)(ptrdiff_t)destMaxLen);
    if (!result) {
        if (dest)
            dest[0] = L'\0';
        return EINVAL;
    }
    if (src[wcslen(result)] != L'\0') {
        assert("buffer is too small" && 0);
        return STRUNCATE;
    }
    return 0;
}

/// \brief Reimplementation of \c strncpy_s compatible with older Windows versions.
/// \details \c msvcrt!strncpy_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_strncpy_s(char* dest, size_t destMaxLen, const char* src, size_t srcLen)
{
    if (srcLen + 1 > destMaxLen) {
        if (dest)
            dest[0] = '\0';
        assert("buffer is too small" && 0);
        return STRUNCATE;
    }
    size_t maxLen = (srcLen != _TRUNCATE) ? __min(destMaxLen, srcLen + 1) : destMaxLen;
    char* result = lstrcpynA(dest, src, (int)(ptrdiff_t)maxLen);
    if (!result) {
        if (dest)
            dest[0] = '\0';
        return EINVAL;
    }
    if (srcLen == _TRUNCATE && src[strlen(result)] != '\0') {
        return STRUNCATE;
    }
    return 0;
}

/// \brief Reimplementation of \c wcsncpy_s compatible with older Windows versions.
/// \details \c msvcrt!wcsncpy_s is available starting with Windows Vista.
static errno_t __cdecl nscurl_wcsncpy_s(wchar_t* dest, size_t destMaxLen, const wchar_t* src, size_t srcLen)
{
    if (srcLen + 1 > destMaxLen) {
        if (dest)
            dest[0] = L'\0';
        assert("buffer is too small" && 0);
        return STRUNCATE;
    }
    size_t maxLen = (srcLen != _TRUNCATE) ? __min(destMaxLen, srcLen + 1) : destMaxLen;
    wchar_t* result = lstrcpynW(dest, src, (int)(ptrdiff_t)maxLen);
    if (!result) {
        if (dest)
            dest[0] = L'\0';
        return EINVAL;
    }
    if (srcLen == _TRUNCATE && src[wcslen(result)] != L'\0') {
        return STRUNCATE;
    }
    return 0;
}

//
// Overwrite CRT routines
//

#ifdef wcstombs_s
#  undef wcstombs_s
#endif
#ifdef _sopen_s
#  undef _sopen_s
#endif
#ifdef _wsopen_s
#  undef _wsopen_s
#endif
#ifdef freopen_s
#  undef freopen_s
#endif
#ifdef _wfreopen_s
#  undef _wfreopen_s
#endif
#ifdef strcpy_s
#  undef strcpy_s
#endif
#ifdef wcscpy_s
#  undef wcscpy_s
#endif
#ifdef strncpy_s
#  undef strncpy_s
#endif
#ifdef wcsncpy_s
#  undef wcsncpy_s
#endif

#define wcstombs_s      nscurl_wcstombs_s
#define _sopen_s        nscurl_sopen_s
#define _wsopen_s       nscurl_wsopen_s
#define freopen_s       nscurl_freopen_s
#define _wfreopen_s     nscurl_wfreopen_s
#define strcpy_s        nscurl_strcpy_s
#define wcscpy_s        nscurl_wcscpy_s
#define strncpy_s       nscurl_strncpy_s
#define wcsncpy_s       nscurl_wcsncpy_s

#endif  // __MINGW32__

#if (_WIN32_WINNT < 0x0600)

/// \brief Reimplementation of \c InitializeCriticalSectionEx.
/// \details \c kernel32!InitializeCriticalSectionEx is available starting with Windows Vista.
static BOOL WINAPI InitializeCriticalSectionEx(LPCRITICAL_SECTION lpCriticalSection, DWORD dwSpinCount, DWORD Flags)
{
    InitializeCriticalSection(lpCriticalSection);
    // todo: call InitializeCriticalSectionEx dynamically
    UNREFERENCED_PARAMETER(dwSpinCount);
    UNREFERENCED_PARAMETER(Flags);
    return TRUE;
}

/// \brief Reimplementation of \c if_nametoindex.
/// \details \c iphlpapi!if_nametoindex is available starting with Windows Vista.
static ULONG WINAPI if_nametoindex(_In_ PCSTR InterfaceName)
{
    // todo: call iphlpapi!if_nametoindex dynamically
    UNREFERENCED_PARAMETER(InterfaceName);
    return 0;
}

#endif

#ifdef __cplusplus
}   // extern "C"
#endif

#endif  // _WIN32