
# NScurl demo
# Marius Negrutiu - https://github.com/negrutiu/nsis-nscurl

!ifdef AMD64
	!define _TARGET_ amd64-unicode
!else ifdef ANSI
	!define _TARGET_ x86-ansi
!else
	!define _TARGET_ x86-unicode        ; Default
!endif

Target ${_TARGET_}

!if /fileexists "${NSISDIR}\Include\ModernXXL.nsh"
	!include "ModernXXL.nsh"		    ; Available in the NSIS fork from https://github.com/negrutiu/nsis
!endif
!include "MUI2.nsh"
!define LOGICLIB_STRCMP
!include "LogicLib.nsh"
!include "Sections.nsh"

!define /ifndef NULL 0
!define TEST_FILE "$SYSDIR\lz32.dll"	; ...random file that exists in every Windows build

# NScurl.dll custom location
!ifdef PLUGIN_DIR
!if ! /FileExists "${PLUGIN_DIR}\NScurl.dll"
	!error "Missing ${PLUGIN_DIR}\NScurl.dll"
!endif
!AddPluginDir "${PLUGIN_DIR}"
!endif

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
Name    "NScurl-Test-${_TARGET_}"
OutFile "NScurl-Test-${_TARGET_}.exe"
XPStyle on
RequestExecutionLevel user		        ; Don't require UAC elevation
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
	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_onInit.zip"
	NScurl::http GET "${LINK}" "${FILE}" /POPUP /CANCEL /END
	Pop $0
*/

    ; Quick .onInit plugin test
    NScurl::query "@PLUGINWEB@" /END
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
		NScurl::http PUT "https://httpbin.org/put" "Memory" /DATA -file "${TEST_FILE}" /BACKGROUND /INSIST /TAG "parallels" /END
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

	!define /redef LINK 'https://httpbin.org/get?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbin.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http get "${LINK}" "${FILE}" /HEADER "Header1: Value1$\r$\nHeader2: Value2" /HEADER "Header3: Value3" /REFERER "https://test.com" /END
	Pop $0

	DetailPrint "Status: $0"

SectionEnd


Section "sysinternals.com/get (Page-Mode)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /CANCEL /INSIST /Zone.Identifier /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "sysinternals.com/get (Popup-Mode)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Popup.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /CANCEL /POPUP /INSIST /Zone.Identifier /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "sysinternals.com/get (Silent-Mode)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_Silent.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /CANCEL /SILENT /INSIST /Zone.Identifier /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "sysinternals.com/get (HTTP/1.1)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuite_http1.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /HTTP1.1 /INSIST /CANCEL /Zone.Identifier /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "sysinternals.com/get (Memory)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuite_memory.zip"

	DetailPrint 'NScurl::http "${LINK}" "Memory"'
	NScurl::http get "${LINK}" "Memory" /CANCEL /INSIST /RETURN "@id@" /END
	Pop $R0
	DetailPrint "  ID: $R0"

    ; For demonstration purposes, we'll retrieve the first two bytes of the remote content stored in memory
    ; If the data begins with the "PK" sequence (the standard zip file magic bytes), we'll save it to disk as a .zip file
    	DetailPrint 'NScurl::query /id $R0 "@RecvData:0,2@"'
	NScurl::query /id $R0 "@RecvData:0,2@" /END
	Pop $0
	DetailPrint '  RecvData[0,2]: "$0"'

    ${If} $0 == "PK"
        DetailPrint 'NScurl::query /id $R0 "@RecvData>${FILE}@"'
        NScurl::query /id $R0 "@RecvData>${FILE}@" /END
        Pop $0      ; @RecvData@ trimmed down to ${NSIS_MAX_STRLEN}
        DetailPrint '  RecvData: $0'
    ${EndIf}

SectionEnd


Section "sysinternals.com/get (SpeedCap: 300KB/s)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "https://download.sysinternals.com/files/SysinternalsSuite.zip"
	!define /redef FILE  "$EXEDIR\_SysinternalsSuiteLive_SpeedCap.zip"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http get "${LINK}" "${FILE}" /RESUME /CANCEL /INSIST /SPEEDCAP 307200 /Zone.Identifier /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "github.com/get (Encoding)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK  "https://raw.githubusercontent.com/negrutiu/nsis-nscurl/master/curl.c"
	!define /redef FILE  "$EXEDIR\_curl.c"
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	; note: /Accept-Encoding is incompatible with /RESUME or MEMORY transfers
	; note: Notice that content-lenght indicates the size of the gzip-compressed data (15kb) and not the actual data size (60kb)
	NScurl::http get "${LINK}" "${FILE}" /Accept-Encoding /CANCEL /INSIST /Zone.Identifier /RETURN "@id@" /END
	Pop $R0
	DetailPrint "ID: $R0"

    NScurl::query /id $R0 "@RecvHeaders:content-encoding@"
    Pop $0
    DetailPrint "Reply Headers[content-encoding]: $0"

