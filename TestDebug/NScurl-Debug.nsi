
# NScurl development script
# Marius Negrutiu - https://github.com/negrutiu/nsis-nscurl#nsis-plugin-nscurl
# Syntax: Test.exe /dll <nscurl.dll>

!ifdef AMD64
	!define _TARGET_ amd64-unicode
!else ifdef ANSI
	!define _TARGET_ x86-ansi
!else
	!define _TARGET_ x86-unicode		; Default
!endif

Target ${_TARGET_}

# /dll commandline parameter
Var /global DLL

!include "MUI2.nsh"
!define LOGICLIB_STRCMP
!include "LogicLib.nsh"
!include "Sections.nsh"

!include "FileFunc.nsh"
!insertmacro GetOptions
!insertmacro GetParameters

!include "StrFunc.nsh"
#${StrRep}				; Declare in advance

!define /ifndef NULL 0


# GUI settings
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install-nsis.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-nsis.bmp"

# Welcome page
;!define MUI_WELCOMEPAGE_TITLE_3LINES
;!insertmacro MUI_PAGE_WELCOME

# Components page
InstType "All"		; 1
InstType "None"		; 2
!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS

# Installation page
!insertmacro MUI_PAGE_INSTFILES

# Language
!insertmacro MUI_LANGUAGE "English"
;!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_RESERVEFILE_LANGDLL

# Installer details
Name    "NScurl-Debug-${_TARGET_}"
OutFile "NScurl-Debug-${_TARGET_}.exe"
XPStyle on
RequestExecutionLevel user		; Don't require UAC elevation
ShowInstDetails show
ManifestDPIAware true

!macro STACK_VERIFY_START
	Push "MyStackTop"			; Mark the top of the stack
!macroend

!macro STACK_VERIFY_END
	Pop $R9						; Validate our stack marker
	StrCmp $R9 "MyStackTop" +2 +1
		MessageBox MB_ICONSTOP "Stack is NOT OK"
!macroend

#---------------------------------------------------------------#
# .onInit                                                       #
#---------------------------------------------------------------#
Function .onInit

	; Initializations
	InitPluginsDir

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
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
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
FunctionEnd


Section "Cleanup test files"
	SectionIn 1	2 ; All
	FindFirst $0 $1 "$EXEDIR\_*.*"
loop:
	StrCmp $1 "" done
	Delete "$EXEDIR\$1"
	FindNext $0 $1
	Goto loop
done:
	FindClose $0
SectionEnd


Section "Parallel (50 * put)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	StrCpy $1 ""
	${For} $R0 1 50
		Push /END
		Push /BACKGROUND
		Push "@$PLUGINSDIR\cacert.pem"
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
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/get?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbin.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push 30000
	Push "/CONNECTTIMEOUT"

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


Section "httpbin.org/get (SysinternalsSuite.zip)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30000
	Push "/TIMEOUT"
	Push "/CANCEL"
	Push "${FILE}"
	Push "${LINK}"
	Push "GET"
	CallInstDLL $DLL http

	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section "httpbin.org/get (SysinternalsSuite.zip : Popup)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Popup.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30000
	Push "/TIMEOUT"
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


Section "httpbin.org/get (SysinternalsSuite.zip : Silent)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Silent.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30000
	Push "/TIMEOUT"
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


Section "httpbin.org/post (multipart/form-data)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_multipart.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push 30000
	Push "/CONNECTTIMEOUT"

	Push "@$PLUGINSDIR\cacert.pem"
	Push "cacert2.pem"
	Push "filename=cacert2.pem"
	Push "/POSTVAR"

	Push "@$PLUGINSDIR\cacert.pem"
	Push "cacert.pem"
	Push "filename=cacert.pem"
	Push "/POSTVAR"

	Push "<Your password here>"
	Push "Password"
	Push "/POSTVAR"

	Push "<Your name here>"
	Push "Name"
	Push "/POSTVAR"

	Push '{ "number_of_the_beast": 666 }'
	Push "maiden.json"
	Push "type=application/json"
	Push "filename=maiden.json"
	Push "/POSTVAR"

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


