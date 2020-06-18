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

"%NSIS%\makensis.exe" /DDEVEL /V4 "%~dp0\NScurl-Test.nsi"
if %errorlevel% neq 0 pause && exit /B %errorlevel%

REM echo ----------------------------------------------------------
REM set exe=NScurl-Test-x86-unicode.exe
REM set /P answer=Execute %exe% (y/N)? 
REM if /I "%answer%" equ "y" "%~dp0\%exe%"
