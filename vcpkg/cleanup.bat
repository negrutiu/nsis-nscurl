
@echo off

call :clean
call :clean
call :clean
exit /b


:clean
echo.
pushd "%~dp0"

rd /q /s archives
rd /q /s clone
rd /q /s latest

popd
exit /b