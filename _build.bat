REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
setlocal enabledelayedexpansion
echo.

cd /d "%~dp0"
set original_path=%PATH%

REM | build.bat [mingw|msbuild] [debug|release] [win32|amd64|x86|x64] [unicode|ansi]

set compiler=mingw
set configuration=Release
set platform=x64
set charset=unicode

:arg
    if "%~1" equ "" goto :arg_end
    if /i "%~1" equ "mingw"     set compiler=mingw
    if /i "%~1" equ "msbuild"   set compiler=msbuild
    if /i "%~1" equ "debug"     set configuration=Debug
    if /i "%~1" equ "release"   set configuration=Release
    if /i "%~1" equ "win32"     set platform=x86
    if /i "%~1" equ "x86"       set platform=x86
    if /i "%~1" equ "amd64"     set platform=x64
    if /i "%~1" equ "x64"       set platform=x64
    if /i "%~1" equ "unicode"   set charset=unicode
    if /i "%~1" equ "ansi"      set charset=ansi
    shift /1
    goto :arg
:arg_end

if "%platform%" neq "x86" set charset=unicode

if "%platform_nsis%" equ ""    set platform_nsis=%platform%
if "%platform_nsis%" equ "x64" set platform_nsis=amd64

if /i "%compiler%" equ "mingw"   set triplet=%platform%-mingw-static
if /i "%compiler%" equ "msbuild" set triplet=%platform%-windows-static

title %configuration%-%compiler%-%platform%-%charset%
echo --- compiler = %compiler%
echo --- configuration = %configuration%
echo --- platform = %platform%
echo --- platform_nsis = %platform_nsis%
echo --- triplet = %triplet%
echo --- charset = %charset%

if "%compiler%" equ "mingw" goto :mingw
if "%compiler%" equ "msbuild" goto :msbuild

:: -------------------------------------------------------------------------------

:mingw
if "%platform%" equ "x86" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys2\mingw32
if "%platform%" equ "x86" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys64\mingw32
if "%platform%" equ "x86" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\mingw32
if "%platform%" equ "x64" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys2\mingw64
if "%platform%" equ "x64" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys64\mingw64
if "%platform%" equ "x64" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\mingw64
echo --- mingw = %mingw%
if not exist "%mingw%\bin\gcc.exe" echo ERROR: Missing "%mingw%\bin\gcc.exe" && exit /b 2
set PATH=%mingw%\bin;%PATH%

echo.
gcc --version

REM --- build ---
echo --- mingw32-make.exe ARCH=%platform% CHAR=%charset% OUTDIR="%~dp0%configuration%-%compiler%-%platform_nsis%-%charset%" CONFIG=%configuration% clean all
echo.
mingw32-make.exe ARCH=%platform% CHAR=%charset% OUTDIR="%~dp0%configuration%-%compiler%-%platform_nsis%-%charset%" CONFIG=%configuration% clean all || exit /b !errorlevel!

goto :end_build

:: -------------------------------------------------------------------------------

:msbuild
set solution=%CD%\NScurl.sln
set verbosity=normal

if not exist "%VcVarsAll%" for /f "tokens=1* delims=: " %%i in ('"%PROGRAMFILES(X86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version 17 -requires Microsoft.Component.MSBuild 2^> nul') do if /i "%%i"=="installationPath" set VcVarsAll=%%j\VC\Auxiliary\Build\VCVarsAll.bat&& set platformtoolset=v143
if not exist "%VcVarsAll%" for /f "tokens=1* delims=: " %%i in ('"%PROGRAMFILES(X86)%\Microsoft Visual Studio\Installer\vswhere.exe" -version 16 -requires Microsoft.Component.MSBuild 2^> nul') do if /i "%%i"=="installationPath" set VcVarsAll=%%j\VC\Auxiliary\Build\VCVarsAll.bat&& set platformtoolset=v142
if not exist "%VcVarsAll%" echo ERROR: Can't find Visual Studio 2017-2022 && exit /b 2

echo --- %VcVarsAll%
echo --- platformtoolset = %platformtoolset%
echo --- solution = %solution%
echo --- verbosity = %verbosity%

echo.
pushd "%CD%"
call "%VcVarsAll%" %platform%
popd

set platform_msbuild=%platform%
if "%platform_msbuild%" equ "x86" set platform_msbuild=Win32

REM --- build ---
if "%charset%" equ "unicode" set ParamCharacterSet= /p:CharacterSet=Unicode
if "%charset%" equ "ansi"    set ParamCharacterSet= /p:CharacterSet=MultiByte

echo.
echo --- msbuild /m /t:build "%solution%" /p:Configuration=%configuration% /p:Platform=%platform_msbuild% /p:PlatformToolset=%platformtoolset% /verbosity:%verbosity%%ParamCharacterSet%
echo.
msbuild /m /t:build "%solution%" /p:Configuration=%configuration% /p:Platform=%platform_msbuild% /p:PlatformToolset=%platformtoolset% /verbosity:%verbosity%%ParamCharacterSet% || exit /b !errorlevel!

goto :end_build

:end_build

goto :package
:copy
echo %~1 -^> %~2
mkdir "%~dp2" 2> nul
copy "%~1" "%~2" || pause&& exit !errorlevel!
exit /b

:package
echo.

set bindir=Release-%compiler%-%platform_nsis%-%charset%
set pkgdir=packages\%configuration%-%compiler%-%platform_nsis%-%charset%
set curldir=packages\%configuration%-%compiler%-%platform_nsis%-curl
set vcpkginstdir=vcpkg\clone\installed\%triplet%

rmdir /s /q %pkgdir% 2> nul

call :copy README.md   %pkgdir%\README.md
call :copy LICENSE.md  %pkgdir%\LICENSE.md
call :copy %vcpkginstdir%\share\brotli\copyright   %pkgdir%\LICENSE.brotli.md
call :copy %vcpkginstdir%\share\curl\copyright     %pkgdir%\LICENSE.curl.md
call :copy %vcpkginstdir%\share\nghttp2\copyright  %pkgdir%\LICENSE.nghttp2.md
call :copy %vcpkginstdir%\share\openssl\copyright  %pkgdir%\LICENSE.openssl.md
call :copy %vcpkginstdir%\share\zlib\copyright     %pkgdir%\LICENSE.zlib.md
call :copy %vcpkginstdir%\share\zstd\copyright     %pkgdir%\LICENSE.zstd.md

call :copy %bindir%\NScurl.dll %pkgdir%\Plugins\%platform_nsis%-%charset%\

call :copy tests\NScurl-Test.nsi       %pkgdir%\Examples\NScurl\
call :copy tests\NScurl-Test-build.bat %pkgdir%\Examples\NScurl\

call :copy src\nscurl\NScurl.readme.md %pkgdir%\Docs\NScurl\
call :copy tests\NScurl-Test-build.bat %pkgdir%\Examples\NScurl\
:end_package

:curl_package
if /i "%charset%" neq "unicode" goto :end_curl_package
echo.
call :copy %vcpkginstdir%\tools\curl\curl.exe %curldir%\
call :copy src\nscurl\curl-ca-bundle.crt %curldir%\
:end_curl_package

:versions
echo.
echo --------------------------------------------------------
echo.

py -3 _versions.py --indent=2 --curl=%curldir%\curl.exe --gcc=gcc.exe> "%bindir%\versions.json"
type %bindir%\versions.json
:end_versions

echo all done. errorlevel %errorlevel%
REM pause
