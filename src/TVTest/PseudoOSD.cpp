#include "stdafx.h"
#include "TVTest.h"
#include "PseudoOSD.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TIMER_ID_HIDE		1
#define TIMER_ID_ANIMATION	2

#define ANIMATION_FRAMES	4
#define ANIMATION_INTERVAL	50




const LPCTSTR CPseudoOSD::m_pszWindowClass=APP_NAME TEXT(" Pseudo OSD");
HINSTANCE CPseudoOSD::m_hinst=NULL;


bool CPseudoOSD::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


bool CPseudoOSD::IsPseudoOSD(HWND hwnd)
{
	TCHAR szClass[64];

	return ::GetClassName(hwnd,szClass,lengthof(szClass))>0
		&& ::lstrcmpi(szClass,m_pszWindowClass)==0;
}


CPseudoOSD::CPseudoOSD()
{
	m_hwnd=NULL;
	m_crBackColor=RGB(16,0,16);
	m_crTextColor=RGB(0,255,128);
#if 1
	LOGFONT lf;
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	lf.lfHeight=32;
	lf.lfQuality=NONANTIALIASED_QUALITY;
	m_hFont=::CreateFontIndirect(&lf);
#else
	NONCLIENTMETRICS ncm;
#if WINVER<0x0600
	ncm.cbSize=sizeof(ncm);
#else
	ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
	ncm.lfMessageFont.lfHeight=32;
	ncm.lfMessageFont.lfQuality=NONANTIALIASED_QUALITY;
	m_hFont=::CreateFontIndirect(&ncm.lfMessageFont);
#endif
	m_pszText=NULL;
	m_hbm=NULL;
	m_Position.Left=0;
	m_Position.Top=0;
	m_Position.Width=0;
	m_Position.Height=0;
	m_TimerID=0;
}


CPseudoOSD::~CPseudoOSD()
{
	Destroy();
	::DeleteObject(m_hFont);
	delete [] m_pszText;
	if (m_hbm)
		::DeleteObject(m_hbm);
}


bool CPseudoOSD::Create(HWND hwndParent)
{
	if (m_hwnd!=NULL)
		return false;
	return ::CreateWindowEx(0,m_pszWindowClass,NULL,WS_CHILD,
							m_Position.Left,m_Position.Top,
							m_Position.Width,m_Position.Height,
							hwndParent,NULL,m_hinst,this)!=NULL;
}


bool CPseudoOSD::Destroy()
{
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);
	return true;
}


bool CPseudoOSD::Show(DWORD Time,bool fAnimation)
{
	if (m_hwnd==NULL)
		return false;
	if (Time>0) {
		m_TimerID|=::SetTimer(m_hwnd,TIMER_ID_HIDE,Time,NULL);
		if (fAnimation) {
			m_AnimationCount=0;
			::MoveWindow(m_hwnd,m_Position.Left,m_Position.Top,
						 m_Position.Width/ANIMATION_FRAMES,m_Position.Height,
						 TRUE);
			m_TimerID|=::SetTimer(m_hwnd,TIMER_ID_ANIMATION,ANIMATION_INTERVAL,NULL);
		} else {
			::MoveWindow(m_hwnd,m_Position.Left,m_Position.Top,
						 m_Position.Width,m_Position.Height,TRUE);
		}
	} else if ((m_TimerID&TIMER_ID_HIDE)!=0) {
		::KillTimer(m_hwnd,TIMER_ID_HIDE);
		m_TimerID&=~TIMER_ID_HIDE;
	}
	::ShowWindow(m_hwnd,SW_SHOW);
	//::SetWindowPos(m_hwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
	::BringWindowToTop(m_hwnd);
	::UpdateWindow(m_hwnd);
	return true;
}


bool CPseudoOSD::Hide()
{
	if (m_hwnd==NULL)
		return false;
	::ShowWindow(m_hwnd,SW_HIDE);
	SAFE_DELETE_ARRAY(m_pszText);
	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
	}
	return true;
}


bool CPseudoOSD::IsVisible() const
{
	if (m_hwnd==NULL)
		return false;
	return ::IsWindowVisible(m_hwnd)!=FALSE;
}


bool CPseudoOSD::SetText(LPCTSTR pszText)
{
	ReplaceString(&m_pszText,pszText);
	if (m_hbm!=NULL) {
		::DeleteObject(m_hbm);
		m_hbm=NULL;
	}
	if (m_hwnd!=NULL)
		::InvalidateRect(m_hwnd,NULL,TRUE);
	return true;
}


bool CPseudoOSD::SetPosition(int Left,int Top,int Width,int Height)
{
	if (Width<=0 || Height<=0)
		return false;
	m_Position.Left=Left;
	m_Position.Top=Top;
	m_Position.Width=Width;
	m_Position.Height=Height;
	if (m_hwnd!=NULL)
		::SetWindowPos(m_hwnd,HWND_TOP,Left,Top,Width,Height,0);
	return true;
}


void CPseudoOSD::GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const
{
	if (pLeft)
		*pLeft=m_Position.Left;
	if (pTop)
		*pTop=m_Position.Top;
	if (pWidth)
		*pWidth=m_Position.Width;
	if (pHeight)
		*pHeight=m_Position.Height;
}


void CPseudoOSD::SetTextColor(COLORREF crText)
{
	m_crTextColor=crText;
	if (m_hwnd!=NULL)
		::InvalidateRect(m_hwnd,NULL,TRUE);
}


