target = 'NScurl'

files = Split("""
	crypto.c
	curl.c
	gui.c
	main.c
	queue.c
	utils.c
""")

resources = Split("""
	resource.rc
""")

# NOTE: Library order does matter!
libs = Split("""
	curl
    ssl
    crypto
    nghttp2_static
    zlibstatic
	advapi32
	user32
	version
	ws2_32
    crypt32
	mingwex
	msvcrt
	gcc
	kernel32
""")

examples = Split("""
	Test/NScurl-Test.nsi
	Test/NScurl-Test-build.bat
""")

docs = Split("""
	NScurl.Readme.htm
""")

Import('BuildPlugin env plugin_env plugin_uenv')

unicodetarget = 'UNICODE' in env['CPPDEFINES']
plugin_envT = plugin_env
if unicodetarget:
	plugin_envT = plugin_uenv

plugin_envT.Append(CPPPATH = ['libcurl-devel/include', 'libcurl-devel/include/openssl'])
if env['TARGET_ARCH'] == 'amd64':
	plugin_envT.Append(LIBPATH = ['libcurl-devel/mingw-curl_openssl-Release-x64-Legacy/lib'])
else:
	plugin_envT.Append(LIBPATH = ['libcurl-devel/mingw-curl_openssl-Release-Win32-Legacy/lib'])

# Disable benign warnings
plugin_envT.Append(CCFLAGS = ['-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-but-set-variable'])

# print( '----------------------------------------------------------------------' )
# print( plugin_envT.Dump() )
# print( '----------------------------------------------------------------------' )

BuildPlugin(target, files, libs, examples, docs, res = resources)
