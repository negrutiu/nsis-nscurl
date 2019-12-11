
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/12/08
//? User Interface

#include "main.h"
#include "gui.h"
#include "utils.h"
#include <CommCtrl.h>


#define MY_PBS_MARQUEE			0x08				/// XP+
#define MY_PBM_SETMARQUEE		(WM_USER+10)


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
	} else if (lstrcmpi( pszParam, _T( "/TITLEWND" ) ) == 0) {
		pGui->hTitle = (HWND)popintptr();
	} else if (lstrcmpi( pszParam, _T( "/TEXTWND" ) ) == 0) {
		pGui->hText = (HWND)popintptr();
	} else if (lstrcmpi( pszParam, _T( "/PROGRESSWND" ) ) == 0) {
		pGui->hProgress = (HWND)popintptr();
	} else {
		bRet = FALSE;
	}

	return bRet;
}


//+ [internal] GuiQueryKeywordCallback
void CALLBACK GuiQueryKeywordCallback( _Inout_ LPTSTR pszKeyword, _In_ ULONG iMaxLen, _In_ PVOID pParam )
{
	PGUI_REQUEST pGui = (PGUI_REQUEST)pParam;
	assert( pszKeyword );
	assert( pGui );

	if (lstrcmpi( pszKeyword, _T( "@TITLE0@" ) ) == 0) {
		lstrcpyn( pszKeyword, pGui->Runtime.pszTitle0 ? pGui->Runtime.pszTitle0 : _T( "" ), iMaxLen );
	} else if (lstrcmpi( pszKeyword, _T( "@TEXT0@" ) ) == 0) {
		lstrcpyn( pszKeyword, pGui->Runtime.pszText0 ? pGui->Runtime.pszText0 : _T( "" ), iMaxLen );
	} else if (lstrcmpi( pszKeyword, _T( "@ANIMDOTS@" ) ) == 0) {
		int i = (pGui->Runtime.iAnimIndex++ % 4);
		if (i == 0) {
			lstrcpyn( pszKeyword, _T( "" ), iMaxLen );
		} else if (i == 1) {
			lstrcpyn( pszKeyword, _T( "." ), iMaxLen );
		} else if (i == 2) {
			lstrcpyn( pszKeyword, _T( ".." ), iMaxLen );
		} else if (i == 3) {
			lstrcpyn( pszKeyword, _T( "..." ), iMaxLen );
		}
	} else if (lstrcmpi( pszKeyword, _T( "@ANIMLINE@" ) ) == 0) {
		int i = (pGui->Runtime.iAnimIndex++ % 4);
		if (i == 0) {
			lstrcpyn( pszKeyword, _T( "|" ), iMaxLen );
		} else if (i == 1) {
			lstrcpyn( pszKeyword, _T( "/" ), iMaxLen );
		} else if (i == 2) {
			lstrcpyn( pszKeyword, _T( "-" ), iMaxLen );
		} else if (i == 3) {
			lstrcpyn( pszKeyword, _T( "\\" ), iMaxLen );
		}
	}
}


//++ GuiQuery
LONG GuiQuery( _Inout_ PGUI_REQUEST pGui, _Inout_ LPTSTR pszStr, _In_ LONG iStrMaxLen )
{
	LONG iStrLen = -1;
	if (pszStr && iStrMaxLen) {

		// Replace queue keywords
		MainQuery( pGui->Runtime.iId, pszStr, iStrMaxLen );

		// Replace global keywords
		iStrLen = MyReplaceKeywords( pszStr, iStrMaxLen, _T( '@' ), _T( '@' ), GuiQueryKeywordCallback, pGui );
	}
	return iStrLen;
}


//++ GuiWaitLoop
void GuiWaitLoop( _Inout_ PGUI_REQUEST pGui )
{
	MSG msg;
	HANDLE hDummyEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	ULONG t0 = 0;
	QUEUE_STATS qs;

	// First paint
	GuiRefresh( pGui );

	#define HEARTBEAT 200
	while (TRUE) {

		ResetEvent( hDummyEvent );
		MsgWaitForMultipleObjects( 1, &hDummyEvent, FALSE, HEARTBEAT + 10, QS_ALLEVENTS );

		while (PeekMessage( &msg, NULL, 0, 0, PM_REMOVE )) {
			if (!IsDialogMessage( g_hwndparent, &msg )) {
				TranslateMessage( &msg );
				DispatchMessage( &msg );
			}
		}

		if (GetTickCount() - t0 >= HEARTBEAT) {

			QueueLock();
			QueueStatistics( pGui->iId, &qs );
			QueueUnlock();

			TRACE( _T( "Waiting( Count:%u )\n" ), qs.iWaiting + qs.iRunning );
			if (qs.iWaiting + qs.iRunning > 0) {

				// Display latest info
				GuiRefresh( pGui );

			} else {
				break;
			}

			t0 = GetTickCount();
		}

	}	/// while
	#undef HEARTBEAT

	CloseHandle( hDummyEvent );
}


