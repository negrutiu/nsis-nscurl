
//? Marius Negrutiu (mailto:marius.negrutiu@protonmail.com) :: 2019/12/08
//? User Interface

#pragma once
#include "queue.h"

//+ struct GUI_REQUEST
typedef struct {
	ULONG iId;							/// Can be QUEUE_NO_ID
	BOOLEAN bBackground : 1;			/// Background transfer, no waiting
	BOOLEAN bSilent : 1;				/// Wait silently
	BOOLEAN bPopup : 1;					/// Wait popuply
	HWND hTitle, hText, hProgress;		/// Optional window handles. Can be NULL
	struct {
		HWND hTitle, hText, hProgress;	/// Runtime controls
	} Runtime;
} GUI_REQUEST, *PGUI_REQUEST;

//+ GuiRequestInit
static void GuiRequestInit( _Inout_ PGUI_REQUEST pGui ) {
	if (!pGui) return;
	ZeroMemory( pGui, sizeof( *pGui ) );
	pGui->iId = QUEUE_NO_ID;
}

//+ GuiRequestDestroy
static void GuiRequestDestroy( _Inout_ PGUI_REQUEST pGui ) {
	if (!pGui) return;
	ZeroMemory( pGui, sizeof( *pGui ) );
}

// ____________________________________________________________________________________________________________________________________ //
//                                                                                                                                      //

//+ Initialization
void GuiInitialize();
void GuiDestroy();

//+ GuiParseRequestParam
BOOL GuiParseRequestParam(
	_In_ LPTSTR pszParam,		/// Working buffer with the current parameter
	_In_ int iParamMaxLen,
	_Out_ PGUI_REQUEST pGui
);


//+ GuiWait
void GuiWait(
	_Inout_ PGUI_REQUEST pGui,
	_Out_ LPTSTR pszResult,
	_In_ int iResultMaxLen
);


//+ GuiRefresh
void GuiRefresh(
	_Inout_ PGUI_REQUEST pGui
);