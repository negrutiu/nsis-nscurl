
# NScurl development script
# Marius Negrutiu - https://github.com/negrutiu/nsis-nscurl
# Syntax: Test.exe /dll <nscurl.dll>

!ifdef TARGET
	Target ${TARGET}                    ; x86-unicode, x86-ansi, amd64-unicode
!else
	!define TARGET x86-unicode          ; Default
!endif

# /dll commandline parameter
Var /global DLL

!if /fileexists "${NSISDIR}\Include\ModernXXL.nsh"
	!include "ModernXXL.nsh"		    ; Available in the NSIS fork from https://github.com/negrutiu/nsis
!endif
!include "MUI2.nsh"
!define LOGICLIB_STRCMP
!include "LogicLib.nsh"
!include "Sections.nsh"

!include "FileFunc.nsh"
!insertmacro GetFileName
!insertmacro GetOptions
!insertmacro GetParameters

!define /ifndef NULL 0
!define /ifndef TRUE 1
!define /ifndef FALSE 0
!define TEST_FILE "$SYSDIR\lz32.dll"	; ...random file that exists in every Windows build


# GUI settings
!define /ifndef MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install-nsis.ico"
!define /ifndef MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-nsis.bmp"

# Welcome page
;!define MUI_WELCOMEPAGE_TITLE_3LINES
;!insertmacro MUI_PAGE_WELCOME

# Components page
!define INSTTYPE_NONE	1
!define INSTTYPE_MOST	2
InstType "None"			; 1
InstType "Most"			; 2
!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS

# Installation page
!insertmacro MUI_PAGE_INSTFILES

# Language
!insertmacro MUI_LANGUAGE "English"
;!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_RESERVEFILE_LANGDLL

# Installer details
Name    "NScurl-Debug-${TARGET}"
OutFile "NScurl-Debug-${TARGET}.exe"
XPStyle on
RequestExecutionLevel user		        ; Don't require UAC elevation
ShowInstDetails show
ManifestDPIAware true

Var /global g_testCount
Var /global g_testFails

!macro STACK_VERIFY_START
	Push "MyStackTop"			        ; Mark the top of the stack
!macroend

!macro STACK_VERIFY_END
	Pop $R9						        ; Validate our stack marker
	StrCmp $R9 "MyStackTop" +2 +1
		MessageBox MB_ICONSTOP 'Stack is NOT OK$\r$\nStack top: "$R9"'
!macroend

#---------------------------------------------------------------#
# .onInit                                                       #
#---------------------------------------------------------------#
Function .onInit

	; Initializations
	InitPluginsDir
    StrCpy $g_testCount 0
    StrCpy $g_testFails 0

	; Language selection
	!define MUI_LANGDLL_ALLLANGUAGES
	!insertmacro MUI_LANGDLL_DISPLAY

	; Command line
	${GetParameters} $R0
	${GetOptions} "$R0" "/dll" $DLL
	${If} ${Errors}
		MessageBox MB_ICONSTOP 'Syntax:$\n"$EXEFILE" /DLL <NScurl.dll>'
		Abort
	${EndIf}

/*
	; .onInit download demo
	; NOTE: Transfers from .onInit can be either Silent or Popup (no Page!)
	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_onInit.zip"
	Push "/END"
	Push "/POPUP"
	Push "/CANCEL"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	!insertmacro STACK_VERIFY_END
*/

    ; Quick .onInit plugin test
	Push "/END"
	Push "@PLUGINWEB@"
	CallInstDLL $DLL query
    Pop $0
    StrCpy $1 $0 8
    ${If} $1 != "https://"
        MessageBox MB_ICONSTOP '[.onInit]$\nFailed to query plugin webpage$\nReturn value: "$0"'
    ${EndIf}

FunctionEnd


Section "Cleanup test files"
	SectionIn ${INSTTYPE_NONE} ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	FindFirst $0 $1 "$EXEDIR\_*.*"
loop:
	StrCmp $1 "" done
	Delete "$EXEDIR\$1"
	FindNext $0 $1
	Goto loop
done:
	FindClose $0
SectionEnd


