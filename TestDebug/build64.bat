@echo off
SetLocal EnableDelayedExpansion

if not exist "%NSIS%\makensis.exe" pushd "%~dp0..\.." && set NSIS=!CD!&& popd
if not exist "%NSIS%\makensis.exe" set NSIS=%NSIS_INSTDIR%
if not exist "%NSIS%\makensis.exe" set NSIS=%PROGRAMFILES%\NSIS
if not exist "%NSIS%\makensis.exe" set NSIS=%PROGRAMFILES(X86)%\NSIS
if not exist "%NSIS%\makensis.exe" echo ERROR: NSIS not found ^(Tip: NSIS_INSTDIR can be defined to point to NSIS binaries^) && pause && exit /B 2

echo ********************************************************************************
echo %NSIS%\makensis.exe
echo ********************************************************************************

"%NSIS%\makensis.exe" /V4 /DAMD64 "%~dp0\NScurl-Debug.nsi"
if %errorlevel% neq 0 pause && exit /B %errorlevel%
