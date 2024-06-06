REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
echo.

cd /d "%~dp0"

call tests\cleanup.bat
call vcpkg\cleanup.bat

call :clean
call :clean
call :clean

exit /b


:clean
rd /S /Q .vs
rd /S /Q ipch

for /D %%a in (Debug-*)   do rd /S /Q "%%~a"
for /D %%a in (Release-*) do rd /S /Q "%%~a"

rd  /Q /S "src\nscurl\nsis"
del /Q    "src\nscurl\curl-ca-bundle.crt"

rd  /Q /S "packages\current"
for /D %%a in (packages\Debug-*)   do rd /S /Q "%%~a"
for /D %%a in (packages\Release-*) do rd /S /Q "%%~a"

del *.aps
del *.bak
::del *.user
del *.ncb
del /AH *.suo
del *.sdf
del *.VC.db
exit /b