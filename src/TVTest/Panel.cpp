#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "Panel.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define PANEL_WINDOW_CLASS			APP_NAME TEXT(" Panel")
#define PANEL_FRAME_WINDOW_CLASS	APP_NAME TEXT(" Panel Frame")
#define DROP_HELPER_WINDOW_CLASS	APP_NAME TEXT(" Drop Helper")




HINSTANCE CPanel::m_hinst=NULL;


bool CPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=PANEL_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CPanel::CPanel()
{
	NONCLIENTMETRICS ncm;

	m_TitleMargin=4;
	m_ButtonSize=14;
#if WINVER<0x0600
	ncm.cbSize=sizeof(ncm);
#else
	ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
	m_hfont=CreateFontIndirect(&ncm.lfCaptionFont);
	m_TitleHeight=max(abs(ncm.lfCaptionFont.lfHeight),m_ButtonSize)+m_TitleMargin*2;
	m_pWindow=NULL;
	m_pszTitle=NULL;
	m_fShowTitle=false;
	m_fEnableFloating=true;
	m_TitleBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_TitleBackGradient.Direction=Theme::DIRECTION_VERT;
	m_TitleBackGradient.Color1=GetSysColor(COLOR_INACTIVECAPTION);
	m_TitleBackGradient.Color2=m_TitleBackGradient.Color1;
	m_crTitleTextColor=GetSysColor(COLOR_INACTIVECAPTIONTEXT);
	m_pEventHandler=NULL;
}


CPanel::~CPanel()
{
	delete [] m_pszTitle;
}


bool CPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 PANEL_WINDOW_CLASS,NULL,m_hinst);
}


bool CPanel::SetWindow(CBasicWindow *pWindow,LPCTSTR pszTitle)
{
	RECT rc;

	m_pWindow=pWindow;
	if (m_pWindow!=NULL) {
		if (pWindow->GetParent()!=m_hwnd)
			pWindow->SetParent(m_hwnd);
		pWindow->SetVisible(true);
		ReplaceString(&m_pszTitle,pszTitle);
		GetPosition(&rc);
		rc.right=rc.left+pWindow->GetWidth();
		SetPosition(&rc);
	} else {
		ReplaceString(&m_pszTitle,NULL);
	}
	return true;
}


void CPanel::ShowTitle(bool fShow)
{
	if (m_fShowTitle!=fShow) {
		RECT rc;

		m_fShowTitle=fShow;
		GetClientRect(&rc);
		OnSize(rc.right,rc.bottom);
	}
}


void CPanel::EnableFloating(bool fEnable)
{
	m_fEnableFloating=fEnable;
}


void CPanel::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


bool CPanel::SetTitleColor(const Theme::GradientInfo *pBackGradient,COLORREF crTitleText)
{
	m_TitleBackGradient=*pBackGradient;
	m_crTitleTextColor=crTitleText;
	if (m_hwnd!=NULL && m_fShowTitle) {
		RECT rc;

		GetTitleRect(&rc);
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
	return true;
}


bool CPanel::GetTitleRect(RECT *pRect) const
{
	if (m_hwnd==NULL)
		return false;

	GetClientRect(pRect);
	pRect->bottom=m_TitleHeight;
	return true;
}


bool CPanel::GetContentRect(RECT *pRect) const
{
	if (m_hwnd==NULL)
		return false;

	GetClientRect(pRect);
	if (m_fShowTitle) {
		pRect->top=m_TitleHeight;
		if (pRect->bottom<pRect->top)
			pRect->bottom=pRect->top;
	}
	return true;
}


void CPanel::OnSize(int Width,int Height)
{
	if (m_pWindow!=NULL) {
		int y;

		if (m_fShowTitle)
			y=m_TitleHeight;
		else
			y=0;
		m_pWindow->SetPosition(0,y,Width,max(Height-y,0));
	}
	if (m_pEventHandler!=NULL)
		m_pEventHandler->OnSizeChanged(Width,Height);
}


void CPanel::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	GetClientRect(&rc);
	rc.right-=m_TitleMargin;
	rc.left=rc.right-m_ButtonSize;
	rc.top=(m_TitleHeight-m_ButtonSize)/2;
	rc.bottom=rc.top+m_ButtonSize;
	*pRect=rc;
}


