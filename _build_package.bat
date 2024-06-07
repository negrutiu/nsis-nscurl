@echo off
setlocal EnableDelayedExpansion

cd /d "%~dp0"

set PATH=%PATH%;%PROGRAMFILES%\7-Zip

REM :: Read version from the .rc file
for /f usebackq^ tokens^=3^ delims^=^"^,^  %%f in (`type src\nscurl\resource.rc ^| findstr /r /c:"\s*\"FileVersion\"\s*"`) do set version=%%f

set workdir=packages\current
set outdir=packages\%version%

rmdir /s /q "%outdir%"  > nul 2> nul
rmdir /s /q "%workdir%" > nul 2> nul
for %%d in (%workdir%\Plugins\amd64-unicode, %workdir%\Plugins\x86-unicode, %workdir%\Plugins\x86-ansi) do mkdir %%~d
for %%d in (%workdir%\Examples\NScurl, %workdir%\Docs\NScurl) do mkdir %%~d

goto :file_end
:file
    mklink /H "%~1" "%~2" || echo --- WARNING --- failed to add "%~2"&& pause
    exit /b
:file_end

call :file %workdir%\Plugins\amd64-unicode\NScurl.dll		    packages\Release-mingw-amd64-unicode\Plugins\amd64-unicode\NScurl.dll
call :file %workdir%\Plugins\x86-unicode\NScurl.dll			    packages\Release-mingw-x86-unicode\Plugins\x86-unicode\NScurl.dll
call :file %workdir%\Plugins\x86-ansi\NScurl.dll			    packages\Release-mingw-x86-ansi\Plugins\x86-ansi\NScurl.dll
call :file %workdir%\Examples\NScurl\NScurl-Test.nsi			packages\Release-mingw-x86-unicode\Examples\NScurl\NScurl-Test.nsi
call :file %workdir%\Examples\NScurl\NScurl-Test-build.bat		packages\Release-mingw-x86-unicode\Examples\NScurl\NScurl-Test-build.bat
call :file %workdir%\Docs\NScurl\NScurl.readme.md				packages\Release-mingw-x86-unicode\Docs\NScurl\NScurl.readme.md
call :file %workdir%\README.md						            packages\Release-mingw-x86-unicode\README.md
call :file %workdir%\LICENSE.md						            packages\Release-mingw-x86-unicode\LICENSE.md
call :file %workdir%\LICENSE.brotli.md				            packages\Release-mingw-x86-unicode\LICENSE.brotli.md
call :file %workdir%\LICENSE.curl.md				            packages\Release-mingw-x86-unicode\LICENSE.curl.md
call :file %workdir%\LICENSE.nghttp2.md				            packages\Release-mingw-x86-unicode\LICENSE.nghttp2.md
call :file %workdir%\LICENSE.openssl.md				            packages\Release-mingw-x86-unicode\LICENSE.openssl.md
call :file %workdir%\LICENSE.zlib.md				            packages\Release-mingw-x86-unicode\LICENSE.zlib.md
call :file %workdir%\LICENSE.zstd.md				            packages\Release-mingw-x86-unicode\LICENSE.zstd.md

mkdir %outdir% 2> nul
pushd %workdir%
7z a "%~dp0%outdir%\NScurl.zip" * -r || pause && exit /b !errorlevel!
popd

echo.
echo -------------------------------------------------
REM  -- curl packages

pushd packages\Release-mingw-x86-curl
7z a "%~dp0%outdir%\curl-x86.zip" * || pause && exit /b !errorlevel!
popd

echo -------------------------------------------------

pushd packages\Release-mingw-amd64-curl
7z a "%~dp0%outdir%\curl-amd64.zip" * || pause && exit /b !errorlevel!
popd


echo.
echo *************************************************
REM  ** Versions
set versions=%outdir%\versions.txt

echo.> "%versions%"
echo NScurl/%version%>> "%versions%"

echo.>> "%versions%"
packages\Release-mingw-x86-curl\curl.exe -V>> "%versions%"

echo.>> "%versions%"
packages\Release-mingw-amd64-curl\curl.exe -V>> "%versions%"

echo.>> "%versions%"
echo curl-ca-bundle.crt>> "%versions%"
type "%~dp0\src\nscurl\curl-ca-bundle.crt" | findstr /C:"as of:">> "%versions%"

echo.>> "%versions%"
for %%d in (%SystemDrive%\msys64\mingw64\bin %SystemDrive%\msys64\mingw32\bin %SystemDrive%\msys2\mingw64\bin %SystemDrive%\msys2\mingw32\bin) do (
    if exist "%%~d\gcc.exe" echo %%~d\gcc.exe>> %versions% && "%%~d\gcc.exe" --version>> %versions%
)

type "%versions%"
pause

rmdir /S /Q %workdir% > nul 2> nul
