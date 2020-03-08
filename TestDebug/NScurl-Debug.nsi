
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

!define /ifndef NULL 0


# GUI settings
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\orange-install-nsis.ico"
!define MUI_WELCOMEFINISHPAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Wizard\orange-nsis.bmp"

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
		Push "$PLUGINSDIR\cacert.pem"
		Push "(file)"
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


Section "httpbin.org/get (SysinternalsSuite.zip)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
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


Section "httpbin.org/get (SysinternalsSuite.zip : Popup)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Popup.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
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


Section "httpbin.org/get (SysinternalsSuite.zip : Silent)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Silent.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
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
	Push "(memory)"
	Push "Binary"
	Push "type=application/octet-stream"
	Push "/POST"

	Push "$PLUGINSDIR\cacert.pem"
	Push "(file)"
	Push "cacert2.pem"
	Push "filename=cacert2.pem"
	Push "/POST"

	Push "$PLUGINSDIR\cacert.pem"
	Push "(file)"
	Push "cacert.pem"
	Push "filename=cacert.pem"
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
	Push 30000
	Push "/TIMEOUT"
	Push "/INSIST"

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


Section "Big file (10GB)"
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://speed.hetzner.de/10GB.bin'
	!define /redef FILE '$EXEDIR\_GET_10GB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30000
	Push "/TIMEOUT"
	Push "/INSIST"

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


SectionGroup /e "Errors"

Section "httpbin.org/get/status/40x"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/status/400,401,402,403,404,405'
	!define /redef FILE '$EXEDIR\_GET_httpbin_40x.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	Push "/END"
	Push 30000
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
	Push 30000
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
	Push 30000
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


Section Test
	;SectionIn ${INSTTYPE_CUSTOM}
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


Section Hashes
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!define S1 "Hash this string"

	; NScurl::md5 (file) filename
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	Push "(file)"
	CallInstDLL $DLL md5
	Pop $0
	DetailPrint 'NScurl::md5 (file) "$EXEFILE" = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::md5 (strint) string
	!insertmacro STACK_VERIFY_START
	Push "${S1}"
	Push "(string)"
	CallInstDLL $DLL md5
	Pop $0
	DetailPrint 'NScurl::md5 (string) "${S1}" = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::md5 (memory) ptr size
	!insertmacro STACK_VERIFY_START
	Push $R0
	Push $R1

	StrLen $R1 "${S1}"
	System::Call '*(&m128 "${S1}") p.r10'
	IntFmt $R0 "0x%Ix" $R0

	Push $R1
	Push $R0
	Push "(memory)"
	CallInstDLL $DLL md5
	Pop $0
	DetailPrint 'NScurl::md5 (memory) ($R0:"${S1}", $R1) = "$0"'

	System::Free $R0

	Pop $R1
	Pop $R0
	!insertmacro STACK_VERIFY_END

	; NScurl::sha1
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	Push "(file)"
	CallInstDLL $DLL sha1
	Pop $0
	DetailPrint 'NScurl::sha1 (file) "$EXEFILE" = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::sha256
	!insertmacro STACK_VERIFY_START
	Push $EXEPATH
	Push "(file)"
	CallInstDLL $DLL sha256
	Pop $0
	DetailPrint 'NScurl::sha256 (file) "$EXEFILE" = "$0"'
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
