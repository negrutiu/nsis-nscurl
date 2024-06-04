# NScurl ([NSIS](https://github.com/negrutiu/nsis) plugin)

`NScurl` is a [NSIS](https://github.com/negrutiu/nsis) (Nullsoft Scriptable Install System) plugin with advanced HTTP/HTTPS capabilities.  
It's included in the unofficial [NSIS](https://github.com/negrutiu/nsis) fork.

Implemented in `C` on top of [libcurl](https://curl.haxx.se/libcurl) with [OpenSSL](https://www.openssl.org) as SSL backend.


[![License: BSD3](https://img.shields.io/badge/License-BSD3-blue.svg)](LICENSE.md)
[![Latest Release](https://img.shields.io/badge/dynamic/json.svg?label=Latest%20Release&url=https%3A%2F%2Fapi.github.com%2Frepos%2Fnegrutiu%2Fnsis-nscurl%2Freleases%2Flatest&query=%24.name&colorB=orange)](../../releases/latest)
[![Downloads](https://img.shields.io/github/downloads/negrutiu/nsis-nscurl/total.svg?label=Downloads&colorB=orange)](../../releases/latest)
[![GitHub issues](https://img.shields.io/github/issues/negrutiu/nsis-nscurl.svg?label=Issues)](../../issues)

## Features

- Supports modern protocols and ciphers including `HTTP/2`, `TLS1.3`, etc.
- Works well on Windows NT4, Windows 11 and everything in between
- Multi-threaded design to transfer multiple files in parallel
- Background transfers are available, while your installer performs other installation tasks
- Multiple attempts to connect and resume failed/dropped transfers
- Plenty of useful information is available for querying (transfer size, speed, HTTP status, HTTP headers, etc.)
- Works at any `NSIS` install stage (in `.onInit` callback function, in un/install sections, custom pages, silent installers, etc.)
- Supports custom certificate stores and certificate pinning
- Supports `HTTP` and `TLS` authentication
- Supports all relevant HTTP methods (`GET`, `POST`, `PUT`, `HEAD`, etc.)
- Supports `DNS-over-HTTPS` secure name resolution
- Supports custom HTTP headers and data
- Supports proxy servers (both authenticated and open)
- Supports files larger than 4GB
- Can download remote content in-memory instead of a file
- Works well in **64-bit** installers created with this [NSIS](https://github.com/negrutiu/nsis) fork
- Many more...

## Basic usage

Check out the [Getting Started](https://github.com/negrutiu/nsis-nscurl/wiki/Getting-Started) wiki page.  
Check out the [documentation](src/nscurl/NScurl.readme.md) page.  
Check out the [NSIS test script](tests/NScurl-Test.nsi).  

```nsis
; Quick transfer
NScurl::http GET "https://download.sysinternals.com/files/SysinternalsSuite.zip" "$TEMP\SysinternalsSuite.zip" /INSIST /CANCEL /RESUME /END
Pop $0 ; transfer status ("OK" for success)

; Quick transfer with GET parameters and request headers
NScurl::http GET "https://httpbin.org/get?param1=value1&param2=value2" "$TEMP\httpbin_get.json" /HEADER "Header1: Value1" /HEADER "Header2: Value2" /END
Pop $0

; POST json data
NScurl::http POST "https://httpbin.org/post" Memory /HEADER "Content-Type: application/json" /DATA '{"number_of_the_beast":666}' /END
Pop $0

; POST json data as MIME multi-part form
NScurl::http POST "https://httpbin.org/post" Memory /POST "User" "My user name" /POST "Password" "My password" /POST FILENAME=MyFile.json TYPE=application/json "Details" '{"number_of_the_beast":666}' /END
Pop $0
```

## Licenses
Project        | License
:------        | :----------------------------------------------------
NScurl itself  | [BSD3](LICENSE.md)
brotli         | [MIT](https://github.com/google/brotli/blob/master/LICENSE)
curl           | [MIT/X inspired](https://curl.haxx.se/docs/copyright.html)
nghttp2        | [MIT](https://github.com/nghttp2/nghttp2/blob/master/COPYING)
OpenSSL        | [Apache v2](https://www.openssl.org/source/license.html)
zlib           | [zlib](https://www.zlib.net/zlib_license.html)
zstd           | [BSD3](https://github.com/facebook/zstd/blob/dev/LICENSE)
