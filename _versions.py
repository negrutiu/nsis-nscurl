import argparse
from subprocess import Popen, PIPE
import re
import json

def get_resource_version(filename):
    with open(filename, 'rb') as fin:
        for line in fin.read().decode('utf-8').split("\r\n"):
            # print(line)
            match = re.match(r'^\s*VALUE\s*"FileVersion"\s*,\s*"(.*)"\s*$', line)
            if match != None:
                return match[1]
    return ""

def get_gcc_version(gccPath):
    try:
        process = Popen([gccPath, "-v"], stdout=PIPE, stderr=PIPE)
        (cout, cerr) = process.communicate()
        exit_code = process.wait()

        # possible examples:
        # "gcc version 14.1.0 (Rev3, Built by MSYS2 project)"
        # "gcc version 13.1.0 (MinGW-W64 x86_64-msvcrt-posix-seh, built by anonymous)"

        # if cout != None:
        #     for line in cout.decode('utf-8').split("\r\n"):
        #         print(f"{line}")
        if cerr != None:
            for line in cerr.decode('utf-8').split("\r\n"):
                match = re.match(r'^gcc version (.+)\s\(', line)
                if match != None:
                    version = match[1]
                    match = re.search(r'\(Rev(\d+),', line)
                    if match != None:
                        version += '-' + match[1]
                    return version
        return ""
    except:
        return ""

def get_curl_engines_versions(curlPath):
    try:
        process = Popen([curlPath, "--version"], stdout=PIPE, stderr=PIPE)
        (cout, cerr) = process.communicate()
        exit_code = process.wait()

        # possible outputs:
        # "curl 7.70.0 (x86_64-pc-win32) libcurl/7.70.0 OpenSSL/1.1.1g (Schannel) zlib/1.2.11 brotli/1.0.7 WinIDN libssh2/1.9.0 nghttp2/1.41.0"
        # "curl 8.8.0 (x86_64-windows-gnu) libcurl/8.8.0 OpenSSL/3.3.0 zlib/1.3.1 brotli/1.1.0 zstd/1.5.6 nghttp2/1.62.1"
        # "curl 8.4.0 (Windows) libcurl/8.4.0 Schannel WinIDN"
        if cout != None:
            for line in cout.decode('utf-8').split("\r\n"):
                # print(f"out: {line}")
                if re.match(r'^curl [\d\.]+\s', line) != None:
                    versions = {}
                    for group in re.findall(r'(\S+\/\S+)', line):
                        versions[group.split('/')[0].lower()] = group.split('/')[1]
                    return versions
        if cerr != None:
            for line in cerr.decode('utf-8').split("\r\n"):
                print(f"err: {line}")
        return {}
    except:
        return {}

def get_curl_versions(curlPath, markdown=False):
    versions = ''
    for name, version in get_curl_engines_versions(curlPath).items():
        if markdown:
            versions += f"`{name}/{version}`, "
        else:
            versions += f"{name}/{version} "
    return versions.removesuffix(', ')

parser = argparse.ArgumentParser()
parser.add_argument("-c", "--curl", type=str, default="curl.exe")
parser.add_argument("-g", "--gcc", type=str, default="gcc.exe")
parser.add_argument("-i", "--indent", type=int, default=None)
args = parser.parse_args()

versions = {
    "nscurl": get_resource_version('src/nscurl/resource.rc'),
    "curl": get_curl_versions(args.curl),
    "curl_md": get_curl_versions(args.curl, True),
    "engines": get_curl_engines_versions(args.curl),
    "gcc": get_gcc_version(args.gcc),
}
print(json.dumps(versions, indent=args.indent))
