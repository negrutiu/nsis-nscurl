@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0"

set Z7=%PROGRAMFILES%\7-Zip\7z.exe
if not exist "%Z7%" echo ERROR: Missing %Z7% && pause && exit /b 2

REM :: Read version from the .rc file
for /f usebackq^ tokens^=3^ delims^=^"^,^  %%f in (`type resource.rc ^| findstr /r /c:"\s*\"FileVersion\"\s*"`) do set RCVER=%%f

rmdir /S /Q _Package > nul 2> nul
mkdir _Package
mkdir _Package\amd64-unicode
mkdir _Package\x86-unicode
mkdir _Package\x86-ansi
mkdir _Package\Test

goto :file_end
:file
    mklink /H "%~1" "%~2" || echo --- WARNING --- failed to add "%~2"&& pause
    exit /b
:file_end

call :file _Package\amd64-unicode\NScurl.dll		Release-mingw-amd64-unicode\NScurl.dll
call :file _Package\x86-unicode\NScurl.dll			Release-mingw-x86-unicode\NScurl.dll
call :file _Package\x86-ansi\NScurl.dll				Release-mingw-x86-ansi\NScurl.dll
call :file _Package\Test\NScurl-Test.nsi			Test\NScurl-Test.nsi
call :file _Package\NScurl.readme.md				NScurl.readme.md
call :file _Package\README.md						README.md
call :file _Package\LICENSE.md						LICENSE.md
call :file _Package\LICENSE.curl.md					libcurl-devel\src\curl\COPYING
call :file _Package\LICENSE.libcurl-devel.md		libcurl-devel\LICENSE.md
call :file _Package\LICENSE.nghttp2.md				libcurl-devel\src\nghttp2\COPYING
call :file _Package\LICENSE.openssl.md				libcurl-devel\src\openssl\LICENSE.txt
call :file _Package\LICENSE.zlib.md					libcurl-devel\src\zlib\LICENSE

pushd _Package
"%Z7%" a "..\NScurl-%RCVER%.7z" * -r || pause && exit /b !errorlevel!
popd

echo.
pause

rmdir /S /Q _Package > NUL 2> NUL
