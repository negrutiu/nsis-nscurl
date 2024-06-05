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
set outdir=packages\%configuration%-%compiler%-%platform_nsis%-%charset%
rmdir /s /q %outdir% 2> nul

call :copy README.md   %outdir%\README.md
call :copy LICENSE.md  %outdir%\LICENSE.md
call :copy vcpkg\%triplet%\installed\%triplet%\share\brotli\copyright   %outdir%\LICENSE.brotli.md
call :copy vcpkg\%triplet%\installed\%triplet%\share\curl\copyright     %outdir%\LICENSE.curl.md
call :copy vcpkg\%triplet%\installed\%triplet%\share\nghttp2\copyright  %outdir%\LICENSE.nghttp2.md
call :copy vcpkg\%triplet%\installed\%triplet%\share\openssl\copyright  %outdir%\LICENSE.openssl.md
call :copy vcpkg\%triplet%\installed\%triplet%\share\zlib\copyright     %outdir%\LICENSE.zlib.md
call :copy vcpkg\%triplet%\installed\%triplet%\share\zstd\copyright     %outdir%\LICENSE.zstd.md

call :copy Release-%compiler%-%platform_nsis%-%charset%\NScurl.dll %outdir%\Plugins\%platform_nsis%-%charset%\

call :copy tests\NScurl-Test.nsi       %outdir%\Examples\NScurl\
call :copy tests\NScurl-Test-build.bat %outdir%\Examples\NScurl\

call :copy src\nscurl\NScurl.readme.md %outdir%\Docs\NScurl\
call :copy tests\NScurl-Test-build.bat %outdir%\Examples\NScurl\
:end_package

:curl_package
if /i "%charset%" neq "unicode" goto :end_curl_package
echo.
set outdir=packages\%configuration%-%compiler%-%platform_nsis%-curl
call :copy vcpkg\%triplet%\installed\%triplet%\tools\curl\curl.exe %outdir%\
call :copy src\nscurl\curl-ca-bundle.crt %outdir%\
:end_curl_package

echo all done. errorlevel %errorlevel%
REM pause
