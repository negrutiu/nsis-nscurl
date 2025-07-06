REM :: Marius Negrutiu :: 2019/08/25
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

for %%t in (amd64-unicode x86-unicode x86-ansi) do (
    if exist "%nsisdir%\Stubs\lzma-%%t" (
        if exist "%~dp0..\Release-mingw-%%t\NScurl.dll" set param_plugin_dir=/DPLUGIN_DIR=%~dp0..\Release-mingw-%%t
        if exist "%~dp0..\..\Plugins\%%t\NScurl.dll" set param_plugin_dir=/DPLUGIN_DIR=%~dp0..\..\Plugins\%%t
        title Build: %%t
        "%nsisdir%\makensis.exe" /V4 /DTARGET=%%t !param_plugin_dir! "%~dp0\NScurl-Test.nsi" || pause && exit /b !errorlevel!
    )
)
