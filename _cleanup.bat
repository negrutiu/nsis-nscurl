REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
echo.

cd /d "%~dp0"

call "%cd%\tests\cleanup.bat"

call :CLEANUP
call :CLEANUP
call :CLEANUP
goto :EOF


:CLEANUP
rd /S /Q .vs
rd /S /Q ipch

for /D %%a in (Debug*)   do rd /S /Q "%%a"
for /D %%a in (Release*) do rd /S /Q "%%a"

rd  /Q /S "src\nscurl\nsis"
del /Q    "src\nscurl\curl-ca-bundle.crt"
rd  /Q /S "packages\current"

del *.aps
del *.bak
::del *.user
del *.ncb
del /AH *.suo
del *.sdf
del *.VC.db
