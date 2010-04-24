#include "stdafx.h"
#if _WIN32_WINNT<0x0600 || _WINVER<0x0600
#undef _WIN32_WINNT
#undef _WINVER
#define _WIN32_WINNT	0x0600
#define _WINVER			0x0600
#endif
#include <dwmapi.h>
#include <uxtheme.h>
#include "Aero.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


template<typename T> bool ProcAddress(T &func,HMODULE lib,const char *symbol)
{
	return (func=reinterpret_cast<T>(::GetProcAddress(lib,symbol)))!=NULL;
}


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


// コンポジションが有効か取得する
bool CAeroGlass::IsEnabled()
{
	if (!LoadDwmLib())
		return false;

	DwmIsCompositionEnabledFunc pIsCompositionEnabled;
	ProcAddress(pIsCompositionEnabled,m_hDwmLib,"DwmIsCompositionEnabled");
	BOOL fEnabled;
	return pIsCompositionEnabled!=NULL
		&& pIsCompositionEnabled(&fEnabled)==S_OK && fEnabled;
}


// クライアント領域を透けさせる
bool CAeroGlass::ApplyAeroGlass(HWND hwnd,const RECT *pRect)
{
	if (!IsEnabled())
		return false;

	DwmExtendFrameIntoClientAreaFunc pExtendFrame;
	if (!ProcAddress(pExtendFrame,m_hDwmLib,"DwmExtendFrameIntoClientArea"))
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

	DwmSetWindowAttributeFunc pSetWindowAttribute;
	if (!ProcAddress(pSetWindowAttribute,m_hDwmLib,"DwmSetWindowAttribute"))
		return false;

	DWMNCRENDERINGPOLICY ncrp=fEnable?DWMNCRP_USEWINDOWSTYLE:DWMNCRP_DISABLED;
	return pSetWindowAttribute(hwnd,DWMWA_NCRENDERING_POLICY,&ncrp,sizeof(ncrp))==S_OK;
}




typedef HPAINTBUFFER (WINAPI *BeginBufferedPaintFunc)(HDC hdcTarget,const RECT *prcTarget,BP_BUFFERFORMAT dwFormat,BP_PAINTPARAMS *pPaintParams,HDC *phdc);
typedef HRESULT (WINAPI *EndBufferedPaintFunc)(HPAINTBUFFER hBufferedPaint,BOOL fUpdateTarget);
typedef HRESULT (WINAPI *BufferedPaintClearFunc)(HPAINTBUFFER hBufferedPaint,const RECT *prc);
typedef HRESULT (WINAPI *BufferedPaintSetAlphaFunc)(HPAINTBUFFER hBufferedPaint,const RECT *prc,BYTE alpha);


CBufferedPaint::CBufferedPaint()
	: m_hPaintBuffer(NULL)
{
	m_hThemeLib=::LoadLibrary(TEXT("uxtheme.dll"));
}


CBufferedPaint::~CBufferedPaint()
{
	End();
	if (m_hThemeLib!=NULL)
		::FreeLibrary(m_hThemeLib);
}


HDC CBufferedPaint::Begin(HDC hdc,const RECT *pRect,bool fErase)
{
	if (m_hThemeLib==NULL || m_hPaintBuffer!=NULL)
		return NULL;

	BeginBufferedPaintFunc pBeginBufferedPaint;
	if (!ProcAddress(pBeginBufferedPaint,m_hThemeLib,"BeginBufferedPaint"))
		return NULL;

	BP_PAINTPARAMS Params={sizeof(BP_PAINTPARAMS),0,NULL,NULL};
	if (fErase)
		Params.dwFlags|=BPPF_ERASE;
	HDC hdcBuffer;
	m_hPaintBuffer=pBeginBufferedPaint(hdc,pRect,BPBF_TOPDOWNDIB,&Params,&hdcBuffer);
	if (m_hPaintBuffer==NULL)
		return NULL;
	return hdcBuffer;
}


void CBufferedPaint::End()
{
	if (m_hPaintBuffer!=NULL) {
		EndBufferedPaintFunc pEndBufferedPaint;
		if (ProcAddress(pEndBufferedPaint,m_hThemeLib,"EndBufferedPaint")) {
			pEndBufferedPaint(m_hPaintBuffer,TRUE);
			m_hPaintBuffer=NULL;
		}
	}
}


bool CBufferedPaint::Clear(const RECT *pRect)
{
	if (m_hPaintBuffer==NULL)
		return false;
	BufferedPaintClearFunc pBufferedPaintClear;
	if (!ProcAddress(pBufferedPaintClear,m_hThemeLib,"BufferedPaintClear"))
		return false;
	return pBufferedPaintClear(m_hPaintBuffer,pRect)==S_OK;
}


bool CBufferedPaint::SetAlpha(BYTE Alpha)
{
	if (m_hPaintBuffer==NULL)
		return false;
	BufferedPaintSetAlphaFunc pBufferedPaintSetAlpha;
	if (!ProcAddress(pBufferedPaintSetAlpha,m_hThemeLib,"BufferedPaintSetAlpha"))
		return false;
	return pBufferedPaintSetAlpha(m_hPaintBuffer,NULL,Alpha)==S_OK;
}