CPanel *CPanel::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPanel*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CPanel::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			//CPanel *pThis=dynamic_cast<CPanel*>(OnCreate(hwnd,lParam));
			OnCreate(hwnd,lParam);
		}
		return 0;

	case WM_SIZE:
		{
			CPanel *pThis=GetThis(hwnd);

			pThis->OnSize(LOWORD(lParam),HIWORD(lParam));
		}
		return 0;

	case WM_PAINT:
		{
			CPanel *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			if (pThis->m_fShowTitle && ps.rcPaint.top<pThis->m_TitleHeight) {
				RECT rc;
				HFONT hfontOld;
				COLORREF crOldTextColor;
				int OldBkMode;

				pThis->GetClientRect(&rc);
				rc.bottom=pThis->m_TitleHeight;
				Theme::FillGradient(ps.hdc,&rc,&pThis->m_TitleBackGradient);
				if (pThis->m_pszTitle!=NULL) {
					hfontOld=SelectFont(ps.hdc,pThis->m_hfont);
					crOldTextColor=SetTextColor(ps.hdc,pThis->m_crTitleTextColor);
					OldBkMode=SetBkMode(ps.hdc,TRANSPARENT);
					rc.left+=pThis->m_TitleMargin;
					rc.right-=pThis->m_TitleMargin+pThis->m_ButtonSize;
					DrawText(ps.hdc,pThis->m_pszTitle,-1,&rc,
						DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
					SetTextColor(ps.hdc,crOldTextColor);
					SetBkMode(ps.hdc,OldBkMode);
					SelectFont(ps.hdc,hfontOld);
				}
				pThis->GetCloseButtonRect(&rc);
				DrawFrameControl(ps.hdc,&rc,DFC_CAPTION,DFCS_CAPTIONCLOSE | DFCS_MONO);
			}
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CPanel *pThis=GetThis(hwnd);
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (pThis->m_fShowTitle && pt.y<pThis->m_TitleHeight) {
				RECT rc;

				pThis->GetCloseButtonRect(&rc);
				if (PtInRect(&rc,pt)) {
					if (pThis->m_pEventHandler!=NULL)
						pThis->m_pEventHandler->OnClose();
					return 0;
				}
				if (pThis->m_fEnableFloating) {
					ClientToScreen(hwnd,&pt);
					pThis->m_ptDragStartPos=pt;
					SetCapture(hwnd);
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd)
			ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture()==hwnd) {
			CPanel *pThis=GetThis(hwnd);
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			ClientToScreen(hwnd,&pt);
			if (abs(pt.x-pThis->m_ptDragStartPos.x)>=4
					|| abs(pt.y-pThis->m_ptDragStartPos.y)>=4) {
				ReleaseCapture();
				if (pThis->m_pEventHandler!=NULL
						&& pThis->m_pEventHandler->OnFloating()) {
					::SendMessage(pThis->GetParent(),WM_NCLBUTTONDOWN,HTCAPTION,MAKELPARAM(pt.x,pt.y));
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CPanel *pThis=GetThis(hwnd);
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (pThis->m_fShowTitle && pt.y<pThis->m_TitleHeight) {
				static const LPCTSTR pszMenu[] = {
					TEXT("閉じる(&C)"),
					TEXT("フローティング(&F)")
				};
				HMENU hmenu;
				int i;

				ClientToScreen(hwnd,&pt);
				hmenu=CreatePopupMenu();
				for (i=0;i<lengthof(pszMenu);i++)
					AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,i+1,pszMenu[i]);
				if (!pThis->m_fEnableFloating)
					EnableMenuItem(hmenu,2,MF_BYCOMMAND | MFS_GRAYED);
				switch (TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,
													pt.x,pt.y,0,hwnd,NULL)) {
				case 1:
					if (pThis->m_pEventHandler!=NULL)
						pThis->m_pEventHandler->OnClose();
					break;
				case 2:
					if (pThis->m_pEventHandler!=NULL)
						pThis->m_pEventHandler->OnFloating();
					break;
				}
			}
		}
		return 0;

	case WM_KEYDOWN:
		{
			CPanel *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
				return 0;
		}
		break;

	case WM_DESTROY:
		{
			CPanel *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}




HINSTANCE CPanelFrame::m_hinst=NULL;


bool CPanelFrame::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=PANEL_FRAME_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return CPanel::Initialize(hinst) && CDropHelper::Initialize(hinst);
}


CPanelFrame::CPanelFrame()
{
	m_WindowPosition.Left=120;
	m_WindowPosition.Top=120;
	m_WindowPosition.Width=200;
	m_WindowPosition.Height=240;
	m_fFloating=true;
	m_DockingWidth=-1;
	m_Opacity=255;
	m_DragDockingTarget=DOCKING_NONE;
	m_pEventHandler=NULL;
}


CPanelFrame::~CPanelFrame()
{
}


bool CPanelFrame::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (!CreateBasicWindow(hwndParent,Style,ExStyle,ID,
						   PANEL_FRAME_WINDOW_CLASS,TEXT("パネル"),m_hinst))
		return false;
	if (m_Opacity<255) {
		SetExStyle(ExStyle|WS_EX_LAYERED);
		::SetLayeredWindowAttributes(m_hwnd,0,m_Opacity,LWA_ALPHA);
	}
	return true;
}