Section "httpbin.org/post (application/x-www-form-urlencoded)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_postfields.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push 30000
	Push "/CONNECTTIMEOUT"

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
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_json.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push 30000
	Push "/CONNECTTIMEOUT"

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
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/put?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_PUT_httpbin.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push "https://test.com"
	Push "/REFERER"
	Push 30000
	Push "/CONNECTTIMEOUT"

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


Section /o "Big file (100MB)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://speed.hetzner.de/100MB.bin'
	!define /redef FILE '$EXEDIR\_GET_100MB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30000
	Push "/TIMEOUT"

	Push "$HWNDPARENT"
	Push "/TITLEWND"

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


Section /o "Big file (10GB)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://speed.hetzner.de/10GB.bin'
	!define /redef FILE '$EXEDIR\_GET_10GB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30000
	Push "/TIMEOUT"

	Push "$HWNDPARENT"
	Push "/TITLEWND"

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


SectionGroup /e "Authentication"

Section "httpbin.org/basic-auth"
	SectionIn 1	; All
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
	SectionIn 1	; All
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
	SectionIn 1	; All
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
	SectionIn 1	; All
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
	SectionIn 1	; All
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
	SectionIn 1	; All
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
	SectionIn 1	; All
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


Section "Wait for all"
	SectionIn 1	2 ; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	DetailPrint 'Waiting...'
	!insertmacro STACK_VERIFY_START
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

	; NScurl::enumerate
	!insertmacro STACK_VERIFY_START
	Push "/END"
	CallInstDLL $DLL enumerate
	
_enum_loop:

	StrCpy $0 ""
	Pop $0
	StrCmp $0 "" _enum_end

	DetailPrint '[ID: $0] -----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	Push 'Status: @Status@, @ERROR@, Percent: @PERCENT@%, Size: @XFERSIZE@, Speed: @SPEED@, Time: @TIMEELAPSED@'
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
	Push 'Remote Content: @RECVDATA@'
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


Section /o Test
	SectionIn 1
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	; NScurl::echo
	!insertmacro STACK_VERIFY_START
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


Section /o Hashes
	SectionIn 1
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	; NScurl::md5
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	CallInstDLL $DLL md5
	Pop $0
	DetailPrint 'NScurl::md5( $EXEFILE ) = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::sha1
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	CallInstDLL $DLL sha1
	Pop $0
	DetailPrint 'NScurl::sha1( $EXEFILE ) = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::sha256
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	CallInstDLL $DLL sha256
	Pop $0
	DetailPrint 'NScurl::sha256( $EXEFILE ) = "$0"'
	!insertmacro STACK_VERIFY_END

SectionEnd


Section /o "Un/Escape"
	SectionIn 1	; All
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
	SectionIn 1
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	; NScurl::query
	!insertmacro STACK_VERIFY_START
	Push "[Version] @PLUGINVERSION@, [Agent] @USERAGENT@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint 'NScurl::query = "$0"'
	!insertmacro STACK_VERIFY_END

	!insertmacro STACK_VERIFY_START
	Push "[SSL] curl/@CURLVERSION@, mbedtls/@MBEDTLSVERSION@"
	CallInstDLL $DLL query
	Pop $0
	DetailPrint 'NScurl::query = "$0"'
	!insertmacro STACK_VERIFY_END
SectionEnd


SectionGroupEnd		; Extra


