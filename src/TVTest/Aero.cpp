#include "stdafx.h"
#include <dwmapi.h>
#include "Aero.h"


typedef HRESULT (WINAPI *DwmExtendFrameIntoClientAreaFunc)(HWND hWnd,const MARGINS *pMarInset);
typedef HRESULT (WINAPI *DwmIsCompositionEnabledFunc)(BOOL *pfEnabled);
typedef HRESULT (WINAPI *DwmEnableCompositionFunc)(UINT uCompositionAction);
typedef HRESULT (WINAPI *DwmSetWindowAttributeFunc)(HWND hwnd,DWORD dwAttribute,LPCVOID pvAttribute,DWORD cbAttribute);




CAeroGlass::CAeroGlass()
	: m_hDwmLib(NULL)
{
}


CAeroGlass::~CAeroGlass()
{
	if (m_hDwmLib)
		::FreeLibrary(m_hDwmLib);
}


bool CAeroGlass::LoadDwmLib()
{
	if (m_hDwmLib==NULL) {
		m_hDwmLib=::LoadLibrary(TEXT("dwmapi.dll"));
		if (m_hDwmLib==NULL)
			return false;
	}
	return true;
}


// クライアント領域を透けさせる
bool CAeroGlass::ApplyAeroGlass(HWND hwnd,const RECT *pRect)
{
	if (!LoadDwmLib())
		return false;

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


// フレームの描画を無効にする
bool CAeroGlass::EnableNcRendering(HWND hwnd,bool fEnable)
{
	if (!LoadDwmLib())
		return false;

	DwmSetWindowAttributeFunc pSetWindowAttribute=reinterpret_cast<DwmSetWindowAttributeFunc>(::GetProcAddress(m_hDwmLib,"DwmSetWindowAttribute"));
	if (pSetWindowAttribute==NULL)
		return false;

	DWMNCRENDERINGPOLICY ncrp=fEnable?DWMNCRP_USEWINDOWSTYLE:DWMNCRP_DISABLED;
	return pSetWindowAttribute(hwnd,DWMWA_NCRENDERING_POLICY,&ncrp,sizeof(ncrp))==S_OK;
}
