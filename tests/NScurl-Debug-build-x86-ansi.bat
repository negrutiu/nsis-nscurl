@echo off
setlocal EnableDelayedExpansion

if not exist "%nsisdir%\makensis.exe" pushd "%~dp0..\.." && set nsisdir=!cd!&& popd
if not exist "%nsisdir%\makensis.exe" for /f "delims=*" %%f in ('where makensis.exe 2^> nul') do pushd "%%~dpf" && set nsisdir=!cd!&& popd
if not exist "%nsisdir%\makensis.exe" set nsisdir=%PROGRAMFILES%\NSIS
if not exist "%nsisdir%\makensis.exe" set nsisdir=%PROGRAMFILES(X86)%\NSIS
if not exist "%nsisdir%\makensis.exe" echo ERROR: NSIS not found&& pause && exit /b 2

echo ________________________________________________________________________________
echo.
echo %nsisdir%\makensis.exe
echo ________________________________________________________________________________
echo.

"%nsisdir%\makensis.exe" /V3 /DTARGET=x86-ansi "%~dp0\NScurl-Debug.nsi" || pause && exit /b !errorlevel!
