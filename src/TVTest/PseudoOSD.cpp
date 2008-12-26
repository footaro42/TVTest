#include "stdafx.h"
#include "TVTest.h"
#include "PseudoOSD.h"


#define PSEUDO_OSD_WINDOW_CLASS APP_NAME TEXT(" Pseudo OSD")




HINSTANCE CPseudoOSD::m_hinst=NULL;


bool CPseudoOSD::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=PSEUDO_OSD_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CPseudoOSD::CPseudoOSD()
{
	m_hwnd=NULL;
	m_crBackColor=RGB(16,0,16);
	m_crTextColor=RGB(0,255,128);
	LOGFONT lf;
	GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	lf.lfHeight=32;
	m_hFont=CreateFontIndirect(&lf);
	m_pszText=NULL;
	m_hbm=NULL;
	m_HideTimerID=0;
}


CPseudoOSD::~CPseudoOSD()
{
	Destroy();
	DeleteObject(m_hFont);
	delete [] m_pszText;
	if (m_hbm)
		DeleteObject(m_hbm);
}


bool CPseudoOSD::Create(HWND hwndParent)
{
	if (m_hwnd!=NULL)
		return false;
	return CreateWindowEx(0,PSEUDO_OSD_WINDOW_CLASS,NULL,WS_CHILD,
								0,0,0,0,hwndParent,NULL,m_hinst,this)!=NULL;
}


bool CPseudoOSD::Destroy()
{
	if (m_hwnd!=NULL)
		DestroyWindow(m_hwnd);
	return true;
}


bool CPseudoOSD::Show(DWORD Time)
{
	if (m_hwnd==NULL)
		return false;
	ShowWindow(m_hwnd,SW_SHOW);
	SetWindowPos(m_hwnd,HWND_TOP,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
	UpdateWindow(m_hwnd);
	if (Time>0) {
		m_HideTimerID=SetTimer(m_hwnd,1,Time,NULL);
	} else if (m_HideTimerID!=0) {
		KillTimer(m_hwnd,m_HideTimerID);
		m_HideTimerID=0;
	}
	return true;
}


bool CPseudoOSD::Hide()
{
	if (m_hwnd==NULL)
		return false;
	ShowWindow(m_hwnd,SW_HIDE);
	if (m_pszText!=NULL) {
		delete [] m_pszText;
		m_pszText=NULL;
	}
	if (m_hbm!=NULL) {
		DeleteObject(m_hbm);
		m_hbm=NULL;
	}
	return true;
}


bool CPseudoOSD::SetText(LPCTSTR pszText)
{
	ReplaceString(&m_pszText,pszText);
	if (m_hbm!=NULL) {
		DeleteObject(m_hbm);
		m_hbm=NULL;
	}
	if (m_hwnd!=NULL)
		InvalidateRect(m_hwnd,NULL,TRUE);
	return true;
}


bool CPseudoOSD::SetPosition(int Left,int Top,int Width,int Height)
{
	if (m_hwnd==NULL)
		return false;
	MoveWindow(m_hwnd,Left,Top,Width,Height,TRUE);
	return true;
}


void CPseudoOSD::SetTextColor(COLORREF crText)
{
	m_crTextColor=crText;
	if (m_hwnd!=NULL)
		InvalidateRect(m_hwnd,NULL,TRUE);
}


bool CPseudoOSD::SetTextHeight(int Height)
{
	LOGFONT lf;
	HFONT hfont;

	GetObject(m_hFont,sizeof(LOGFONT),&lf);
	lf.lfHeight=Height;
	hfont=CreateFontIndirect(&lf);
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
		hdc=GetDC(m_hwnd);
	else
		hdc=CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
	hfontOld=static_cast<HFONT>(SelectObject(hdc,m_hFont));
	fResult=GetTextExtentPoint32(hdc,m_pszText,lstrlen(m_pszText),pSize)!=0;
	SelectObject(hdc,hfontOld);
	if (m_hwnd!=NULL)
		ReleaseDC(m_hwnd,hdc);
	else
		DeleteDC(hdc);
	return fResult;
}


bool CPseudoOSD::SetImage(HBITMAP hbm,int Left,int Top)
{
	if (m_hbm)
		DeleteObject(m_hbm);
	m_hbm=hbm;
	if (m_pszText!=NULL) {
		delete [] m_pszText;
		m_pszText=NULL;
	}
	if (m_hwnd!=NULL) {
		BITMAP bm;

		GetObject(m_hbm,sizeof(BITMAP),&bm);
		MoveWindow(m_hwnd,Left,Top,bm.bmWidth,bm.bmHeight,TRUE);
	}
	return true;
}


CPseudoOSD *CPseudoOSD::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPseudoOSD*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
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
			SetWindowLongPtr(hwnd,GWLP_USERDATA,
											reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_PAINT:
		{
			CPseudoOSD *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			if (pThis->m_pszText!=NULL) {
				HBRUSH hbr;
				HFONT hfontOld;
				COLORREF crOldTextColor;
				int OldBkMode;
				RECT rc;

				hbr=CreateSolidBrush(pThis->m_crBackColor);
				FillRect(ps.hdc,&ps.rcPaint,hbr);
				DeleteObject(hbr);
				hfontOld=static_cast<HFONT>(SelectObject(ps.hdc,pThis->m_hFont));
				crOldTextColor=::SetTextColor(ps.hdc,pThis->m_crTextColor);
				OldBkMode=SetBkMode(ps.hdc,TRANSPARENT);
				GetClientRect(hwnd,&rc);
				DrawText(ps.hdc,pThis->m_pszText,-1,&rc,
					DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP);
				SetBkMode(ps.hdc,OldBkMode);
				::SetTextColor(ps.hdc,crOldTextColor);
				SelectObject(ps.hdc,hfontOld);
			} else if (pThis->m_hbm!=NULL) {
				HDC hdcMem=CreateCompatibleDC(ps.hdc);
				HBITMAP hbmOld;
				BITMAP bm;

				hbmOld=static_cast<HBITMAP>(SelectObject(hdcMem,pThis->m_hbm));
				GetObject(pThis->m_hbm,sizeof(BITMAP),&bm);
				BitBlt(ps.hdc,0,0,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);
				SelectObject(hdcMem,hbmOld);
				DeleteDC(hdcMem);
			}
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			pThis->Hide();
			KillTimer(hwnd,wParam);
			pThis->m_HideTimerID=0;
		}
		return 0;

	case WM_DESTROY:
		{
			CPseudoOSD *pThis=GetThis(hwnd);

			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}
