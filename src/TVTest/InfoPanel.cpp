#include "stdafx.h"
#include "TVTest.h"
#include "InfoPanel.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define INFO_PANEL_WINDOW_CLASS APP_NAME TEXT(" Info Panel")




HINSTANCE CInfoPanel::m_hinst=NULL;


bool CInfoPanel::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=INFO_PANEL_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CInfoPanel::CInfoPanel()
{
	LOGFONT lf;

	m_WindowPosition.Width=200;
	m_WindowPosition.Height=240;
	m_NumWindows=0;
	GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	m_hfont=CreateFontIndirect(&lf);
	m_crBackColor=GetSysColor(COLOR_3DFACE);
	m_crMarginColor=m_crBackColor;
	m_TabBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_TabBackGradient.Direction=Theme::DIRECTION_VERT;
	m_TabBackGradient.Color1=GetSysColor(COLOR_3DFACE);
	m_TabBackGradient.Color2=m_TabBackGradient.Color1;
	m_crTabTextColor=GetSysColor(COLOR_WINDOWTEXT);
	m_crTabBorderColor=GetSysColor(COLOR_3DDKSHADOW);
	m_CurTabBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_CurTabBackGradient.Direction=Theme::DIRECTION_VERT;
	m_CurTabBackGradient.Color1=GetSysColor(COLOR_3DHIGHLIGHT);
	m_CurTabBackGradient.Color2=m_CurTabBackGradient.Color1;
	m_crCurTabTextColor=GetSysColor(COLOR_WINDOWTEXT);
	m_crCurTabBorderColor=GetSysColor(COLOR_3DDKSHADOW);
	m_TabHeight=/*abs(lf.lfHeight)+*/TAB_MARGIN*2;
	m_TabWidth=8+TAB_MARGIN*2;
	m_ClientMargin=4;
	m_CurTab=-1;
	m_pEventHandler=NULL;
}


CInfoPanel::~CInfoPanel()
{
	for (int i=0;i<m_NumWindows;i++)
		delete m_pWindowList[i];
	DeleteObject(m_hfont);
}


bool CInfoPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,INFO_PANEL_WINDOW_CLASS,TEXT("ƒpƒlƒ‹"),m_hinst);
}


void CInfoPanel::SetVisible(bool fVisible)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->OnVisibleChange(fVisible);
	CBasicWindow::SetVisible(fVisible);
}


bool CInfoPanel::AddWindow(CInfoPanelPage *pWindow,LPCTSTR pszTitle)
{
	if (m_NumWindows==MAX_WINDOWS)
		return false;
	m_pWindowList[m_NumWindows]=new CWindowInfo(pWindow,pszTitle);
	m_NumWindows++;
	if (m_hwnd!=NULL) {
		CalcTabSize();
		Invalidate();
	}
	return true;
}


bool CInfoPanel::SetCurTab(int Index)
{
	if (Index<-1 || Index>=m_NumWindows)
		return false;
	if (m_CurTab!=Index) {
		if (m_CurTab>=0)
			m_pWindowList[m_CurTab]->m_pWindow->SetVisible(false);
		if (Index>=0) {
			RECT rc;

			GetClientRect(&rc);
			m_pWindowList[Index]->m_pWindow->SetPosition(
				m_ClientMargin,m_TabHeight+m_ClientMargin,
				rc.right-m_ClientMargin*2,rc.bottom-m_TabHeight-m_ClientMargin*2);
			m_pWindowList[Index]->m_pWindow->SetVisible(true);
			//SetFocus(m_pWindowList[Index]->m_pWindow->GetHandle());
		}
		m_CurTab=Index;
		Invalidate();
		Update();
	}
	return true;
}


