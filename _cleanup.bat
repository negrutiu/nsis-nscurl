REM :: Marius Negrutiu (marius.negrutiu@protonmail.com)

@echo off
echo.

cd /d "%~dp0"

call "%CD%\Test\cleanup.bat"
call :CLEANUP
call :CLEANUP
call :CLEANUP
goto :EOF


:CLEANUP
rd /S /Q .vs
rd /S /Q ipch

for /D %%a in (Debug*)   do rd /S /Q "%%a"
for /D %%a in (Release*) do rd /S /Q "%%a"
rd /S /Q nsis

del *.aps
del *.bak
::del *.user
del *.ncb
del /AH *.suo
del *.sdf
del *.VC.db