//++ GuiSilentWait
BOOLEAN GuiSilentWait( _Inout_ PGUI_REQUEST pGui )
{
	TRACE( _T( "%hs\n" ), __FUNCTION__ );
	GuiWaitLoop( pGui );
	return TRUE;
}


//++ GuiPopupWait
BOOLEAN GuiPopupWait( _Inout_ PGUI_REQUEST pGui )
{
	TRACE( _T( "%hs\n" ), __FUNCTION__ );
	return FALSE;
}


//++ GuiPageWait
BOOLEAN GuiPageWait( _Inout_ PGUI_REQUEST pGui )
{
	HWND hDetailsBtn = NULL, hDetailsList = NULL;
	RECT rcDetailsBtn, rcDetailsList;
	int iDetailsOffsetY = 0;

	TRACE( _T( "%hs\n" ), __FUNCTION__ );

	// Reset invalid parameters
	if (pGui->hTitle && !IsWindow( pGui->hTitle ))
		pGui->hTitle = NULL;
	if (pGui->hText && !IsWindow( pGui->hText ))
		pGui->hText = NULL;
	if (pGui->hProgress && !IsWindow( pGui->hProgress ))
		pGui->hProgress = NULL;

	// Original Title text
	if (pGui->hTitle) {
		ULONG l = GetWindowTextLength( pGui->hTitle ) + 1;
		if ((pGui->Runtime.pszTitle0 = (LPTSTR)MyAlloc( l * sizeof(TCHAR) )) != NULL)
			GetWindowText( pGui->hTitle, pGui->Runtime.pszTitle0, l );
	}

	// Original Text text
	if (pGui->hText) {
		ULONG l = GetWindowTextLength( pGui->hText ) + 1;
		if ((pGui->Runtime.pszText0 = (LPTSTR)MyAlloc( l * sizeof(TCHAR) )) != NULL)
			GetWindowText( pGui->hText, pGui->Runtime.pszText0, l );
	}

	if (/*pGui->hTitle ||*/ pGui->hText || pGui->hProgress) {
		
		// Use caller-supplied controls, probably on a custom Page
		pGui->Runtime.hTitle = pGui->hTitle;
		pGui->Runtime.hText = pGui->hText;
		pGui->Runtime.hProgress = pGui->hProgress;

	} else {

		// Check if we're on InstFiles built-in page
		HWND hInstFilesPage = g_hwndparent ? FindWindowEx( g_hwndparent, NULL, _T( "#32770" ), NULL ) : NULL;
		if (hInstFilesPage) {

			// Status and Progress controls must exist
			HWND hNsisText = GetDlgItem( hInstFilesPage, 1006 );
			HWND hNsisProgress = GetDlgItem( hInstFilesPage, 1004 );
			if (hNsisText && hNsisProgress) {

				HWND hNewText = NULL, hNewProgress = NULL;
				RECT rcText, rcProgress, rcNewText, rcNewProgress;
				LONG iTextStyle, iTextStyleEx;
				LONG iProgressStyle, iProgressStyleEx;
				#define LTWH( rc ) (rc).left, (rc).top, (rc).right - (rc).left, (rc).bottom - (rc).top

				/// InstFiles page text control
				GetWindowRect( hNsisText, &rcText );
				ScreenToClient( hInstFilesPage, (LPPOINT)&rcText.left );
				ScreenToClient( hInstFilesPage, (LPPOINT)&rcText.right );
				iTextStyle = (LONG)GetWindowLongPtr( hNsisText, GWL_STYLE );
				iTextStyleEx = (LONG)GetWindowLongPtr( hNsisText, GWL_EXSTYLE );

				/// InstFiles page progress bar
				GetWindowRect( hNsisProgress, &rcProgress );
				ScreenToClient( hInstFilesPage, (LPPOINT)&rcProgress.left );
				ScreenToClient( hInstFilesPage, (LPPOINT)&rcProgress.right );
				iProgressStyle = (LONG)GetWindowLongPtr( hNsisProgress, GWL_STYLE );
				iProgressStyleEx = (LONG)GetWindowLongPtr( hNsisProgress, GWL_EXSTYLE );

				/// InstFiles page details button
				hDetailsBtn = GetDlgItem( hInstFilesPage, 1027 );
				if (hDetailsBtn) {
					GetWindowRect( hDetailsBtn, &rcDetailsBtn );
					ScreenToClient( hInstFilesPage, (LPPOINT)&rcDetailsBtn.left );
					ScreenToClient( hInstFilesPage, (LPPOINT)&rcDetailsBtn.right );
				}

				/// InstFiles page details list
				hDetailsList = GetDlgItem( hInstFilesPage, 1016 );
				if (hDetailsList) {
					GetWindowRect( hDetailsList, &rcDetailsList );
					ScreenToClient( hInstFilesPage, (LPPOINT)&rcDetailsList.left );
					ScreenToClient( hInstFilesPage, (LPPOINT)&rcDetailsList.right );
				}

				/// New text control
				CopyRect( &rcNewText, &rcText );
				OffsetRect( &rcNewText, 0, rcProgress.bottom + rcProgress.top - rcNewText.top );
				hNewText = CreateWindowEx( iTextStyleEx, WC_STATIC, _T( "" ), iTextStyle, LTWH( rcNewText ), hInstFilesPage, NULL, NULL, NULL );
				SendMessage( hNewText, WM_SETFONT, (WPARAM)SendMessage( hNsisText, WM_GETFONT, 0, 0 ), MAKELPARAM( FALSE, 0 ) );

				/// New progress bar
				CopyRect( &rcNewProgress, &rcProgress );
				OffsetRect( &rcNewProgress, 0, rcNewText.bottom + (rcText.bottom - rcProgress.top) - rcNewProgress.top );
				iProgressStyle |= MY_PBS_MARQUEE;		/// Marquee capability
				hNewProgress = CreateWindowEx( iProgressStyleEx, PROGRESS_CLASS, _T( "" ), iProgressStyle, LTWH( rcNewProgress ), hInstFilesPage, NULL, NULL, NULL );

				iDetailsOffsetY = rcNewProgress.bottom + (rcDetailsList.top - rcProgress.bottom) - rcDetailsList.top;

				/// Move details button
				if (hDetailsBtn) {
					OffsetRect( &rcDetailsBtn, 0, iDetailsOffsetY );
					SetWindowPos( hDetailsBtn, NULL, LTWH( rcDetailsBtn ), SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME );
				}

				/// Move details list
				if (hDetailsList) {
					rcDetailsList.top += iDetailsOffsetY;
					SetWindowPos( hDetailsList, NULL, LTWH( rcDetailsList ), SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME );
				}

				/// Use the new controls
				pGui->Runtime.hTitle = pGui->hTitle;
				pGui->Runtime.hText = hNewText;
				pGui->Runtime.hProgress = hNewProgress;
			}
		}
	}

	// Can we wait in Page-mode ?
	if (pGui->Runtime.hText || pGui->Runtime.hProgress) {

		// Wait
		GuiWaitLoop( pGui );

		// Restore controls
		if (pGui->Runtime.pszTitle0)
			SetWindowText( pGui->hTitle, pGui->Runtime.pszTitle0 );

		if (pGui->Runtime.pszText0)
			SetWindowText( pGui->hText, pGui->Runtime.pszText0 );

		if (pGui->Runtime.hText) {
			if (pGui->Runtime.hText != pGui->hText)				/// Don't destroy caller-supplied Text control
				DestroyWindow( pGui->Runtime.hText );
			pGui->Runtime.hText = NULL;
		}
		if (pGui->Runtime.hProgress) {
			if (pGui->Runtime.hProgress != pGui->hProgress)		/// Don't destroy caller-supplied Progress control
				DestroyWindow( pGui->Runtime.hProgress );
			pGui->Runtime.hProgress = NULL;
		}

		if (hDetailsBtn) {
			OffsetRect( &rcDetailsBtn, 0, -iDetailsOffsetY );
			SetWindowPos( hDetailsBtn, NULL, LTWH( rcDetailsBtn ), SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME );
		}
		if (hDetailsList) {
			rcDetailsList.top -= iDetailsOffsetY;
			SetWindowPos( hDetailsList, NULL, LTWH( rcDetailsList ), SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME );
		}

		return TRUE;
	}

	return FALSE;
}