Section "Background (50 * put)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	StrCpy $1 ""
	${For} $R0 1 50
		Push /END
		Push "parallels"		; Custom string
		Push /TAG
		Push /INSIST
		Push /BACKGROUND
		Push "${TEST_FILE}"
		Push "-file"
		Push /DATA
		Push "Memory"
		Push "https://httpbin.org/put"
		Push "PUT"
		CallInstDLL $DLL http
		Pop $0
		IntCmp $R0 1 +2 +1 +1
			StrCpy $1 "$1, "
		StrCpy $1 "$1$0"
	${Next}
	DetailPrint "IDs = {$1}"
SectionEnd


Section "httpbin.org/get"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/get?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbin.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"

	;Push "${FILE}.md"
	;Push /DEBUG
	;Push "https://cloudflare-dns.com/dns-query"
	;Push /DOH

	Push "https://test.com"
	Push "/REFERER"

	Push "Header3: Value3"
	Push "/HEADER"
	Push "Header1: Value1$\r$\nHeader2: Value2"
	Push "/HEADER"

	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0

	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbun.com/cookies/set"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbun.com/cookies/set?cookie1=value1&cookie2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbun_cookies_set'
    !define /redef JAR  '$EXEDIR\_GET_httpbun_cookiejar.txt'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"

	; Push "${FILE}.md"
	; Push /DEBUG

    Push "${JAR}"
    Push "/COOKIEJAR"

	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0

    ${If} ${FileExists} "${JAR}"
        StrCpy $1 "OK (file exists)"
    ${Else}
        StrCpy $1 "FAIL (not found: ${JAR})"
    ${EndIf}

	DetailPrint "Status: $0"
	DetailPrint "Cookie jar: $1"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "sysinternals.com/get (Page-Mode)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "/Zone.Identifier"
	Push "/INSIST"
	Push "/CANCEL"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "sysinternals.com/get (Popup-Mode)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Popup.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "/Zone.Identifier"
	Push "/INSIST"
	Push "/POPUP"
	Push "/CANCEL"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "sysinternals.com/get (Silent-Mode)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Silent.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "/Zone.Identifier"
	Push "/INSIST"
	Push "/SILENT"
	Push "/CANCEL"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "sysinternals.com/get (HTTP/1.1)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuite_http1.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "/Zone.Identifier"
	Push "/CANCEL"
	Push "/INSIST"
	Push "/HTTP1.1"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "sysinternals.com/get (Memory)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuite_memory.zip"

	DetailPrint 'NScurl::http "${LINK}" "Memory"'
	Push "/END"
    Push "@id@"     ; _transfer ID_ as return value (instead of _transfer status_)
	Push "/RETURN"
	Push "/INSIST"
	Push "/CANCEL"
	Push "Memory"   ; Memory
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $R0
	DetailPrint "ID: $R0"

    ; For demonstration purposes, we'll retrieve the first two bytes of the remote content stored in memory
    ; If the data begins with the "PK" sequence (the standard zip file magic bytes), we'll save it to disk as a .zip file
    DetailPrint 'NScurl::query /id $R0 "@RecvData:0,2@"'
    Push "@RecvData:0,2@"   ; offset:0, size:2
    Push $R0
    Push "/id"
    CallInstDLL $DLL query
    Pop $0
    DetailPrint '  RecvData[0,2]: "$0"'

    ${If} $0 == "PK"
        DetailPrint 'NScurl::query /id $R0 "@RecvData>${FILE}@"'
        Push "  @RecvData>${FILE}@"
        Push $R0
        Push "/id"
        CallInstDLL $DLL query
        Pop $0      ; @RecvData@ trimmed down to ${NSIS_MAX_STRLEN}
        DetailPrint '  RecvData: $0'
    ${EndIf}

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "sysinternals.com/get (SpeedCap: 300KB/s)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_SpeedCap.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "/Zone.Identifier"
	Push 307200			; 300 * 1024
	Push "/SPEEDCAP"
	Push "/INSIST"
	Push "/RESUME"
	Push "/CANCEL"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "github.com/get (Encoding)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://raw.githubusercontent.com/negrutiu/nsis-nscurl/master/src/nscurl/curl.c"
	!define /redef FILE  "$EXEDIR\_curl.c"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
    Push "${FILE}.debug.txt"
    Push "nodata"
    Push "/DEBUG"
    Push "@id@"
    Push "/RETURN"
	Push "/Zone.Identifier"
	Push "/INSIST"
	;Push "/RESUME"
	Push "/CANCEL"
	Push "/ENCODING"	; incompatible with /RESUME or MEMORY transfers
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $R0
	DetailPrint "ID: $R0"

	Push "@RecvHeaders:content-encoding@"
	Push $R0
	Push "/id"
	CallInstDLL $DLL query
    Pop $0
    DetailPrint "Reply Headers[content-encoding]: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/post (multipart/form-data)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_multipart.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	!define S1 "<Your memory data here>"
	Push $R0
	Push $R1
	StrLen $R1 "${S1}"
	System::Call '*(&m128 "${S1}") p.r10'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push "/INSIST"

	Push $R1
	Push $R0
	Push "-memory"
	Push "Binary"
	Push "type=application/octet-stream"
	Push "/POST"

	Push "${TEST_FILE}"
	Push "-file"
	Push "test2.bin"
	Push "filename=test2.bin"
	Push "/POST"

	Push "${TEST_FILE}"
	Push "-file"
	Push "test.bin"
	Push "filename=test.bin"
	Push "/POST"

	Push "<Your password here>"
	Push "Password"
	Push "/POST"

	Push "<Your name here>"
	Push "Name"
	Push "/POST"

	Push '{ "number_of_the_beast": 666 }'
	Push "maiden.json"
	Push "type=application/json"
	Push "filename=maiden.json"
	Push "/POST"

	Push "Header3: Value3"
	Push "/HEADER"
	Push "Header1: Value1$\r$\nHeader2: Value2"
	Push "/HEADER"

	Push "${FILE}"
	Push "${LINK}"
	Push "POST"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"

	System::Free $R0
	Pop $R1
	Pop $R0
	!undef S1

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/post (application/x-www-form-urlencoded)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_postfields.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push "/INSIST"

	Push 'User=Your+name+here&Password=Your+password+here'
	Push "/DATA"

	Push "Header3: Value3"
	Push "/HEADER"
	Push "Header1: Value1$\r$\nHeader2: Value2"
	Push "/HEADER"

	Push "${FILE}"
	Push "${LINK}"
	Push "POST"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/post (application/json)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_json.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push "/INSIST"

	Push '{ "number_of_the_beast" : 666 }'
	Push "/DATA"

	Push "Content-Type: application/json"
	Push "/HEADER"

	Push "${FILE}"
	Push "${LINK}"
	Push "POST"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/put"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/put?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_PUT_httpbin.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push "/INSIST"

	Push '{ "number_of_the_beast" : 666 }'
	Push "/DATA"

	Push "Content-Type: application/json"
	Push "/HEADER"

	Push "Header3: Value3"
	Push "/HEADER"
	Push "Header1: Value1$\r$\nHeader2: Value2"
	Push "/HEADER"

	Push "${FILE}"
	Push "${LINK}"
	Push "PUT"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "Big file (100MB)"
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://speed.hetzner.de/100MB.bin'
	!define /redef FILE '$EXEDIR\_GET_100MB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
    Push "[@PERCENT@%] @TIMEELAPSED@ / @TIMEREMAINING@, @XFERSIZE@ / @FILESIZE@, Average @AVGSPEED@, Speed @SPEED@"
    Push "TEXT"
    Push "/STRING"

    Push "curl/@CURLVERSION@"
    Push "/USERAGENT"

	Push "$HWNDPARENT"
	Push "/TITLEWND"

	Push "1m"
	Push "/TIMEOUT"
	Push "/RESUME"
	Push "/CANCEL"
	Push "/INSIST"

	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "Big file (10GB)"
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://speed.hetzner.de/10GB.bin'
	!define /redef FILE '$EXEDIR\_GET_10GB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
    Push "[@PERCENT@%] @TIMEELAPSED@ / @TIMEREMAINING@, @XFERSIZE@ / @FILESIZE@, Average @AVGSPEED@, Speed @SPEED@"
    Push "TEXT"
    Push "/STRING"

	Push "$HWNDPARENT"
	Push "/TITLEWND"

    Push "curl/@CURLVERSION@"
    Push "/USERAGENT"

	Push "1m"
	Push "/TIMEOUT"
	Push "/RESUME"
	Push "/CANCEL"
	Push "/INSIST"

	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