bool CPseudoOSD::SetTextHeight(int Height)
{
	LOGFONT lf;
	HFONT hfont;

	::GetObject(m_hFont,sizeof(LOGFONT),&lf);
	lf.lfHeight=Height;
	hfont=::CreateFontIndirect(&lf);
	if (hfont==NULL)
		return false;
	DeleteObject(m_hFont);
	m_hFont=hfont;
	return true;
}


bool CPseudoOSD::CalcTextSize(SIZE *pSize)
{
	HDC hdc;
	HFONT hfontOld;
	bool fResult;

	if (m_pszText==NULL) {
		pSize->cx=0;
		pSize->cy=0;
		return true;
	}
	if (m_hwnd!=NULL)
		hdc=::GetDC(m_hwnd);
	else
		hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
	hfontOld=static_cast<HFONT>(::SelectObject(hdc,m_hFont));
	//fResult=::GetTextExtentPoint32(hdc,m_pszText,lstrlen(m_pszText),pSize)!=0;
	RECT rc={0,0,0,0};
	fResult=::DrawText(hdc,m_pszText,-1,&rc,DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX)!=0;
	if (fResult) {
		pSize->cx=rc.right-rc.left;
		pSize->cy=rc.bottom-rc.top;
	}
	::SelectObject(hdc,hfontOld);
	if (m_hwnd!=NULL)
		::ReleaseDC(m_hwnd,hdc);
	else
		::DeleteDC(hdc);
	return fResult;
}


bool CPseudoOSD::SetImage(HBITMAP hbm,int Left,int Top)
{
	if (m_hbm)
		::DeleteObject(m_hbm);
	m_hbm=hbm;
	SAFE_DELETE_ARRAY(m_pszText);
	if (m_hwnd!=NULL) {
		BITMAP bm;

		::GetObject(m_hbm,sizeof(BITMAP),&bm);
		m_Position.Left=Left;
		m_Position.Top=Top;
		m_Position.Width=bm.bmWidth;
		m_Position.Height=bm.bmHeight;
		::MoveWindow(m_hwnd,Left,Top,bm.bmWidth,bm.bmHeight,TRUE);
	}
	return true;
}


CPseudoOSD *CPseudoOSD::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPseudoOSD*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CPseudoOSD::WndProc(HWND hwnd,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CPseudoOSD *pThis=static_cast<CPseudoOSD*>(pcs->lpCreateParams);

			pThis->m_hwnd=hwnd;
			::SetWindowLongPtr(hwnd,GWLP_USERDATA,
											reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_PAINT:
		{
			CPseudoOSD *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			if (pThis->m_pszText!=NULL) {
				HBRUSH hbr;
				HFONT hfontOld;
				COLORREF crOldTextColor;
				int OldBkMode;
				RECT rc;

				hbr=CreateSolidBrush(pThis->m_crBackColor);
				::FillRect(ps.hdc,&ps.rcPaint,hbr);
				::DeleteObject(hbr);
				hfontOld=static_cast<HFONT>(::SelectObject(ps.hdc,pThis->m_hFont));
				crOldTextColor=::SetTextColor(ps.hdc,pThis->m_crTextColor);
				OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
				::GetClientRect(hwnd,&rc);
				::DrawText(ps.hdc,pThis->m_pszText,-1,&rc,
					DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
				::SetBkMode(ps.hdc,OldBkMode);
				::SetTextColor(ps.hdc,crOldTextColor);
				::SelectObject(ps.hdc,hfontOld);
			} else if (pThis->m_hbm!=NULL) {
				HDC hdcMem=::CreateCompatibleDC(ps.hdc);
				HBITMAP hbmOld;
				BITMAP bm;

				hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,pThis->m_hbm));
				::GetObject(pThis->m_hbm,sizeof(BITMAP),&bm);
				::BitBlt(ps.hdc,0,0,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);
				::SelectObject(hdcMem,hbmOld);
				::DeleteDC(hdcMem);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			switch (wParam) {
			case TIMER_ID_HIDE:
				pThis->Hide();
				::KillTimer(hwnd,TIMER_ID_HIDE);
				pThis->m_TimerID&=~TIMER_ID_HIDE;
				if ((pThis->m_TimerID&TIMER_ID_ANIMATION)!=0) {
					::KillTimer(hwnd,TIMER_ID_ANIMATION);
					pThis->m_TimerID&=~TIMER_ID_ANIMATION;
				}
				break;

			case TIMER_ID_ANIMATION:
				pThis->m_AnimationCount++;
				::MoveWindow(hwnd,pThis->m_Position.Left,pThis->m_Position.Top,
							 pThis->m_Position.Width*(pThis->m_AnimationCount+1)/ANIMATION_FRAMES,
							 pThis->m_Position.Height,
							 TRUE);
				::UpdateWindow(hwnd);
				if (pThis->m_AnimationCount+1==ANIMATION_FRAMES) {
					::KillTimer(hwnd,TIMER_ID_ANIMATION);
					pThis->m_TimerID&=~TIMER_ID_ANIMATION;
				}
				break;
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			POINT pt;
			HWND hwndParent=::GetParent(hwnd);

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd,hwndParent,&pt,1);
			return ::SendMessage(hwndParent,uMsg,wParam,MAKELPARAM(pt.x,pt.y));
		}

	case WM_DESTROY:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}
