REM :: Marius Negrutiu :: 2019/08/25
@echo off

if exist "W:\GIT\NSIS\NSISbin"      set NSIS_PATH=W:\GIT\NSIS\NSISbin& goto :build
if exist "%PROGRAMFILES%\NSIS"      set NSIS_PATH=%PROGRAMFILES%\NSIS& goto :build
if exist "%PROGRAMFILES(X86)%\NSIS" set NSIS_PATH=%PROGRAMFILES(X86)%\NSIS& goto :build
echo ERROR: NSIS not found & pause & exit /B 2

:build

Title Build: amd64-unicode
"%NSIS_PATH%\makensis.exe" /DAMD64 /V4 "%~dp0\NScurl-Test.nsi"
if %errorlevel% neq 0 pause & exit /B %errorlevel%

Title Build: x86-ansi
"%NSIS_PATH%\makensis.exe" /DANSI /V4 "%~dp0\NScurl-Test.nsi"
if %errorlevel% neq 0 pause & exit /B %errorlevel%

Title Build: x86-unicode
"%NSIS_PATH%\makensis.exe" /V4 "%~dp0\NScurl-Test.nsi"
if %errorlevel% neq 0 pause & exit /B %errorlevel%
