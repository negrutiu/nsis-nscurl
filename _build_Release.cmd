REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
setlocal enabledelayedexpansion
echo.

REM | script.bat [Release|Debug]

:CHDIR
cd /d "%~dp0"

:dependencies
call _acquire_pluginapi.bat      || exit /b !errorlevel!
call _acquire_curl-ca-bundle.bat || exit /b !errorlevel!

REM | https://stackoverflow.com/questions/33584587/how-to-wait-all-batch-files-to-finish-before-exiting
echo Building vcpkg ...
(
   start "vcpkg x86" cmd /C _build_vcpkg.bat Win32 msvc
   start "vcpkg x64" cmd /C _build_vcpkg.bat x64 msvc
) | set /P "="


:DEFINITIONS
set BUILD_SOLUTION=%CD%\NScurl.sln
if "%1" neq "" set BUILD_CONFIG=%~1
if "%1" equ "" set BUILD_CONFIG=Release
set BUILD_VERBOSITY=normal
:: Verbosity: quiet, minimal, normal, detailed, diagnostic

:COMPILER
if not exist "%PF%" set PF=%PROGRAMFILES(X86)%
if not exist "%PF%" set PF=%PROGRAMFILES%
set VSWHERE=%PF%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VCVARSALL%" for /f "tokens=1* delims=: " %%i in ('"%VSWHERE%" -version 17 -requires Microsoft.Component.MSBuild 2^> NUL') do if /i "%%i"=="installationPath" set VCVARSALL=%%j\VC\Auxiliary\Build\VCVarsAll.bat&& set BUILD_PLATFORMTOOLSET=v143
if not exist "%VCVARSALL%" for /f "tokens=1* delims=: " %%i in ('"%VSWHERE%" -version 16 -requires Microsoft.Component.MSBuild 2^> NUL') do if /i "%%i"=="installationPath" set VCVARSALL=%%j\VC\Auxiliary\Build\VCVarsAll.bat&& set BUILD_PLATFORMTOOLSET=v142
if not exist "%VCVARSALL%" for /f "tokens=1* delims=: " %%i in ('"%VSWHERE%" -version 15 -requires Microsoft.Component.MSBuild 2^> NUL') do if /i "%%i"=="installationPath" set VCVARSALL=%%j\VC\Auxiliary\Build\VCVarsAll.bat&& set BUILD_PLATFORMTOOLSET=v141
if not exist "%VCVARSALL%" set VCVARSALL=%PF%\Microsoft Visual Studio 14.0\VC\VcVarsAll.bat&& set BUILD_PLATFORMTOOLSET=v140
if not exist "%VCVARSALL%" set VCVARSALL=%PF%\Microsoft Visual Studio 12.0\VC\VcVarsAll.bat&& set BUILD_PLATFORMTOOLSET=v120
if not exist "%VCVARSALL%" set VCVARSALL=%PF%\Microsoft Visual Studio 11.0\VC\VcVarsAll.bat&& set BUILD_PLATFORMTOOLSET=v110
if not exist "%VCVARSALL%" set VCVARSALL=%PF%\Microsoft Visual Studio 10.0\VC\VcVarsAll.bat&& set BUILD_PLATFORMTOOLSET=v100
if not exist "%VCVARSALL%" echo ERROR: Can't find Visual Studio 2010-2022 && pause && exit /B 2

:BUILD
pushd "%CD%"
call "%VCVARSALL%" x86
popd

title %BUILD_CONFIG%-x86-ansi
msbuild /m /t:build "%BUILD_SOLUTION%" /p:Configuration=%BUILD_CONFIG%-x86-ansi /p:Platform=Win32 /p:PlatformToolset=%BUILD_PLATFORMTOOLSET% /p:WindowsTargetPlatformVersion=%WindowsSDKVersion% /nologo /verbosity:%BUILD_VERBOSITY%
if %errorlevel% neq 0 pause && exit /B %errorlevel%

title %BUILD_CONFIG%-x86-unicode
msbuild /m /t:build "%BUILD_SOLUTION%" /p:Configuration=%BUILD_CONFIG%-x86-unicode /p:Platform=Win32 /p:PlatformToolset=%BUILD_PLATFORMTOOLSET% /p:WindowsTargetPlatformVersion=%WindowsSDKVersion% /nologo /verbosity:%BUILD_VERBOSITY%
if %errorlevel% neq 0 pause && exit /B %errorlevel%