SectionGroup /e "Tests"

; Valid to: ‎Sunday, ‎August ‎9, ‎2026 7:09:21 PM
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

!define BADSSL_SELFSIGNED_THUMBPRINT '8577cec7988ad89d72400f5933988221984e3009'


Var /global testCacertName
Var /global testCacertValue
Var /global testCastoreName
Var /global testCastoreValue
Var /global testCertName
Var /global testCertValue
Var /global testSecurityName
Var /global testSecurityValue

!macro REPORT_TEST _errorTypeExpected _errorCodeExpected _errorTypeEncountered _errorCodeEncountered
    IntOp $g_testCount $g_testCount + 1
    ${If} `${_errorTypeEncountered}` == `${_errorTypeExpected}`
    ${AndIf} `${_errorCodeEncountered}` = `${_errorCodeExpected}`
        DetailPrint `[ OK ] ${_errorTypeEncountered}/${_errorCodeEncountered}`
    ${Else}
        IntOp $g_testFails $g_testFails + 1
        DetailPrint `----- FAIL ----- ${_errorTypeEncountered}/${_errorCodeEncountered} => expected ${_errorTypeExpected}/${_errorCodeExpected}`
    ${EndIf}
!macroend

!macro TRANSFER_TEST url file cacert castore cert security errortype errorcode
    StrCpy $R0 '${file}'

    ${If} `${cacert}` == ""
        StrCpy $testCacertName ""
        StrCpy $testCacertValue ""
        StrCpy $R0 '$R0_defcacert'
    ${ElseIf} `${cacert}` == "none"
    ${OrIf} `${cacert}` == "builtin"
        StrCpy $testCacertName "/CACERT"
        StrCpy $testCacertValue `${cacert}`
        StrCpy $R0 '$R0_${cacert}'
    ${Else}
        StrCpy $testCacertName "/CACERT"
        StrCpy $testCacertValue `${cacert}`
        StrCpy $R0 '$R0_file'
    ${EndIf}

    ${If} `${castore}` == ""
        StrCpy $testCastoreName ""
        StrCpy $testCastoreValue ""
        StrCpy $R0 '$R0_defcastore'
    ${Else}
        StrCpy $testCastoreName "/CASTORE"
        StrCpy $testCastoreValue `${castore}`
        StrCpy $R0 '$R0_${castore}'
    ${EndIf}

    ${If} `${cert}` == ""
        StrCpy $testCertName ""
        StrCpy $testCertValue ""
        StrCpy $R0 '$R0_defcert'
    ${Else}
        StrCpy $testCertName "/CERT"
        StrCpy $testCertValue `${cert}`
        StrCpy $0 `${cert}` 8
        StrCpy $R0 '$R0_$0'
    ${EndIf}

    ${If} `${security}` == ""
        StrCpy $testSecurityName ""
        StrCpy $testSecurityValue ""
        StrCpy $R0 '$R0_defsecurity'
    ${Else}
        StrCpy $testSecurityName "/SECURITY"
        StrCpy $testSecurityValue `${security}`
        StrCpy $R0 '$R0_${security}'
    ${EndIf}

    ${GetFileName} $R0 $0
	DetailPrint 'NScurl::http "${url}" "$0"'

	!insertmacro STACK_VERIFY_START
	Push "/END"
    Push "$R0.debug.txt"
    Push "nodata"
    Push /DEBUG
    Push "test"
    Push /TAG
    Push "@ID@"
    Push /RETURN
    Push /CANCEL
    Push /INSIST
    Push 60s
    Push /TIMEOUT           ; badssl.com can be laggy sometimes
    Push $testCertValue
    Push $testCertName
    Push $testCastoreValue
    Push $testCastoreName
    Push $testCacertValue
    Push $testCacertName
    Push $testSecurityValue
    Push $testSecurityName
	Push memory
	Push "${url}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0

    Push "@Error@"
    Push $0
    Push /ID
    CallInstDLL $DLL query
    Pop $1
    ${If} $1 != "OK"
        DetailPrint "Status: $1"
    ${EndIf}

    Push "@ErrorType@"
    Push $0
    Push /ID
    CallInstDLL $DLL query
    Pop $1

    Push "@ErrorCode@"
    Push $0
    Push /ID
    CallInstDLL $DLL query
    Pop $2

	!insertmacro REPORT_TEST ${errortype} ${errorcode} $1 $2

	!insertmacro STACK_VERIFY_END
