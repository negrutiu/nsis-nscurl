REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
setlocal enabledelayedexpansion
echo.

if not exist "%MINGW32%\bin\gcc.exe" set MINGW32=%MINGW32_INSTDIR%
if not exist "%MINGW32%\bin\gcc.exe" set MINGW32=%SYSTEMROOT%\msys64\mingw32
if not exist "%MINGW32%\bin\gcc.exe" set MINGW32=%SYSTEMROOT%\msys2\mingw32

if not exist "%MINGW64%\bin\gcc.exe" set MINGW64=%MINGW64_INSTDIR%
if not exist "%MINGW64%\bin\gcc.exe" set MINGW64=%SYSTEMROOT%\msys64\mingw64
if not exist "%MINGW64%\bin\gcc.exe" set MINGW64=%SYSTEMROOT%\msys2\mingw64

set ORIGINAL_PATH=%PATH%

cd /d "%~dp0"

:dependencies
call _acquire_pluginapi.bat      || exit /b !errorlevel!
call _acquire_curl-ca-bundle.bat || exit /b !errorlevel!

REM | https://stackoverflow.com/questions/33584587/how-to-wait-all-batch-files-to-finish-before-exiting
echo Building vcpkg ...
(
   start "vcpkg x86" cmd /C _build_vcpkg.bat Win32 mingw
   start "vcpkg x64" cmd /C _build_vcpkg.bat x64 mingw
) | set /P "="
echo.

:x86
if not exist "%MINGW32%\bin\gcc.exe" echo ERROR: Missing "%MINGW32%" && pause && exit /B 2
set PATH=%MINGW32%\bin;%ORIGINAL_PATH%

echo.
echo -------------------------------------------------------------------
set OUTDIR=%~dp0Release-mingw-x86-ansi
echo %OUTDIR%
title %OUTDIR%
echo -------------------------------------------------------------------
pushd "%~dp0src\nscurl"
mingw32-make.exe ARCH=X86 CHAR=ANSI OUTDIR=%OUTDIR% clean all
popd
if %errorlevel% neq 0 pause && exit /B %errorlevel%

echo.
echo -------------------------------------------------------------------
set OUTDIR=%~dp0Release-mingw-x86-unicode
echo %OUTDIR%
title %OUTDIR%
echo -------------------------------------------------------------------
pushd "%~dp0src\nscurl"
mingw32-make.exe ARCH=X86 CHAR=Unicode OUTDIR=%OUTDIR% clean all
popd
if %errorlevel% neq 0 pause && exit /B %errorlevel%


:amd64
if not exist "%MINGW64%\bin\gcc.exe" echo ERROR: Missing "%MINGW64%" && pause && exit /B 2
set PATH=%MINGW64%\bin;%ORIGINAL_PATH%

echo.
echo -------------------------------------------------------------------
set OUTDIR=%~dp0Release-mingw-amd64-unicode
echo %OUTDIR%
title %OUTDIR%
echo -------------------------------------------------------------------
pushd "%~dp0src\nscurl"
mingw32-make.exe ARCH=X64 CHAR=Unicode OUTDIR=%OUTDIR% clean all
popd
if %errorlevel% neq 0 pause && exit /B %errorlevel%

echo.
REM pause
