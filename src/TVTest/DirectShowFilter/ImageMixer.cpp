#include "stdafx.h"
#include <d3d9.h>
#include <Vmr9.h>
#include "ImageMixer.h"




CImageMixer::CImageMixer(IBaseFilter *pRenderer)
{
	m_pRenderer=pRenderer;
	m_pRenderer->AddRef();
}


CImageMixer::~CImageMixer()
{
	if (m_pRenderer)
		m_pRenderer->Release();
}


CImageMixer *CImageMixer::CreateImageMixer(CVideoRenderer::RendererType RendererType,
										   IBaseFilter *pRendererFilter)
{
	switch (RendererType) {
	case CVideoRenderer::RENDERER_VMR7:
		return new CImageMixer_VMR7(pRendererFilter);
	case CVideoRenderer::RENDERER_VMR9:
		return new CImageMixer_VMR9(pRendererFilter);
	}
	return NULL;
}


bool CImageMixer::IsSupported(CVideoRenderer::RendererType RendererType)
{
	switch (RendererType) {
	case CVideoRenderer::RENDERER_VMR7:
	case CVideoRenderer::RENDERER_VMR9:
		return true;
	}
	return false;
}




CImageMixer_VMR7::CImageMixer_VMR7(IBaseFilter *pRenderer)
	: CImageMixer(pRenderer)
{
	m_hdc=NULL;
	m_hbm=NULL;
}


CImageMixer_VMR7::~CImageMixer_VMR7()
{
	if (m_hdc!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		::DeleteDC(m_hdc);
	}
	if (m_hbm!=NULL)
		::DeleteObject(m_hbm);
}


bool CImageMixer_VMR7::CreateMemDC()
{
	if (m_hdc==NULL) {
		m_hdc=::CreateCompatibleDC(NULL);
		if (m_hdc==NULL)
			return false;
		m_hbmOld=static_cast<HBITMAP>(::GetCurrentObject(m_hdc,OBJ_BITMAP));
	}
	return true;
}


void CImageMixer_VMR7::Clear()
{
	if (m_hdc!=NULL) {
		IVMRMixerBitmap *pMixerBitmap;

		if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRMixerBitmap,
								reinterpret_cast<LPVOID*>(&pMixerBitmap)))) {
			VMRALPHABITMAP ab;

			ab.dwFlags=VMRBITMAP_DISABLE;
			ab.fAlpha=0.0f;
			pMixerBitmap->UpdateAlphaBitmapParameters(&ab);
			pMixerBitmap->Release();
		}
	}
}


bool CImageMixer_VMR7::SetBitmap(HBITMAP hbm,int Opacity,COLORREF TransColor,RECT *pDestRect)
{
	IVMRMixerBitmap *pMixerBitmap;
	IVMRWindowlessControl *pWindowlessControl;
	LONG NativeWidth,NativeHeight;
	BITMAP bm;
	VMRALPHABITMAP ab;
	HRESULT hr;

	if (!CreateMemDC())
		return false;
	if (FAILED(m_pRenderer->QueryInterface(IID_IVMRMixerBitmap,
									reinterpret_cast<LPVOID*>(&pMixerBitmap))))
		return false;
	m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl));
	hr=pWindowlessControl->GetNativeVideoSize(&NativeWidth,&NativeHeight,NULL,NULL);
	if (FAILED(hr) || NativeWidth==0 || NativeHeight==0) {
		NativeWidth=1440;
		NativeHeight=1080;
	}
	pWindowlessControl->Release();
	::SelectObject(m_hdc,hbm);
	ab.dwFlags=VMRBITMAP_HDC;
	if (TransColor!=CLR_INVALID)
		ab.dwFlags|=VMRBITMAP_SRCCOLORKEY;
	ab.hdc=m_hdc;
	ab.pDDS=NULL;
	::GetObject(hbm,sizeof(BITMAP),&bm);
	::SetRect(&ab.rSrc,0,0,bm.bmWidth,bm.bmHeight);
	ab.rDest.left=(float)pDestRect->left/(float)NativeWidth;
	ab.rDest.top=(float)pDestRect->top/(float)NativeHeight;
	ab.rDest.right=(float)pDestRect->right/(float)NativeWidth;
	ab.rDest.bottom=(float)pDestRect->bottom/(float)NativeHeight;
	ab.fAlpha=(float)Opacity/100.0f;
	ab.clrSrcKey=TransColor;
	hr=pMixerBitmap->SetAlphaBitmap(&ab);
	pMixerBitmap->Release();
	if (FAILED(hr)) {
		::SelectObject(m_hdc,m_hbmOld);
		return false;
	}
	return true;
}