!macroend

Section "Expired certificate"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://expired.badssl.com'
	!define /redef FILE '$EXEDIR\_test_expired'

    !define /ifndef X509_V_ERR_CERT_HAS_EXPIRED 10

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' ''     ''      '' '' x509 ${X509_V_ERR_CERT_HAS_EXPIRED}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'true'  '' '' x509 ${X509_V_ERR_CERT_HAS_EXPIRED}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'false' '' '' http 200       ; SSL validation disabled

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Wrong host"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://wrong.host.badssl.com'
	!define /redef FILE '$EXEDIR\_test_wronghost'

    !define /ifndef CURLE_PEER_FAILED_VERIFICATION 60

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' ''     ''      '' '' curl ${CURLE_PEER_FAILED_VERIFICATION}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'true'  '' '' curl ${CURLE_PEER_FAILED_VERIFICATION}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'false' '' '' http 200       ; SSL validation disabled

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Untrusted root"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://untrusted-root.badssl.com'
	!define /redef FILE '$EXEDIR\_test_untrustroot'

    !define /ifndef UNTRUSTED_CERT '7890C8934D5869B25D2F8D0D646F9A5D7385BA85'
    !define /ifndef X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN 19

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' ''                '' x509 ${X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' ${UNTRUSTED_CERT} '' http 200

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'true'  '' '' x509 ${X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'false' '' '' http 200       ; SSL validation disabled

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Self-signed certificate"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://self-signed.badssl.com'
	!define /redef FILE '$EXEDIR\_test_selfsigned'

    !define /ifndef X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT 18

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' ''        '' '' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'builtin' '' '' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none'    '' '' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'false' '${BADSSL_SELFSIGNED_CRT}' '' http 200

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'builtin' 'true'  '' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'builtin' 'false' '' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none'    'true'  '' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'false' '' '' http 200       ; SSL validation disabled

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'true'  '1111111111111111111111111111111111111111' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'false' '1111111111111111111111111111111111111111' '' x509 ${X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT}

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'true'  ${BADSSL_SELFSIGNED_THUMBPRINT} '' http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' 'none' 'false' ${BADSSL_SELFSIGNED_THUMBPRINT} '' http 200

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Unsafe legacy renegociation"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://publicinfobanjir.water.gov.my'
	!define /redef FILE '$EXEDIR\_test_legacynego'

    !define /redef CURLE_SSL_CONNECT_ERROR 35

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' ''       curl ${CURLE_SSL_CONNECT_ERROR} ; 'strong' by default
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'weak'   http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'strong' curl ${CURLE_SSL_CONNECT_ERROR} ; OpenSSL/3.3.1: error:0A000152:SSL routines::unsafe legacy renegotiation disabled

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Weak protocols"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

    !define /redef CURLE_SSL_CONNECT_ERROR 35
    
	!define /redef LINK 'https://tls-v1-0.badssl.com:1010/'
	!define /redef FILE '$EXEDIR\_test_weaktls10'

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' ''       curl ${CURLE_SSL_CONNECT_ERROR}    ; 'strong' by default
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'weak'   http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'strong' curl ${CURLE_SSL_CONNECT_ERROR}    ; OpenSSL/3.3.1: error:0A000102:SSL routines::unsupported protocol


	!define /redef LINK 'https://tls-v1-1.badssl.com:1011/'
	!define /redef FILE '$EXEDIR\_test_weaktls11'

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' ''       curl ${CURLE_SSL_CONNECT_ERROR}    ; 'strong' by default
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'weak'   http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'strong' curl ${CURLE_SSL_CONNECT_ERROR}    ; OpenSSL/3.3.1: error:0A000102:SSL routines::unsupported protocol


    !define /redef LINK 'https://tls-v1-2.badssl.com:1012/'
	!define /redef FILE '$EXEDIR\_test_weaktls12'

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' ''       http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'weak'   http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'strong' http 200   ; TLS 1.2 should always work

    ; ----------------------------------------------

    !define /redef LINK 'https://dh2048.badssl.com'
	!define /redef FILE '$EXEDIR\_test_weakdh2k'

    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' ''       http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'weak'   http 200
    !insertmacro TRANSFER_TEST '${LINK}' '${FILE}' '' '' '' 'strong' http 200

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Cookie jar"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbun.com/cookies/set?cookie1=value1&cookie2=value2'
	!define /redef FILE '$EXEDIR\_test_cookiejar_body'
	!define /redef JAR  '$EXEDIR\_test_cookiejar.txt'

    Delete "${JAR}"

    DetailPrint 'NScurl::http "${LINK}" /COOKIEJAR "${JAR}"'
	Push /END
	Push "test"
	Push /TAG
	Push "${JAR}"
	Push /COOKIEJAR
	Push "${FILE}"
	Push "${LINK}"
	Push get
	CallInstDLL $DLL http
    Pop $0

    ${If} ${FileExists} "${JAR}"
        StrCpy $1 ${TRUE}
    ${Else}
        StrCpy $1 ${FALSE}
    ${EndIf}
	!insertmacro REPORT_TEST "jar exists" ${TRUE} "jar exists" $1

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "HTTP/3"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	; https://bagder.github.io/HTTP3-test
	!define /redef LINK 'https://nghttp2.org:4433'
	!define /redef FILE '$EXEDIR\_test_nghttp2-org.html'

	DetailPrint 'NScurl::http GET "${LINK}" "${FILE}" /HTTP3'
	Push /END
	;Push "${FILE}.md"
	;Push /DEBUG
	Push "test"
	Push /tag
	Push /http3
    Push "@id@"
    Push /RETURN
	Push "${FILE}"
	Push "${LINK}"
	Push GET
	CallInstDLL $DLL http
	Pop $0	; transfer ID

    Push "@ErrorType@"
    Push $0
    Push /ID
    CallInstDLL $DLL query
    Pop $1

    Push "@ErrorCode@"
    Push $0
    Push /ID
    CallInstDLL $DLL query
    Pop $2

    Push "@RecvHeaders@"
    Push $0
    Push /ID
    CallInstDLL $DLL query
    Pop $3

	StrCpy $1 $3 6 ; extract leading "HTTP/x"
	!insertmacro REPORT_TEST "HTTP/3" 200 $1 $2

    Push /REMOVE
    Push "test"
    Push /TAG
    CallInstDLL $DLL cancel     ; no return

	!insertmacro STACK_VERIFY_END
SectionEnd


SectionGroupEnd


SectionGroup /e "Errors"

Section "httpbin.org/get/status/40x"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/status/400,401,402,403,404,405'
	!define /redef FILE '$EXEDIR\_GET_httpbin_40x.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30s
	Push "/TIMEOUT"
	Push "/INSIST"

	Push "${FILE}.md"
	Push "/DEBUG"

	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "httpbin.org/post/status/40x"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/status/400,401,402,403,404,405'
	!define /redef FILE '$EXEDIR\_POST_httpbin_40x.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30s
	Push "/TIMEOUT"
	Push "/INSIST"

	Push "${FILE}.md"
	Push "/DEBUG"

	Push "${FILE}"
	Push "${LINK}"
	Push "POST"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "httpbin.org/put/status/40x"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/status/400,401,402,403,404,405'
	!define /redef FILE '$EXEDIR\_PUT_httpbin_40x.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30s
	Push "/TIMEOUT"
	Push "/INSIST"

	Push '{ "number_of_the_beast" : 666 }'
	Push "/DATA"

	Push "Content-Type: application/json"
	Push "/HEADER"

	Push "${FILE}.md"
	Push "/DEBUG"

	Push "${FILE}"
	Push "${LINK}"
	Push "PUT"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

SectionGroupEnd			; Errors


SectionGroup /e "Authentication"

Section "httpbin.org/basic-auth"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/basic-auth/MyUser/MyPass'
	!define /redef FILE '$EXEDIR\_GET_httpbin_basic-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}.md"
	Push "/DEBUG"
	Push "MyPass"
	Push "MyUser"
	Push "/AUTH"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/hidden-basic-auth"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/hidden-basic-auth/MyUser/MyPass'
	!define /redef FILE '$EXEDIR\_GET_httpbin_hidden-basic-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}.md"
	Push "/DEBUG"
	Push "MyPass"
	Push "MyUser"
	Push "type=basic"		; Enforce basic authentication
	Push "/AUTH"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/bearer"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/bearer'
	!define /redef FILE '$EXEDIR\_GET_httpbin_bearer.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}.md"
	Push "/DEBUG"
	Push "MyOauth2Token"
	Push "TYPE=bearer"
	Push "/AUTH"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/digest-auth/auth"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/digest-auth/auth/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_digest-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}.md"
	Push "/DEBUG"
	Push "MyPass"
	Push "MyUser"
	Push "type=digest"
	Push "/AUTH"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/digest-auth/auth-int"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/digest-auth/auth-int/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_digest-auth-int.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}.md"
	Push "/DEBUG"
	Push "MyPass"
	Push "MyUser"
	Push "/AUTH"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

SectionGroupEnd		; Authentication


SectionGroup /e "Proxy"

Section "httpbin.org/get"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/get?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbin_proxy.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}.md"
	Push "/DEBUG"
	Push "http://136.243.47.220:3128"		; Germany
	Push "/PROXY"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/digest-auth/auth-int"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://httpbin.org/digest-auth/auth-int/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_proxy_digest-auth-int.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}.md"
	Push "/DEBUG"
	Push "http://136.243.47.220:3128"		; Germany
	Push "/PROXY"
	Push "MyPass"
	Push "MyUser"
	Push "/AUTH"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