bool CPanelFrame::Create(HWND hwndOwner,CSplitter *pSplitter,int PanelID,CBasicWindow *pWindow,LPCTSTR pszTitle)
{
	RECT rc;

	m_pSplitter=pSplitter;
	m_PanelID=PanelID;
	//if (!Create(hwndOwner,WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN))
	//	return false;
	if (!Create(hwndOwner,
			WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
			WS_EX_TOOLWINDOW))
		return false;
	m_Panel.Create(m_hwnd,WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN);
	m_Panel.SetWindow(pWindow,pszTitle);
	m_Panel.SetEventHandler(this);
	if (m_DockingWidth<0) {
		m_Panel.GetClientRect(&rc);
		m_DockingWidth=rc.right;
	}
	if (m_fFloating) {
		m_Panel.SetVisible(false);
		m_Panel.SetParent(this);
		GetClientRect(&rc);
		m_Panel.SetPosition(&rc);
		m_Panel.ShowTitle(false);
		m_Panel.SetVisible(true);
	} else {
		m_pSplitter->SetPane(m_pSplitter->IDToIndex(PanelID),&m_Panel,PanelID);
		m_pSplitter->SetPaneSize(PanelID,m_DockingWidth);
		m_Panel.ShowTitle(true);
		//m_pSplitter->SetPaneVisible(PanelID,true);
	}
	return true;
}


bool CPanelFrame::SetFloating(bool fFloating)
{
	if (m_fFloating!=fFloating) {
		if (m_hwnd!=NULL) {
			if (fFloating) {
				RECT rc;

				//m_DockingWidth=m_pSplitter->GetPaneSize(m_PanelID);
				m_pSplitter->SetPane(m_pSplitter->IDToIndex(m_PanelID),NULL,m_PanelID);
				m_Panel.SetParent(this);
				m_Panel.SetVisible(true);
				GetClientRect(&rc);
				m_Panel.SetPosition(&rc);
				m_Panel.ShowTitle(false);
				SetVisible(true);
			} else {
				SetVisible(false);
				m_Panel.SetVisible(false);
				m_Panel.ShowTitle(true);
				m_pSplitter->SetPane(m_pSplitter->IDToIndex(m_PanelID),&m_Panel,m_PanelID);
				m_pSplitter->SetPaneSize(m_PanelID,m_DockingWidth);
				m_pSplitter->SetPaneVisible(m_PanelID,true);
			}
		}
		m_fFloating=fFloating;
	}
	return true;
}


bool CPanelFrame::SetDockingWidth(int Width)
{
	m_DockingWidth=Width;
	return true;
}


