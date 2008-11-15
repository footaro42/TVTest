#include "stdafx.h"
#include "TVTest.h"
#include "Fullscreen.h"




HINSTANCE CFullscreen::m_hinst=NULL;


bool CFullscreen::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hInst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=CreateSolidBrush(RGB(0,0,0));
		wc.lpszMenuName=NULL;
		wc.lpszClassName=FULLSCREEN_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CFullscreen *CFullscreen::GetThis(HWND hwnd)
{
	return static_cast<CFullscreen*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CFullscreen::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,
																LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CFullscreen *pThis=dynamic_cast<CFullscreen*>(OnCreate(hwnd,lParam));

			pThis->m_pVideoContainer->SetParent(pThis);
			RECT rc;
			pThis->GetClientRect(&rc);
			pThis->m_pVideoContainer->SetPosition(&rc);
			pThis->m_pDtvEngine->m_MediaViewer.SetViewStretchMode(pThis->m_StretchMode);
			pThis->m_fShowCursor=true;
			pThis->m_fMenu=false;
			pThis->m_fShowStatusView=false;
			pThis->m_fShowTitleBar=false;
			::SetTimer(hwnd,1,1000,NULL);
		}
		return 0;

	case WM_SIZE:
		GeThis(hwnd)->m_pVideoContainer->SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		return 0;

	case WM_RBUTTONDOWN:
		{
			CFullscreen *pThis=GetThis(hwnd);

			pThis->OnRButtonDown();
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		MainWindow.SendCommand(CM_FULLSCREEN);
		return 0;

	case WM_MOUSEMOVE:
		{
			CFullscreen *pThis=GetThis(hwnd);

			pThis->OnMouseMove();
		}
		return 0;

	case WM_TIMER:
		{
			CFullscreen *pThis=GetThis(hwnd);

			if (!pThis->m_fMenu) {
				::SetCursor(NULL);
				pThis->m_pDtvEngine->m_MediaViewer.HideCursor(true);
				pThis->m_fShowCursor=false;
			}
			::KillTimer(hwnd,1);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CFullscreen *pThis=GetThis(hwnd);

			::SetCursor(pThis->m_fShowCursor?LoadCursor(NULL,IDC_ARROW):NULL);
			return TRUE;
		}
		break;

	case WM_MOUSEWHEEL:
		{
			CFullscreen *pThis=GetThis(hwnd);

			MainWindow.OnMouseWheel(wParam,lParam,pThis->m_fShowStatusView);
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			MainWindow.SendCommand(CM_FULLSCREEN);
			return 0;
		}
	case WM_COMMAND:
		return MainWindow.SendMessage(uMsg,wParam,lParam);

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_MONITORPOWER:
			if (fNoMonitorLowPower)
				return TRUE;
			break;
		}
		break;

	case WM_DESTROY:
		{
			CFullscreen *pThis=GetThis(hwnd);
			SIZE sz;

			ViewWindow.GetClientSize(&sz);
			VideoContainerWindow.SetParent(&ViewWindow);
			ViewWindow.SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
			//if (!m_fShowCursor)
				CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
			CoreEngine.m_DtvEngine.m_MediaViewer.SetViewStretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO);
			pThis->ShowStatusView(false);
			pThis->ShowTitleBar(false);
			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CFullscreen::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 FULLSCREEN_WINDOW_CLASS,NULL,hInst);
}


bool CFullscreen::Create(CBasicWindow *pOwnerWindow,CVideoContainerWindow *pVideoContainer,CStatusView *pStatusView,CTitleBar *pTitleBar)
{
	HMONITOR hMonitor;
	int x,y,Width,Height;

	hMonitor=::MonitorFromWindow(MainWindow.GetHandle(),MONITOR_DEFAULTTONEAREST);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(MONITORINFO);
		::GetMonitorInfo(hMonitor,&mi);
		x=mi.rcMonitor.left;
		y=mi.rcMonitor.top;
		Width=mi.rcMonitor.right-mi.rcMonitor.left;
		Height=mi.rcMonitor.bottom-mi.rcMonitor.top;
	} else {
		x=y=0;
		Width=::GetSystemMetrics(SM_CXSCREEN);
		Height=::GetSystemMetrics(SM_CYSCREEN);
	}
	SetPosition(x,y,Width,Height);
	m_pOwnerWindow=pOwnerWindow;
	m_pVideoContainer=pVideoContainer;
	m_pStatusView=pStatusView;
	m_pTitleBar=pTitleBar;
	return Create(pOwnerWindow->GetHandle(),WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN,WS_EX_TOPMOST);
}


