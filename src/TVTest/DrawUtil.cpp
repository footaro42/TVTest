#include "stdafx.h"
#include "DrawUtil.h"




bool DrawUtil::Fill(HDC hdc,const RECT *pRect,COLORREF Color)
{
	HBRUSH hbr=::CreateSolidBrush(Color);

	if (hbr==NULL)
		return false;
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	return true;
}


bool DrawUtil::FillGradient(HDC hdc,const RECT *pRect,COLORREF Color1,COLORREF Color2,FillDirection Direction)
{
	HPEN hpenOld,hpenCur,hpen;
	int i,Max;
	COLORREF cr,crPrev;

	if (pRect->left>pRect->right || pRect->top>pRect->bottom)
		return false;
	hpenOld=static_cast<HPEN>(::GetCurrentObject(hdc,OBJ_PEN));
	hpenCur=NULL;
	crPrev=CLR_INVALID;
	if (Direction==DIRECTION_HORZ)
		Max=pRect->right-pRect->left-1;
	else
		Max=pRect->bottom-pRect->top-1;
	for (i=0;i<=Max;i++) {
		cr=RGB((GetRValue(Color1)*(Max-i)+GetRValue(Color2)*i)/Max,
			   (GetGValue(Color1)*(Max-i)+GetGValue(Color2)*i)/Max,
			   (GetBValue(Color1)*(Max-i)+GetBValue(Color2)*i)/Max);
		if (cr!=crPrev) {
			hpen=::CreatePen(PS_SOLID,1,cr);
			if (hpen) {
				::SelectObject(hdc,hpen);
				if (hpenCur)
					::DeleteObject(hpenCur);
				hpenCur=hpen;
			}
			crPrev=cr;
		}
		if (Direction==DIRECTION_HORZ) {
			::MoveToEx(hdc,pRect->left+i,pRect->top,NULL);
			::LineTo(hdc,pRect->left+i,pRect->bottom);
		} else {
			::MoveToEx(hdc,pRect->left,pRect->top+i,NULL);
			::LineTo(hdc,pRect->right,pRect->top+i);
		}
	}
	if (hpenCur) {
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hpenCur);
	}
	return true;
}


bool DrawUtil::FillBorder(HDC hdc,const RECT *pBorderRect,const RECT *pEmptyRect,const RECT *pPaintRect,HBRUSH hbr)
{
	RECT rc;

	if (pPaintRect->left<pBorderRect->right && pPaintRect->right>pBorderRect->left) {
		rc.left=max(pPaintRect->left,pBorderRect->left);
		rc.right=min(pPaintRect->right,pBorderRect->right);
		rc.top=max(pPaintRect->top,pBorderRect->top);
		rc.bottom=min(pPaintRect->bottom,pEmptyRect->top);
		if (rc.top<rc.bottom)
			::FillRect(hdc,&rc,hbr);
		rc.top=max(pEmptyRect->bottom,pPaintRect->top);
		rc.bottom=min(pPaintRect->bottom,pBorderRect->bottom);
		if (rc.top<rc.bottom)
			::FillRect(hdc,&rc,hbr);
	}
	if (pPaintRect->top<pEmptyRect->bottom && pPaintRect->bottom>pEmptyRect->top) {
		rc.top=max(pEmptyRect->top,pPaintRect->top);
		rc.bottom=min(pEmptyRect->bottom,pPaintRect->bottom);
		rc.left=max(pPaintRect->left,pBorderRect->left);
		rc.right=min(pEmptyRect->left,pPaintRect->right);
		if (rc.left<rc.right)
			::FillRect(hdc,&rc,hbr);
		rc.left=max(pPaintRect->left,pEmptyRect->right);
		rc.right=min(pPaintRect->right,pBorderRect->right);
		if (rc.left<rc.right)
			::FillRect(hdc,&rc,hbr);
	}
	return true;
}




#include <gdiplus.h>


#pragma comment(lib, "gdiplus.lib")
//#pragma comment(linker, "/DELAYLOAD:gdiplus.dll")




CGdiPlus::CGdiPlus()
	: m_fInitialized(false)
{
}


CGdiPlus::~CGdiPlus()
{
	Finalize();
}


bool CGdiPlus::Initialize()
{
	if (!m_fInitialized) {
		HMODULE hLib=::LoadLibrary(TEXT("gdiplus.dll"));
		if (hLib==NULL)
			return false;
		::FreeLibrary(hLib);

		Gdiplus::GdiplusStartupInput si;
		si.GdiplusVersion=1;
		si.DebugEventCallback=NULL;
		si.SuppressBackgroundThread=FALSE;
		si.SuppressExternalCodecs=FALSE;
		if (Gdiplus::GdiplusStartup(&m_Token,&si,NULL)!=Gdiplus::Ok)
			return false;
		m_fInitialized=true;
	}
	return true;
}


void CGdiPlus::Finalize()
{
	if (m_fInitialized) {
		Gdiplus::GdiplusShutdown(m_Token);
		m_fInitialized=false;
	}
}


