@echo off
setlocal enabledelayedexpansion

set outdir=%~dp0\nsis
mkdir "%outdir%" > nul 2> nul

if exist "%outdir%/pluginapi.h" if exist "%outdir%/pluginapi.c" if exist "%outdir%/nsis_tchar.h" if exist "%outdir%/api.h" (
    echo Use existing NSIS SDK...
    exit /b 0
)

set nsis_gitroot=%~dp0..\..\..\..
if exist "%nsis_gitroot%\Source\exehead\api.h" (
    echo Copying NSIS SDK...
    xcopy "%nsis_gitroot%\Contrib\ExDLL\pluginapi.*"  "%outdir%\" /DYI || exit /b !errorlevel!
    xcopy "%nsis_gitroot%\Contrib\ExDLL\nsis_tchar.h" "%outdir%\" /DYI || exit /b !errorlevel!
    xcopy "%nsis_gitroot%\Source\exehead\api.h"       "%outdir%\" /DYI || exit /b !errorlevel!
) else (
    echo Downloading NSIS SDK...
    curl --no-progress-meter -L -f -o "%outdir%/pluginapi.h"  -z "%outdir%/pluginapi.h"   https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/pluginapi.h || exit /b !errorlevel!
    curl --no-progress-meter -L -f -o "%outdir%/pluginapi.c"  -z "%outdir%/pluginapi.c"   https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/pluginapi.c || exit /b !errorlevel!
    curl --no-progress-meter -L -f -o "%outdir%/nsis_tchar.h" -z "%outdir%/nsis_tchar.h"  https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/nsis_tchar.h || exit /b !errorlevel!
    curl --no-progress-meter -L -f -o "%outdir%/api.h"        -z "%outdir%/api.h"         https://raw.githubusercontent.com/kichik/nsis/master/Source/exehead/api.h || exit /b !errorlevel!
)