CFullscreen::CFullscreen()
	: m_pOwnerWindow(NULL)
	, m_pVideoContainer(NULL)
	, m_pStatusView(NULL)
	, m_pTitleBar(NULL)
	, m_StretchMode(CMediaViewer::STRETCH_KEEPASPECTRATIO)
{
}


CFullscreen::~CFullscreen()
{
	Destroy();
}


void CFullscreen::OnRButtonDown()
{
	// メニュー表示中はカーソルを消さない
	KillTimer(m_hwnd,1);
	m_fShowCursor=true;
	CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
	::SetCursor(LoadCursor(NULL,IDC_ARROW));
	m_fMenu=true;
	MainWindow.PopupMenu();
	m_fMenu=false;
	::SetTimer(m_hwnd,1,1000,NULL);
}


void CFullscreen::OnMouseMove()
{
	POINT pt;
	RECT rcClient,rc;

	if (m_fMenu)
		return;
	if (!m_fShowCursor) {
		::SetCursor(LoadCursor(NULL,IDC_ARROW));
		CoreEngine.m_DtvEngine.m_MediaViewer.HideCursor(false);
		m_fShowCursor=true;
	}
	GetCursorPos(&pt);
	::ScreenToClient(m_hwnd,&pt);
	GetClientRect(&rcClient);
	rc=rcClient;
	rc.top=rc.bottom-StatusView.GetHeight();
	if (::PtInRect(&rc,pt)) {
		if (!m_fShowStatusView) {
			ShowStatusView(true);
			::KillTimer(m_hwnd,1);
		}
	} else {
		if (m_fShowStatusView)
			ShowStatusView(false);
		::SetTimer(m_hwnd,1,1000,NULL);
	}
	rc.top=rcClient.top;
	rc.bottom=rc.top+TitleBar.GetHeight();
	if (::PtInRect(&rc,pt)) {
		if (!m_fShowTitleBar) {
			ShowTitleBar(true);
			::KillTimer(m_hwnd,1);
		}
	} else {
		if (m_fShowTitleBar)
			ShowTitleBar(false);
		::SetTimer(m_hwnd,1,1000,NULL);
	}
}


void CFullscreen::ShowStatusView(bool fShow)
{
	if (fShow==m_fShowStatusView)
		return;
	if (fShow) {
		RECT rc;

		GetClientRect(&rc);
		rc.top=rc.bottom-StatusView.GetHeight();
		StatusView.SetVisible(false);
		StatusView.SetParent(&VideoContainerWindow);
		StatusView.SetPosition(&rc);
		StatusView.SetVisible(true);
	} else {
		StatusView.SetVisible(false);
		StatusView.SetParent(&MainWindow);
		if (MainWindow.GetStatusBarVisible()) {
			SIZE sz;

			MainWindow.GetClientSize(&sz);
			MainWindow.SendMessage(WM_SIZE,0,MAKELPARAM(sz.cx,sz.cy));
			StatusView.SetVisible(true);
		}
	}
	m_fShowStatusView=fShow;
}


void CFullscreen::ShowTitleBar(bool fShow)
{
	if (fShow==m_fShowTitleBar)
		return;
	if (fShow) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=rc.top+TitleBar.GetHeight();
		TitleBar.SetVisible(false);
		TitleBar.SetParent(&VideoContainerWindow);
		TitleBar.SetPosition(&rc);
		TitleBar.SetVisible(true);
	} else {
		TitleBar.SetVisible(false);
		TitleBar.SetParent(&MainWindow);
	}
	m_fShowTitleBar=fShow;
}
