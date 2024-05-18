@echo off
setlocal enabledelayedexpansion

set outdir=%~dp0

echo Downloading https://curl.se/ca/cacert.pem ...
curl --no-progress-meter -L -f -o "%outdir%/curl-ca-bundle.crt" -z "%outdir%/curl-ca-bundle.crt" https://curl.se/ca/cacert.pem || exit /b !errorlevel!
