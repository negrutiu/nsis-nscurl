setlocal enabledelayedexpansion

if exist "%~dp0libcurl-devel" echo Use existing libcurl-devel && echo. && exit /b

set Z7=%PROGRAMFILES%\7-Zip\7z.exe
if not exist "%Z7%" echo ERROR: Missing "%Z7%" && exit /b 2

echo Downloading libcurl-devel ...

curl --no-progress-meter -L -f ^
-o "%~dp0libcurl-devel-negrutiu.7z" ^
--etag-save "%~dp0libcurl-devel-negrutiu.7z.etag" ^
--etag-compare "%~dp0libcurl-devel-negrutiu.7z.etag" ^
https://github.com/negrutiu/libcurl-devel/releases/latest/download/libcurl-devel-negrutiu.7z || exit /b !errorlevel!

echo.
echo Extracting libcurl-devel ...

"%Z7%" x "%~dp0libcurl-devel-negrutiu.7z" "-o%~dp0libcurl-devel" * -r || exit /b !errorlevel!
echo.