//++ GuiWait
void GuiWait( _Inout_ PGUI_REQUEST pGui, _Out_ LPTSTR pszResult, _In_ int iResultMaxLen )
{
	assert( pGui && pszResult && iResultMaxLen );

	if (!pGui->bBackground) {

		// Initialize controls
		if (pGui->bSilent) {
			/// Silent
			GuiSilentWait( pGui );
		} else if (pGui->bPopup) {
			/// Try Popup
			if (!GuiPopupWait( pGui )) {
				/// Silent
				GuiSilentWait( pGui );
			}
		} else {
			/// Try Page
			if (!GuiPageWait( pGui )) {
				/// Silent
				GuiSilentWait( pGui );
			}
		}

		if (pGui->iId != QUEUE_NO_ID) {
			//? Wait for ID: Return status
			lstrcpyn( pszResult, _T( "@ERROR@" ), iResultMaxLen );
			GuiQuery( pGui, pszResult, iResultMaxLen );
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


//++ GuiRefresh
void GuiRefresh( _Inout_ PGUI_REQUEST pGui )
{
	ULONG iBufSize = 2048;
	LPTSTR pszBuf = NULL;
	QUEUE_STATS qs;
	LONG iPercent;

	assert( pGui );
	if (!pGui->Runtime.hTitle && !pGui->Runtime.hProgress && !pGui->Runtime.hText)
		return;

	pszBuf = (LPTSTR)MyAlloc( iBufSize * sizeof( TCHAR ) );
	if (!pszBuf)
		return;

	QueueLock();
	QueueStatistics( pGui->iId, &qs );

	if (qs.iRunning == 1 && qs.iWaiting == 0) {

		pGui->Runtime.iId = qs.iSingleID;

		// Single Running transfer
		lstrcpyn( pszBuf, _T( "@PERCENT@" ), iBufSize );
		GuiQuery( pGui, pszBuf, iBufSize );
		iPercent = myatoi( pszBuf );
		assert( iPercent >= -1 && iPercent <= 100 );

		if (pGui->Runtime.hTitle) {
			if (iPercent == -1) {
				lstrcpyn( pszBuf, _T( "@TITLE0@" ), iBufSize );
			} else {
				lstrcpyn( pszBuf, _T( "[@PERCENT@%] @TITLE0@" ), iBufSize );
			}
			GuiQuery( pGui, pszBuf, iBufSize );
			SetWindowText( pGui->Runtime.hTitle, pszBuf );
		}
		if (pGui->Runtime.hText) {
			if (iPercent == -1) {
				lstrcpyn( pszBuf, _T( "@OUTFILE@, @XFERSIZE@ @ @SPEED@ @ANIMDOTS@" ), iBufSize );
			} else {
				lstrcpyn( pszBuf, _T( "[@PERCENT@%] @OUTFILE@, @XFERSIZE@ / @FILESIZE@ @ @SPEED@ @ANIMDOTS@" ), iBufSize );
			}
			GuiQuery( pGui, pszBuf, iBufSize );
			SetWindowText( pGui->Runtime.hText, pszBuf );
		}

	} else {

		// Multiple or zero Running transfers
		iPercent = (qs.iComplete * 100) / (qs.iWaiting + qs.iRunning + qs.iComplete);

		if (pGui->Runtime.hTitle) {
			lstrcpyn( pszBuf, _T( "[@TOTALCOMPLETE@ / @TOTALCOUNT@] @TITLE0@" ), iBufSize );
			GuiQuery( pGui, pszBuf, iBufSize );
			SetWindowText( pGui->Runtime.hTitle, pszBuf );
		}
		if (pGui->Runtime.hText) {
			lstrcpyn( pszBuf, _T( "@TOTALCOMPLETE@ / @TOTALCOUNT@, @TOTALSIZE@ @ @TOTALSPEED@" ), iBufSize );
			GuiQuery( pGui, pszBuf, iBufSize );
			SetWindowText( pGui->Runtime.hText, pszBuf );
		}
	}

	if (pGui->Runtime.hProgress) {
		LONG_PTR iStyle = GetWindowLongPtr( pGui->Runtime.hProgress, GWL_STYLE );
		if (iPercent == -1) {
			if (!(iStyle & MY_PBS_MARQUEE)) {
				SetWindowLongPtr( pGui->Runtime.hProgress, GWL_STYLE, iStyle | MY_PBS_MARQUEE );
				SendMessage( pGui->Runtime.hProgress, MY_PBM_SETMARQUEE, TRUE, 0 );
			}
		} else {
			if (iStyle & MY_PBS_MARQUEE) {
				SendMessage( pGui->Runtime.hProgress, MY_PBM_SETMARQUEE, FALSE, 0 );
				SetWindowLongPtr( pGui->Runtime.hProgress, GWL_STYLE, iStyle & ~MY_PBS_MARQUEE );
			}
			SendMessage( pGui->Runtime.hProgress, PBM_SETRANGE, 0, MAKELONG( 0, 100 ) );
			SendMessage( pGui->Runtime.hProgress, PBM_SETPOS, iPercent, 0 );
		}
	}

	QueueUnlock();

	MyFree( pszBuf );
}