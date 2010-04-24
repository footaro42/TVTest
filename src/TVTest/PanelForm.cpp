#include "stdafx.h"
#include "TVTest.h"
#include "PanelForm.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




const LPCTSTR CPanelForm::m_pszClassName=APP_NAME TEXT(" Panel Form");
HINSTANCE CPanelForm::m_hinst=NULL;


bool CPanelForm::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=m_pszClassName;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CPanelForm::CPanelForm()
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
	m_fFitTabWidth=true;
	m_ClientMargin=4;
	m_CurTab=-1;
	m_pEventHandler=NULL;
}


CPanelForm::~CPanelForm()
{
	for (int i=0;i<m_NumWindows;i++)
		delete m_pWindowList[i];
	::DeleteObject(m_hfont);
}


bool CPanelForm::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,m_pszClassName,TEXT("ƒpƒlƒ‹"),m_hinst);
}


void CPanelForm::SetVisible(bool fVisible)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->OnVisibleChange(fVisible);
	CBasicWindow::SetVisible(fVisible);
}


bool CPanelForm::AddWindow(CPage *pWindow,int ID,LPCTSTR pszTitle)
{
	if (m_NumWindows==MAX_WINDOWS)
		return false;
	m_pWindowList[m_NumWindows]=new CWindowInfo(pWindow,ID,pszTitle);
	m_TabOrder[m_NumWindows]=m_NumWindows;
	m_NumWindows++;
	if (m_hwnd!=NULL) {
		CalcTabSize();
		Invalidate();
	}
	return true;
}


CPanelForm::CPage *CPanelForm::GetPageByIndex(int Index)
{
	if (Index<0 || Index>=m_NumWindows)
		return NULL;
	return m_pWindowList[Index]->m_pWindow;
}


CPanelForm::CPage *CPanelForm::GetPageByID(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_pWindowList[Index]->m_pWindow;
}


bool CPanelForm::SetCurTab(int Index)
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


int CPanelForm::IDToIndex(int ID) const
{
	for (int i=0;i<m_NumWindows;i++) {
		if (m_pWindowList[i]->m_ID==ID)
			return i;
	}
	return -1;
}


int CPanelForm::GetCurPageID() const
{
	if (m_CurTab<0)
		return -1;
	return m_pWindowList[m_CurTab]->m_ID;
}


bool CPanelForm::SetCurPageByID(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	return SetCurTab(Index);
}


bool CPanelForm::SetTabVisible(int ID,bool fVisible)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	if (m_pWindowList[Index]->m_fVisible!=fVisible) {
		m_pWindowList[Index]->m_fVisible=fVisible;
		if (m_hwnd!=NULL) {
			CalcTabSize();
			Invalidate();
		}
	}
	return true;
}


bool CPanelForm::GetTabVisible(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return false;
	return m_pWindowList[Index]->m_fVisible;
}


bool CPanelForm::SetTabOrder(const int *pOrder)
{
	for (int i=0;i<m_NumWindows;i++) {
		int j;
		for (j=0;j<m_NumWindows;j++) {
			if (m_pWindowList[j]->m_ID==pOrder[i])
				break;
		}
		if (j==m_NumWindows)
			return false;
	}
	::CopyMemory(m_TabOrder,pOrder,m_NumWindows*sizeof(int));
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CPanelForm::GetTabInfo(int Index,TabInfo *pInfo) const
{
	if (Index<0 || Index>=m_NumWindows || pInfo==NULL)
		return false;
	const CWindowInfo *pWindowInfo=m_pWindowList[m_TabOrder[Index]];
	pInfo->ID=pWindowInfo->m_ID;
	pInfo->fVisible=pWindowInfo->m_fVisible;
	return true;
}


void CPanelForm::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
}


void CPanelForm::SetBackColors(COLORREF crBack,COLORREF crMargin)
{
	m_crBackColor=crBack;
	m_crMarginColor=crMargin;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CPanelForm::SetTabColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,COLORREF crBorder)
{
	m_TabBackGradient=*pBackGradient;
	m_crTabTextColor=crText;
	m_crTabBorderColor=crBorder;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CPanelForm::SetCurTabColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,COLORREF crBorder)
{
	m_CurTabBackGradient=*pBackGradient;
	m_crCurTabTextColor=crText;
	m_crCurTabBorderColor=crBorder;
	if (m_hwnd!=NULL)
		Invalidate();
}


bool CPanelForm::SetTabFont(const LOGFONT *pFont)
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


bool CPanelForm::SetPageFont(const LOGFONT *pFont)
{
	for (int i=0;i<m_NumWindows;i++)
		m_pWindowList[i]->m_pWindow->SetFont(pFont);
	return true;
}


