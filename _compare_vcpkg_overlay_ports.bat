@echo off

REM | Manually compare our custom overlay ports with the latest vcpkg ports from GitHub

set vcpkg_dir=%~dp0vcpkg\latest
mkdir "%vcpkg_dir%" > nul 2> nul

if exist "%vcpkg_dir%\.gitignore" (
    pushd "%vcpkg_dir%"
    echo Pulling latest vcpkg.git . . .&& echo.
    git pull --verbose || exit /b !errorlevel!
    popd
) else (
    pushd "%vcpkg_dir%\.."
    git clone https://github.com/Microsoft/vcpkg.git "%vcpkg_dir%" || exit /b !errorlevel!
    popd
)

echo.

for /d %%d in (%~dp0\vcpkg_overlay_ports\*) do (
    echo Comparing %%~nxd . . .
    "%ProgramFiles%\WinMerge\WinMergeU.exe" "%vcpkg_dir%\ports\%%~nxd" "%~dp0\vcpkg_overlay_ports\%%~nxd"
)
