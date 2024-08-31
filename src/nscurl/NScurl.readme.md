# About

`NScurl` is a [NSIS](https://github.com/negrutiu/nsis) (Nullsoft Scriptable Install System) plugin with advanced HTTP/HTTPS capabilities.

Implemented in `C` on top of [libcurl](https://curl.haxx.se/libcurl) with [OpenSSL](https://www.openssl.org) as SSL backend.

Resource              | Link
--------------------- | ------------------------------------------------
Project page          | https://github.com/negrutiu/nsis-nscurl  
Dependency            | https://github.com/negrutiu/libcurl-devel


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
Check out the [NSIS test script](https://github.com/negrutiu/nsis-nscurl/blob/master/tests/NScurl-Test.nsi).  

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

*******************************************************************************

# NScurl::http

## Syntax
```
NScurl::http method url output parameters /END
```

## Description
Create a new HTTP request and push it to the internal _transfer queue_.

New requests wait in the queue until a _worker thread_ becomes available to execute them.
Once completed, they remain in the _transfer queue_ and their data stays available for [querying](#nscurlquery).

By default, `NScurl::http` waits synchronously for the new transfer to complete, unless [`/BACKGROUND`](#background) parameter is used.

## Return value
The return value is pushed to the NSIS stack.

By default, the function returns the _transfer status_ string (equivalent to [`/RETURN "@error@"`](#return)).  
Successful transfers receive _transfer status_ `"OK"`.
Failed transfers receive various error messages (e.g `0x2a "Callback aborted"`, etc.)

[`/RETURN "query string"`](#return) parameter can be used to request custom return values.  
[Query keywords](#transfer-keywords) are automatically expanded with runtime data.

[`/BACKGROUND`](#background) parameter can be used to request background transfers.
The new HTTP request is pushed to the _transfer queue_ and the call returns immediately.
A unique _transfer ID_ is returned (equivalent to [`/RETURN "@id@"`](#return)) which can later be used to query more information.

## Parameters

### `method`
HTTP method such as `GET`, `POST`, `PUT`, `HEAD`, etc.

> [!important]
> This parameter is mandatory

### `url`
Full URI, including query parameters.  
The caller must [`NScurl::escape`](#nscurlescape) illegal URL characters.

> [!important]
> This parameter is mandatory

### `output`
```
filename | "MEMORY"
```

Absolute and relative file names are both allowed.  
Relative names use the [current directory](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcurrentdirectory) as base.

> [!tip]
> The current directory might change in unpredictable ways. Using absolute paths is recommended

`MEMORY` instructs the plugin to download the remote content _in-memory_.  
Data can be retrieved later using [`NScurl::query "@RECVDATA@"`](#transfer-keywords)  
In-memory downloads are limited in size by:
- the amount of physical memory installed
- the size of the virtual address space available to the _Installer_ process. That's usually up to 2GB in x86 processes (most common) and 128TB in x64 processes (these numbers might vary, see details [here](https://learn.microsoft.com/en-us/windows/win32/memory/memory-limits-for-windows-releases))

> [!note]
> The `output` parameter is mandatory

### /RETURN
```
/RETURN "query string"
```
Request a custom return value, for example [`/RETURN "@ERRORCODE@ - @ELAPSEDTIME@"`](#transfer-keywords)   
Default value is the _transfer status_ ([`/RETURN "@error@"`](#transfer-keywords))  

### /HTTP1.1
Disable `ALPN` negotiation for `HTTP/2`.  
Some servers might achieve better speed over `HTTP/1.1`.

### /PROXY
```
/PROXY "scheme://address.domain[:port]"
```
Connect through a web proxy server.  
Supported schemes: `http`, `https`, `socks4`, `socks4a`, `socks5`, `socks5a`.  
For more information visit libcurl [CURLOPT_PROXY](https://curl.haxx.se/libcurl/c/CURLOPT_PROXY.html) documentation.

### /DOH
```
/DOH "https://<domain>/dns-query"
```
Specify a [DNS over HTTPS](https://en.wikipedia.org/wiki/DNS_over_HTTPS) server to resolve DNS requests securely.

Examples:
- `/DOH "https://dns.quad9.net/dns-query"`  
- `/DOH "https://cloudflare-dns.com/dns-query"`  
- `/DOH "https://doh.opendns.com/dns-query"`  
- `/DOH "https://dns.google/dns-query"`  

> [!note]
> DoH server is used for the current transfer only, it's not a global setting

### /TIMEOUT
### /CONNECTTIMEOUT
```
/TIMEOUT time
/CONNECTTIMEOUT time
```

Connect timeout (default: 5m)

`time` applies to each re/connection attempt.  
By default, `NScurl` aborts the transfer if connecting times out. Use `/INSIST` parameter to request multiple attempts to re/connect.

`time` represents the timeout period in milliseconds. `s`, `m` or `h` suffixes are allowed.

Examples:
- `/TIMEOUT 5000`
- `/TIMEOUT 5s`
- `/TIMEOUT 15m`
- `/TIMEOUT 24h`

### /COMPLETETIMEOUT
```
/COMPLETETIMEOUT time
```
Total transfer timeout (default: _infinite_)  
This value sets a maximum time limit that a transfer is allowed to run.  
When the time runs out the transfer is automatically cancelled.  
See [`/TIMEOUT`](#timeout) for `time` syntax.

### /LOWSPEEDLIMIT
```
/LOWSPEEDLIMIT speed time
```
Aborts the transfer if the speed falls below `bps` for a period of `time`.  
Default value is `0bps for 1m` meaning that the current connection gets dropped after 1m of inactivity.  
`speed` unit is bytes/s.  
See [`/TIMEOUT`](#timeout) for `time` syntax.

Example: `/LOWSPEEDLIMIT 204800 30s`

### /SPEEDCAP
```
/SPEEDCAP speed
```
Speed cap (default: _none_)  
The transfer speed will not be allowed to exceed this value.  
`speed` unit is bytes/s.

Example: `/SPEEDCAP 409600` for a maximum of 400kb/s

### /INSIST
Instructs `NScurl` to re/connect to the webserver more aggressively.  
It will continue trying to connect even if critical errors occur (e.g. no network connectivity, webserver temporarily down, etc.)  
For `GET` requests it'll attempt to reestablish dropped connections and optionally [`/RESUME`](#resume) them.  
Without `/INSIST`, the transfer is cancelled at the first connection failure.

### /RESUME
Resume the transfer if (part of) the output file already exists locally.  
By default, the output file is always overwritten and the transfer starts over.

> [!important]
> When resuming, the local (partial) data is not validated to match the latest remote content. If the remote content has changed since the last partial download, the output file might be inconsistent. Avoid resuming arbitrary files

### /NOREDIRECT
Don't follow HTTP redirections.  
They are followed by default.

### /USERAGENT
```
/USERAGENT "agent"
```
Overwrite the default user agent ([`/USERAGENT "nscurl/@PluginVersion@"`](#global-keywords)).  
[Global keywords](#global-keywords) are automatically expanded with runtime data.  

### /REFERER
```
/REFERER url
```
Optional [referrer URL](https://en.wikipedia.org/wiki/HTTP_referer).

### /DEBUG
```
/DEBUG [nodata] file
```
Write transfer HTTP/SSL details to a debugging file.  
`nodata` is optional and prevents the remote content from being written to `file`.  
Disabled by default.

### /AUTH
```
/AUTH [TYPE=basic|digest|digestie] user pass
/AUTH TYPE=bearer token
```

* HTTP `user` and `pass` authentication  
`TYPE` is inferred automatically if not specified. `TYPE` is mandatory when connecting to servers with _hidden_ authentication.

* HTTP `OAuth 2.0` token authentication  
`TYPE=bearer` is mandatory

> [!caution]
> `user`, `pass` and `token` must be cleartext and unescaped

For more information visit libcurl [CURLOPT_HTTPAUTH](https://curl.haxx.se/libcurl/c/CURLOPT_HTTPAUTH.html) documentation.

### /TLSAUTH
```
/TLSAUTH user pass
```

[TLS-SRP](https://en.wikipedia.org/wiki/TLS-SRP) (Secure Remote Password) authentication.  
For more information visit libcurl [CURLOPT_TLSAUTH_TYPE](https://curl.haxx.se/libcurl/c/CURLOPT_TLSAUTH_TYPE.html) documentation.

> [!caution]
> `user` and `pass` must be cleartext and unescaped

### /HEADER
```
/HEADER header(s)
```

Send additional HTTP request header(s).  
Multiple headers can be separated by CRLF (`$\r$\n` in NSIS).  
Multiple `/HEADER` parameters are allowed.

### /DATA
```
/DATA [-string|-file] data
/DATA -memory address size
```

Upload local data to the web server.

`-string` and `-file` are optional hints to indicate `data` source.  
If unspecified, `NScurl` tries to guess whether `data` represents a file name or a generic string.

`-memory` is not automatically inferred and must be specified explicitly.

> [!note]
> `/DATA` works with `POST` or `PUT` methods. Ignored otherwise

Examples:
```nsis
NScurl::http PUT ${url} ${file} /DATA "Send generic string" /END
NScurl::http PUT ${url} ${file} /DATA "C:\path\to\file.dat" /END
NScurl::http PUT ${url} ${file} /DATA -string "Custom text" /END
NScurl::http PUT ${url} ${file} /DATA -memory 0xdeadbeef 256 /END
```

### /POST
```
/POST
    [FILENAME=remote_filename]
    [TYPE=mime_type]
    name
    [-string|-file|-memory] data
```

Upload data as a multipart form.

Parameter  | Details
---------- | ---------------------------------------
`FILENAME` | Optional remote file name
`TYPE`     | Optional [MIME type](https://www.iana.org/assignments/media-types/media-types.xhtml)
`name`     | Form part name
`data`     | Form part data. See [`NScurl::http /DATA`](#data) for `data` syntax

Multiple `/POST` parameters are allowallowed. All individual parts are sent as one multipart form.

Example:
```nsis
NScurl::http POST ${url} ${file} \
    /POST "User" "My User Name" \
    /POST "Pass" "My Password" \
    /POST "InfoFile" -string "$TEMP\MyFile.json" /* Upload the file path as string */ \
    /POST "InfoData" -file "$TEMP\MyFile.json"   /* Upload the file content */ \
    /POST "Image" FILENAME=MyImage.jpg TYPE=image/jpeg "$TEMP\MyImage.jpg" \
    /POST "Details" FILENAME=MyFile.json TYPE=application/json '{"number_of_the_beast":666}' \
    /END
```

> [!note]
> `/POST` works with `POST` method only. Ignored otherwise.

### /Zone.Identifier
### /MARKOFTHEWEB
Marks the output file with the [Mark of the Web](https://en.wikipedia.org/wiki/Mark_of_the_Web).  
An alternate NTFS data stream named `Zone.Identifier` is attached to the output file.

### /accept-encoding
### /ENCODING

Send the `Accept-Encoding: deflate, gzip` request header to the webserver.  
Servers that support encoding may decide to send compressed data.  
Be aware that during the transfer, the _content length_ indicates the compressed length and not the actual data size.

> [!important]
> Incompatible with `/RESUME`  
> Incompatible with `MEMORY` transfers

### /CACERT
```nsis
/CACERT builtin|none|""|<file>
```

Specify a `cacert.pem` database to be used for SSL certificate validation.

Parameter      | Details
:------------- | :---------------------------------------
`builtin`      | Use a built-in `cacert.pem` database, embedded into `NScurl.dll` at build time <br> This is the default option
`none` or `""` | Disable `cacert.pem` database usage
`<file>`       | Use an external `cacert.pem` file

> [!caution]
> The built-in `cacert.pem` can become outdated.  
> That could lead to legitimate webservers failing the SSL validation.  
> `libcurl project` maintains an online [cacert.pem](https://curl.haxx.se/docs/caextract.html) database that is generally considered trusted.
> Feel free to embed the latest version into your installer and feed it to `NScurl`

> [!caution]
> If all certificate sources are empty (e.g. `/CACERT none /CASTORE false` and no `/CERT` arguments), SSL certificate validation is disabled. `NScurl` would connect to any server, including untrusted ones (aka _insecure transfers_).
> By default, both the built-in `cacert.pem` and the __native CA store__ are used for validation.
> See [/SECURITY](#security) for more security options.

### /CASTORE
```nsis
/CASTORE true|false
```
Specify that Windows' native CA store should be used for SSL certificate validation.  
This option is `true` by default.  
When enabled, the native CA store is used __in addition__ to the other trusted certificate sources ([/CACERT](#cacert) and [/CERT](#cert))

### /CERT
```
/CERT sha1|pem
```
Specify additional trusted certificates to be used for SSL certificate validation __in addition__ to other certificate sources ([/CACERT](#cacert) and [/CASTORE](#castore)).  
Multiple `/CERT` parameters are allowed.

Parameter | Details
:---------------- | :---------------------
sha1 | `sha1` certificate thumbprint. The thumbprint can reference any certificate in the chain (the root, intermediate certificates, end-entity certificate)
pem  | `pem` blob containing one or more trusted root certificates <br> NOTE: Limited in size to `${NSIS_MAX_STRLEN}`

Examples:
```nsis
# Certificate pinning (accepts only 1111.. and 2222.. certificates)
NScurl::http GET ${url} ${file} /CACERT none /CASTORE false /CERT 1111111111111111111111111111111111111111 /CERT 2222222222222222222222222222222222222222 /END
Pop $0
```

```nsis
; Trust self-signed certificate
; NOTE: This cert might quickly become deprecated. Make sure you use the latest for testing
!define BADSSL_SELFSIGNED_CRT \
"-----BEGIN CERTIFICATE-----$\n\
MIIDeTCCAmGgAwIBAgIJAPhNZrCAQp0/MA0GCSqGSIb3DQEBCwUAMGIxCzAJBgNV$\n\
BAYTAlVTMRMwEQYDVQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNp$\n\
c2NvMQ8wDQYDVQQKDAZCYWRTU0wxFTATBgNVBAMMDCouYmFkc3NsLmNvbTAeFw0y$\n\
NDA4MjAxNjI0NDVaFw0yNjA4MjAxNjI0NDVaMGIxCzAJBgNVBAYTAlVTMRMwEQYD$\n\
VQQIDApDYWxpZm9ybmlhMRYwFAYDVQQHDA1TYW4gRnJhbmNpc2NvMQ8wDQYDVQQK$\n\
DAZCYWRTU0wxFTATBgNVBAMMDCouYmFkc3NsLmNvbTCCASIwDQYJKoZIhvcNAQEB$\n\
BQADggEPADCCAQoCggEBAMIE7PiM7gTCs9hQ1XBYzJMY61yoaEmwIrX5lZ6xKyx2$\n\
PmzAS2BMTOqytMAPgLaw+XLJhgL5XEFdEyt/ccRLvOmULlA3pmccYYz2QULFRtMW$\n\
hyefdOsKnRFSJiFzbIRMeVXk0WvoBj1IFVKtsyjbqv9u/2CVSndrOfEk0TG23U3A$\n\
xPxTuW1CrbV8/q71FdIzSOciccfCFHpsKOo3St/qbLVytH5aohbcabFXRNsKEqve$\n\
ww9HdFxBIuGa+RuT5q0iBikusbpJHAwnnqP7i/dAcgCskgjZjFeEU4EFy+b+a1SY$\n\
QCeFxxC7c3DvaRhBB0VVfPlkPz0sw6l865MaTIbRyoUCAwEAAaMyMDAwCQYDVR0T$\n\
BAIwADAjBgNVHREEHDAaggwqLmJhZHNzbC5jb22CCmJhZHNzbC5jb20wDQYJKoZI$\n\
hvcNAQELBQADggEBAF9F2x4tuIATEa5jZY86nEaa3Py2Rd0tjNywlryS1TKXWIqu$\n\
yim+0HpNU/R6cpkN1MZ1iN7dUKTtryLJIAXgaZC1TC6sRyuOMzV/rDHShT3WY0MW$\n\
+/sebaJZ4kkLUzQ1k5/FW/AmZ3su739vLQbcEEfn7UUK5cdRgcqEHA4SePhq5zQX$\n\
5/FSILsStpu+9hZ6OGxVdLVWKOM5GZ8LCXw3cJCNbJvW1APCz+3bP3bGBANeCUJp$\n\
gt0b83u4YBs1t66ZV/rcDQiyQzjAY6th2UfRggZxeIRDO7qbRa+M0pVW3qugMytf$\n\
bPw02aMbgH96rX61u0sd1M0slJHFEeqquqbtPcU=$\n\
-----END CERTIFICATE-----"

NScurl::http GET "https://self-signed.badssl.com" "${file}" /CERT '${BADSSL_SELFSIGNED_CRT}' /END
Pop $0
```

### /SECURITY
```
/SECURITY strong|weak
```
Configure the security level for the current transfer.  
The default security is `strong`.

Security level `strong`:
- use the default `openssl` crypto algorithms and standards that are considered secure

Security level `weak`:
- call [SSL_CTX_set_options(..., SSL_OP_ALLOW_UNSAFE_LEGACY_RENEGOTIATION);](https://docs.openssl.org/3.1/man3/SSL_CTX_set_options/#notes) to enable unsafe legacy renegociation
- call [SSL_CTX_set_security_level( 0 )](https://docs.openssl.org/1.1.1/man3/SSL_CTX_set_security_level/#default-callback-behaviour) to enable weak cryptographic algorithms
- call [curl_easy_setopt(..., CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1);](https://curl.se/libcurl/c/CURLOPT_SSLVERSION.html) to enable `SSL3`, `TLS 1.0` and `TLS 1.1` protocols


### /DEPEND
```
/DEPEND id
```
Make the new HTTP request dependent on another existing request.  
The new request waits in the queue until its dependency completes.  
Useful to establish a precise download order among multiple [`/BACKGROUND`](#background) transfers.

### /TAG
```
/TAG tag
```
Assign a tag (aka group) to the new HTTP request.  
Multiple transfers can be grouped together under the same tag.  
NOTE: Tags are arbitrary strings with no character restrictions.

Example:
```nsis
NScurl::http GET ${URL1} ${File1} /BACKGROUND /TAG "most important" /END
Pop $0  ; transfer ID, not used in this example
NScurl::http GET ${URL2} ${File2} /BACKGROUND /TAG "most important" /END
Pop $0
NScurl::http GET ${URL3} ${File3} /BACKGROUND /END
Pop $0

; do useful stuff

NScurl::wait /TAG "most important" /END         ; wait for important files...
NScurl::cancel /TAG "most important" /REMOVE    ; remove from queue (unnecessary, demo only)

; do useful stuff

NScurl::wait /TAG "less important" /END         ; wait for the remaining files...
```

### /BACKGROUND
By default [`NScurl::http`](#nscurlhttp) creates a new HTTP request and **waits** for it to complete (aka _synchronous transfer_).

`/BACKGROUND` creates a new HTTP background transfer and returns immediately without waiting (aka _asynchronous transfer_).  
No visual progress is displayed on the GUI.  
Returns a unique _transfer ID_  to the caller (aka [`/RETURN @id@`](#return))

Example:
```nsis
NScurl::http GET ${URL1} ${File1} /BACKGROUND /END
Pop $1  ; transfer ID
NScurl::http GET ${URL2} ${File2} /BACKGROUND /END
Pop $2  ; transfer ID

; do useful stuff while transfers run in the background

NScurl::wait /ID $1 /END    ; wait for file1
NScurl::wait /ID $2 /END    ; wait for file2

NScurl::query /ID $1 "[@OUTFILE@] @ERROR@, @AVGSPEED@, @RECVHEADERS@"
Pop $0  ; output string for file1
DetailPrint $0

NScurl::query /ID $2 "[@OUTFILE@] @ERROR@, @AVGSPEED@, @RECVHEADERS@"
Pop $0  ; output string for file2
DetailPrint $0
```

### /PAGE
Wait in _Page-mode_ for transfer completion.  
When waiting from an NSIS section (while on the `InstFiles` page), the function creates a dedicated progress bar to visually display the progress.  
`/PAGE` is the default waiting mode.
> [!caution]
> `/PAGE` is incompatible with `/BACKGROUND`

### /POPUP
Wait in _Popup-mode_ for transfer completion.  
Progress is displayed in a pop-up window.  
> [!caution]
> `/POPUP` is incompatible with `/BACKGROUND`

### /SILENT
Wait silently for transfer completion.  
No visual progress is displayed.  
`/SILENT` is the default waiting mode for _silent installers_.
> [!caution]
> `/SILENT` is incompatible with `/BACKGROUND`

### /CANCEL
Enable `Cancel` button when waiting in [`/PAGE`](#page) or [`/POPUP`](#popup) modes.  
`Cancel` is disabled by default.

### /TITLEWND
### /TEXTWND
### /PROGRESSWND
### /CANCELWND
```nsis
/TITLEWND hwnd
/TEXTWND hwnd
/PROGRESSWND hwnd
/CANCELWND hwnd
```
Optional _control handles_ (`HWND`) for `Title`, `Text/Status`, `progress bar`, `Cancel` controls.

### /STRING
```nsis
/STRING TITLE "string"
/STRING TITLE_NOSIZE "string"
/STRING TITLE_MULTI "string"
/STRING TEXT "string"
/STRING TEXT_NOSIZE "string"
/STRING TEXT_MULTI "string"
```
Overwrite the default (English) GUI messages.  
Useful to create localized installers.

[Query keywords](#transfer-keywords) are automatically expanded with runtime data.

### /END
`/END` marks the end of the variable parameter list.
> [!caution]
> This parameter is mandatory

*******************************************************************************

# NScurl::query

## Syntax

```nsis
NScurl::query [/ID id] [/TAG tag] "query string"
```

## Description
Query information about a transfer.  
The function expands [query keywords](#transfer-keywords) inside `query string` with runtime data.

There are two categories of keywords:
- [Transfer keywords](#transfer-keywords) are available only for single transfers (/ID id)
- [Global keywords](#global-keywords) are always available

## Return
The return value is pushed to the NSIS stack.

## Examples
```nsis
; Query information about a specific HTTP request
NScurl::http GET "https://download.sysinternals.com/files/SysinternalsSuite.zip" "$TEMP\SysinternalsSuite.zip" /RETURN `@id@` /END
Pop $0 ; transfer ID

NScurl::query /ID $0 "Status: @ERROR@, Headers: @RECVHEADERS@"
Pop $1 ; status + response headers</font>

; Query global information
NScurl::query "@TOTALSIZE@ - @TOTALSPEED@"
Pop $0
```

## Parameters

### /ID
```
/ID id
```
Query information about a specific transfer.  
The _transfer ID_ is returned by [`NScurl::http`](#nscurlhttp) called either with `/BACKGROUND` or with `/RETURN "@id@"`.

### /TAG
```
/TAG tag
```
Query information about multiple transfers tagged with `tag`.  
See [`NScurl::http /TAG`](#tag).

### `query string`
The input string.  
The function returns the expanded string back to the caller.

*******************************************************************************

## Transfer Keywords

### @ID@
Unique non-zero _transfer ID_.

### @STATUS@
Transfer queue status: `Waiting`, `Running` or `Complete`

### @METHOD@
HTTP method (e.g. `GET`, `POST`, `PUT`, etc.)

### @URL@
The original HTTP request URI.

### @FINALURL@
The final HTTP request URI, after all redirections had been followed.

### @OUT@
Transfer output location.  
Can be either a local file or `MEMORY`  
See [`NScurl::http output`](#output)

### @OUTFILE@
Output file name extracted from @OUT@ (e.g. "SysinternalsSuite.zip")

### @OUTDIR@
Output directory extracted from @OUT@ (e.g. "C:\Downloads")

### @SERVERIP@
Webserver IP address.

### @SERVERPORT@
Webserver IP port number (usually 443 or 80).

### @FILESIZE@
### @FILESIZE_B@
Remote file size extracted from `Content-Length` HTTP header.  
`@FILESIZE@` is replaced with a human-readable size (e.g. "100 bytes", "250 KB", "10 MB", "1.2 GB", etc.)  
`@FILESIZE_B@` is replaced with the size in bytes.  
> [!caution]
> Some servers don't send the `Content-Length` header, leading to unknown file size

### @XFERSIZE@
### @XFERSIZE_B@
The amount of data actually transferred.  
`@XFERSIZE@` is replaced with a human-readable size (e.g. "100 bytes", "250 KB", "10 MB", "1.2 GB", etc.)  
`@XFERSIZE_B@` is replaced with the size in bytes.  
The value is usually identical with `@FILESIZE@` except for _failed_ or _cancelled_ transfers.

### @PERCENT@
Transfer progress (a value between `0` and `100`).  
> [!caution]
> Some servers don't send the `Content-Length` header, resulting in unknown progress

### @SPEED@
### @SPEED_B@
The current transfer speed.  
`@SPEED@` is replaced with a human-readable value (e.g. "100 KB/s", "1.2 MB/s", etc.)  
`@SPEED_B@` is replaced with the speed in bytes/s

### @AVGSPEED@
### @AVGSPEED_B@
The average transfer speed.  
`@AVGSPEED@` is replaced with a human-readable value (e.g. "100 KB/s", "1.2 MB/s", etc.)  
`@AVGSPEED_B@` is replaced with the speed in bytes/s

### @TIMEELAPSED@
### @TIMEELAPSED_MS@
The elapsed transfer time.  
It doesn't include the time this request has waited in the queue.  
`@TIMEELAPSED@` is replaced with a human-readable value like \[d.][hh:]mm:ss (e.g. "05:02" for 5m and 2s)  
`@TIMEELAPSED_MS@` is replaced with the time value in milliseconds

### @TIMEREMAINING@
### @TIMEREMAINING_MS@
The estimated time until this transfer completes.  
`@TIMEREMAINING@` is replaced with a human-readable value like \[d.][hh:]mm:ss (e.g. "05:02" for 5m and 2s)  
`@TIMEREMAINING_MS@` is replaced with the time value in milliseconds.

### @SENTHEADERS@
### @SENTHEADERS_RAW@
### @SENTHEADERS:Header-Name@
HTTP request headers.  
`@SENTHEADERS@` returns a one-liner string containing all headers. Special characters `\t`, `\r`, `\n` are replaced by their string representations `"\t"`, `"\r"`, `"\n"`  
`@SENTHEADERS_RAW@` returns the original headers with no characters replaced  
`@SENTHEADERS:Header-Name@` returns the value of a specific header. (e.g. "@SENTHEADERS:Accept-Encoding@")

### @RECVHEADERS@
### @RECVHEADERS_RAW@
### @RECVHEADERS:Header-Name@
HTTP response headers.  
`@RECVHEADERS@` returns a one-liner string containing all headers. Special characters `\t`, `\r`, `\n` are replaced by their string representations `"\t"`, `"\r"`, `"\n"`  
`@RECVHEADERS_RAW@` returns the original headers with no characters replaced  
`@RECVHEADERS:Header-Name@` returns the value of a specific header. (e.g. "@SENTHEADERS:Content-Type@")

### @RECVDATA@
### @RECVDATA_RAW@
The remote content.  
`@RECVDATA[:offset[,size]][>file]@` receives a printable string. Non-printable characters are replaced with `.`  
`@RECVDATA_RAW[:offset[,size]][>file]@` receives the original data no characters replaced

Optional parameters are available to customize the query.
Parameter  | Details
---------- | ----------------------
offset     | Data offset. <br> Negative values are subtracted from the remote content length.
size       | Data size.
file       | Save data to file. Absolute and relative file names are both allowed. <br> NOTE: The saved content is always _RAW_ (aka no characters replaced)

```nsis
# Examples:
NScurl::query /id $0 "@RecvData:0,1024@"     ; get the first 1024 bytes of the remote content (offset 0, size 1024)
Pop $0
NScurl::query /id $0 "@RecvData:-1024,1024@" ; get the last 1024 bytes (offset -1024, size 1024)
Pop $0
NScurl::query /id $0 "@RecvData>$INSTDIR\file.ext@"         ; save all remote content to a file
Pop $0
NScurl::query /id $0 "@RecvData:0,4096>$INSTDIR\file.ext@"  ; save the first 4KB to a file
Pop $0
```

> [!tip]
> Can retrieve the remote content downloaded to `MEMORY`

> [!caution]
> The value returned by the `NScurl::query` function is subject to a size limit defined by the `${NSIS_MAX_STRLEN}` constant. Typically, the maximum length is 2KB, 4KB, or 8KB, depending on the specific NSIS build or fork you're using. If you need to access data beyond this limit, you can make multiple `NScurl::query` calls, each with a different _(offset, size)_ pair.

### @TAG@
Transfer tag, empty by default.  
Multiple transfers can be tagged with the same `tag`

### @ERROR@
The final _transfer status_.  
Successful transfers return status `"OK"`  
Failed transfers return various error messages (e.g `0x2a "Callback aborted"`, etc.)

### @ERRORCODE@
The numeric _transfer status_ code.  
A value of `0` indicates success. See [@ERRORTYPE@](#errortype) for details.

### @ERRORTYPE@
The _transfer status_ error type.

Type       | Meaning                        | Docs
:--------- | :----------------------------- | :-----
`win32`    | Win32 error code               | `ERROR_SUCCESS`(0) indicates success <br> https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes
`x509`     | `openssl/x509` certificate error | `X509_V_OK`(0) indicates success <br> https://github.com/openssl/openssl/blob/ca1d2db291530a827555b40974ed81efb91c2d19/include/openssl/x509_vfy.h.in#L206
`curl`     | `libcurl` error code           | `CURLE_OK`(0) indicates success <br> https://curl.se/libcurl/c/libcurl-errors.html
`HTTP`     | HTTP status code               | `2xx` indicates success <br> https://en.wikipedia.org/wiki/List_of_HTTP_status_codes

### @CANCELLED@
Indicates whether the transfer was cancelled by the user.  
Returns boolean value `0` or `1`

*******************************************************************************

## Global Keywords

### @PLUGINNAME@
Plugin name (`NScurl`)

### @PLUGINVERSION@
Plugin version.  
Returns the `FileVersion` value from the Version Information resource block.

### @PLUGINAUTHOR@
Author name.  
Returns the `CompanyName` value from the Version Information resource block.

### @PLUGINWEB@
Project website.  
Returns the `LegalTrademarks` value from the Version Information resource block.

### @CURLVERSION@
libcurl version (e.g. "8.7.1")

### @CURLSSLVERSION@
SSL backend version (e.g. "OpenSSL/3.3.0")

### @CURLPROTOCOLS@
libcurl built-in protocols (e.g. "http https")

### @CURLFEATURES@
libcurl built-in features (e.g. "SSL NTLM Debug AsynchDNS Largefile TLS-SRP UnixSockets")

### @USERAGENT@
The default user agent (e.g. "nscurl/1.2020.3.1")

### @TOTALCOUNT@
The number of HTTP requests in the _transfer queue_.  
Includes all `Waiting`, `Running` and `Complete` requests.

### @TOTALWAITING@
The number of `Waiting` requests in the _transfer queue_.

### @TOTALRUNNING@
The number of `Running` requests in the _transfer queue_.

### @TOTALCOMPLETE@
The number of `Complete` requests in the _transfer queue_.

### @TOTALACTIVE@
The number of `Waiting` + `Running` requests in the _transfer queue_.

### @TOTALSTARTED@
The number of `Running` + `Completed` requests in the _transfer queue_.

### @TOTALERRORS@
The number of failed requests in the queue.

### @TOTALSPEED@
### @TOTALSPEED_B@
The aggregated speed of all `Running` transfers.  
`@TOTALSPEED@` is replaced with a human-readable value (e.g. "120 KB/s", "1.2 MB/s", etc.)  
`@TOTALSPEED_B@` is replaced with the speed in bytes/s

### @TOTALSIZE@
### @TOTALSIZE_B@
The aggregated amount of Downloaded + Uploaded data.  
`@TOTALSIZE@` is replaced with a human-readable value (e.g. "100 MB", "5 GB", etc.)  
`@TOTALSIZE_B@` is replaced with the size in bytes

### @TOTALSIZEUP@
### @TOTALSIZEUP_B@
The aggregated amount of Uploaded data.  
`@TOTALSIZEUP@` is replaced with a human-readable value (e.g. "100 MB", "5 GB", etc.)  
`@TOTALSIZEUP_B@` is replaced with the size in bytes

### @TOTALSIZEDOWN@
### @TOTALSIZEDOWN_B@
The aggregated amount of Downloaded data.  
`@TOTALSIZEDOWN@` is replaced with a human-readable value (e.g. "100 MB", "5 GB", etc.)  
`@TOTALSIZEDOWN_B@` is replaced with the size in bytes

### @THREADS@
Current number of worker threads.

### @MAXTHREADS@
Maximum number of worker threads.

*******************************************************************************

# NScurl::wait

## Syntax
```
NScurl::wait [/ID id] [/TAG tag] parameters /END
```

## Description
Wait synchronously for one or more [`/BACKGROUND`](#background) transfers to complete.  
Depending on parameters visual progress may or may not be displayed.

## Return
None.

## Example
```nsis
; Start multiple background transfers
NScurl::http GET ${URL1} ${FILE1} /TAG "filegroup1" /BACKGROUND /END
Pop $0	; transfer ID
NScurl::http GET ${URL2} ${FILE2} /TAG "filegroup1" /BACKGROUND /END
Pop $1	; transfer ID

; >>> do some useful work

; Wait for transfers...
NScurl::wait /TAG "filegroup1" /CANCEL /END
```

## Parameters

### /ID
```
/ID id
```
Wait for a specific transfer.  
The _transfer ID_ is returned by [`NScurl::http`](#nscurlhttp) called either with `/BACKGROUND` or with `/RETURN "@id@"`.

### /TAG
```
/TAG tag
```
Wait for multiple transfers tagged with `tag`  
See [`NScurl::http /TAG`](#tag)

### /PAGE
See [`NScurl::http /PAGE`](#page)

### /POPUP
See [`NScurl::http /POPUP`](#popup)

### /SILENT
See [`NScurl::http /SILENT`](#silent)

### /CANCEL
See [`NScurl::http /CANCEL](#cancel)

### /TITLEWND
### /TEXTWND
### /PROGRESSWND
### /CANCELWND
```nsis
/TITLEWND hwnd
/TEXTWND hwnd
/PROGRESSWND hwnd
/CANCELWND hwnd
```
See [`NScurl /...`](#titlewnd)

### /STRING
```nsis
/STRING TITLE "string"
/STRING TITLE_NOSIZE "string"
/STRING TITLE_MULTI "string"
/STRING TEXT "string"
/STRING TEXT_NOSIZE "string"
/STRING TEXT_MULTI "string"
```
See [`NScurl::http`](#string)

### /END
`/END` marks the end of the variable parameter list.
> [!caution]
> This parameter is mandatory

*******************************************************************************

# NScurl::enumerate

## Syntax
```
NScurl::enumerate [/TAG tag] [/STATUS status] /END
```

## Description
Enumerate HTTP transfers from the internal transfer queue.

## Return
Transfer ID's are pushed one by one to the stack.  
An empty string ("") is pushed to mark the end of the enumeration.

## Example
```nsis
NScurl::enumerate /END
_enum_loop:
  Pop $0
  StrCmp $0 "" _enum_end
  DetailPrint "> transfer ID $0"
  Goto _enum_loop
_enum_end:
```

## Parameters

### /TAG
```
/TAG tag
```
Enumerate transfers tagged with `tag`.  
See [`NScurl::http /TAG`](#tag)

### /STATUS
```nsis
/STATUS "Waiting|Running|Complete"
```
Enumerate transfers based on their _queue status_:
- `Waiting`: transfers that are still waiting in the queue
- `Running`: transfers currently in progress
- `Complete`: transfers complete, aborted or failed

Multiple `/STATUS` parameters are allowed.

### /END
`/END` marks the end of the variable parameter list.
> [!caution]
> This parameter is mandatory

*******************************************************************************

# NScurl::cancel

## Syntax
```
NScurl::cancel [/ID id] [/TAG tag] [/REMOVE]
```
## Description
Cancel (background) transfers and optionally remove them from the queue.

## Return
None.

## Parameters

### /ID
```
/ID id
```
Cancel a specific transfer.  
The _transfer ID_ is returned by [`NScurl::http`](#nscurlhttp) called either with `/BACKGROUND` or with `/RETURN "@id@"`

### /TAG
```
/TAG tag
```
Cancel multiple transfers tagged with `tag`.  
See [`NScurl::http /TAG`](#tag)

### /REMOVE
In addition to cancelling, the transfer(s) are also permanently removed from the queue.  
Further [`NScurl::query`](#nscurlquery) calls will fail.

*******************************************************************************

# NScurl::escape
# NScurl::unescape

## Syntax
```nsis
NScurl::escape "string"
NScurl::unescape "string"
```

## Description
Utility function to un/escape URL strings.  
Illegal URL characters are converted to/from their hexadecimal `%XX` code.

## Return
The un/escaped string is pushed to the NSIS stack.

## Example
```nsis
NScurl::escape "aaa bbb ccc=ddd&eee"
Pop $0	; Returns "aaa%20bbb%20ccc%3Dddd%26eee"

NScurl::unescape $0
Pop $0	; Returns the original string
```
*******************************************************************************

# NScurl::md5
# NScurl::sha1
# NScurl::sha256

## Syntax
```
NScurl::md5 [-string|-file|-memory] data
NScurl::sha1 [-string|-file|-memory] data
NScurl::sha256 [-string|-file|-memory] data
```

## Description
Utility functions that compute MD5 / SHA1 / SHA256 hashes.  
The data can be read either from a file or directly from memory.  
See [`NScurl::http /DATA`](#data) for `data` syntax.

## Return
The hash string is pushed to the NSIS stack.

## Examples

```nsis
NScurl::md5 "Hash this string"
Pop $0	; e.g. "376630459092d7682c2a2e745d74aa6b"

NScurl::md5 $EXEPATH
Pop $0	; e.g. "93a52d04f7b56bc267d45bd95c6de49b"

NScurl::sha1 -file $EXEPATH
Pop $0	; e.g. "faff487654d4dfa1deb5e5d462c8cf51b00a4acd"

NScurl::sha1 -string $EXEPATH ; The file path as string
Pop $0

NScurl::sha256 $EXEPATH
Pop $0	; e.g. "e6fababe9530b1d5c4395ce0a1379c201ebb017997e4671a442a8410d1e2e6ac"
```