CPanelForm *CPanelForm::GetThis(HWND hwnd)
{
	return reinterpret_cast<CPanelForm*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CPanelForm::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CPanelForm *pThis=static_cast<CPanelForm*>(OnCreate(hwnd,lParam));

			pThis->CalcTabSize();
		}
		return 0;

	case WM_PAINT:
		{
			CPanelForm *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			if (ps.rcPaint.top<pThis->m_TabHeight) {
				const int TabWidth=pThis->GetRealTabWidth();
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
				rc.right=TabWidth;
				rc.bottom=pThis->m_TabHeight;
				for (i=0;i<pThis->m_NumWindows;i++) {
					int Index=pThis->m_TabOrder[i];
					const CWindowInfo *pWindow=pThis->m_pWindowList[Index];

					if (!pWindow->m_fVisible)
						continue;

					const Theme::GradientInfo *pGradient;
					COLORREF crText,crBorder;
					RECT rcText;

					if (Index==pThis->m_CurTab) {
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
								Index==pThis->m_CurTab?rc.bottom+1:rc.bottom);
					SetTextColor(ps.hdc,crText);
					rcText.left=rc.left+TAB_MARGIN;
					rcText.top=rc.top+TAB_MARGIN;
					rcText.right=rc.right-TAB_MARGIN;
					rcText.bottom=rc.bottom-TAB_MARGIN;
					DrawText(ps.hdc,pWindow->m_pszTitle,-1,&rcText,
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
					SelectPen(ps.hdc,hpenOld);
					DeleteObject(hpen);
					rc.left=rc.right;
					rc.right=rc.left+TabWidth;
				}
				SelectBrush(ps.hdc,hbrOld);
				SelectFont(ps.hdc,hfontOld);
				SetBkMode(ps.hdc,OldBkMode);
				SetTextColor(ps.hdc,crOldTextColor);
				if (ps.rcPaint.right>rc.left) {
					if (ps.rcPaint.left>rc.left)
						rc.left=ps.rcPaint.left;
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
			if (ps.rcPaint.bottom>pThis->m_TabHeight) {
				RECT rc;

				rc.left=ps.rcPaint.left;
				rc.top=max(ps.rcPaint.top,(long)pThis->m_TabHeight);
				rc.right=ps.rcPaint.right;
				rc.bottom=ps.rcPaint.bottom;
				DrawUtil::Fill(ps.hdc,&rc,pThis->m_crMarginColor);
			}
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		{
			CPanelForm *pThis=GetThis(hwnd);

			if (pThis->m_fFitTabWidth) {
				RECT rc;
				::SetRect(&rc,0,0,LOWORD(lParam),pThis->m_TabHeight);
				::InvalidateRect(hwnd,&rc,FALSE);
			}
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
			CPanelForm *pThis=GetThis(hwnd);
			int Index=pThis->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));

			if (Index>=0 && Index!=pThis->m_CurTab) {
				pThis->SetCurTab(Index);
				if (pThis->m_pEventHandler!=NULL)
					pThis->m_pEventHandler->OnSelChange();
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CPanelForm *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL) {
				POINT pt;
				RECT rc;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				pThis->GetClientRect(&rc);
				if (::PtInRect(&rc,pt)) {
					if (pt.y<pThis->m_TabHeight)
						pThis->m_pEventHandler->OnTabRButtonDown(pt.x,pt.y);
					else
						pThis->m_pEventHandler->OnRButtonDown();
					return 0;
				}
			}
		}
		break;

	case WM_SETCURSOR:
		{
			CPanelForm *pThis=GetThis(hwnd);
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			int Index=pThis->HitTest(pt.x,pt.y);
			if (Index>=0) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_KEYDOWN:
		{
			CPanelForm *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
				return 0;
		}
		break;

	case WM_DESTROY:
		{
			CPanelForm *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CPanelForm::CalcTabSize()
{
	HDC hdc;
	HFONT hfontOld;
	TEXTMETRIC tm;
	int MaxWidth;
	SIZE sz;

	hdc=::GetDC(m_hwnd);
	hfontOld=SelectFont(hdc,m_hfont);
	::GetTextMetrics(hdc,&tm);
	m_TabHeight=(tm.tmHeight/*-tm.tmInternalLeading*/)+TAB_MARGIN*2;
	MaxWidth=0;
	for (int i=0;i<m_NumWindows;i++) {
		if (m_pWindowList[i]->m_fVisible) {
			::GetTextExtentPoint32(hdc,m_pWindowList[i]->m_pszTitle,
								   ::lstrlen(m_pWindowList[i]->m_pszTitle),&sz);
			if (sz.cx>MaxWidth)
				MaxWidth=sz.cx;
		}
	}
	SelectFont(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
	m_TabWidth=MaxWidth+TAB_MARGIN*2;
}


int CPanelForm::GetRealTabWidth() const
{
	if (m_fFitTabWidth) {
		int NumVisibleTabs=0;
		for (int i=0;i<m_NumWindows;i++) {
			if (m_pWindowList[i]->m_fVisible)
				NumVisibleTabs++;
		}
		RECT rc;
		GetClientRect(&rc);
		if (NumVisibleTabs*m_TabWidth>rc.right)
			return max(rc.right/NumVisibleTabs,16+TAB_MARGIN*2);
	}
	return m_TabWidth;
}


int CPanelForm::HitTest(int x,int y) const
{
	if (y<0 || y>=m_TabHeight)
		return -1;

	const int TabWidth=GetRealTabWidth();
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	::SetRect(&rc,0,0,TabWidth,m_TabHeight);
	for (int i=0;i<m_NumWindows;i++) {
		int Index=m_TabOrder[i];
		if (m_pWindowList[Index]->m_fVisible) {
			if (::PtInRect(&rc,pt))
				return Index;
			::OffsetRect(&rc,TabWidth,0);
		}
	}
	return -1;
}




CPanelForm::CWindowInfo::CWindowInfo(CPage *pWindow,int ID,LPCTSTR pszTitle)
	: m_pWindow(pWindow)
	, m_ID(ID)
	, m_pszTitle(DuplicateString(pszTitle))
	, m_fVisible(true)
{
}


CPanelForm::CWindowInfo::~CWindowInfo()
{
	delete [] m_pszTitle;
}




CPanelForm::CPage::CPage()
{
}


CPanelForm::CPage::~CPage()
{
}


bool CPanelForm::CPage::GetDefaultFont(LOGFONT *pFont)
{
	return ::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),pFont)!=0;
}


HFONT CPanelForm::CPage::CreateDefaultFont()
{
	LOGFONT lf;

	if (!GetDefaultFont(&lf))
		return NULL;
	return ::CreateFontIndirect(&lf);
}