bool CGdiPlus::DrawImage(CCanvas *pCanvas,CImage *pImage,int x,int y)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			 && pImage!=NULL && pImage->m_pBitmap!=NULL) {
		return pCanvas->m_pGraphics->DrawImage(pImage->m_pBitmap,x,y,
											   pImage->Width(),
											   pImage->Height())==Gdiplus::Ok;
	}
	return false;
}


bool CGdiPlus::FillRect(CCanvas *pCanvas,CBrush *pBrush,const RECT *pRect)
{
	if (pCanvas!=NULL && pCanvas->m_pGraphics!=NULL
			&& pBrush!=NULL && pBrush->m_pBrush!=NULL && pRect!=NULL) {
		return pCanvas->m_pGraphics->FillRectangle(pBrush->m_pBrush,
												   pRect->left,pRect->top,
												   pRect->right-pRect->left,
												   pRect->bottom-pRect->top)==Gdiplus::Ok;
	}
	return false;
}




CGdiPlus::CImage::CImage()
	: m_pBitmap(NULL)
{
}


CGdiPlus::CImage::~CImage()
{
	Free();
}


void CGdiPlus::CImage::Free()
{
	if (m_pBitmap!=NULL) {
		delete m_pBitmap;
		m_pBitmap=NULL;
	}
}


bool CGdiPlus::CImage::LoadFromFile(LPCWSTR pszFileName)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromFile(pszFileName);
	return m_pBitmap!=NULL;
}


bool CGdiPlus::CImage::LoadFromResource(HINSTANCE hinst,LPCWSTR pszName)
{
	Free();
	m_pBitmap=Gdiplus::Bitmap::FromResource(hinst,pszName);
	return m_pBitmap!=NULL;
}


bool CGdiPlus::CImage::LoadFromResource(HINSTANCE hinst,LPCTSTR pszName,LPCTSTR pszType)
{
	Free();

	HRSRC hRes=::FindResource(hinst,pszName,pszType);
	if (hRes==NULL)
		return false;
	DWORD Size=::SizeofResource(hinst,hRes);
	if (Size==0)
		return false;
	HGLOBAL hData=::LoadResource(hinst,hRes);
	const void *pData=::LockResource(hData);
	if (pData==NULL)
		return false;
	HGLOBAL hBuffer=::GlobalAlloc(GMEM_MOVEABLE,Size);
	if (hBuffer==NULL)
		return false;
	void *pBuffer=::GlobalLock(hBuffer);
	if (pBuffer==NULL) {
		::GlobalFree(hBuffer);
		return false;
	}
	::CopyMemory(pBuffer,pData,Size);
	::GlobalUnlock(hBuffer);
	IStream *pStream;
	if (::CreateStreamOnHGlobal(hBuffer,TRUE,&pStream)!=S_OK) {
		::GlobalFree(hBuffer);
		return false;
	}
	m_pBitmap=Gdiplus::Bitmap::FromStream(pStream);
	pStream->Release();
	return m_pBitmap!=NULL;
}


int CGdiPlus::CImage::Width() const
{
	if (m_pBitmap==NULL)
		return 0;
	return m_pBitmap->GetWidth();
}


int CGdiPlus::CImage::Height() const
{
	if (m_pBitmap==NULL)
		return 0;
	return m_pBitmap->GetHeight();
}




CGdiPlus::CBrush::CBrush()
	: m_pBrush(NULL)
{
}


CGdiPlus::CBrush::CBrush(BYTE r,BYTE g,BYTE b,BYTE a)
{
	m_pBrush=new Gdiplus::SolidBrush(Gdiplus::Color(a,r,g,b));
}


CGdiPlus::CBrush::CBrush(COLORREF Color)
{
	m_pBrush=new Gdiplus::SolidBrush(Gdiplus::Color(255,GetRValue(Color),GetGValue(Color),GetBValue(Color)));
}


CGdiPlus::CBrush::~CBrush()
{
	Free();
}


void CGdiPlus::CBrush::Free()
{
	if (m_pBrush!=NULL) {
		delete m_pBrush;
		m_pBrush=NULL;
	}
}


bool CGdiPlus::CBrush::CreateSolidBrush(BYTE r,BYTE g,BYTE b,BYTE a)
{
	Gdiplus::Color Color(a,r,g,b);

	if (m_pBrush!=NULL) {
		m_pBrush->SetColor(Color);
	} else {
		m_pBrush=new Gdiplus::SolidBrush(Color);
		if (m_pBrush==NULL)
			return false;
	}
	return true;
}




CGdiPlus::CCanvas::CCanvas(HDC hdc)
{
	m_pGraphics=new Gdiplus::Graphics(hdc);
}


CGdiPlus::CCanvas::~CCanvas()
{
	if (m_pGraphics!=NULL)
		delete m_pGraphics;
}


bool CGdiPlus::CCanvas::Clear(BYTE r,BYTE g,BYTE b,BYTE a)
{
	if (m_pGraphics==NULL)
		return false;
	return m_pGraphics->Clear(Gdiplus::Color(a,r,g,b))==Gdiplus::Ok;
}
