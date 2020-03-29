@echo off

cd /d "%~dp0"

if not exist Release-mingw-amd64-unicode\NScurl.dll		echo ERROR: Missing Release-mingw-amd64-unicode\NScurl.dll && pause && exit /B 2
if not exist Release-mingw-x86-ansi\NScurl.dll			echo ERROR: Missing Release-mingw-x86-ansi\NScurl.dll      && pause && exit /B 2
if not exist Release-mingw-x86-unicode\NScurl.dll		echo ERROR: Missing Release-mingw-x86-unicode\NScurl.dll   && pause && exit /B 2

set Z7=%PROGRAMFILES%\7-Zip\7z.exe
if not exist "%Z7%" echo ERROR: Missing %Z7% && pause && exit /B 2

REM :: Read version from the .rc file
for /f usebackq^ tokens^=3^ delims^=^"^,^  %%f in (`type resource.rc ^| findstr /r /c:"\s*\"FileVersion\"\s*"`) do set RCVER=%%f

rmdir /S /Q _Package > NUL 2> NUL
mkdir _Package
mkdir _Package\amd64-unicode
mkdir _Package\x86-unicode
mkdir _Package\x86-ansi

mklink /H _Package\amd64-unicode\NScurl.dll			Release-mingw-amd64-unicode\NScurl.dll
mklink /H _Package\x86-unicode\NScurl.dll			Release-mingw-x86-unicode\NScurl.dll
mklink /H _Package\x86-ansi\NScurl.dll				Release-mingw-x86-ansi\NScurl.dll
mklink /H _Package\NScurl.Readme.htm				NScurl.Readme.htm
mklink /H _Package\README.md						README.md
mklink /H _Package\LICENSE.md						LICENSE.md
mklink /H _Package\LICENSE.curl.md					LICENSE.curl.md
mklink /H _Package\LICENSE.libcurl-devel.md			LICENSE.libcurl-devel.md
mklink /H _Package\LICENSE.mbedTLS.md				LICENSE.mbedTLS.md

pushd _Package
"%Z7%" a "..\NScurl-%RCVER%.7z" * -r
popd

echo.
pause

rmdir /S /Q _Package > NUL 2> NUL
