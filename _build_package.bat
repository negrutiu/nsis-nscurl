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

call :file %workdir%\Plugins\amd64-unicode\NScurl.dll		    Release-mingw-amd64-unicode\NScurl.dll
call :file %workdir%\Plugins\x86-unicode\NScurl.dll			    Release-mingw-x86-unicode\NScurl.dll
call :file %workdir%\Plugins\x86-ansi\NScurl.dll			    Release-mingw-x86-ansi\NScurl.dll
call :file %workdir%\Examples\NScurl\NScurl-Test.nsi			tests\NScurl-Test.nsi
call :file %workdir%\Examples\NScurl\NScurl-Test-build.bat		tests\NScurl-Test-build.bat
call :file %workdir%\Docs\NScurl\NScurl.readme.md				src\nscurl\NScurl.readme.md
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
7z a "%~dp0%outdir%\NScurl.zip" * -r || pause && exit /b !errorlevel!
popd

echo.
echo -------------------------------------------------
REM  -- curl packages

pushd vcpkg\x86-mingw-static\installed\x86-mingw-static\tools\curl
7z a "%~dp0%outdir%\curl-x86.zip" curl.exe || pause && exit /b !errorlevel!
popd

echo -------------------------------------------------

pushd vcpkg\x64-mingw-static\installed\x64-mingw-static\tools\curl
7z a "%~dp0%outdir%\curl-amd64.zip" curl.exe || pause && exit /b !errorlevel!
popd

echo -------------------------------------------------

pushd src\nscurl
7z a "%~dp0%outdir%\curl-x86.zip"   curl-ca-bundle.crt || pause && exit /b !errorlevel!
7z a "%~dp0%outdir%\curl-amd64.zip" curl-ca-bundle.crt || pause && exit /b !errorlevel!
popd

echo.
echo *************************************************
REM  ** Versions
set versions=%outdir%\versions.txt

echo.> "%versions%"
echo NScurl/%version%>> "%versions%"

echo.>> "%versions%"
"%~dp0\vcpkg\x86-mingw-static\installed\x86-mingw-static\tools\curl\curl.exe" -V>> "%versions%"

echo.>> "%versions%"
"%~dp0\vcpkg\x64-mingw-static\installed\x64-mingw-static\tools\curl\curl.exe" -V>> "%versions%"

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
