REM | marius.negrutiu@protonmail.com
@echo off
setlocal EnableDelayedExpansion

REM | script.bat <Win32|x64|arm64> <msbuild|mingw> <static|dynamic>
set arch=%~1
set compiler=%~2
set runtime=%~3
set vcpkg_dir=
set vcpkg_triplet=

if /i "%arch%" equ "" set arch=x86
if /i "%arch%" equ "Win32" set arch=x86
echo _x86_x64_arm64_ | findstr /I "_%arch%_" > nul
if %errorlevel% neq 0 echo ERROR: Unexpected architecture. Use Win32^|x64^|arm64&& exit /b -57

if "%compiler%" equ "" set compiler=mingw
echo _msbuild_mingw_ | findstr /I "_%compiler%_" > nul
if %errorlevel% neq 0 echo ERROR: Unexpected compiler. Use msbuild^|mingw&& exit /b -57

if "%runtime%" equ "" set runtime=static
echo _static_dynamic_ | findstr /I "_%runtime%_" > nul
if %errorlevel% neq 0 echo ERROR: Unexpected linkage type. Use static^|dynamic&& exit /b -57

if /i "%compiler%" equ "mingw" set vcpkg_triplet=%arch%-mingw-%runtime%
if /i "%compiler%" equ "msbuild" if /i "%runtime%" equ "static"  set vcpkg_triplet=%arch%-windows-%runtime%
if /i "%compiler%" equ "msbuild" if /i "%runtime%" equ "dynamic" set vcpkg_triplet=%arch%-windows

set vcpkg_dir=%~dp0clone

REM | -------------------------------------------------------------------------

if /i "%compiler%" equ "mingw" if /i "%arch%" equ "x64" if exist "%SystemDrive%\msys64\mingw64\bin" set PATH=%SystemDrive%\msys64\mingw64\bin;%PATH%
if /i "%compiler%" equ "mingw" if /i "%arch%" equ "x86" if exist "%SystemDrive%\msys64\mingw32\bin" set PATH=%SystemDrive%\msys64\mingw32\bin;%PATH%

if /i "%compiler%" equ "mingw" if /i "%arch%" equ "x64" if exist "%SystemDrive%\msys2\mingw64\bin"  set PATH=%SystemDrive%\msys2\mingw64\bin;%PATH%
if /i "%compiler%" equ "mingw" if /i "%arch%" equ "x86" if exist "%SystemDrive%\msys2\mingw32\bin"  set PATH=%SystemDrive%\msys2\mingw32\bin;%PATH%

REM | -------------------------------------------------------------------------

if exist "%vcpkg_dir%\.gitignore" (
    echo Pull https://github.com/Microsoft/vcpkg.git ...
    pushd "%vcpkg_dir%"
    git pull --verbose || exit /b !errorlevel!
    echo Bootstrapping vcpkg ...
    call "%vcpkg_dir%\bootstrap-vcpkg.bat" -disableMetrics || exit /b !errorlevel!
    popd
) else (
    echo Clone https://github.com/Microsoft/vcpkg.git ...
    pushd "%vcpkg_dir%\.."
    git clone https://github.com/Microsoft/vcpkg.git "%vcpkg_dir%" || exit /b !errorlevel!
    echo Bootstrapping vcpkg ...
    call "%vcpkg_dir%\bootstrap-vcpkg.bat" -disableMetrics || exit /b !errorlevel!
    popd
)

REM | -------------------------------------------------------------------------

set vcpkg_installed=%vcpkg_dir%\installed
set vcpkg_downloads=%vcpkg_dir%\downloads
set vcpkg_buildtrees=%vcpkg_dir%\buildtrees
set vcpkg_packages=%vcpkg_dir%\packages
REM set vcpkg_archives=%vcpkg_dir%\archives
set vcpkg_archives=%~dp0archives

echo -------------------------------------------------------------------------
echo Build ^& install vcpkg ...
echo -------------------------------------------------------------------------

mkdir "%vcpkg_dir%" > nul 2> nul
pushd "%vcpkg_dir%"

vcpkg.exe ^
  install ^
  --triplet=%vcpkg_triplet% ^
  --x-install-root="%vcpkg_installed%" ^
  --downloads-root="%vcpkg_downloads%" ^
  --x-buildtrees-root="%vcpkg_buildtrees%" ^
  --binarysource=clear;files,"%vcpkg_archives%",readwrite ^
  --x-packages-root="%vcpkg_packages%" ^
  --no-print-usage

popd

echo -------------------------------------------------------------------------
exit /b %errorlevel%
