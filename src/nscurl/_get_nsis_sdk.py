# Download NSIS SDK files from GitHub

from pathlib import Path
from urllib import request

cd = Path(__file__).parent
files = {
    cd.joinpath('nsis', 'api.h')        : "https://raw.githubusercontent.com/kichik/nsis/master/Source/exehead/api.h",
    cd.joinpath('nsis', 'nsis_tchar.h') : "https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/nsis_tchar.h",
    cd.joinpath('nsis', 'pluginapi.c')  : "https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/pluginapi.c",
    cd.joinpath('nsis', 'pluginapi.h')  : "https://raw.githubusercontent.com/kichik/nsis/master/Contrib/ExDLL/pluginapi.h",
}

download = False
for file in files:
    if not file.exists(): download = True

if download:
    print("Downloading NSIS SDK ...")
    for file, url in files.items():
        print(file.name)
        with request.urlopen(url) as http:
            file.parent.mkdir(parents=True, exist_ok=True)
            with open(file, 'wb') as outfile:
                outfile.write(http.read())
            print(f"  {http.status} {http.reason}, {http.getheader('Content-Length')} bytes")
else:
    print("Use existing NSIS SDK ...")