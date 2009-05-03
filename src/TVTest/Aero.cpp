#include "stdafx.h"
#include <uxtheme.h>
#include "Aero.h"


typedef HRESULT (WINAPI *DwmExtendFrameIntoClientAreaFunc)(HWND hWnd,const MARGINS *pMarInset);
typedef HRESULT (WINAPI *DwmIsCompositionEnabledFunc)(BOOL *pfEnabled);
typedef HRESULT (WINAPI *DwmEnableCompositionFunc)(UINT uCompositionAction);




CAeroGlass::CAeroGlass()
{
	m_hDwmLib=NULL;
}


CAeroGlass::~CAeroGlass()
{
	if (m_hDwmLib)
		::FreeLibrary(m_hDwmLib);
}


bool CAeroGlass::ApplyAeroGlass(HWND hwnd,const RECT *pRect)
{
	if (m_hDwmLib==NULL) {
		m_hDwmLib=::LoadLibrary(TEXT("dwmapi.dll"));
		if (m_hDwmLib==NULL)
			return false;
	}

	DwmIsCompositionEnabledFunc pIsCompositionEnabled=reinterpret_cast<DwmIsCompositionEnabledFunc>(::GetProcAddress(m_hDwmLib,"DwmIsCompositionEnabled"));
	BOOL fEnabled;

	if (pIsCompositionEnabled==NULL || pIsCompositionEnabled(&fEnabled)!=S_OK
			|| !fEnabled)
		return false;

	DwmExtendFrameIntoClientAreaFunc pExtendFrame=reinterpret_cast<DwmExtendFrameIntoClientAreaFunc>(::GetProcAddress(m_hDwmLib,"DwmExtendFrameIntoClientArea"));

	if (pExtendFrame==NULL)
		return false;

	MARGINS Margins;

	Margins.cxLeftWidth=pRect->left;
	Margins.cxRightWidth=pRect->right;
	Margins.cyTopHeight=pRect->top;
	Margins.cyBottomHeight=pRect->bottom;
	return pExtendFrame(hwnd,&Margins)==S_OK;
}