void CInfoPanel::SetEventHandler(CInfoPanelEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


void CInfoPanel::SetBackColors(COLORREF crBack,COLORREF crMargin)
{
	m_crBackColor=crBack;
	m_crMarginColor=crMargin;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CInfoPanel::SetTabColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,COLORREF crBorder)
{
	m_TabBackGradient=*pBackGradient;
	m_crTabTextColor=crText;
	m_crTabBorderColor=crBorder;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CInfoPanel::SetCurTabColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,COLORREF crBorder)
{
	m_CurTabBackGradient=*pBackGradient;
	m_crCurTabTextColor=crText;
	m_crCurTabBorderColor=crBorder;
	if (m_hwnd!=NULL)
		Invalidate();
}


bool CInfoPanel::SetTabFont(const LOGFONT *pFont)
{
	HFONT hfont=::CreateFontIndirect(pFont);

	if (hfont==NULL)
		return false;
	::DeleteObject(m_hfont);
	m_hfont=hfont;
	if (m_hwnd!=NULL) {
		CalcTabSize();
		RECT rc;
		GetClientRect(&rc);
		SendMessage(WM_SIZE,0,MAKELPARAM(rc.right,rc.bottom));
		Invalidate();
	}
	return true;
}


bool CInfoPanel::SetPageFont(const LOGFONT *pFont)
{
	for (int i=0;i<m_NumWindows;i++)
		m_pWindowList[i]->m_pWindow->SetFont(pFont);
	return true;
}


CInfoPanel *CInfoPanel::GetThis(HWND hwnd)
{
	return reinterpret_cast<CInfoPanel*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CInfoPanel::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CInfoPanel *pThis=static_cast<CInfoPanel*>(OnCreate(hwnd,lParam));

			pThis->CalcTabSize();
		}
		return 0;

	case WM_PAINT:
		{
			CInfoPanel *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			if (ps.rcPaint.top<pThis->m_TabHeight) {
				COLORREF crOldTextColor;
				int OldBkMode;
				HFONT hfontOld;
				int i;
				RECT rc;
				HBRUSH hbrOld;
				HPEN hpen,hpenOld;

				crOldTextColor=GetTextColor(ps.hdc);
				OldBkMode=SetBkMode(ps.hdc,TRANSPARENT);
				hfontOld=SelectFont(ps.hdc,pThis->m_hfont);
				hbrOld=SelectBrush(ps.hdc,::GetStockObject(NULL_BRUSH));
				rc.left=0;
				rc.top=0;
				rc.right=pThis->m_TabWidth;
				rc.bottom=pThis->m_TabHeight;
				for (i=0;i<pThis->m_NumWindows;i++) {
					const Theme::GradientInfo *pGradient;
					COLORREF crText,crBorder;
					RECT rcText;

					if (i==pThis->m_CurTab) {
						pGradient=&pThis->m_CurTabBackGradient;
						crText=pThis->m_crCurTabTextColor;
						crBorder=pThis->m_crCurTabBorderColor;
					} else {
						pGradient=&pThis->m_TabBackGradient;
						crText=pThis->m_crTabTextColor;
						crBorder=pThis->m_crTabBorderColor;
					}
					Theme::FillGradient(ps.hdc,&rc,pGradient);
					hpen=CreatePen(PS_SOLID,1,crBorder);
					hpenOld=SelectPen(ps.hdc,hpen);
					Rectangle(ps.hdc,rc.left,rc.top,rc.right,
									i==pThis->m_CurTab?rc.bottom+1:rc.bottom);
					SetTextColor(ps.hdc,crText);
					rcText.left=rc.left+TAB_MARGIN;
					rcText.top=rc.top+TAB_MARGIN;
					rcText.right=rc.right-TAB_MARGIN;
					rcText.bottom=rc.bottom-TAB_MARGIN;
					DrawText(ps.hdc,pThis->m_pWindowList[i]->m_pszTitle,-1,&rcText,
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
					SelectPen(ps.hdc,hpenOld);
					DeleteObject(hpen);
					rc.left=rc.right;
					rc.right=rc.left+pThis->m_TabWidth;
				}
				SelectBrush(ps.hdc,hbrOld);
				SelectFont(ps.hdc,hfontOld);
				SetBkMode(ps.hdc,OldBkMode);
				SetTextColor(ps.hdc,crOldTextColor);
				if (ps.rcPaint.right>pThis->m_NumWindows*pThis->m_TabWidth) {
					rc.left=max(ps.rcPaint.left,(long)(pThis->m_NumWindows*pThis->m_TabWidth));
					rc.top=ps.rcPaint.top;
					rc.right=ps.rcPaint.right;
					rc.bottom=min(ps.rcPaint.bottom,(long)pThis->m_TabHeight-1);
					DrawUtil::Fill(ps.hdc,&rc,pThis->m_crBackColor);
					hpen=CreatePen(PS_SOLID,1,pThis->m_crTabBorderColor);
					hpenOld=SelectPen(ps.hdc,hpen);
					MoveToEx(ps.hdc,rc.left,rc.bottom,NULL);
					LineTo(ps.hdc,rc.right,rc.bottom);
					SelectPen(ps.hdc,hpenOld);
					DeleteObject(hpen);
				}
			}
			if (ps.rcPaint.bottom>pThis->m_TabHeight+pThis->m_ClientMargin) {
				HBRUSH hbr;
				RECT rc;

				rc.left=ps.rcPaint.left;
				rc.top=max(ps.rcPaint.top,(long)pThis->m_TabHeight);
				rc.right=ps.rcPaint.right;
				rc.bottom=ps.rcPaint.bottom;
				hbr=CreateSolidBrush(pThis->m_crMarginColor);
				FillRect(ps.hdc,&rc,hbr);
				DeleteObject(hbr);
			}
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		{
			CInfoPanel *pThis=GetThis(hwnd);

			if (pThis->m_CurTab>=0) {
				pThis->m_pWindowList[pThis->m_CurTab]->m_pWindow->SetPosition(
					pThis->m_ClientMargin,pThis->m_TabHeight+pThis->m_ClientMargin,
					LOWORD(lParam)-pThis->m_ClientMargin*2,
					HIWORD(lParam)-pThis->m_TabHeight-pThis->m_ClientMargin*2);
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CInfoPanel *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (y>=0 && y<pThis->m_TabHeight) {
				int Index=x/pThis->m_TabWidth;

				if (Index>=0 && Index<pThis->m_NumWindows
						&& Index!=pThis->m_CurTab) {
					pThis->SetCurTab(Index);
					if (pThis->m_pEventHandler!=NULL)
						pThis->m_pEventHandler->OnSelChange();
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CInfoPanel *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL) {
				POINT pt;
				RECT rc;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				pThis->GetClientRect(&rc);
				if (PtInRect(&rc,pt)) {
					pThis->m_pEventHandler->OnRButtonDown();
					return 0;
				}
			}
		}
		break;

	case WM_SETCURSOR:
		{
			CInfoPanel *pThis=GetThis(hwnd);
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			if (pt.y>=0 && pt.y<pThis->m_TabHeight) {
				int Index=pt.x/pThis->m_TabWidth;

				if (Index>=0 && Index<pThis->m_NumWindows) {
					::SetCursor(::LoadCursor(NULL,IDC_HAND));
					return TRUE;
				}
			}
		}
		break;

	case WM_KEYDOWN:
		{
			CInfoPanel *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnKeyDown(wParam,lParam))
				return 0;
		}
		break;

	case WM_DESTROY:
		{
			CInfoPanel *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CInfoPanel::CalcTabSize()
{
	HDC hdc;
	HFONT hfontOld;
	TEXTMETRIC tm;
	int MaxWidth;
	SIZE sz;

	hdc=::GetDC(m_hwnd);
	hfontOld=SelectFont(hdc,m_hfont);
	::GetTextMetrics(hdc,&tm);
	m_TabHeight=(tm.tmHeight+tm.tmInternalLeading)+TAB_MARGIN*2;
	MaxWidth=0;
	for (int i=0;i<m_NumWindows;i++) {
		::GetTextExtentPoint32(hdc,m_pWindowList[i]->m_pszTitle,
							   ::lstrlen(m_pWindowList[i]->m_pszTitle),&sz);
		if (sz.cx>MaxWidth)
			MaxWidth=sz.cx;
	}
	SelectFont(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
	m_TabWidth=MaxWidth+TAB_MARGIN*2;
}




CInfoPanel::CWindowInfo::CWindowInfo(CInfoPanelPage *pWindow,LPCTSTR pszTitle)
{
	m_pWindow=pWindow;
	m_pszTitle=DuplicateString(pszTitle);
}


CInfoPanel::CWindowInfo::~CWindowInfo()
{
	delete [] m_pszTitle;
}




CInfoPanelPage::CInfoPanelPage()
{
}


CInfoPanelPage::~CInfoPanelPage()
{
}
