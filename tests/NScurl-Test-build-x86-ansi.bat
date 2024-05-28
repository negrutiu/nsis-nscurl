@echo off
setlocal enabledelayedexpansion

if not exist "%NSIS%\makensis.exe" pushd "%~dp0..\.." && set NSIS=!cd!&& popd
if not exist "%NSIS%\makensis.exe" set NSIS=%PROGRAMFILES%\NSIS
if not exist "%NSIS%\makensis.exe" set NSIS=%PROGRAMFILES(X86)%\NSIS
for /f "delims=*" %%f in ('where makensis.exe 2^> nul') do pushd "%%~dpf" && set NSIS=!cd!&& popd
if not exist "%NSIS%\makensis.exe" echo ERROR: NSIS not found&& pause && exit /b 2

echo ********************************************************************************
echo %NSIS%\makensis.exe
echo ********************************************************************************

"%NSIS%\makensis.exe" /DDEVEL /V4 /DANSI "%~dp0\NScurl-Test.nsi"
if %errorlevel% neq 0 pause && exit /B %errorlevel%

REM echo ----------------------------------------------------------
REM set exe=NScurl-Test-x86-ansi.exe
REM set /P answer=Execute %exe% (y/N)? 
REM if /I "%answer%" equ "y" "%~dp0\%exe%"