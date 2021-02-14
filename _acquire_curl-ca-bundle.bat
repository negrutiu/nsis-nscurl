@echo off
SetLocal

curl.exe -V > NUL 2> NUL
if %errorlevel% neq 0 echo ERROR: Missing curl.exe && pause && exit /B 2

set fn=%~dp0\curl-ca-bundle.crt
set url=https://curl.se/ca/cacert.pem
echo Downloading %url% ...
curl.exe -L -z "%fn%" -o "%fn%" --no-progress-meter -w "HTTP %%{response_code}" %url%
echo.
if %errorlevel% neq 0 pause && exit /B %errorlevel%
