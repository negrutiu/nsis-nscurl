REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
setlocal enabledelayedexpansion

call "%~dp0_build.bat" mingw release x86 unicode || exit /b !errorlevel!
call "%~dp0_build.bat" mingw release x86 ansi    || exit /b !errorlevel!
call "%~dp0_build.bat" mingw release x64 unicode || exit /b !errorlevel!
