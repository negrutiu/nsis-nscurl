setlocal enabledelayedexpansion

set outdir=%~dp0src/nscurl

echo Downloading curl-ca-bundle.crt ...
curl --no-progress-meter -L -f -o "%outdir%/curl-ca-bundle.crt" -z "%outdir%/curl-ca-bundle.crt" https://curl.se/ca/cacert.pem || exit /b !errorlevel!

echo.