bool CImageMixer_VMR7::SetText(LPCTSTR pszText,int x,int y,HFONT hfont,COLORREF Color,int Opacity)
{
	HDC hdc;
	HFONT hfontOld;
	RECT rc;
	HBITMAP hbm;
	HBRUSH hbr;
	COLORREF crTransColor,crOldTextColor;
	int OldBkMode;
	RECT rcDest;

	if (!CreateMemDC())
		return false;
	hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
	hfontOld=static_cast<HFONT>(::SelectObject(hdc,hfont));
	::SetRect(&rc,0,0,0,0);
	::DrawText(hdc,pszText,-1,&rc,DT_LEFT | DT_TOP | DT_NOPREFIX | DT_CALCRECT);
	::OffsetRect(&rc,-rc.left,-rc.top);
	hbm=::CreateCompatibleBitmap(hdc,rc.right,rc.bottom);
	::SelectObject(hdc,hfontOld);
	::DeleteDC(hdc);
	if (hbm==NULL)
		return false;
	::SelectObject(m_hdc,hbm);
	crTransColor=Color^0x00FFFFFF;
	hbr=::CreateSolidBrush(crTransColor);
	::FillRect(m_hdc,&rc,hbr);
	::DeleteObject(hbr);
	hfontOld=static_cast<HFONT>(::SelectObject(m_hdc,hfont));
	crOldTextColor=::SetTextColor(m_hdc,Color);
	OldBkMode=::SetBkMode(m_hdc,TRANSPARENT);
	::DrawText(m_hdc,pszText,-1,&rc,DT_LEFT | DT_TOP | DT_NOPREFIX);
	::SetTextColor(m_hdc,crOldTextColor);
	::SetBkMode(m_hdc,OldBkMode);
	::SelectObject(m_hdc,hfontOld);
	::SelectObject(m_hdc,m_hbmOld);
	rcDest.left=x;
	rcDest.top=y;
	rcDest.right=x+rc.right;
	rcDest.bottom=y+rc.bottom;
	if (!SetBitmap(hbm,Opacity,crTransColor,&rcDest)) {
		::DeleteObject(hbm);
		return false;
	}
	if (m_hbm!=NULL)
		::DeleteObject(m_hbm);
	m_hbm=hbm;
	return true;
}


bool CImageMixer_VMR7::GetMapSize(int *pWidth,int *pHeight)
{
	bool fOK=false;
	IVMRWindowlessControl *pWindowlessControl;

	if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
		LONG NativeWidth,NativeHeight;

		if (SUCCEEDED(pWindowlessControl->GetNativeVideoSize(&NativeWidth,&NativeHeight,NULL,NULL))) {
			if (pWidth)
				*pWidth=NativeWidth;
			if (pHeight)
				*pHeight=NativeHeight;
			fOK=true;
		}
		pWindowlessControl->Release();
	}
	return fOK;
}




CImageMixer_VMR9::CImageMixer_VMR9(IBaseFilter *pRenderer)
	: CImageMixer(pRenderer)
{
	m_hdc=NULL;
	m_hbm=NULL;
}


CImageMixer_VMR9::~CImageMixer_VMR9()
{
	if (m_hdc!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		::DeleteDC(m_hdc);
	}
	if (m_hbm!=NULL)
		::DeleteObject(m_hbm);
}


bool CImageMixer_VMR9::CreateMemDC()
{
	if (m_hdc==NULL) {
		m_hdc=::CreateCompatibleDC(NULL);
		if (m_hdc==NULL)
			return false;
		m_hbmOld=static_cast<HBITMAP>(::GetCurrentObject(m_hdc,OBJ_BITMAP));
	}
	return true;
}


void CImageMixer_VMR9::Clear()
{
	if (m_hdc!=NULL) {
		IVMRMixerBitmap9 *pMixerBitmap;
		VMR9AlphaBitmap ab;

		m_pRenderer->QueryInterface(IID_IVMRMixerBitmap9,
									reinterpret_cast<LPVOID*>(&pMixerBitmap));
		ab.dwFlags=VMR9AlphaBitmap_Disable;
		ab.fAlpha=0.0f;
		pMixerBitmap->UpdateAlphaBitmapParameters(&ab);
		pMixerBitmap->Release();
	}
}


