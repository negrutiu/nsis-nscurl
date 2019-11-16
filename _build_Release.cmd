REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
echo.

:CHDIR
cd /d "%~dp0"

:pluginapi
call _acquire_pluginapi.bat
if %errorlevel% neq 0 exit /B %errorlevel%

:DEFINITIONS
if not exist "%PF%" set PF=%PROGRAMFILES(X86)%
if not exist "%PF%" set PF=%PROGRAMFILES%

set BUILD_SOLUTION=%CD%\NScurl.sln
set BUILD_CONFIG=Release
set BUILD_VERBOSITY=normal
:: Verbosity: quiet, minimal, normal, detailed, diagnostic

:COMPILER
set VSWHERE=%PF%\Microsoft Visual Studio\Installer\vswhere.exe
if exist "%VSWHERE%" (
	for /f "usebackq tokens=1* delims=: " %%i in (`"%VSWHERE%" -version 15 -requires Microsoft.Component.MSBuild`) do (
		if /i "%%i"=="installationPath" (
			set VCVARSALL=%%j\VC\Auxiliary\Build\VCVarsAll.bat
			set BUILD_PLATFORMTOOLSET=v141
		)
	)
)
if exist "%VCVARSALL%" goto :BUILD

set VCVARSALL=%PF%\Microsoft Visual Studio 14.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v140
if exist "%VCVARSALL%" goto :BUILD

set VCVARSALL=%PF%\Microsoft Visual Studio 12.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v120
if exist "%VCVARSALL%" goto :BUILD

set VCVARSALL=%PF%\Microsoft Visual Studio 11.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v110
if exist "%VCVARSALL%" goto :BUILD

set VCVARSALL=%PF%\Microsoft Visual Studio 10.0\VC\VcVarsAll.bat
set BUILD_PLATFORMTOOLSET=v100
if exist "%VCVARSALL%" goto :BUILD

echo ERROR: Can't find Visual Studio 2010/2012/2013/2015/2017
pause
exit /B 2

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
