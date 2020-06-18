
set NSIS_REPO=%~dp0..\..

xcopy "%NSIS_REPO%\Contrib\ExDLL\pluginapi.*"  "nsis\" /DYI
xcopy "%NSIS_REPO%\Contrib\ExDLL\nsis_tchar.h" "nsis\" /DYI
xcopy "%NSIS_REPO%\Source\exehead\api.h"       "nsis\" /DYI
if not exist "nsis\*.*" if not exist "%NSIS_REPO%" echo ERROR: NSIS sources not found. Clone NSIS repository to "%NSIS_REPO%" && pause && exit /B 2

echo.
exit /B 0