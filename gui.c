
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/12/08
//? User Interface

#include "main.h"
#include <CommCtrl.h>
#include "gui.h"
#include "utils.h"
#include "resource.h"


#define MY_PBS_MARQUEE						0x08				/// XP+
#define MY_PBM_SETMARQUEE					(WM_USER+10)

#define PROP_WNDPROC						_T( "NSCURL_WNDPROC" )
#define PROP_CONTEXT						_T( "NSCURL_CONTEXT" )

#define DEFAULT_TITLE						_T( "[@PERCENT@%] @TITLE0@" )
#define DEFAULT_TITLE_NOSIZE				_T( "@TITLE0@" )
#define DEFAULT_TITLE_MULTI					_T( "[@TOTALCOMPLETE@ / @TOTALCOUNT@] @TITLE0@" )

#define DEFAULT_TEXT						_T( "[@PERCENT@%] @OUTFILE@, @XFERSIZE@ / @FILESIZE@ @ @SPEED@ @ANIMDOTS@" )
#define DEFAULT_TEXT_NOSIZE					_T( "@OUTFILE@, @XFERSIZE@ @ @SPEED@ @ANIMDOTS@" )
#define DEFAULT_TEXT_MULTI					_T( "@TOTALCOMPLETE@ / @TOTALCOUNT@, @TOTALSIZE@ @ @TOTALSPEED@ @ANIMDOTS@" )


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

	if (lstrcmpi( pszParam, _T( "/RETURN" ) ) == 0) {
		if (popstring( pszParam ) == NOERROR) {
			MyFree( pGui->pszReturn );
			pGui->pszReturn = MyStrDup( eT2T, pszParam );
		}
	} else if (lstrcmpi( pszParam, _T( "/BACKGROUND" ) ) == 0) {
		pGui->bBackground = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/PAGE" ) ) == 0) {
		pGui->bPopup = FALSE;
		pGui->bSilent = FALSE;
	} else if (lstrcmpi( pszParam, _T( "/POPUP" ) ) == 0) {
		pGui->bPopup = TRUE;
		pGui->bSilent = FALSE;
	} else if (lstrcmpi( pszParam, _T( "/SILENT" ) ) == 0) {
		pGui->bPopup = FALSE;
		pGui->bSilent = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/CANCEL" ) ) == 0) {
		pGui->bCancel = TRUE;
	} else if (lstrcmpi( pszParam, _T( "/TITLEWND" ) ) == 0) {
		pGui->hTitle = (HWND)popintptr();
	} else if (lstrcmpi( pszParam, _T( "/TEXTWND" ) ) == 0) {
		pGui->hText = (HWND)popintptr();
	} else if (lstrcmpi( pszParam, _T( "/PROGRESSWND" ) ) == 0) {
		pGui->hProgress = (HWND)popintptr();
	} else if (lstrcmpi( pszParam, _T( "/STRING" ) ) == 0) {
		if (popstring( pszParam ) == NO_ERROR) {
			TCHAR szName[64];
			lstrcpyn( szName, pszParam, ARRAYSIZE( szName ) );
			if (popstring( pszParam ) == NO_ERROR) {
				if (lstrcmpi( szName, _T( "title" ) ) == 0) {
					pGui->pszTitle = MyStrDup( eT2T, pszParam );
				} else if (lstrcmpi( szName, _T( "title_nosize" ) ) == 0) {
					pGui->pszTitleNoSize = MyStrDup( eT2T, pszParam );
				} else if (lstrcmpi( szName, _T( "title_multi" ) ) == 0) {
					pGui->pszTitleMulti = MyStrDup( eT2T, pszParam );
				} else if (lstrcmpi( szName, _T( "text" ) ) == 0) {
					pGui->pszText = MyStrDup( eT2T, pszParam );
				} else if (lstrcmpi( szName, _T( "text_nosize" ) ) == 0) {
					pGui->pszTextNoSize = MyStrDup( eT2T, pszParam );
				} else if (lstrcmpi( szName, _T( "text_multi" ) ) == 0) {
					pGui->pszTextMulti = MyStrDup( eT2T, pszParam );
				}
			}
		}
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
			pszKeyword[0] = 0;
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
		QueueQuery( pGui->Runtime.iId, pszStr, iStrMaxLen );

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

	// Final paint
	GuiRefresh( pGui );

	CloseHandle( hDummyEvent );
	pGui->Runtime.iId = QUEUE_NO_ID;
}


