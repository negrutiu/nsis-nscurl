REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
echo.

if not exist "%MINGW32%\bin\gcc.exe" set MINGW32=%MINGW32_INSTDIR%
if not exist "%MINGW32%\bin\gcc.exe" set MINGW32=%SYSTEMROOT%\msys64\mingw32
if not exist "%MINGW32%\bin\gcc.exe" set MINGW32=%SYSTEMROOT%\msys2\mingw32

if not exist "%MINGW64%\bin\gcc.exe" set MINGW64=%MINGW64_INSTDIR%
if not exist "%MINGW64%\bin\gcc.exe" set MINGW64=%SYSTEMROOT%\msys64\mingw64
if not exist "%MINGW64%\bin\gcc.exe" set MINGW64=%SYSTEMROOT%\msys2\mingw64

set ORIGINAL_PATH=%PATH%

cd /d "%~dp0"

:pluginapi
call _acquire_pluginapi.bat
if %errorlevel% neq 0 exit /B %errorlevel%

:x86
if not exist "%MINGW32%\bin\gcc.exe" echo ERROR: Missing "%MINGW32%" && pause && exit /B 2
set PATH=%MINGW32%\bin;%ORIGINAL_PATH%

echo.
echo -------------------------------------------------------------------
set OUTDIR=Release-mingw-x86-ansi
echo %OUTDIR%
title %OUTDIR%
echo -------------------------------------------------------------------
mingw32-make.exe ARCH=X86 CHAR=ANSI OUTDIR=%OUTDIR% -fMakefile.mingw clean all
if %errorlevel% neq 0 pause && exit /B %errorlevel%

echo.
echo -------------------------------------------------------------------
set OUTDIR=Release-mingw-x86-unicode
echo %OUTDIR%
title %OUTDIR%
echo -------------------------------------------------------------------
mingw32-make.exe ARCH=X86 CHAR=Unicode OUTDIR=%OUTDIR% -fMakefile.mingw clean all
if %errorlevel% neq 0 pause && exit /B %errorlevel%


:amd64
if not exist "%MINGW64%\bin\gcc.exe" echo ERROR: Missing "%MINGW64%" && pause && exit /B 2
set PATH=%MINGW64%\bin;%ORIGINAL_PATH%

echo.
echo -------------------------------------------------------------------
set OUTDIR=Release-mingw-amd64-unicode
echo %OUTDIR%
title %OUTDIR%
echo -------------------------------------------------------------------
mingw32-make.exe ARCH=X64 CHAR=Unicode OUTDIR=%OUTDIR% -fMakefile.mingw clean all
if %errorlevel% neq 0 pause && exit /B %errorlevel%

echo.
REM pause
