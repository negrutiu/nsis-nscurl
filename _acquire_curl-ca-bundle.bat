setlocal enabledelayedexpansion

echo Downloading curl-ca-bundle.crt ...
curl --no-progress-meter -L -f -o "%~dp0/curl-ca-bundle.crt" -z "%~dp0/curl-ca-bundle.crt" https://curl.se/ca/cacert.pem || exit /b !errorlevel!

echo.