bool CImageMixer_VMR9::SetBitmap(HBITMAP hbm,int Opacity,COLORREF TransColor,RECT *pDestRect)
{
	IVMRMixerBitmap9 *pMixerBitmap;
	IVMRWindowlessControl9 *pWindowlessControl;
	LONG NativeWidth,NativeHeight;
	BITMAP bm;
	VMR9AlphaBitmap ab;
	HRESULT hr;

	if (!CreateMemDC())
		return false;
	if (FAILED(m_pRenderer->QueryInterface(IID_IVMRMixerBitmap9,
									reinterpret_cast<LPVOID*>(&pMixerBitmap))))
		return false;
	m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
							reinterpret_cast<LPVOID*>(&pWindowlessControl));
	if (FAILED(pWindowlessControl->GetNativeVideoSize(&NativeWidth,&NativeHeight,
																	NULL,NULL))
			|| NativeWidth==0 || NativeHeight==0) {
		NativeWidth=1440;
		NativeHeight=1080;
	}
	pWindowlessControl->Release();
	::SelectObject(m_hdc,hbm);
	ab.dwFlags=VMR9AlphaBitmap_hDC;
	if (TransColor!=CLR_INVALID)
		ab.dwFlags|=VMR9AlphaBitmap_SrcColorKey;
	ab.hdc=m_hdc;
	ab.pDDS=NULL;
	::GetObject(hbm,sizeof(BITMAP),&bm);
	::SetRect(&ab.rSrc,0,0,bm.bmWidth,bm.bmHeight);
	ab.rDest.left=(float)pDestRect->left/(float)NativeWidth;
	ab.rDest.top=(float)pDestRect->top/(float)NativeHeight;
	ab.rDest.right=(float)pDestRect->right/(float)NativeWidth;
	ab.rDest.bottom=(float)pDestRect->bottom/(float)NativeHeight;
	ab.fAlpha=(float)Opacity/100.0f;
	ab.clrSrcKey=TransColor;
	hr=pMixerBitmap->SetAlphaBitmap(&ab);
	pMixerBitmap->Release();
	if (FAILED(hr)) {
		::SelectObject(m_hdc,m_hbmOld);
		return false;
	}
	return true;
}


bool CImageMixer_VMR9::SetText(LPCTSTR pszText,int x,int y,HFONT hfont,COLORREF Color,int Opacity)
{
	HDC hdc;
	HFONT hfontOld;
	RECT rc;
	HBITMAP hbm;
	HBRUSH hbr;
	COLORREF crTransColor,crOldTextColor;
	int OldBkMode;
	RECT rcDest;

	if (!CreateMemDC())
		return false;
	hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
	hfontOld=static_cast<HFONT>(::SelectObject(hdc,hfont));
	::SetRect(&rc,0,0,0,0);
	::DrawText(hdc,pszText,-1,&rc,DT_LEFT | DT_TOP | DT_NOPREFIX | DT_CALCRECT);
	::OffsetRect(&rc,-rc.left,-rc.top);
	hbm=::CreateCompatibleBitmap(hdc,rc.right,rc.bottom);
	::SelectObject(hdc,hfontOld);
	::DeleteDC(hdc);
	if (hbm==NULL)
		return false;
	::SelectObject(m_hdc,hbm);
	crTransColor=Color^0x00FFFFFF;
	hbr=::CreateSolidBrush(crTransColor);
	::FillRect(m_hdc,&rc,hbr);
	::DeleteObject(hbr);
	hfontOld=static_cast<HFONT>(::SelectObject(m_hdc,hfont));
	crOldTextColor=::SetTextColor(m_hdc,Color);
	OldBkMode=::SetBkMode(m_hdc,TRANSPARENT);
	::DrawText(m_hdc,pszText,-1,&rc,DT_LEFT | DT_TOP | DT_NOPREFIX);
	::SetTextColor(m_hdc,crOldTextColor);
	::SetBkMode(m_hdc,OldBkMode);
	::SelectObject(m_hdc,hfontOld);
	::SelectObject(m_hdc,m_hbmOld);
	rcDest.left=x;
	rcDest.top=y;
	rcDest.right=x+rc.right;
	rcDest.bottom=y+rc.bottom;
	if (!SetBitmap(hbm,Opacity,crTransColor,&rcDest)) {
		::DeleteObject(hbm);
		return false;
	}
	if (m_hbm!=NULL)
		::DeleteObject(m_hbm);
	m_hbm=hbm;
	return true;
}


bool CImageMixer_VMR9::GetMapSize(int *pWidth,int *pHeight)
{
	bool fOK=false;
	IVMRWindowlessControl9 *pWindowlessControl;

	if (SUCCEEDED(m_pRenderer->QueryInterface(IID_IVMRWindowlessControl9,
							reinterpret_cast<LPVOID*>(&pWindowlessControl)))) {
		LONG NativeWidth,NativeHeight;

		if (SUCCEEDED(pWindowlessControl->GetNativeVideoSize(&NativeWidth,&NativeHeight,NULL,NULL))) {
			if (pWidth)
				*pWidth=NativeWidth;
			if (pHeight)
				*pHeight=NativeHeight;
			fOK=true;
		}
		pWindowlessControl->Release();
	}
	return fOK;
}