SectionGroupEnd		; Proxy


SectionGroup /e "SSL Validation"

Section "Expired certificate"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://expired.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Revoked certificate"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://revoked.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Self-signed certificate"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://self-signed.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Untrusted certificate"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://untrusted-root.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "Wrong host"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://wrong.host.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

Section "HTTP public key pinning (HPKP)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define /redef LINK 'https://pinning-test.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http
	Pop $0
	DetailPrint "Status: $0"

	!insertmacro STACK_VERIFY_END
SectionEnd

SectionGroupEnd		; SSL Validation


Section "Wait for all"
	SectionIn ${INSTTYPE_NONE} ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	DetailPrint 'Waiting...'
	Push "/END"
	; Push "$mui.InstFilesPage.ProgressBar"
	; Push "/PROGRESSWND"
	; Push "$mui.InstFilesPage.Text"
	; Push "/TEXTWND"
	Push "$HWNDPARENT"
	Push "/TITLEWND"
	Push "/CANCEL"
	CallInstDLL $DLL wait

	!insertmacro STACK_VERIFY_END

	; Print summary
	Call PrintAllRequests

SectionEnd


Function PrintAllRequests
	!insertmacro STACK_VERIFY_START

	; NScurl::enumerate
	Push "/END"
	CallInstDLL $DLL enumerate