SectionEnd


Section "httpbin.org/post (multipart/form-data)"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/post?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_POST_httpbin_multipart.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	!define S1 "<Your memory data here>"
	StrLen $R1 "${S1}"
	System::Call '*(&m128 "${S1}") p.r10'

	NScurl::http \
		POST \
		"${LINK}" \
		"${FILE}" \
		/HEADER "Header1: Value1$\r$\nHeader2: Value2" \
		/HEADER "Header3: Value3" \
		/POST "filename=maiden.json" "type=application/json" "maiden.json" '{ "number_of_the_beast" : 666 }' \
		/POST "Name" "<Your name here>" \
		/POST "Password" "<Your password here>" \
		/POST "filename=test.bin" "test.bin" -file "${TEST_FILE}" \
		/POST "filename=test2.bin" "test2.bin" -file "${TEST_FILE}" \
		/POST "type=application/octet-stream" "Binary" -memory $R0 $R1 \
		/INSIST \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

	System::Free $R0
	!undef S1

SectionEnd


Section "httpbin.org/post (application/x-www-form-urlencoded)"
	SectionIn ${INSTTYPE_MOST}
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
		/INSIST \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/post (application/json)"
	SectionIn ${INSTTYPE_MOST}
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
		/INSIST \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "httpbin.org/put"
	SectionIn ${INSTTYPE_MOST}
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
		/INSIST \
		/REFERER "https://test.com" \
		/END

	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "Big file (100MB)"
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://speed.hetzner.de/100MB.bin'
	!define /redef FILE '$EXEDIR\_GET_100MB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http GET "${LINK}" "${FILE}" /STRING TEXT "[@PERCENT@%] @TIMEELAPSED@ / @TIMEREMAINING@, @XFERSIZE@ / @FILESIZE@, Average @AVGSPEED@, Speed @SPEED@" /INSIST /CANCEL /RESUME /TIMEOUT 1m /USERAGENT "curl/@CURLVERSION@" /TITLEWND $HWNDPARENT /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


Section "Big file (10GB)"
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://speed.hetzner.de/10GB.bin'
	!define /redef FILE '$EXEDIR\_GET_10GB.bin'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http GET "${LINK}" "${FILE}" /STRING TEXT "[@PERCENT@%] @TIMEELAPSED@ / @TIMEREMAINING@, @XFERSIZE@ / @FILESIZE@, Average @AVGSPEED@, Speed @SPEED@" /INSIST /RESUME /CANCEL /TIMEOUT 1m /USERAGENT "curl/@CURLVERSION@" /TITLEWND $HWNDPARENT /END
	Pop $0
	DetailPrint "Status: $0"

SectionEnd


SectionGroup /e "Errors"

Section "httpbin.org/get/status/40x"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/status/400,401,402,403,404,405'
	!define /redef FILE '$EXEDIR\_GET_httpbin_40x.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http GET "${LINK}" "${FILE}" /DEBUG "${FILE}.md" /INSIST /TIMEOUT 30s /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

Section "httpbin.org/post/status/40x"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/status/400,401,402,403,404,405'
	!define /redef FILE '$EXEDIR\_POST_httpbin_40x.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http POST "${LINK}" "${FILE}" /DEBUG "${FILE}.md" /INSIST /TIMEOUT 30s /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

Section "httpbin.org/put/status/40x"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/status/400,401,402,403,404,405'
	!define /redef FILE '$EXEDIR\_PUT_httpbin_40x.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'

	NScurl::http PUT "${LINK}" "${FILE}" /DEBUG "${FILE}.md" /HEADER "Content-Type: application/json" /DATA '{ "number_of_the_beast" : 666 }' /INSIST /TIMEOUT 30s /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

SectionGroupEnd			; Errors


SectionGroup /e "Authentication"