//++ GuiNsisWindowProc
LRESULT CALLBACK GuiNsisWindowProc( _In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
{
	WNDPROC fnWndProc = (WNDPROC)GetProp( hwnd, PROP_WNDPROC );
	switch (uMsg) {
		case WM_COMMAND:
			if (LOWORD( wParam ) == IDCANCEL) {
				PGUI_REQUEST pGui = (PGUI_REQUEST)GetProp( hwnd, PROP_CONTEXT );
				assert( pGui );
				// TODO: Confirmation?
				QueueAbort( pGui->Runtime.iId );
				return 0;
			}
			break;
	}
	return CallWindowProc( fnWndProc, hwnd, uMsg, wParam, lParam );
}


//++ GuiSilentWait
BOOLEAN GuiSilentWait( _Inout_ PGUI_REQUEST pGui )
{
	HWND hCancelBtn = NULL;
	BOOLEAN bCancelEnabled = FALSE;
	TRACE( _T( "%hs\n" ), __FUNCTION__ );

	// Cancel
	if (pGui->bCancel) {
		hCancelBtn = g_hwndparent ? GetDlgItem( g_hwndparent, IDCANCEL ) : NULL;
		if (hCancelBtn) {
			// Hook NSIS main window to intercept Cancel clicks
			WNDPROC fnOriginalWndProc = (WNDPROC)SetWindowLongPtr( g_hwndparent, GWLP_WNDPROC, (LONG_PTR)GuiNsisWindowProc );
			if (fnOriginalWndProc) {
				SetProp( g_hwndparent, PROP_WNDPROC, (HANDLE)fnOriginalWndProc );
				SetProp( g_hwndparent, PROP_CONTEXT, (HANDLE)pGui );
			}
			// Enable it
			bCancelEnabled = IsWindowEnabled( hCancelBtn );
			EnableWindow( hCancelBtn, TRUE );
		}
	}

	// Wait
	GuiWaitLoop( pGui );

	// Cancel
	if (hCancelBtn) {
		WNDPROC fnWndProc = (WNDPROC)GetProp( g_hwndparent, PROP_WNDPROC );
		EnableWindow( hCancelBtn, bCancelEnabled );
		if (fnWndProc) {
			// Unhook NSIS main window
			SetWindowLongPtr( g_hwndparent, GWLP_WNDPROC, (LONG_PTR)fnWndProc );
			RemoveProp( g_hwndparent, PROP_WNDPROC );
			RemoveProp( g_hwndparent, PROP_CONTEXT );
		}
	}

	return TRUE;
}


//++ GuiPopupDialogProc
INT_PTR CALLBACK GuiPopupDialogProc( _In_ HWND hDlg, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam )
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hParent = GetWindow( hDlg, GW_OWNER );
			PGUI_REQUEST pGui = (PGUI_REQUEST)lParam;			/// CreateDialogParam parameter
			assert( pGui );

			// Remember GUI request
			SetProp( hDlg, PROP_CONTEXT, (HANDLE)pGui );

			// Window handles
			pGui->Runtime.hTitle = hDlg;
			pGui->Runtime.hText = GetDlgItem( hDlg, IDC_POPUP_STATUS );
			pGui->Runtime.hProgress = GetDlgItem( hDlg, IDC_POPUP_PROGRESS );

			// Cancel
			EnableMenuItem( GetSystemMenu( hDlg, FALSE ), SC_CLOSE, MF_BYCOMMAND | (pGui->bCancel ? MF_ENABLED : MF_DISABLED) );

			// Title
			if (hParent && (hParent != GetDesktopWindow())) {
				/// Load installer's title text
				/// Unavailable during .onInit
				ULONG l = GetWindowTextLength( hParent ) + 1;
				if ((pGui->Runtime.pszTitle0 = (LPTSTR)MyAlloc( l * sizeof( TCHAR ) )) != NULL) {
					GetWindowText( hParent, pGui->Runtime.pszTitle0, l );
					SetWindowText( hDlg, pGui->Runtime.pszTitle0 );
				}
			} else {
				/// Load installer's file name
				pGui->Runtime.pszTitle0 = MyStrDup( eT2T, getuservariableEx( INST_EXEFILE ) );
				if (pGui->Runtime.pszTitle0) {
					LPTSTR psz;
					for (psz = pGui->Runtime.pszTitle0 + lstrlen( pGui->Runtime.pszTitle0 ) - 1; psz > pGui->Runtime.pszTitle0; psz--) {
						if (*psz == _T( '.' )) {
							*psz = _T( '\0' );		/// Strip extension
							break;
						}
					}
					SetWindowText( hDlg, pGui->Runtime.pszTitle0 );
				}
			}

			// Icon (load installer's main icon)
			{
				HICON hIco = LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( 103 ), IMAGE_ICON, GetSystemMetrics( SM_CXICON ), GetSystemMetrics( SM_CYICON ), 0 );
				if (hIco)
					SendDlgItemMessage( hDlg, IDC_POPUP_ICON, STM_SETICON, (WPARAM)hIco, 0 );
			}

			// Disable parent window (NSIS main window) for our modeless dialog to behave like a modal one
			if (IsWindowEnabled( hParent ) && (hParent != GetDesktopWindow()))
				EnableWindow( hParent, FALSE );

			return TRUE;	/// Focus (HWND)wParam
		}

		case WM_DESTROY:
		{
			HWND hParent = GetWindow( hDlg, GW_OWNER );
			PGUI_REQUEST pGui = GetProp( hDlg, PROP_CONTEXT );
			assert( pGui );

			// Icon
			{
				HICON hIco = (HICON)SendDlgItemMessage( hDlg, IDC_POPUP_ICON, STM_SETICON, (WPARAM)NULL, 0 );
				if (hIco)
					DestroyIcon( hIco );
			}

			// Parent
			if (hParent && (hParent != GetDesktopWindow()))
				EnableWindow( hParent, TRUE );

			// Cleanup
			pGui->Runtime.hTitle = pGui->Runtime.hText = pGui->Runtime.hProgress = NULL;
			RemoveProp( hDlg, PROP_CONTEXT );
			break;
		}

		case WM_SYSCOMMAND:
			if (wParam == SC_CLOSE) {
				/// [X] button
				PGUI_REQUEST pGui = GetProp( hDlg, PROP_CONTEXT );
				assert( pGui );
				// TODO: Confirmation?
				QueueAbort( pGui->Runtime.iId );
				return 0;
			}
			break;
	}

	return FALSE;	/// Default dialog procedure
}


