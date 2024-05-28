REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
setlocal enabledelayedexpansion
echo.

cd /d "%~dp0"
set original_path=%PATH%

REM | build.bat [mingw|msvc] [debug|release] [win32|amd64|x86|x64] [unicode|ansi]

set compiler=mingw
set configuration=Release
set platform=x64
set charset=unicode

:arg
    if "%~1" equ "" goto :arg_end
    if /i "%~1" equ "mingw"     set compiler=mingw
    if /i "%~1" equ "msvc"      set compiler=msvc
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

title %configuration%-%compiler%-%platform%-%charset%
echo --- compiler = %compiler%
echo --- configuration = %configuration%
echo --- platform = %platform%
echo --- charset = %charset%

if "%compiler%" equ "mingw" goto :mingw
if "%compiler%" equ "msvc" goto :msvc

:: -------------------------------------------------------------------------------

:mingw
if "%platform%" equ "x86" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys2\mingw32
if "%platform%" equ "x86" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys64\mingw32
if "%platform%" equ "x64" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys2\mingw64
if "%platform%" equ "x64" if not exist "%mingw%\bin\gcc.exe" set MINGW=%SystemDrive%\msys64\mingw64
echo --- mingw = %mingw%
if not exist "%mingw%\bin\gcc.exe" echo ERROR: Missing "%mingw%\bin\gcc.exe" && exit /b 2
set PATH=%mingw%\bin;%PATH%

echo.
gcc --version

set platform_nsis=%platform%
if "%platform_nsis%" equ "x64" set platform_nsis=amd64

echo ^> mingw32-make.exe ARCH=%platform% CHAR=%charset% OUTDIR="%~dp0%configuration%-%compiler%-%platform_nsis%-%charset%" CONFIG=%configuration% clean all
mingw32-make.exe ARCH=%platform% CHAR=%charset% OUTDIR="%~dp0%configuration%-%compiler%-%platform_nsis%-%charset%" CONFIG=%configuration% clean all || exit /b !errorlevel!
goto :end

:: -------------------------------------------------------------------------------

:msvc
echo TODO
goto :end

:end
echo all done. errorlevel %errorlevel%
REM pause