
!ifdef AMD64
	Target amd64-unicode
!else ifdef ANSI
	Target x86-ansi
!else
	Target x86-unicode	; Default
!endif

!include "MUI2.nsh"
!define LOGICLIB_STRCMP
!include "LogicLib.nsh"
!include "Sections.nsh"

!include "StrFunc.nsh"
${StrRep}				; Declare in advance

!define /ifndef NULL 0


# NScurl.dll (debug) location
!ifdef NSIS_AMD64
	!define NSCURL "$EXEDIR\..\Debug-amd64-unicode\NScurl.dll"
!else ifdef NSIS_UNICODE
	!define NSCURL "$EXEDIR\..\Debug-x86-unicode\NScurl.dll"
!else
	!define NSCURL "$EXEDIR\..\Debug-x86-ansi\NScurl.dll"
!endif

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
!ifdef NSIS_AMD64
	OutFile "NScurl-Debug-amd64-unicode.exe"
!else ifdef NSIS_UNICODE
	OutFile "NScurl-Debug-x86-unicode.exe"
!else
	OutFile "NScurl-Debug-x86-ansi.exe"
!endif

Name "NScurl-Debug"
XPStyle on
RequestExecutionLevel user		; Don't require UAC elevation
ShowInstDetails show
ManifestDPIAware true

!macro STACK_VERIFY_START
	; This macro is optional. You don't have to use it in your script!
	Push "MyStackTop"
!macroend

!macro STACK_VERIFY_END
	; This macro is optional. You don't have to use it in your script!
	Pop $R9
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

/*	; .onInit download demo
	; NOTE: Transfers from .onInit can be either Silent or Popup (no Page!)
	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'https://httpbin.org/post?param1=1&param2=2'
	!define /redef FILE '$EXEDIR\_Post_onInit.json'
	DetailPrint 'NScurl::Transfer "${LINK}" "${FILE}"'
	Push "/END"
	Push "https://wikipedia.org"
	Push "/REFERER"
	Push 60000
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
	Push "POPUP"
	Push "/MODE"
	Push "POST"
	Push "/METHOD"
	CallInstDLL "${NSCURL}" Transfer
	Pop $0
	!insertmacro STACK_VERIFY_END */
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


Section Test
	SectionIn 1
	
	; NScurl::Echo
	!insertmacro STACK_VERIFY_START
	Push "/END"
	Push 0x2
	Push 1
	Push bbb
	Push "aaa"
	CallInstDLL "${NSCURL}" Echo
	Pop $0
	DetailPrint 'NScurl::Echo(...) = "$0"'
	!insertmacro STACK_VERIFY_END

	; NScurl::DownloadCacert
	!insertmacro STACK_VERIFY_START
	CallInstDLL "${NSCURL}" DownloadCacert
	Pop $0
	DetailPrint "NScurl::DownloadCacert() = $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


/*
Section /o "HTTP GET (Page mode)"
	SectionIn 1	; All

	DetailPrint '-----------------------------------------------'
	DetailPrint '${__SECTION__}'
	DetailPrint '-----------------------------------------------'

	!insertmacro STACK_VERIFY_START
	!define /redef LINK 'http://live.sysinternals.com/Files/SysinternalsSuite.zip'
	!define /redef FILE '$EXEDIR\_SysinternalsSuite.zip'
	DetailPrint 'NScurl::Transfer "${LINK}" "${FILE}"'
	Push "/END"
	Push 30000
	Push "/TIMEOUTRECONNECT"
	Push 15000
	Push "/TIMEOUTCONNECT"
	Push "${FILE}"
	Push "/LOCAL"
	Push "${LINK}"
	Push "/URL"
	CallInstDLL "${NSCURL}" Transfer
	Pop $0
	DetailPrint "Status: $0"
	!insertmacro STACK_VERIFY_END
SectionEnd


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
	CallInstDLL "${NSCURL}" Transfer
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
	CallInstDLL "${NSCURL}" Transfer
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
	CallInstDLL "${NSCURL}" Request
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
	CallInstDLL "${NSCURL}" Request
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
	CallInstDLL "${NSCURL}" Request
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
	CallInstDLL "${NSCURL}" Wait
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
	CallInstDLL "${NSCURL}" Transfer
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
	CallInstDLL "${NSCURL}" Transfer
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
	CallInstDLL "${NSCURL}" Transfer
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
	CallInstDLL "${NSCURL}" Request
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
	CallInstDLL "${NSCURL}" "Set"
	Pop $0	; Error code. Ignored

	; Wait
	DetailPrint 'Waiting . . .'
	Push "/END"
	Push "Are you sure?"
	Push "Abort"
	Push "/ABORT"
	Push "Page"
	Push "/Mode"
	CallInstDLL "${NSCURL}" Wait
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
	CallInstDLL "${NSCURL}" "Set"
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
	CallInstDLL "${NSCURL}" Wait
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
	CallInstDLL "${NSCURL}" Enumerate
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
		CallInstDLL "${NSCURL}" Query

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
	CallInstDLL "${NSCURL}" QueryGlobal
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
