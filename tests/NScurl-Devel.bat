REM :: Marius Negrutiu :: 2019/08/25
@echo off
setlocal EnableDelayedExpansion

set config=%~1
if "%config%" equ "" set config=Debug

rem | look upwards for makensis.exe
for /f "usebackq delims=*" %%d in (`powershell -ExecutionPolicy Bypass -Command "Get-Item '%~dp0' | ForEach-Object { $dir = $_.FullName; while($dir) { if (Test-Path (Join-Path $dir 'makensis.exe')) { $dir; break } $dir = Split-Path -Parent $dir }}"`) do set nsisdir=%%~d

rem | makensis.exe search locations
set PATH=%nsisdir%;%PATH%;%PROGRAMFILES%\NSIS;%PROGRAMFILES(X86)%\NSIS

echo.
echo -- where makensis
where makensis
echo.

rem | for native amd64 installers, check out https://github.com/negrutiu/nsis
for %%t in (x86-unicode x86-ansi amd64-unicode) do (
  title %%t
  makensis -V3 -DTARGET=%%t -DPLUGINDIR=..\%config%-mingw-%%t "%~dp0NScurl-Test.nsi" || pause && exit /b !errorlevel!
  echo ____________________________________________________________
  echo.
)

rem pause