//++ GuiPopupWait
BOOLEAN GuiPopupWait( _Inout_ PGUI_REQUEST pGui )
{
	HWND hDlg;
	TRACE( _T( "%hs\n" ), __FUNCTION__ );
	if ((hDlg = CreateDialogParam( g_hInst, MAKEINTRESOURCE( IDD_POPUP ), g_hwndparent, GuiPopupDialogProc, (LPARAM)pGui )) != NULL) {

		GuiWaitLoop( pGui );
		DestroyWindow( hDlg );

		if (IsWindowVisible( g_hwndparent ))
			SetForegroundWindow( g_hwndparent );

	} else {
		assert( !"CreateDialogParam" );
	}
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

		// Check if InstFiles (built-in) page exists
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
				OffsetRect( &rcNewText, 0, (rcProgress.bottom - rcText.top) + (rcDetailsList.top - rcProgress.bottom) );
				hNewText = CreateWindowEx( iTextStyleEx, WC_STATIC, _T( "" ), iTextStyle, LTWH( rcNewText ), hInstFilesPage, NULL, NULL, NULL );
				SendMessage( hNewText, WM_SETFONT, (WPARAM)SendMessage( hNsisText, WM_GETFONT, 0, 0 ), MAKELPARAM( FALSE, 0 ) );

				/// New progress bar
				CopyRect( &rcNewProgress, &rcProgress );
				OffsetRect( &rcNewProgress, 0, rcNewText.bottom + (rcText.bottom - rcProgress.top) - rcNewProgress.top );
				iProgressStyle |= MY_PBS_MARQUEE;		/// Marquee capability
				hNewProgress = CreateWindowEx( iProgressStyleEx, PROGRESS_CLASS, _T( "" ), iProgressStyle, LTWH( rcNewProgress ), hInstFilesPage, NULL, NULL, NULL );

				iDetailsOffsetY = rcNewProgress.bottom + (rcDetailsList.top - rcProgress.bottom) - rcDetailsList.top;
				iDetailsOffsetY += iDetailsOffsetY / 8;

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

		BOOLEAN bCancelEnabled = FALSE;
		HWND hCancelBtn = GetDlgItem( g_hwndparent, IDCANCEL );

		// Cancel
		if (hCancelBtn) {
			bCancelEnabled = IsWindowEnabled( hCancelBtn );
			if (pGui->bCancel) {
				// Hook NSIS main window to intercept Cancel clicks
				WNDPROC fnOriginalWndProc = (WNDPROC)SetWindowLongPtr( g_hwndparent, GWLP_WNDPROC, (LONG_PTR)GuiNsisWindowProc );
				if (fnOriginalWndProc) {
					SetProp( g_hwndparent, PROP_WNDPROC, (HANDLE)fnOriginalWndProc );
					SetProp( g_hwndparent, PROP_CONTEXT, (HANDLE)pGui );
				}
				// Enable it
				EnableWindow( hCancelBtn, TRUE );
			} else {
				EnableWindow( hCancelBtn, FALSE );
			}
		}

		// Wait
		GuiWaitLoop( pGui );

		// Restore controls
	//x	if (pGui->Runtime.pszTitle0)
	//x		SetWindowText( pGui->hTitle, pGui->Runtime.pszTitle0 );

	//x	if (pGui->Runtime.pszText0)
	//x		SetWindowText( pGui->hText, pGui->Runtime.pszText0 );

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

		if (hCancelBtn) {
			WNDPROC fnWndProc = (WNDPROC)GetProp( g_hwndparent, PROP_WNDPROC );
			EnableWindow( hCancelBtn, bCancelEnabled );
			if (fnWndProc) {
				// Unhook NSIS main window
				SetWindowLongPtr( g_hwndparent, GWLP_WNDPROC, (LONG_PTR)fnWndProc );
				RemoveProp( g_hwndparent, PROP_WNDPROC );
				RemoveProp( g_hwndparent, PROP_CONTEXT );
			}
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
	}

	// Return
	if (!pGui->bBackground) {
		if (pGui->iId != QUEUE_NO_ID) {
			//? Wait for single ID
			lstrcpyn( pszResult, pGui->pszReturn ? pGui->pszReturn : _T( "@ERROR@" ), iResultMaxLen );
			QueueQuery( pGui->iId, pszResult, iResultMaxLen );
		} else {
			//? Wait for all
			lstrcpyn( pszResult, pGui->pszReturn ? pGui->pszReturn : _T( "OK" ), iResultMaxLen );
			QueueQuery( pGui->iId, pszResult, iResultMaxLen );
		}
	} else {
		//? Background transfer, no waiting. Always return transfer ID
		assert( pGui->iId != QUEUE_NO_ID );
		if (pGui->iId != QUEUE_NO_ID) {
			_sntprintf( pszResult, iResultMaxLen, _T( "%u" ), pGui->iId );
		} else {
			lstrcpyn( pszResult, _T( "0" ), iResultMaxLen );		// 0 == invalid ID
		}
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

	// Statistics and running ID
	QueueLock();
	QueueStatistics( pGui->iId, &qs );
	QueueUnlock();

	pGui->Runtime.iId = qs.iSingleID;

	// Anything to refresh?
	if (!pGui->Runtime.hTitle && !pGui->Runtime.hProgress && !pGui->Runtime.hText)
		return;

	pszBuf = (LPTSTR)MyAlloc( iBufSize * sizeof( TCHAR ) );
	if (!pszBuf)
		return;

	if (pGui->Runtime.iId != QUEUE_NO_ID) {

		// Single Running transfer
		lstrcpyn( pszBuf, _T( "@PERCENT@" ), iBufSize );
		GuiQuery( pGui, pszBuf, iBufSize );
		iPercent = myatoi( pszBuf );
		assert( iPercent >= -1 && iPercent <= 100 );

		if (pGui->Runtime.hTitle) {
			if (iPercent == -1) {
				lstrcpyn( pszBuf, pGui->pszTitleNoSize ? pGui->pszTitleNoSize : DEFAULT_TITLE_NOSIZE, iBufSize );
			} else {
				lstrcpyn( pszBuf, pGui->pszTitle ? pGui->pszTitle : DEFAULT_TITLE, iBufSize );
			}
			GuiQuery( pGui, pszBuf, iBufSize );
			SetWindowText( pGui->Runtime.hTitle, pszBuf );
		}
		if (pGui->Runtime.hText) {
			if (iPercent == -1) {
				lstrcpyn( pszBuf, pGui->pszTextNoSize ? pGui->pszTextNoSize : DEFAULT_TEXT_NOSIZE, iBufSize );
			} else {
				lstrcpyn( pszBuf, pGui->pszText ? pGui->pszText : DEFAULT_TEXT, iBufSize );
			}
			GuiQuery( pGui, pszBuf, iBufSize );
			SetWindowText( pGui->Runtime.hText, pszBuf );
		}

	} else {

		// Multiple or zero Running transfers
		if (qs.iWaiting + qs.iRunning + qs.iComplete > 0) {
			iPercent = (qs.iComplete * 100) / (qs.iWaiting + qs.iRunning + qs.iComplete);
		} else {
			iPercent = 0;
		}

		if (pGui->Runtime.hTitle) {
			lstrcpyn( pszBuf, pGui->pszTitleMulti ? pGui->pszTitleMulti : DEFAULT_TITLE_MULTI, iBufSize );
			GuiQuery( pGui, pszBuf, iBufSize );
			SetWindowText( pGui->Runtime.hTitle, pszBuf );
		}
		if (pGui->Runtime.hText) {
			lstrcpyn( pszBuf, pGui->pszTextMulti ? pGui->pszTextMulti : DEFAULT_TEXT_MULTI, iBufSize );
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

	MyFree( pszBuf );
}