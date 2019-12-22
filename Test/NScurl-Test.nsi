
# NScurl demo
# Marius Negrutiu - https://github.com/negrutiu/nsis-nscurl#nsis-plugin-nscurl

!ifdef AMD64
	!define _TARGET_ amd64-unicode
!else ifdef ANSI
	!define _TARGET_ x86-ansi
!else
	!define _TARGET_ x86-unicode		; Default
!endif

Target ${_TARGET_}

!include "MUI2.nsh"
!define LOGICLIB_STRCMP
!include "LogicLib.nsh"
!include "Sections.nsh"

!define /ifndef NULL 0


# NScurl.dll location
!AddPluginDir /amd64-unicode "..\Release-mingw-amd64-unicode"
!AddPluginDir /x86-unicode   "..\Release-mingw-x86-unicode"
!AddPluginDir /x86-ansi      "..\Release-mingw-x86-ansi"

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
Name    "NScurl-Test-${_TARGET_}"
OutFile "NScurl-Test-${_TARGET_}.exe"
XPStyle on
RequestExecutionLevel user		; Don't require UAC elevation
ShowInstDetails show
ManifestDPIAware true

#---------------------------------------------------------------#
# .onInit                                                       #
#---------------------------------------------------------------#
Function .onInit

	; Initializations
	InitPluginsDir

	; Language selection
	!define MUI_LANGDLL_ALLLANGUAGES
	!insertmacro MUI_LANGDLL_DISPLAY

/*
	; .onInit download demo
	; NOTE: Transfers from .onInit can be either Silent or Popup (no Page!)
	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_onInit.zip"
	NScurl::http GET "${LINK}" "${FILE}" /POPUP /CANCEL /END
	Pop $0
*/
FunctionEnd


Section "Cleanup test files"
	SectionIn 1	2 ; All
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


Section "Parallel (50 * put)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	StrCpy $1 ""
	${For} $R0 1 50
		NScurl::http PUT "https://httpbin.org/put" "Memory" /DATA "@$PLUGINSDIR\cacert.pem" /BACKGROUND /END
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

	!define /redef LINK 'https://httpbin.org/get?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbin.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http get "${LINK}" "${FILE}" /HEADER "Header1: Value1$\r$\nHeader2: Value2" /HEADER "Header3: Value3" /CONNECTTIMEOUT 30000 /REFERER "https://test.com" /END
	Pop $0

	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/get (SysinternalsSuite.zip)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /CANCEL /TIMEOUT 30000 /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/get (SysinternalsSuite.zip : Popup)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Popup.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /CANCEL /POPUP /TIMEOUT 30000 /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/get (SysinternalsSuite.zip : Silent)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "http://live.sysinternals.com/Files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Silent.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /CANCEL /SILENT /TIMEOUT 30000 /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/post (multipart/form-data)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_multipart.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http \
		POST \
		"${LINK}" \
		"${FILE}" \
		/HEADER "Header1: Value1$\r$\nHeader2: Value2" \
		/HEADER "Header3: Value3" \
		/POSTVAR "filename=maiden.json" "type=application/json" "maiden.json" '{ "number_of_the_beast" : 666 }' \
		/POSTVAR "Name" "<Your name here>" \
		/POSTVAR "Password" "<Your password here>" \
		/POSTVAR "filename=cacert.pem" "cacert.pem" "@$PLUGINSDIR\cacert.pem" \
		/POSTVAR "filename=cacert2.pem" "cacert2.pem" "@$PLUGINSDIR\cacert.pem" \
		/CONNECTTIMEOUT 30000 \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/post (application/x-www-form-urlencoded)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_postfields.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http \
		POST \
		"${LINK}" \
		"${FILE}" \
		/HEADER "Header1: Value1$\r$\nHeader2: Value2" \
		/HEADER "Header3: Value3" \
		/DATA 'User=Your+name+here&Password=Your+password+here' \
		/CONNECTTIMEOUT 30000 \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/post (application/json)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_json.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http \
		POST \
		"${LINK}" \
		"${FILE}" \
		/HEADER "Content-Type: application/json" \
		/DATA '{ "number_of_the_beast" : 666 }' \
		/CONNECTTIMEOUT 30000 \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/put"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/put?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_PUT_httpbin.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http \
		PUT \
		"${LINK}" \
		"${FILE}" \
		/HEADER "Header1: Value1$\r$\nHeader2: Value2" \
		/HEADER "Header3: Value3" \
		/HEADER "Content-Type: application/json" \
		/DATA '{ "number_of_the_beast" : 666 }' \
		/CONNECTTIMEOUT 30000 \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section /o "Big file (100MB)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://speed.hetzner.de/100MB.bin'
	!define /redef FILE '$EXEDIR\_GET_100MB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http GET "${LINK}" "${FILE}" /CANCEL /RESUME /TITLEWND $HWNDPARENT /TIMEOUT 30000 /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section /o "Big file (10GB)"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://speed.hetzner.de/10GB.bin'
	!define /redef FILE '$EXEDIR\_GET_10GB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http GET "${LINK}" "${FILE}" /CANCEL /RESUME /TITLEWND $HWNDPARENT /TIMEOUT 30000 /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


