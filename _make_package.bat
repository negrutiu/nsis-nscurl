@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0"

set Z7=%PROGRAMFILES%\7-Zip\7z.exe
if not exist "%Z7%" echo ERROR: Missing %Z7% && pause && exit /b 2

REM :: Read version from the .rc file
for /f usebackq^ tokens^=3^ delims^=^"^,^  %%f in (`type src\nscurl\resource.rc ^| findstr /r /c:"\s*\"FileVersion\"\s*"`) do set RCVER=%%f

set workdir=packages\current
set outdir=packages\%RCVER%

rmdir "%outdir%" > nul 2> nul

rmdir /S /Q %workdir% > nul 2> nul
mkdir %workdir%
mkdir %workdir%\amd64-unicode
mkdir %workdir%\x86-unicode
mkdir %workdir%\x86-ansi
mkdir %workdir%\Test

goto :file_end
:file
    mklink /H "%~1" "%~2" || echo --- WARNING --- failed to add "%~2"&& pause
    exit /b
:file_end

call :file %workdir%\amd64-unicode\NScurl.dll		Release-mingw-amd64-unicode\NScurl.dll
call :file %workdir%\x86-unicode\NScurl.dll			Release-mingw-x86-unicode\NScurl.dll
call :file %workdir%\x86-ansi\NScurl.dll			Release-mingw-x86-ansi\NScurl.dll
call :file %workdir%\Test\NScurl-Test.nsi			tests\NScurl-Test.nsi
call :file %workdir%\Test\NScurl-Test-build.bat		tests\NScurl-Test-build.bat
call :file %workdir%\NScurl.readme.md				src\nscurl\NScurl.readme.md
call :file %workdir%\README.md						README.md
call :file %workdir%\LICENSE.md						LICENSE.md
call :file %workdir%\LICENSE.brotli.md				vcpkg\x86-mingw-static\installed\x86-mingw-static\share\brotli\copyright
call :file %workdir%\LICENSE.curl.md				vcpkg\x86-mingw-static\installed\x86-mingw-static\share\curl\copyright
call :file %workdir%\LICENSE.nghttp2.md				vcpkg\x86-mingw-static\installed\x86-mingw-static\share\nghttp2\copyright
call :file %workdir%\LICENSE.openssl.md				vcpkg\x86-mingw-static\installed\x86-mingw-static\share\openssl\copyright
call :file %workdir%\LICENSE.zlib.md				vcpkg\x86-mingw-static\installed\x86-mingw-static\share\zlib\copyright
call :file %workdir%\LICENSE.zstd.md				vcpkg\x86-mingw-static\installed\x86-mingw-static\share\zstd\copyright

mkdir %outdir% 2> nul
pushd %workdir%
"%Z7%" a "%~dp0%outdir%\NScurl.7z" * -r || pause && exit /b !errorlevel!
popd

echo.
echo -------------------------------------------------
REM  -- curl packages

pushd vcpkg\x86-mingw-static\installed\x86-mingw-static\tools\curl
"%Z7%" a "%~dp0%outdir%\curl-x86.7z" curl.exe || pause && exit /b !errorlevel!
popd

echo -------------------------------------------------

pushd vcpkg\x64-mingw-static\installed\x64-mingw-static\tools\curl
"%Z7%" a "%~dp0%outdir%\curl-amd64.7z" curl.exe || pause && exit /b !errorlevel!
popd

echo -------------------------------------------------

pushd src\nscurl
"%Z7%" a "%~dp0%outdir%\curl-x86.7z"   curl-ca-bundle.crt || pause && exit /b !errorlevel!
"%Z7%" a "%~dp0%outdir%\curl-amd64.7z" curl-ca-bundle.crt || pause && exit /b !errorlevel!
popd

echo.
pause

rmdir /S /Q %workdir% > nul 2> nul
