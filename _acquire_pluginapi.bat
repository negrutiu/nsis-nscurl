setlocal enabledelayedexpansion

mkdir "%~dp0/nsis" > nul 2> nul

set NSIS_GIT=%~dp0..\..

if exist "%NSIS_GIT%\Source\exehead\api.h" (
    call :nsis_build
) else (
    call :standalone_build
)
exit /b

REM -----------------------------------------------------------------------------------

:nsis_build
echo Copying NSIS SDK...
xcopy "%NSIS_GIT%\Contrib\ExDLL\pluginapi.*"  "%~dp0\nsis\" /DYI || exit /b !errorlevel!
xcopy "%NSIS_GIT%\Contrib\ExDLL\nsis_tchar.h" "%~dp0\nsis\" /DYI || exit /b !errorlevel!
xcopy "%NSIS_GIT%\Source\exehead\api.h"       "%~dp0\nsis\" /DYI || exit /b !errorlevel!
exit /b

:standalone_build
echo Downloading NSIS SDK...
curl --no-progress-meter -L -o "%~dp0/nsis/pluginapi.h"  -z "%~dp0/nsis/pluginapi.h"   https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/pluginapi.h || exit /b !errorlevel!
curl --no-progress-meter -L -o "%~dp0/nsis/pluginapi.c"  -z "%~dp0/nsis/pluginapi.c"   https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/pluginapi.c || exit /b !errorlevel!
curl --no-progress-meter -L -o "%~dp0/nsis/nsis_tchar.h" -z "%~dp0/nsis/nsis_tchar.h"  https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/nsis_tchar.h || exit /b !errorlevel!
curl --no-progress-meter -L -o "%~dp0/nsis/api.h"        -z "%~dp0/nsis/api.h"         https://raw.githubusercontent.com/kichik/nsis/master/Source/exehead/api.h || exit /b !errorlevel!
exit /b
