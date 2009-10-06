#include "stdafx.h"
#include "TVTest.h"
#include "NotificationBar.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define NOTIFICATION_BAR_WINDOW_CLASS APP_NAME TEXT(" Notification Bar")

#define BAR_MARGIN 4

#define TIMER_ID_HIDE 1




HINSTANCE CNotificationBar::m_hinst=NULL;


bool CNotificationBar::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=NOTIFICATION_BAR_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CNotificationBar::CNotificationBar()
	: m_hfont(NULL)
{
	m_BackGradient.Type=Theme::GRADIENT_NORMAL;
	m_BackGradient.Direction=Theme::DIRECTION_VERT;
	m_BackGradient.Color1=RGB(128,128,128);
	m_BackGradient.Color2=RGB(64,64,64);
	m_TextColor=RGB(224,224,224);
	m_ErrorTextColor=RGB(224,64,64);
	NONCLIENTMETRICS ncm;
#if WINVER<0x0600
	ncm.cbSize=sizeof(ncm);
#else
	ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
	ncm.lfMessageFont.lfHeight=-14;
	SetFont(&ncm.lfMessageFont);
	m_fAnimate=true;
	m_pszText=NULL;
}


CNotificationBar::~CNotificationBar()
{
	::DeleteObject(m_hfont);
	delete [] m_pszText;
}


bool CNotificationBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 NOTIFICATION_BAR_WINDOW_CLASS,NULL,m_hinst);
}


bool CNotificationBar::Show(DWORD Timeout)
{
	if (m_hwnd==NULL)
		return false;

	if (!GetVisible()) {
		RECT rc;

		::GetClientRect(::GetParent(m_hwnd),&rc);
		::BringWindowToTop(m_hwnd);
		if (m_fAnimate) {
			for (int i=0;i<4;i++) {
				rc.bottom=(i+1)*m_BarHeight/4;
				SetPosition(&rc);
				if (i==0)
					SetVisible(true);
				Update();
				::Sleep(50);
			}
		} else {
			rc.bottom=m_BarHeight;
			SetPosition(&rc);
			SetVisible(true);
			Update();
		}
	}
	if (Timeout!=0)
		::SetTimer(m_hwnd,TIMER_ID_HIDE,Timeout,NULL);
	return true;
}


bool CNotificationBar::Hide()
{
	if (m_hwnd==NULL)
		return false;

	RECT rc;

	GetPosition(&rc);
	if (m_fAnimate) {
		for (int i=0;i<3;i++) {
			rc.bottom=(3-i)*m_BarHeight/4;
			SetPosition(&rc);
			Update();
			::Sleep(50);
		}
	}
	SetVisible(false);
	return true;
}


bool CNotificationBar::SetText(LPCTSTR pszText,MessageType Type)
{
	ReplaceString(&m_pszText,pszText);
	m_MessageType=Type;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CNotificationBar::SetColors(const Theme::GradientInfo *pBackGradient,
								 COLORREF crTextColor,COLORREF crErrorTextColor)
{
	m_BackGradient=*pBackGradient;
	m_TextColor=crTextColor;
	m_ErrorTextColor=crErrorTextColor;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CNotificationBar::SetFont(const LOGFONT *pFont)
{
	if (pFont==NULL)
		return false;
	HFONT hfont=::CreateFontIndirect(pFont);
	if (hfont==NULL)
		return false;
	if (m_hfont!=NULL)
		::DeleteObject(m_hfont);
	m_hfont=hfont;
	m_BarHeight=abs(pFont->lfHeight)+BAR_MARGIN*2;
	return true;
}


CNotificationBar *CNotificationBar::GetThis(HWND hwnd)
{
	return static_cast<CNotificationBar*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CNotificationBar::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CNotificationBar *pThis=static_cast<CNotificationBar*>(OnCreate(hwnd,lParam));

		}
		return 0;

	case WM_PAINT:
		{
			CNotificationBar *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			RECT rc;

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rc);
			Theme::FillGradient(ps.hdc,&rc,&pThis->m_BackGradient);
			if (pThis->m_pszText!=NULL) {
				COLORREF OldTextColor;
				int OldBkMode;
				HFONT hfontOld;

				OldTextColor=::SetTextColor(ps.hdc,
					pThis->m_MessageType==MESSAGE_ERROR?
								pThis->m_ErrorTextColor:pThis->m_TextColor);
				OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
				hfontOld=static_cast<HFONT>(::SelectObject(ps.hdc,pThis->m_hfont));
				rc.left+=BAR_MARGIN;
				rc.right-=BAR_MARGIN;
				if (rc.left<rc.right)
					::DrawText(ps.hdc,pThis->m_pszText,-1,&rc,
							   DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				::SetTextColor(ps.hdc,OldTextColor);
				::SetBkMode(ps.hdc,OldBkMode);
				::SelectObject(ps.hdc,hfontOld);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_TIMER:
		{
			CNotificationBar *pThis=GetThis(hwnd);

			pThis->Hide();
			::KillTimer(hwnd,TIMER_ID_HIDE);
		}
		return 0;

	case WM_DESTROY:
		{
			CNotificationBar *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}