Section "httpbin.org/basic-auth"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/basic-auth/MyUser/MyPass'
	!define /redef FILE '$EXEDIR\_GET_httpbin_basic-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "MyUser" "MyPass" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/hidden-basic-auth"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/hidden-basic-auth/MyUser/MyPass'
	!define /redef FILE '$EXEDIR\_GET_httpbin_hidden-basic-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "type=basic" "MyUser" "MyPass" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/bearer"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/bearer'
	!define /redef FILE '$EXEDIR\_GET_httpbin_bearer.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "type=bearer" "MyOauth2Token" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/digest-auth/auth"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/digest-auth/auth/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_digest-auth.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "type=digest" "MyUser" "MyPass" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/digest-auth/auth-int"
	SectionIn ${INSTTYPE_MOST}
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
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/get?param1=value1&param2=value2'
	!define /redef FILE '$EXEDIR\_GET_httpbin_proxy.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /PROXY "http://136.243.47.220:3128" "/DEBUG" "${FILE}.md" /END		; Germany
	Pop $0
	DetailPrint "Status: $0"
SectionEnd


Section "httpbin.org/digest-auth/auth-int"
	SectionIn ${INSTTYPE_MOST}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://httpbin.org/digest-auth/auth-int/MyUser/MyPass/SHA-256'
	!define /redef FILE '$EXEDIR\_GET_httpbin_proxy_digest-auth-int.json'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /AUTH "MyUser" "MyPass" /PROXY "http://136.243.47.220:3128" "/DEBUG" "${FILE}.md" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

SectionGroupEnd		; Proxy


SectionGroup /e "SSL Validation"

Section "Expired certificate"
	SectionIn ${INSTTYPE_MOST}
		
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://expired.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

Section "Revoked certificate"
	SectionIn ${INSTTYPE_MOST}
		
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://revoked.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

Section "Self-signed certificate"
	SectionIn ${INSTTYPE_MOST}
		
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://self-signed.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

Section "Untrusted certificate"
	SectionIn ${INSTTYPE_MOST}
		
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://untrusted-root.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

Section "Wrong host"
	SectionIn ${INSTTYPE_MOST}
		
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://wrong.host.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

Section "HTTP public key pinning (HPKP)"
	SectionIn ${INSTTYPE_MOST}
		
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	!define /redef LINK 'https://pinning-test.badssl.com/'
	!define /redef FILE 'Memory'
	DetailPrint 'NScurl::http "${LINK}" "${FILE}"'
	NScurl::http GET "${LINK}" "${FILE}" /END
	Pop $0
	DetailPrint "Status: $0"
SectionEnd

SectionGroupEnd		; SSL Validation


Section "Wait for all"
	SectionIn ${INSTTYPE_NONE} ${INSTTYPE_MOST}
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

	NScurl::query /ID $0 'Status: @Status@, @ERROR@, Percent: @PERCENT@%, Size: @XFERSIZE@, Speed: @SPEED@, Time: @TIMEELAPSED@, Tag: "@TAG@"'
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

	NScurl::query /ID $0 'Remote Content: @RECVDATA:0,128@'
	Pop $1
	DetailPrint "$1"

	Goto _enum_loop
_enum_end:

FunctionEnd


SectionGroup /e Extra


Section Test
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='

	; NScurl::echo
	NScurl::echo "aaa" bbb 1 0x2 /END
	Pop $0
	DetailPrint 'NScurl::echo(...) = "$0"'

SectionEnd


Section Hashes
	;SectionIn ${INSTTYPE_CUSTOM}
	DetailPrint '=====[ ${__SECTION__} ]==============================='
	!define S1 "Hash this string"

	; NScurl::md5 -file filename
	NScurl::md5 -file $EXEPATH
	Pop $0
	DetailPrint 'NScurl::md5 -file "$EXEFILE" = "$0"'

	; NScurl::md5 -string string
	NScurl::md5 -string "${S1}"
	Pop $0
	DetailPrint 'NScurl::md5 -string "${S1}" = "$0"'

	; NScurl::md5 -memory ptr size
	StrLen $R1 "${S1}"
	System::Call '*(&m128 "${S1}") p.r10'
	; IntFmt $R0 "0x%Ix" $R0    ; not working in nt4

	NScurl::md5 -memory $R0 $R1
	Pop $0
	DetailPrint 'NScurl::md5 -memory ($R0:"${S1}", $R1) = "$0"'

	System::Free $R0

	; NScurl::sha1
	NScurl::sha1 -file $EXEPATH
	Pop $0
	DetailPrint 'NScurl::sha1 -file "$EXEFILE" = "$0"'

	; NScurl::sha256
	NScurl::sha256 -file $EXEPATH
	Pop $0
	DetailPrint 'NScurl::sha256 -file "$EXEFILE" = "$0"'

	!undef S1
SectionEnd


Section "Un/Escape"
	;SectionIn ${INSTTYPE_CUSTOM}
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
	SectionIn ${INSTTYPE_MOST}
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
