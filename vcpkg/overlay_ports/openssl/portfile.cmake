if(EXISTS "${CURRENT_INSTALLED_DIR}/share/libressl/copyright"
    OR EXISTS "${CURRENT_INSTALLED_DIR}/share/boringssl/copyright")
    message(FATAL_ERROR "Can't build openssl if libressl/boringssl is installed. Please remove libressl/boringssl, and try install openssl again if you need it.")
endif()

if(VCPKG_TARGET_IS_EMSCRIPTEN)
    vcpkg_check_linkage(ONLY_STATIC_LIBRARY)
endif()

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO openssl/openssl
    REF "openssl-${VERSION}"
    SHA512 3e1796708155454c118550ba0964b42c0c1055b651fec00cfb55038e8a8abbf5f85df02449e62b50b99d2a4a2f7b47862067f8a965e9c8a72f71dee0153672d9
    PATCHES
        cmake-config.patch
        command-line-length.patch
        script-prefix.patch
        aes_cfb128_vaes_encdec_wrapper.diff # https://github.com/openssl/openssl/issues/28745
        windows/install-layout.patch
        windows/install-pdbs.patch
        windows/install-programs.diff # https://github.com/openssl/openssl/issues/28744
        unix/android-cc.patch
        unix/move-openssldir.patch
        unix/no-empty-dirs.patch
        unix/no-static-libs-for-shared.patch
        nscurl/openssl_cryptsilent.diff     # nscurl: fix CryptAcquireContext(...CRYPT_SILENT...) calls. CRYPT_SILENT inexistent in NT4
        nscurl/openssl_include_crypto.patch # nscurl: deploy "include/openssl/crypto" and "include/openssl/internal"
        nscurl/openssl_winnt_threads.diff   # nscurl: fix OPENSSL_THREADS_WINNT and OPENSSL_THREADS_WINNT_LEGACY when _WIN32_WINNT >= 0x0400
        nscurl/openssl_wspiapi.diff         # nscurl: fix linking to Ws2_32!getaddrinfo and Ws2_32!freeaddrinfo when _WIN32_WINNT <= 0x0500
        nscurl/openssl_utf8_common-h.patch  		# nscurl: replace CP_UTF8 with CP_ACP in NT4
        nscurl/openssl_utf8_defaults-c.patch  		# nscurl: replace CP_UTF8 with CP_ACP in NT4
        nscurl/openssl_utf8_getenv-c.patch  		# nscurl: replace CP_UTF8 with CP_ACP in NT4
        nscurl/openssl_utf8_LPdir_win-c.patch  		# nscurl: replace CP_UTF8 with CP_ACP in NT4
        nscurl/openssl_utf8_o_fopen-c.patch  		# nscurl: replace CP_UTF8 with CP_ACP in NT4
        nscurl/openssl_utf8_randfile-c.patch  		# nscurl: replace CP_UTF8 with CP_ACP in NT4
        nscurl/openssl_utf8_ui_openssl-c.patch  	# nscurl: replace CP_UTF8 with CP_ACP in NT4
)

vcpkg_list(SET CONFIGURE_OPTIONS
    enable-static-engine
    enable-capieng
    no-tests
    no-docs
    386                     # nscurl: basic CPU instruction set
    no-deprecated           # nscurl:
    no-capieng              # nscurl: prevent using BCrypt (inexistent in NT4)
    no-async                # nscurl: prevent link to kernel32!ConvertFiberToThread (inexistent in NT4)
    no-pinshared            # nscurl: prevent link to kernel32!GetModuleHandleEx (inexistent in NT4)
)

# nscurl: prevent crypto/init.c from registering atexit callback
string(APPEND VCPKG_C_FLAGS   " -DOPENSSL_NO_ATEXIT")
string(APPEND VCPKG_CXX_FLAGS " -DOPENSSL_NO_ATEXIT")

# https://github.com/openssl/openssl/blob/master/INSTALL.md#enable-ec_nistp_64_gcc_128
vcpkg_cmake_get_vars(cmake_vars_file)
include("${cmake_vars_file}")
if(VCPKG_DETECTED_CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$"
   AND VCPKG_TARGET_ARCHITECTURE MATCHES "^(x64|arm64|riscv64|ppc64le)$")
    vcpkg_list(APPEND CONFIGURE_OPTIONS enable-ec_nistp_64_gcc_128)
endif()

set(INSTALL_FIPS "")
if("fips" IN_LIST FEATURES)
    vcpkg_list(APPEND INSTALL_FIPS install_fips)
    vcpkg_list(APPEND CONFIGURE_OPTIONS enable-fips)
endif()

if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    vcpkg_list(APPEND CONFIGURE_OPTIONS shared)
else()
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-shared no-module)
endif()

if(NOT "tools" IN_LIST FEATURES)
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-apps)
endif()

if("weak-ssl-ciphers" IN_LIST FEATURES)
    vcpkg_list(APPEND CONFIGURE_OPTIONS enable-weak-ssl-ciphers)
endif()

if("ssl3" IN_LIST FEATURES)
    vcpkg_list(APPEND CONFIGURE_OPTIONS enable-ssl3)
    vcpkg_list(APPEND CONFIGURE_OPTIONS enable-ssl3-method)
endif()

if(DEFINED OPENSSL_USE_NOPINSHARED)
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-pinshared)
endif()

if(OPENSSL_NO_AUTOLOAD_CONFIG)
    vcpkg_list(APPEND CONFIGURE_OPTIONS no-autoload-config)
endif()

if(VCPKG_TARGET_IS_WINDOWS AND NOT VCPKG_TARGET_IS_MINGW)
    include("${CMAKE_CURRENT_LIST_DIR}/windows/portfile.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/install-pc-files.cmake")
else()
    include("${CMAKE_CURRENT_LIST_DIR}/unix/portfile.cmake")
endif()

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

if (NOT "${VERSION}" MATCHES [[^([0-9]+)\.([0-9]+)\.([0-9]+)$]])
    message(FATAL_ERROR "Version regex did not match.")
endif()
set(OPENSSL_VERSION_MAJOR "${CMAKE_MATCH_1}")
set(OPENSSL_VERSION_MINOR "${CMAKE_MATCH_2}")
set(OPENSSL_VERSION_FIX "${CMAKE_MATCH_3}")
configure_file("${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake.in" "${CURRENT_PACKAGES_DIR}/share/${PORT}/vcpkg-cmake-wrapper.cmake" @ONLY)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE.txt")