SectionGroup /e "Authentication"

Section "httpbin.org/basic-auth"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/basic-auth/MyUser/MyPass'
	!define /redef FILE '$EXEDIR\_GET_httpbin_basic-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "MyUser" "MyPass" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/hidden-basic-auth"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/hidden-basic-auth/MyUser/MyPass'
	!define /redef FILE '$EXEDIR\_GET_httpbin_hidden-basic-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "type=basic" "MyUser" "MyPass" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/bearer"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/bearer'
	!define /redef FILE '$EXEDIR\_GET_httpbin_bearer.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "type=bearer" "MyOauth2Token" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/digest-auth/auth"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/digest-auth/auth/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_digest-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "type=digest" "MyUser" "MyPass" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/digest-auth/auth-int"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/digest-auth/auth-int/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_digest-auth-int.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "MyUser" "MyPass" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

SectionGroupEnd		; Authentication


SectionGroup /e "Proxy"

Section "httpbin.org/get"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/get?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbin_proxy.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /PROXY "http://136.243.47.220:3128" "/DEBUG" "${FILE}.md" /END		; Germany
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/digest-auth/auth-int"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/digest-auth/auth-int/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_proxy_digest-auth-int.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "MyUser" "MyPass" /PROXY "http://136.243.47.220:3128" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

SectionGroupEnd		; Proxy


Section "Wait for all"
	SectionIn 1	2 ; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	DetailPrint 'Waiting...'
	NScurl::wait /CANCEL /TITLEWND $HWNDPARENT /end

	; Print summary
	Call PrintAllRequests

SectionEnd


Function PrintAllRequests

	; NScurl::enumerate
	NScurl::enumerate /END
	
_enum_loop:

	StrCpy $0 ""
	Pop $0
	StrCmp $0 "" _enum_end

	DetailPrint '[ID: $0] -----------------------------------------------'

	NScurl::query /ID $0 'Status: @Status@, @ERROR@, Percent: @PERCENT@%, Size: @XFERSIZE@, Speed: @SPEED@, Time: @TIMEELAPSED@'
	Pop $1
	DetailPrint "$1"

	NScurl::query /ID $0 '@METHOD@ @URL@ -> @OUT@'
	Pop $1
	DetailPrint "$1"

	NScurl::query /ID $0 'Request Headers: @SENTHEADERS@'
	Pop $1
	DetailPrint "$1"

	NScurl::query /ID $0 'Reply Headers: @RECVHEADERS@'
	Pop $1
	DetailPrint "$1"

	NScurl::query /ID $0 'Remote Content: @RECVDATA@'
	Pop $1
	DetailPrint "$1"

	Goto _enum_loop
_enum_end:

FunctionEnd


SectionGroup /e Extra


Section /o Test
	SectionIn 1
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	; NScurl::echo
	NScurl::echo "aaa" bbb 1 0x2 /END
	Pop $0
	DetailPrint 'NScurl::echo(...) = "$0"'

SectionEnd


Section /o Hashes
	SectionIn 1
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	; NScurl::md5
	NScurl::md5 $EXEPATH
	Pop $0
	DetailPrint 'NScurl::md5( $EXEFILE ) = "$0"'

	; NScurl::sha1
	NScurl::sha1 $EXEPATH
	Pop $0
	DetailPrint 'NScurl::sha1( $EXEFILE ) = "$0"'

	; NScurl::sha256
	NScurl::sha256 $EXEPATH
	Pop $0
	DetailPrint 'NScurl::sha256( $EXEFILE ) = "$0"'

SectionEnd


Section /o "Un/Escape"
	SectionIn 1	; All
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	StrCpy $R0 "aaa bbb ccc=ddd&eee"
	DetailPrint "Original: $R0"

	NScurl::escape $R0
	Pop $1
	DetailPrint "Escaped: $1"

	NScurl::unescape $1
	Pop $0
	DetailPrint "Unescaped: $0"

SectionEnd


Section About
	SectionIn 1
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	NScurl::query "NScurl/@PLUGINVERSION@"
	Pop $0
	DetailPrint '$0'

	NScurl::query "    @PLUGINAUTHOR@"
	Pop $0
	DetailPrint '$0'

	NScurl::query "    @PLUGINWEB@"
	Pop $0
	DetailPrint '$0'

	NScurl::query "curl/@CURLVERSION@ @CURLSSLVERSION@"
	Pop $0
	DetailPrint '$0'

	NScurl::query "    Protocols: @CURLPROTOCOLS@"
	Pop $0
	DetailPrint '$0'

	NScurl::query "    Features: @CURLFEATURES@"
	Pop $0
	DetailPrint '$0'

	NScurl::query "Agent: @USERAGENT@"
	Pop $0
	DetailPrint '$0'
SectionEnd


SectionGroupEnd		; Extra
