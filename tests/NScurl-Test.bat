REM :: Marius Negrutiu :: 2019/08/25
@echo off
setlocal EnableDelayedExpansion

rem | look upwards for makensis.exe
for /f "usebackq delims=*" %%f in (`powershell -c "Get-Item '%~dp0' | ForEach-Object{ $d = $_.FullName; while($d){ $f = Join-Path $d 'makensis.exe'; if(Test-Path $f){ $f } $d = Split-Path $d }}"`) do if "!nsisdir!" equ "" set nsisdir=%%~dpf

rem | makensis.exe search locations
set PATH=%nsisdir%;%PATH%;%PROGRAMFILES%\NSIS;%PROGRAMFILES(X86)%\NSIS

rem | for native amd64 installers, check out https://github.com/negrutiu/nsis
for %%t in (x86-unicode x86-ansi amd64-unicode) do (
  title %%t
  makensis -V3 -DTARGET=%%t "%~dp0NScurl-Test.nsi" || pause && exit /b !errorlevel!
  echo ____________________________________________________________
  echo.
)

rem pause