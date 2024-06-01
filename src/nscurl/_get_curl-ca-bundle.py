# Download curl-ca-bundle.crt from Curl website

from pathlib import Path
from urllib import request
import ssl
from pip._vendor import certifi     # use pip certifi to fix "Let's Encrypt" issues (urllib.error.URLError: <urlopen error [SSL: CERTIFICATE_VERIFY_FAILED] certificate verify failed: certificate has expired (_ssl.c:1006)>)
import datetime

url = 'https://curl.se/ca/cacert.pem'
file = Path(__file__).parent.joinpath('curl-ca-bundle.crt')

print(f"Download {url} ...")

headers = {}
if file.exists():
    dt = datetime.datetime.fromtimestamp(file.stat().st_mtime, tz=datetime.timezone.utc)
    headers['If-Modified-Since'] = dt.strftime('%a, %d %b %Y %H:%M:%S GMT')
    print(f"  If-Modified-Since: {headers['If-Modified-Since']}")

sslctx = ssl.create_default_context(cafile=certifi.where())
try:
    with request.urlopen(request.Request(url, headers=headers), context=sslctx) as http:
        file.parent.mkdir(parents=True, exist_ok=True)
        with open(file, 'wb') as outfile:
            outfile.write(http.read())
        print(f"  {http.status} {http.reason}, {http.getheader('Content-Length')} bytes")
except request.HTTPError as ex:
    if ex.code == 304:
        print(f"  {ex.code} {ex.reason}, {file.stat().st_size} bytes, the file is already up-to-date")
    else:
        raise