_enum_loop:

	StrCpy $0 ""
	Pop $0
	StrCmp $0 "" _enum_end

	DetailPrint '[ID: $0] -----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	Push 'Status: @Status@, @ERROR@, Percent: @PERCENT@%, Size: @XFERSIZE@, Speed: @SPEED@, Time: @TIMEELAPSED@, Tag: "@TAG@"'
	Push $0
	Push "/ID"
	CallInstDLL $DLL query
	Pop $1
	DetailPrint "$1"
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push '@METHOD@ @URL@ -> @OUT@'
	Push $0
	Push "/ID"
	CallInstDLL $DLL query
	Pop $1
	DetailPrint "$1"
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push 'Request Headers: @SENTHEADERS@'
	Push $0
	Push "/ID"
	CallInstDLL $DLL query
	Pop $1
	DetailPrint "$1"
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push 'Reply Headers: @RECVHEADERS@'
	Push $0
	Push "/ID"
	CallInstDLL $DLL query
	Pop $1
	DetailPrint "$1"
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push 'Remote Content: @RECVDATA:0,128@'
	Push $0
	Push "/ID"
	CallInstDLL $DLL query
	Pop $1
	DetailPrint "$1"
	!insertmacro STACK_VERIFY_END

	Goto _enum_loop
_enum_end:

	!insertmacro STACK_VERIFY_END
