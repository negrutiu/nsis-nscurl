REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
setlocal enabledelayedexpansion

for /f "usebackq delims=*" %%s in (`py -c "from datetime import datetime; print(datetime.now())"`) do set t0=%%~s
echo. && echo -- %t0%

call "%~dp0_build.bat" msbuild release x86 unicode || exit /b !errorlevel!
call "%~dp0_build.bat" msbuild release x86 ansi    || exit /b !errorlevel!
call "%~dp0_build.bat" msbuild release x64 unicode || exit /b !errorlevel!

for /f "usebackq delims=*" %%s in (`py -c "from datetime import datetime; now = datetime.now(); dt = now-datetime.fromisoformat('%t0%'); print(f'{now} (+{dt})')"`) do set t1=%%~s
echo. && echo -- %t1%
