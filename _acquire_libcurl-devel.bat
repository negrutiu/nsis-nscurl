@echo off
SetLocal

set Z7=%PROGRAMFILES%\7-Zip\7z.exe
if not exist "%Z7%" echo ERROR: Missing "%Z7%" && pause && exit /B 2

curl.exe -V > NUL 2> NUL
if %errorlevel% neq 0 echo ERROR: Missing curl.exe && pause && exit /B 2

set dn=%~dp0\libcurl-devel
set fn=%~dp0\libcurl-devel-negrutiu.7z
set url=https://github.com/negrutiu/libcurl-devel/releases/latest/download/libcurl-devel-negrutiu.7z

echo Downloading %url% ...
for /f "" %%h in ('curl.exe -L -z "%fn%" -o "%fn%" -w "%%{response_code}" %url% 2^> NUL') do set httpstatus=%%h
if %errorlevel% neq 0 pause && exit /B %errorlevel%
echo HTTP %httpstatus%
echo.

if %httpstatus% geq 200 if %httpstatus% lss 300 rmdir /S /Q "%dn%" > NUL 2> NUL
REM if %httpstatus% geq 300 if %httpstatus% lss 400 echo Action 3xx
if %httpstatus% geq 400 pause && exit /B 1

if not exist "%dn%" "%Z7%" x "%fn%" "-o%dn%" * -r || echo ERROR: Failed to extract "%fn%" && pause && exit /B 7