FunctionEnd


SectionGroup /e Extra


Section Test
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	; NScurl::echo
	Push "/END"
	Push 0x2
	Push 1
	Push bbb
	Push "aaa"
	CallInstDLL $DLL echo
	Pop $0
	DetailPrint 'NScurl::echo(...) = "$0"'

	!insertmacro STACK_VERIFY_END
SectionEnd


Section Hashes
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!insertmacro STACK_VERIFY_START

	!define S1 "Hash this string"

	; NScurl::md5 -file filename
	Push $EXEPATH
	Push "-file"
	CallInstDLL $DLL md5
	Pop $0
	DetailPrint 'NScurl::md5 -file "$EXEFILE" = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::md5 -string string
	!insertmacro STACK_VERIFY_START
	Push "${S1}"
	Push "-string"
	CallInstDLL $DLL md5
	Pop $0
	DetailPrint 'NScurl::md5 -string "${S1}" = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::md5 -memory ptr size
	!insertmacro STACK_VERIFY_START
	Push $R0
	Push $R1

	StrLen $R1 "${S1}"
	System::Call '*(&m128 "${S1}") p.r10'
	; IntFmt $R0 "0x%Ix" $R0    ; not working in nt4

	Push $R1
	Push $R0
	Push "-memory"
	CallInstDLL $DLL md5
	Pop $0
	DetailPrint 'NScurl::md5 -memory ($R0:"${S1}", $R1) = "$0"'

	System::Free $R0

	Pop $R1
	Pop $R0
	!insertmacro STACK_VERIFY_END

	; NScurl::sha1
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	Push "-file"
	CallInstDLL $DLL sha1
	Pop $0
	DetailPrint 'NScurl::sha1 -file "$EXEFILE" = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::sha256
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	Push "-file"
	CallInstDLL $DLL sha256
	Pop $0
	DetailPrint 'NScurl::sha256 -file "$EXEFILE" = "$0"'
	!insertmacro STACK_VERIFY_END

	!undef S1
SectionEnd


Section "Un/Escape"
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	StrCpy $R0 "aaa bbb ccc=ddd&eee"
	DetailPrint "Original: $R0"

	!insertmacro STACK_VERIFY_START
	Push $R0
	CallInstDLL $DLL escape
	Pop $1
	DetailPrint "Escaped: $1"
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push $1
	CallInstDLL $DLL unescape
	Pop $0
	DetailPrint "Unescaped: $0"
	!insertmacro STACK_VERIFY_END

SectionEnd


Section About
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	; NScurl::query
	!insertmacro STACK_VERIFY_START
	Push "NScurl/@PLUGINVERSION@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint '$0'
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push "    @PLUGINAUTHOR@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint '$0'
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push "    @PLUGINWEB@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint '$0'
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push "curl/@CURLVERSION@ @CURLSSLVERSION@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint '$0'
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push "    Protocols: @CURLPROTOCOLS@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint '$0'
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push "    Features: @CURLFEATURES@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint '$0'
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push "Agent: @USERAGENT@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint '$0'
	!insertmacro STACK_VERIFY_END
SectionEnd


SectionGroupEnd		; Extra


Section -Final
    DetailPrint '[ TESTS ] Total: $g_testCount, Failed: $g_testFails'
SectionEnd
