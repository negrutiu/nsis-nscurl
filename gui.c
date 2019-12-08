
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/12/08
//? User Interface

#include "main.h"
#include "gui.h"
#include "utils.h"


//++ GuiInitialize
void GuiInitialize()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );
}


//++ GuiDestroy
void GuiDestroy()
{
	TRACE( _T( "%hs()\n" ), __FUNCTION__ );
}


//++ GuiParseRequestParam
BOOL GuiParseRequestParam( _In_ LPTSTR pszParam, _In_ int iParamMaxLen, _Out_ PGUI_REQUEST pGui )
{
	BOOL bRet = TRUE;
	assert( iParamMaxLen && pszParam && pGui );

	if (lstrcmpi( pszParam, _T( "/BACKGROUND" ) ) == 0) {
		pGui->bBackground = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/SILENT" ) ) == 0) {
		pGui->bSilent = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/POPUP" ) ) == 0) {
		pGui->bPopup = TRUE;
	} else {
		bRet = FALSE;
	}

	return bRet;
}


//++ GuiWait
void GuiWait( _Inout_ PGUI_REQUEST pGui, _Out_ LPTSTR pszResult, _In_ int iResultMaxLen )
{
	assert( pGui && pszResult && iResultMaxLen );

	if (!pGui->bBackground) {

		while (TRUE) {
			ULONG n;
			QueueLock();
			n = QueueCount( STATUS_RUNNING, pGui->iId );
			QueueUnlock();
			TRACE( _T( "Waiting( Count:%u )\n" ), n );
			if (n > 0) {
				// TODO: Refresh( pGui )
				Sleep( 200 );
			} else {
				break;
			}
		}

		if (pGui->iId != QUEUE_NO_ID) {
			//? Wait for ID: Return status
			lstrcpyn( pszResult, _T( "@ERROR@" ), iResultMaxLen );
			QueueQuery( pGui->iId, pszResult, iResultMaxLen );
		} else {
			//? Wait for all: Return OK
			lstrcpyn( pszResult, _T( "OK" ), iResultMaxLen );
		}

	} else {
		//? Background: Return request ID
		assert( pGui->iId != QUEUE_NO_ID );
		_sntprintf( pszResult, iResultMaxLen, _T( "%u" ), pGui->iId );
	}
}