/*
Section /o "HTTP GET (Popup mode)"
	SectionIn 1	; All

	DetailPrint '-----------------------------------------------'
	DetailPrint '${__SECTION__}'
	DetailPrint '-----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	; NOTE: github.com doesn't support Range headers
	!define /redef LINK `https://github.com/cuckoobox/cuckoo/archive/master.zip`
	!define /redef FILE "$EXEDIR\_CuckooBox_master.zip"
	DetailPrint 'NScurl::Transfer "${LINK}" "${FILE}"'
	Push "/END"
	Push "Popup"
	Push "/Mode"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	CallInstDLL $DLL Transfer
	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section /o "HTTP GET (Silent mode)"
	SectionIn 1	; All

	DetailPrint '-----------------------------------------------'
	DetailPrint '${__SECTION__}'
	DetailPrint '-----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	!define /redef LINK `https://download.mozilla.org/?product=firefox-stub&os=win&lang=en-US`
	!define /redef FILE "$EXEDIR\_Firefox.exe"
	DetailPrint 'NScurl::Transfer "${LINK}" "${FILE}"'
	Push "/END"
	Push "Silent"
	Push "/MODE"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	CallInstDLL $DLL Transfer
	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section /o "HTTP GET (Parallel transfers)"
	SectionIn 1	; All

	DetailPrint '-----------------------------------------------'
	DetailPrint '${__SECTION__}'
	DetailPrint '-----------------------------------------------'

	; Request 1
	!insertmacro STACK_VERIFY_START
	!define /redef LINK `https://download.mozilla.org/?product=firefox-stub&os=win&lang=en-US`
	!define /redef FILE "$EXEDIR\_Firefox(2).exe"
	DetailPrint 'NScurl::Request "${LINK}" "${FILE}"'
	Push "/END"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	CallInstDLL $DLL Request
	Pop $1
	!insertmacro STACK_VERIFY_END

	; Request 2
	!insertmacro STACK_VERIFY_START
	!define /redef LINK `https://download.mozilla.org/?product=firefox-stub&os=win&lang=en-US`
	!define /redef FILE "$EXEDIR\_Firefox(3).exe"
	DetailPrint 'NScurl::Request "${LINK}" "${FILE}"'
	Push "/END"
	Push 15000
	Push "/TIMEOUTCONNECT"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	CallInstDLL $DLL Request
	Pop $2
	!insertmacro STACK_VERIFY_END

	; Request 3
	!insertmacro STACK_VERIFY_START
	!define /redef LINK `http://download.osmc.tv/installers/osmc-installer.exe`
	!define /redef FILE "$EXEDIR\_osmc_installer.exe"
	DetailPrint 'NScurl::Request "${LINK}" "${FILE}"'
	Push "/END"
	Push 15000
	Push "/TIMEOUTCONNECT"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	CallInstDLL $DLL Request
	Pop $3
	!insertmacro STACK_VERIFY_END

	; Wait for all
	!insertmacro STACK_VERIFY_START
	DetailPrint 'Waiting . . .'
	Push "/END"
	Push "Are you sure?"
	Push "Abort"
	Push "/ABORT"
	Push "Page"
	Push "/Mode"
	CallInstDLL $DLL Wait
	Pop $0
	!insertmacro STACK_VERIFY_END

	; Check for individual transfer status...
	; TODO

	DetailPrint 'Done'
SectionEnd


Section /o "HTTP GET (proxy)"
	SectionIn 1	; All

	DetailPrint '-----------------------------------------------'
	DetailPrint '${__SECTION__}'
	DetailPrint '-----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "https://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_proxy.zip"
	!define /redef PROXY "http=54.36.139.108:8118 https=54.36.139.108:8118"			; France
	DetailPrint 'NScurl::Transfer /proxy ${PROXY} "${LINK}" "${FILE}"'
	Push "/END"
	Push "Are you sure?"
	Push "Abort"
	Push "/ABORT"
	Push 30000
	Push "/TIMEOUTRECONNECT"
	Push 15000
	Push "/TIMEOUTCONNECT"
	Push "${PROXY}"
	Push "/PROXY"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	Push 10
	Push "/PRIORITY"
	CallInstDLL $DLL Transfer
	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section /o "HTTP POST (application/json)"
	SectionIn 1	; All

	DetailPrint '-----------------------------------------------'
	DetailPrint '${__SECTION__}'
	DetailPrint '-----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=1&param2=2'
	!define /redef FILE '$EXEDIR\_Post_json.json'
	DetailPrint 'NScurl::Transfer "${LINK}" "${FILE}"'
	Push "/END"
	Push "https://wikipedia.org"
	Push "/REFERER"
	Push 30000
	Push "/TIMEOUTRECONNECT"
	Push 15000
	Push "/TIMEOUTCONNECT"
	Push "Content-Type: application/json"
	Push "/HEADERS"
	Push '{"number_of_the_beast" : 666}'
	Push "/DATA"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	Push "POST"
	Push "/METHOD"
	CallInstDLL $DLL Transfer
	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


Section /o "HTTP POST (application/x-www-form-urlencoded)"
	SectionIn 1	; All

	DetailPrint '-----------------------------------------------'
	DetailPrint '${__SECTION__}'
	DetailPrint '-----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'http://httpbin.org/post?param1=1&param2=2'
	!define /redef FILE '$EXEDIR\_Post_form.json'
	DetailPrint 'NScurl::Transfer "${LINK}" "${FILE}"'
	Push "/END"
	Push "https://wikipedia.org"
	Push "/REFERER"
	Push 30000
	Push "/TIMEOUTRECONNECT"
	Push 15000
	Push "/TIMEOUTCONNECT"
	Push "Content-Type: application/x-www-form-urlencoded$\r$\nContent-Dummy: Dummy"
	Push "/HEADERS"
	Push 'User=My+User&Pass=My+Pass'
	Push "/DATA"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	Push "POST"
	Push "/METHOD"
	CallInstDLL $DLL Transfer
	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


!macro TEST_DEPENDENCY_REQUEST _Filename _DependsOn
	!define /redef LINK `http://httpbin.org/post`
	DetailPrint 'NScurl::Request "${LINK}" "${_Filename}.txt"'
	Push "/END"
	Push "https://wikipedia.org"
	Push "/REFERER"
	Push 30000
	Push "/TIMEOUTRECONNECT"
	Push 15000
	Push "/TIMEOUTCONNECT"
	Push "Content-Type: application/x-www-form-urlencoded$\r$\nContent-Test: TEST"
	Push "/HEADERS"
	Push "User=My+User+Name&Pass=My+Password"
	Push "/DATA"
	Push "$EXEDIR\${_Filename}.txt"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	Push "POST"
	Push "/METHOD"
	Push ${_DependsOn}
	Push "/DEPEND"
	Push 2000
	Push "/PRIORITY"
	CallInstDLL $DLL Request
	Pop $0	; Request ID
!macroend


Section /o "Test Dependencies (depend on first request)"
	;SectionIn 1	; All
	!insertmacro STACK_VERIFY_START

	StrCpy $R0 0	; First request ID
	StrCpy $R1 0	; Last request ID

	; First request
	!insertmacro TEST_DEPENDENCY_REQUEST "_DependOnFirst1" -1
	StrCpy $R0 $0	; Remember the first ID
	StrCpy $R1 $0	; Remember the last request ID

	; Subsequent requests
	${For} $1 2 20
		!insertmacro TEST_DEPENDENCY_REQUEST "_DependOnFirst$1" $R0
		StrCpy $R1 $0	; Remember the last request ID
	${Next}

	; Sleep
	;Sleep 2000

	; Unlock the first request, and consequently all the others...
	Push "/END"
	Push 0			; No dependency
	Push "/SETDEPEND"
	Push $R0		; First request ID
	Push "/ID"
	CallInstDLL $DLL "Set"
	Pop $0	; Error code. Ignored

	; Wait
	DetailPrint 'Waiting . . .'
	Push "/END"
	Push "Are you sure?"
	Push "Abort"
	Push "/ABORT"
	Push "Page"
	Push "/Mode"
	CallInstDLL $DLL Wait
	Pop $0

	!insertmacro STACK_VERIFY_END
SectionEnd


Section /o "Test Dependencies (depend on previous request)"
	;SectionIn 1	; All
	!insertmacro STACK_VERIFY_START

	StrCpy $R0 0	; First request ID
	StrCpy $R1 0	; Last request ID

	; First request
	!insertmacro TEST_DEPENDENCY_REQUEST "_DependOnPrevious1" -1
	StrCpy $R0 $0	; Remember the first ID
	StrCpy $R1 $0	; Remember the last request ID

	; Subsequent requests
	${For} $1 2 20
		!insertmacro TEST_DEPENDENCY_REQUEST "_DependOnPrevious$1" $R1
		StrCpy $R1 $0	; Remember the last request ID
	${Next}

	; Sleep
	;Sleep 2000

	; Unlock the first request, and consequently all the others...
	Push "/END"
	Push 0			; No dependency
	Push "/SETDEPEND"
	Push $R0		; First request ID
	Push "/ID"
	CallInstDLL $DLL "Set"
	Pop $0	; Error code. Ignored

	; Wait
	DetailPrint 'Waiting . . .'

	Push "/END"
	Push "Are you sure?"
	Push "Abort"
	Push "/ABORT"
	;Push $HWNDPARENT
	;Push "/TITLEHWND"
	Push "Page"
	Push "/MODE"
	CallInstDLL $DLL Wait
	Pop $0

	!insertmacro STACK_VERIFY_END
SectionEnd


Function PrintSummary

	!insertmacro STACK_VERIFY_START
	Push $0
	Push $1
	Push $2
	Push $3
	Push $R0
	Push $R1
	Push $R2
	Push $R3
	Push $R4

	; Enumerate all transfers (completed + pending + waiting)
	DetailPrint "NScurl::Enumerate"
	Push "/END"
	CallInstDLL $DLL Enumerate
	Pop $1	; Count
	DetailPrint "    $1 requests"
	${For} $0 1 $1

		Pop $2	; Request ID

		Push "/END"
		Push "/CONTENT"
		Push "/CONNECTIONDROPS"
		Push "/ERRORTEXT"
		Push "/ERRORCODE"
		Push "/TIMEDOWNLOADING"
		Push "/TIMEWAITING"
		Push "/SPEED"
		Push "/SPEEDBYTES"
		Push "/PERCENT"
		Push "/FILESIZE"
		Push "/RECVSIZE"
		Push "/RECVHEADERS"
		Push "/SENTHEADERS"
		Push "/DATA"
		Push "/LOCAL"
		Push "/IP"
		Push "/PROXY"
		Push "/URL"
		Push "/METHOD"
		Push "/WININETSTATUS"
		Push "/STATUS"
		Push "/DEPEND"
		Push "/PRIORITY"
		Push $2	; Request ID
		Push "/ID"
		CallInstDLL $DLL Query

		StrCpy $R0 "[>] ID:$2"
		Pop $3 ;PRIORITY
		StrCpy $R0 "$R0, Prio:$3"
		Pop $3 ;DEPEND
		IntCmp $3 0 +2 +1 +1
			StrCpy $R0 "$R0, DependsOn:$3"
		Pop $3 ;STATUS
		StrCpy $R0 "$R0, [$3]"
		Pop $3 ;WININETSTATUS
		StrCpy $R0 "$R0, WinINet:$3"
		DetailPrint $R0

		StrCpy $R0 "  [Request]"
		Pop $3 ;METHOD
		StrCpy $R0 "$R0 $3"
		Pop $3 ;URL
		StrCpy $R0 "$R0 $3"
		DetailPrint $R0

		Pop $3 ;PROXY
		StrCmp $3 "" +2 +1
			DetailPrint "  [Proxy] $3"
		Pop $3 ;IP
		StrCmp $3 "" +2 +1
			DetailPrint "  [Server] $3"

		Pop $3 ;LOCAL
		DetailPrint "  [Local] $3"

		Pop $3 ;DATA
		${If} $3 != ""
			${StrRep} $3 "$3" "$\r" "\r"
			${StrRep} $3 "$3" "$\n" "\n"
			DetailPrint "  [Sent Data] $3"
		${EndIf}
		Pop $3 ;SENTHEADERS
		${If} $3 != ""
			${StrRep} $3 "$3" "$\r" "\r"
			${StrRep} $3 "$3" "$\n" "\n"
			DetailPrint "  [Sent Headers] $3"
		${EndIf}
		Pop $3 ;RECVHEADERS
		${If} $3 != ""
			${StrRep} $3 "$3" "$\r" "\r"
			${StrRep} $3 "$3" "$\n" "\n"
			DetailPrint "  [Recv Headers] $3"
		${EndIf}

		StrCpy $R0 "  [Size]"
		Pop $3 ;RECVSIZE
		StrCpy $R0 "$R0 $3"
		Pop $3 ;FILESIZE
		StrCpy $R0 "$R0/$3"
		Pop $3 ;PERCENT
		StrCpy $R0 "$R0 ($3%)"
		Pop $3 ;SPEEDBYTES
		Pop $3 ;SPEED
		StrCmp $3 "" +2 +1
			StrCpy $R0 "$R0 @ $3"
		DetailPrint "$R0"

		StrCpy $R0 "  [Time]"
		Pop $3 ;TIMEWAITING
		StrCpy $R0 "$R0 Waiting $3ms"
		Pop $3 ;TIMEDOWNLOADING
		StrCpy $R0 "$R0, Downloading $3ms"
		DetailPrint "$R0"

		StrCpy $R0 "  [Error]"
		Pop $3 ;ERRORCODE
		StrCpy $R0 "$R0 $3"
		Pop $3 ;ERRORTEXT
		StrCpy $R0 "$R0, $3"
		Pop $3 ;CONNECTIONDROPS
		IntCmp $3 0 +2 +1 +1
			StrCpy $R0 "$R0, Drops:$3"
		DetailPrint "$R0"

		Pop $3 ;CONTENT
		${If} $3 != ""
			${StrRep} $3 "$3" "$\r" "\r"
			${StrRep} $3 "$3" "$\n" "\n"
			DetailPrint "  [Content] $3"
		${EndIf}
	${Next}

	Push "/END"
	Push "/USERAGENT"
	Push "/PLUGINVERSION"
	Push "/PLUGINNAME"
	Push "/TOTALTHREADS"
	Push "/TOTALSPEED"
	Push "/TOTALDOWNLOADING"
	Push "/TOTALCOMPLETED"
	Push "/TOTALCOUNT"
	CallInstDLL $DLL QueryGlobal
	Pop $R0 ; Total
	Pop $R1 ; Completed
	Pop $R2 ; Downloading
	Pop $R3 ; Speed
	Pop $R4 ; Worker threads
	Pop $1	; Plugin Name
	Pop $2	; Plugin Version
	Pop $3	; Useragent

	DetailPrint "Transferring $R1+$R2/$R0 items at $R3 using $R4 worker threads"
	DetailPrint "[>] $1 $2"
	DetailPrint "[>] User agent: $3"

	Pop $R4
	Pop $R3
	Pop $R2
	Pop $R1
	Pop $R0
	Pop $3
	Pop $2
	Pop $1
	Pop $0
	!insertmacro STACK_VERIFY_END

FunctionEnd

Section -Summary
	DetailPrint '-----------------------------------------------'
	DetailPrint ' ${__SECTION__}'
	DetailPrint '-----------------------------------------------'
	Call PrintSummary
SectionEnd
*/
