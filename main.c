
//? Marius Negrutiu (marius.negrutiu@protonmail.com) :: 2014/01/19

#include "main.h"
#include "utils.h"


HINSTANCE g_hInst = NULL;


//++ PluginInit
BOOL PluginInit( _In_ HINSTANCE hInst )
{
	if (!g_hInst) {

		TRACE( _T( "PluginInit\n" ) );
		g_hInst = hInst;

		// Utils
		UtilsInitialize();

		// TODO: Initialize engine
		return TRUE;
	}
	return FALSE;
}


//++ PluginUninit
BOOL PluginUninit()
{
	if ( g_hInst ) {

		TRACE( _T( "PluginUninit\n" ) );

		// TODO: Stop engine

		UtilsDestroy();

		g_hInst = NULL;
		return TRUE;
	}
	return FALSE;
}


//++ UnloadCallback
//+  USAGE: extra->RegisterPluginCallback( g_hInst, UnloadCallback );
//?  By registering UnloadCallback the framework will keep the current plugin locked in memory
//?  Otherwise, unless the caller specifies the /NOUNLOAD parameter, the plugin gets unloaded
UINT_PTR __cdecl UnloadCallback( enum NSPIM iMessage )
{
	switch ( iMessage ) {
		case NSPIM_UNLOAD:		TRACE( _T( "%hs( NSPIM_UNLOAD )\n" ),    __FUNCTION__ ); break;
		case NSPIM_GUIUNLOAD:	TRACE( _T( "%hs( NSPIM_GUIUNLOAD )\n" ), __FUNCTION__ ); break;
	}
	return 0;
}


//++ ExtractCacertPem
//?  Extracts $PLUGINSDIR\cacert.pem from plugin's resource block
//?  If the file already exists the does nothing
ULONG ExtractCacertPem()
{
	ULONG e = ERROR_SUCCESS;

	assert( g_hInst != NULL );
	assert( g_variables != NULL );

	{
		TCHAR szPem[MAX_PATH];
		_sntprintf( szPem, ARRAYSIZE( szPem ), _T( "%s\\cacert.pem" ), getuservariableEx( INST_PLUGINSDIR ) );
		if (!FileExists( szPem ))
			e = ExtractResourceFile( (HMODULE)g_hInst, _T( "cacert.pem" ), MAKEINTRESOURCE( 1 ), 1033, szPem );
	}

	return e;
}


//++ Test
EXTERN_C __declspec(dllexport)
void __cdecl Test(
	HWND   parent,
	int    string_size,
	TCHAR   *variables,
	stack_t **stacktop,
	extra_parameters *extra
	)
{
	LPTSTR psz;

	EXDLL_INIT();
	EXDLL_VALIDATE();

	psz = (LPTSTR)MyAlloc( string_size * sizeof( TCHAR ) );
	assert( psz );

	for (;;) {
		if (popstring( psz ) != 0)
			break;
		if (lstrcmpi( psz, _T( "/END" ) ) == 0)
			break;
	}

	MyFree( psz );
}


//++ DllMain
EXTERN_C
BOOL WINAPI DllMain( HMODULE hInst, UINT iReason, LPVOID lpReserved )
{
	if ( iReason == DLL_PROCESS_ATTACH ) {
		PluginInit( hInst );
	} else if ( iReason == DLL_PROCESS_DETACH ) {
		PluginUninit();
	}
	return TRUE;
}
