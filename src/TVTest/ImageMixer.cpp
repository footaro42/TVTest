#include "stdafx.h"
#include <d3d9.h>
#include <Vmr9.h>
#include "ImageMixer.h"




CImageMixer::CImageMixer(IBaseFilter *pVMR9)
{
	m_pVMR9=pVMR9;
	m_hdc=NULL;
	m_hbm=NULL;
}


CImageMixer::~CImageMixer()
{
	if (m_hdc!=NULL) {
		::SelectObject(m_hdc,m_hbmOld);
		::DeleteDC(m_hdc);
	}
	if (m_hbm!=NULL)
		::DeleteObject(m_hbm);
}


bool CImageMixer::CreateMemDC()
{
	if (m_hdc==NULL) {
		m_hdc=::CreateCompatibleDC(NULL);
		if (m_hdc==NULL)
			return false;
		m_hbmOld=static_cast<HBITMAP>(::GetCurrentObject(m_hdc,OBJ_BITMAP));
	}
	return true;
}


void CImageMixer::Clear()
{
	if (m_hdc!=NULL) {
		IVMRMixerBitmap9 *pMixerBitmap;
		VMR9AlphaBitmap ab;

		m_pVMR9->QueryInterface(IID_IVMRMixerBitmap9,
									reinterpret_cast<LPVOID*>(&pMixerBitmap));
		ab.dwFlags=VMR9AlphaBitmap_Disable;
		ab.fAlpha=0.0f;
		pMixerBitmap->UpdateAlphaBitmapParameters(&ab);
		pMixerBitmap->Release();
	}
}


bool CImageMixer::SetBitmap(HBITMAP hbm,int Opacity,COLORREF TransColor,
															RECT *pDestRect)
{
	IVMRMixerBitmap9 *pMixerBitmap;
	IVMRWindowlessControl9 *pWindowlessControl;
	LONG VideoWidth,VideoHeight;
	BITMAP bm;
	VMR9AlphaBitmap ab;
	HRESULT hr;

	if (!CreateMemDC())
		return false;
	if (FAILED(m_pVMR9->QueryInterface(IID_IVMRMixerBitmap9,
									reinterpret_cast<LPVOID*>(&pMixerBitmap))))
		return false;
	m_pVMR9->QueryInterface(IID_IVMRWindowlessControl9,
							reinterpret_cast<LPVOID*>(&pWindowlessControl));
	if (FAILED(pWindowlessControl->GetNativeVideoSize(&VideoWidth,&VideoHeight,
																	NULL,NULL))
			|| VideoWidth==0 || VideoHeight==0) {
		VideoWidth=1920;
		VideoHeight=1080;
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
	ab.rDest.left=(float)pDestRect->left/(float)VideoWidth;
	ab.rDest.top=(float)pDestRect->top/(float)VideoHeight;
	ab.rDest.right=(float)pDestRect->right/(float)VideoWidth;
	ab.rDest.bottom=(float)pDestRect->bottom/(float)VideoHeight;
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


bool CImageMixer::SetText(LPCTSTR pszText,HFONT hfont,COLORREF Color,
												int Opacity,RECT *pDestRect)
{
	HDC hdc;
	HFONT hfontOld;
	RECT rc;
	HBITMAP hbm;
	HBRUSH hbr;
	COLORREF crTransColor,crOldTextColor;
	int OldBkMode;

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
	if (!SetBitmap(hbm,Opacity,crTransColor,pDestRect)) {
		::DeleteObject(hbm);
		return false;
	}
	if (m_hbm!=NULL)
		::DeleteObject(m_hbm);
	m_hbm=hbm;
	return true;
}