void CPanelFrame::SetEventHandler(CPanelFrameEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


bool CPanelFrame::SetPanelVisible(bool fVisible,bool fNoActivate)
{
	if (m_hwnd==NULL)
		return false;
	if (m_pEventHandler!=NULL)
		m_pEventHandler->OnVisibleChange(fVisible);
	if (m_fFloating) {
		if (fVisible && fNoActivate)
			::ShowWindow(m_hwnd,SW_SHOWNA);
		else
			SetVisible(fVisible);
	} else {
		m_pSplitter->SetPaneVisible(m_PanelID,fVisible);
	}
	return true;
}


bool CPanelFrame::SetTitleColor(const Theme::GradientInfo *pBackGradient,COLORREF crTitleText)
{
	return m_Panel.SetTitleColor(pBackGradient,crTitleText);
}


bool CPanelFrame::SetOpacity(int Opacity)
{
	if (Opacity<0 || Opacity>255)
		return false;
	if (Opacity!=m_Opacity) {
		if (m_hwnd!=NULL) {
			DWORD ExStyle=GetExStyle();

			if (Opacity<255) {
				if ((ExStyle&WS_EX_LAYERED)==0)
					SetExStyle(ExStyle|WS_EX_LAYERED);
				::SetLayeredWindowAttributes(m_hwnd,0,Opacity,LWA_ALPHA);
			} else {
				if ((ExStyle&WS_EX_LAYERED)!=0)
					SetExStyle(ExStyle^WS_EX_LAYERED);
			}
		}
		m_Opacity=Opacity;
	}
	return true;
}


CPanelFrame *CPanelFrame::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPanelFrame*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CPanelFrame::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CPanelFrame *pThis=dynamic_cast<CPanelFrame*>(OnCreate(hwnd,lParam));

			pThis->m_fDragMoving=false;
			HMENU hmenu=GetSystemMenu(hwnd,FALSE);
			InsertMenu(hmenu,0,MF_BYPOSITION | MFT_STRING | MFS_ENABLED,SC_DOCKING,TEXT("ドッキング(&D)"));
			InsertMenu(hmenu,1,MF_BYPOSITION | MFT_SEPARATOR,0,NULL);
		}
		return 0;

	case WM_SIZE:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			if (pThis->m_fFloating)
				pThis->m_Panel.SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		}
		return 0;

	case WM_KEYDOWN:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
				return 0;
		}
		break;

	case WM_MOUSEWHEEL:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnMouseWheel(wParam,lParam))
				return 0;
		}
		break;

	case WM_ACTIVATE:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnActivate(LOWORD(wParam)!=WA_INACTIVE))
				return 0;
		}
		break;

	case WM_MOVING:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnMoving(
											reinterpret_cast<LPRECT>(lParam)))
				return TRUE;
		}
		break;

	case WM_MOVE:
		{
			CPanelFrame *pThis=GetThis(hwnd);
			POINT pt;
			RECT rcTarget,rc;
			DockingPlace Target;

			if (!pThis->m_fDragMoving)
				break;
			GetCursorPos(&pt);
			GetWindowRect(pThis->m_pSplitter->GetHandle(),&rcTarget);
			Target=DOCKING_NONE;
			rc=rcTarget;
			rc.right=rc.left+16;
			if (PtInRect(&rc,pt)) {
				Target=DOCKING_LEFT;
			} else {
				rc.right=rcTarget.right;
				rc.left=rc.right-16;
				if (PtInRect(&rc,pt))
					Target=DOCKING_RIGHT;
			}
			if (Target!=pThis->m_DragDockingTarget) {
				if (Target==DOCKING_NONE) {
					pThis->m_DropHelper.Hide();
				} else{
					if (Target==DOCKING_LEFT) {
						rc.right=rcTarget.left;
						rc.left=rc.right-pThis->m_DockingWidth;
					} else if (Target==DOCKING_RIGHT) {
						rc.left=rcTarget.right;
						rc.right=rc.left+pThis->m_DockingWidth;
					}
					pThis->m_DropHelper.Show(&rc);
				}
				pThis->m_DragDockingTarget=Target;
			}
		}
		break;

	case WM_EXITSIZEMOVE:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			if (pThis->m_DragDockingTarget!=DOCKING_NONE) {
				int Index;

				pThis->m_DropHelper.Hide();
				if (pThis->m_DragDockingTarget==DOCKING_LEFT)
					Index=0;
				else
					Index=1;
				if (pThis->m_pSplitter->IDToIndex(pThis->m_PanelID)!=Index)
					pThis->m_pSplitter->SwapPane();
				::SendMessage(hwnd,WM_SYSCOMMAND,SC_DOCKING,0);
			}
			pThis->m_fDragMoving=false;
		}
		return 0;

	case WM_ENTERSIZEMOVE:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			pThis->m_DragDockingTarget=DOCKING_NONE;
			pThis->m_fDragMoving=true;
			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnEnterSizeMove())
				return 0;
		}
		break;

	case WM_SYSCOMMAND:
		switch (wParam) {
		case SC_DOCKING:
			{
				CPanelFrame *pThis=GetThis(hwnd);

				if (pThis->m_pEventHandler!=NULL
						&& !pThis->m_pEventHandler->OnFloatingChange(false))
					return 0;
				pThis->SetFloating(false);
			}
			return 0;
		}
		break;

	case WM_CLOSE:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& !pThis->m_pEventHandler->OnClose())
				return 0;
		}
		break;

	case WM_DESTROY:
		{
			CPanelFrame *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CPanelFrame::OnFloating()
{
	if (m_fFloating)
		return false;

	RECT rc;
	m_Panel.GetContentRect(&rc);
	MapWindowRect(m_Panel.GetHandle(),NULL,&rc);
	if (m_pEventHandler!=NULL && !m_pEventHandler->OnFloatingChange(true))
		return false;
	AdjustWindowRectEx(&rc,GetWindowStyle(m_hwnd),FALSE,GetWindowExStyle(m_hwnd));
	SetPosition(&rc);
	SetFloating(true);
	return true;
}


bool CPanelFrame::OnClose()
{
	return m_pEventHandler==NULL || m_pEventHandler->OnClose();
}


bool CPanelFrame::OnMoving(RECT *pRect)
{
	return m_pEventHandler!=NULL && m_pEventHandler->OnMoving(pRect);
}


bool CPanelFrame::OnEnterSizeMove()
{
	return m_pEventHandler!=NULL && m_pEventHandler->OnEnterSizeMove();
}


bool CPanelFrame::OnKeyDown(UINT KeyCode,UINT Flags)
{
	return m_pEventHandler!=NULL && m_pEventHandler->OnKeyDown(KeyCode,Flags);
}


void CPanelFrame::OnSizeChanged(int Width,int Height)
{
	if (!m_fFloating)
		m_DockingWidth=Width;
}




HINSTANCE CDropHelper::m_hinst=NULL;


bool CDropHelper::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=DROP_HELPER_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CDropHelper::CDropHelper()
	: m_Opacity(128)
{
}


CDropHelper::~CDropHelper()
{
}


bool CDropHelper::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 DROP_HELPER_WINDOW_CLASS,TEXT(""),m_hinst);
}


bool CDropHelper::Show(const RECT *pRect)
{
	if (m_hwnd==NULL) {
		if (!Create(NULL,WS_POPUP,
				WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE))
			return false;
		::SetLayeredWindowAttributes(m_hwnd,CLR_INVALID,m_Opacity,LWA_ALPHA);
	}
	SetPosition(pRect);
	::ShowWindow(m_hwnd,SW_SHOWNA);
	return true;
}


bool CDropHelper::Hide()
{
	if (m_hwnd!=NULL)
		SetVisible(false);
	return true;
}


LRESULT CALLBACK CDropHelper::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_PAINT:
		{
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			::FillRect(ps.hdc,&ps.rcPaint,static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
			::EndPaint(hwnd,&ps);
			return 0;
		}
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}
