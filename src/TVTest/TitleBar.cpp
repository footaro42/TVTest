#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "TitleBar.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TITLE_BAR_CLASS APP_NAME TEXT(" Title Bar")

#define TITLE_BORDER				1
#define TITLE_MARGIN				4
#define TITLE_BUTTON_ICON_WIDTH		12
#define TITLE_BUTTON_ICON_HEIGHT	12
#define TITLE_BUTTON_WIDTH			(TITLE_BUTTON_ICON_WIDTH+TITLE_MARGIN*2)
#define TITLE_ICON_WIDTH			16
#define TITLE_ICON_HEIGHT			16
#define ICON_TEXT_MARGIN			4
#define NUM_BUTTONS 4




HINSTANCE CTitleBar::m_hinst=NULL;


bool CTitleBar::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=TITLE_BAR_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CTitleBar::CTitleBar()
	: m_Font(DrawUtil::FONT_CAPTION)
	, m_FontHeight(m_Font.GetHeight(false))
	, m_hbmIcons(NULL)
	, m_hIcon(NULL)
	, m_HotItem(-1)
	, m_fTrackMouseEvent(false)
	, m_fMaximized(false)
	, m_pEventHandler(NULL)
{
	m_Theme.CaptionStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.CaptionStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.CaptionStyle.Gradient.Color1=RGB(192,192,192);
	m_Theme.CaptionStyle.Gradient.Color2=RGB(192,192,192);
	m_Theme.CaptionStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.CaptionStyle.TextColor=RGB(255,255,255);
	m_Theme.IconStyle=m_Theme.CaptionStyle;
	m_Theme.HighlightIconStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.HighlightIconStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.HighlightIconStyle.Gradient.Color1=RGB(0,0,128);
	m_Theme.HighlightIconStyle.Gradient.Color2=RGB(0,0,128);
	m_Theme.HighlightIconStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.HighlightIconStyle.TextColor=RGB(255,255,255);
	m_Theme.Border.Type=Theme::BORDER_RAISED;
	m_Theme.Border.Color=RGB(192,192,192);
}


CTitleBar::~CTitleBar()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pTitleBar=NULL;
	if (m_hbmIcons!=NULL)
		::DeleteObject(m_hbmIcons);
}


bool CTitleBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 TITLE_BAR_CLASS,NULL,m_hinst);
}


void CTitleBar::SetVisible(bool fVisible)
{
	m_HotItem=-1;
	m_fTrackMouseEvent=false;
	CBasicWindow::SetVisible(fVisible);
}


bool CTitleBar::SetLabel(LPCTSTR pszLabel)
{
	if (!m_Label.Set(pszLabel))
		return false;
	if (m_hwnd!=NULL)
		UpdateItem(ITEM_LABEL);
	return true;
}


bool CTitleBar::SetMaximizeMode(bool fMaximize)
{
	if (m_fMaximized!=fMaximize) {
		m_fMaximized=fMaximize;
		if (m_hwnd!=NULL)
			UpdateItem(ITEM_MAXIMIZE);
	}
	return true;
}


bool CTitleBar::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pTitleBar=NULL;
	if (pHandler!=NULL)
		pHandler->m_pTitleBar=this;
	m_pEventHandler=pHandler;
	return true;
}


/*
void CTitleBar::SetColor(const Theme::GradientInfo *pBackGradient,COLORREF TextColor,COLORREF IconColor,
						 const Theme::GradientInfo *pHighlightBackGradient,COLORREF HighlightIconColor)
{
	m_BackGradient=*pBackGradient;
	m_TextColor=TextColor;
	m_IconColor=IconColor;
	m_HighlightBackGradient=*pHighlightBackGradient;
	m_HighlightIconColor=HighlightIconColor;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CTitleBar::SetBorder(const Theme::BorderInfo *pInfo)
{
	if (m_BorderInfo!=*pInfo) {
		m_BorderInfo=*pInfo;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}
*/


bool CTitleBar::SetTheme(const ThemeInfo *pTheme)
{
	if (pTheme==NULL)
		return false;
	m_Theme=*pTheme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CTitleBar::GetTheme(ThemeInfo *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CTitleBar::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	m_FontHeight=m_Font.GetHeight(false);
	return true;
}


void CTitleBar::SetIcon(HICON hIcon)
{
	if (m_hIcon!=hIcon) {
		m_hIcon=hIcon;
		if (m_hwnd!=NULL) {
			RECT rc;

			GetItemRect(ITEM_LABEL,&rc);
			Invalidate(&rc);
		}
	}
}


CTitleBar *CTitleBar::GetThis(HWND hwnd)
{
	return static_cast<CTitleBar*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CTitleBar::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CTitleBar *pThis=dynamic_cast<CTitleBar*>(OnCreate(hwnd,lParam));
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			RECT rc;

			rc.left=0;
			rc.top=0;
			rc.right=0;
			rc.bottom=max(pThis->m_FontHeight,TITLE_BUTTON_ICON_HEIGHT)+TITLE_MARGIN*2+TITLE_BORDER*2;
			::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
			::MoveWindow(hwnd,0,0,0,rc.bottom-rc.top,FALSE);

			if (pThis->m_hbmIcons==NULL)
				pThis->m_hbmIcons=static_cast<HBITMAP>(::LoadImage(
					GetAppClass().GetResourceInstance(),
					MAKEINTRESOURCE(IDB_TITLEBAR),IMAGE_BITMAP,0,0,
					LR_DEFAULTCOLOR | LR_CREATEDIBSECTION));

			pThis->m_Tooltip.Create(hwnd);
			for (int i=ITEM_BUTTON_FIRST;i<=ITEM_LAST;i++) {
				RECT rc;
				pThis->GetItemRect(i,&rc);
				pThis->m_Tooltip.AddTool(i,rc);
			}

			pThis->m_HotItem=-1;
			pThis->m_fTrackMouseEvent=false;
		}
		return 0;

	case WM_SIZE:
		{
			CTitleBar *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				pThis->UpdateItem(pThis->m_HotItem);
				pThis->m_HotItem=-1;
			}
			pThis->UpdateTooltipsRect();
		}
		return 0;

	case WM_PAINT:
		{
			CTitleBar *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			pThis->Draw(ps.hdc,ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CTitleBar *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int HotItem=pThis->HitTest(x,y);

			if (GetCapture()==hwnd) {
				if (HotItem!=pThis->m_ClickItem)
					HotItem=-1;
				if (HotItem!=pThis->m_HotItem) {
					int OldHotItem;

					OldHotItem=pThis->m_HotItem;
					pThis->m_HotItem=HotItem;
					if (OldHotItem>=0)
						pThis->UpdateItem(OldHotItem);
					if (pThis->m_HotItem>=0)
						pThis->UpdateItem(pThis->m_HotItem);
				}
			} else {
				if (HotItem!=pThis->m_HotItem) {
					int OldHotItem;

					OldHotItem=pThis->m_HotItem;
					pThis->m_HotItem=HotItem;
					if (OldHotItem>=0)
						pThis->UpdateItem(OldHotItem);
					if (pThis->m_HotItem>=0)
						pThis->UpdateItem(pThis->m_HotItem);
				}
				if (!pThis->m_fTrackMouseEvent) {
					TRACKMOUSEEVENT tme;

					tme.cbSize=sizeof(TRACKMOUSEEVENT);
					tme.dwFlags=TME_LEAVE;
					tme.hwndTrack=hwnd;
					if (::TrackMouseEvent(&tme))
						pThis->m_fTrackMouseEvent=true;
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			CTitleBar *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				pThis->UpdateItem(pThis->m_HotItem);
				pThis->m_HotItem=-1;
			}
			pThis->m_fTrackMouseEvent=false;
			if (pThis->m_pEventHandler)
				pThis->m_pEventHandler->OnMouseLeave();
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CTitleBar *pThis=GetThis(hwnd);

			pThis->m_ClickItem=pThis->m_HotItem;
			if (pThis->m_ClickItem==ITEM_LABEL) {
				if (pThis->m_pEventHandler!=NULL) {
					int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

					if (x<16)
						pThis->m_pEventHandler->OnIconLButtonDown(x,y);
					else
						pThis->m_pEventHandler->OnLabelLButtonDown(x,y);
				}
			} else {
				::SetCapture(hwnd);
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CTitleBar *pThis=GetThis(hwnd);

			if (pThis->m_HotItem==ITEM_LABEL) {
				if (pThis->m_pEventHandler!=NULL)
					pThis->m_pEventHandler->OnLabelRButtonDown(
									GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd) {
			CTitleBar *pThis=GetThis(hwnd);

			::ReleaseCapture();
			if (pThis->m_HotItem>=0) {
				if (pThis->m_pEventHandler!=NULL) {
					switch (pThis->m_HotItem) {
					case ITEM_MINIMIZE:
						pThis->m_pEventHandler->OnMinimize();
						break;
					case ITEM_MAXIMIZE:
						pThis->m_pEventHandler->OnMaximize();
						break;
					case ITEM_FULLSCREEN:
						pThis->m_pEventHandler->OnFullscreen();
						break;
					case ITEM_CLOSE:
						pThis->m_pEventHandler->OnClose();
						break;
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			CTitleBar *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (pThis->m_HotItem<0 && pThis->HitTest(x,y)==ITEM_LABEL)
				pThis->m_HotItem=ITEM_LABEL;
			if (pThis->m_HotItem==ITEM_LABEL) {
				if (pThis->m_pEventHandler!=NULL) {
					if (x>=TITLE_BORDER+TITLE_MARGIN
							&& x<TITLE_BORDER+TITLE_MARGIN+TITLE_ICON_WIDTH)
						pThis->m_pEventHandler->OnIconLButtonDoubleClick(x,y);
					else
						pThis->m_pEventHandler->OnLabelLButtonDoubleClick(x,y);
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CTitleBar *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>0) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				static const LPTSTR pszToolTip[] = {
					TEXT("最小化"),
					TEXT("最大化"),
					TEXT("全画面表示"),
					TEXT("閉じる"),
				};
				CTitleBar *pThis=GetThis(hwnd);
				LPNMTTDISPINFO pnmttdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);

				if (pThis->m_fMaximized && pnmttdi->hdr.idFrom==ITEM_MAXIMIZE)
					pnmttdi->lpszText=TEXT("元のサイズに戻す");
				else
					pnmttdi->lpszText=pszToolTip[pnmttdi->hdr.idFrom-ITEM_BUTTON_FIRST];
				pnmttdi->hinst=NULL;
			}
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			CTitleBar *pThis=GetThis(hwnd);

			pThis->m_Tooltip.Destroy();
			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CTitleBar::GetItemRect(int Item,RECT *pRect) const
{
	RECT rc;
	int ButtonPos;

	if (m_hwnd==NULL || Item<0 || Item>ITEM_LAST)
		return false;
	GetClientRect(&rc);
	rc.left+=TITLE_BORDER;
	rc.top+=TITLE_BORDER;
	rc.right-=TITLE_BORDER;
	rc.bottom-=TITLE_BORDER;
	ButtonPos=rc.right-NUM_BUTTONS*TITLE_BUTTON_WIDTH;
	if (ButtonPos<0)
		ButtonPos=0;
	if (Item==ITEM_LABEL) {
		rc.right=ButtonPos;
	} else {
		rc.left=ButtonPos+(Item-1)*TITLE_BUTTON_WIDTH;
		rc.right=rc.left+TITLE_BUTTON_WIDTH;
	}
	*pRect=rc;
	return true;
}


bool CTitleBar::UpdateItem(int Item)
{
	RECT rc;

	if (m_hwnd==NULL)
		return false;
	if (!GetItemRect(Item,&rc))
		return false;
	::InvalidateRect(m_hwnd,&rc,FALSE);
	return true;
}


int CTitleBar::HitTest(int x,int y) const
{
	POINT pt;
	int i;
	RECT rc;

	pt.x=x;
	pt.y=y;
	for (i=ITEM_LAST;i>=0;i--) {
		GetItemRect(i,&rc);
		if (::PtInRect(&rc,pt))
			break;
	}
	return i;
}


void CTitleBar::UpdateTooltipsRect()
{
	for (int i=ITEM_BUTTON_FIRST;i<=ITEM_LAST;i++) {
		RECT rc;
		GetItemRect(i,&rc);
		m_Tooltip.SetToolRect(i,rc);
	}
}


void CTitleBar::Draw(HDC hdc,const RECT &PaintRect)
{
	HDC hdcMem=NULL;
	HBITMAP hbmOld;
	RECT rc,rcDraw;

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF crOldTextColor=::GetTextColor(hdc);
	COLORREF crOldBkColor=::GetBkColor(hdc);
	for (int i=0;i<=ITEM_LAST;i++) {
		GetItemRect(i,&rc);
		if (rc.right>rc.left
				&& rc.left<PaintRect.right && rc.right>PaintRect.left
				&& rc.top<PaintRect.bottom && rc.bottom>PaintRect.top) {
			bool fHighlight=i==m_HotItem && i!=ITEM_LABEL;

			rcDraw.left=rc.left+TITLE_MARGIN;
			rcDraw.top=rc.top+TITLE_MARGIN;
			rcDraw.right=rc.right-TITLE_MARGIN;
			rcDraw.bottom=rc.bottom-TITLE_MARGIN;
			if (i==ITEM_LABEL) {
				Theme::DrawStyleBackground(hdc,&rc,&m_Theme.CaptionStyle);
				if (m_hIcon!=NULL) {
					::DrawIconEx(hdc,
								 rcDraw.left,
								 rc.top+((rc.bottom-rc.top)-TITLE_ICON_HEIGHT)/2,
								 m_hIcon,
								 TITLE_ICON_WIDTH,TITLE_ICON_HEIGHT,
								 0,NULL,DI_NORMAL);
					rcDraw.left+=TITLE_ICON_WIDTH+ICON_TEXT_MARGIN;
				}
				if (!m_Label.IsEmpty()) {
					::SetTextColor(hdc,m_Theme.CaptionStyle.TextColor);
					::DrawText(hdc,m_Label.Get(),-1,&rcDraw,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				}
			} else {
				const Theme::Style &Style=
					fHighlight?m_Theme.HighlightIconStyle:m_Theme.IconStyle;

				if (hdcMem==NULL) {
					hdcMem=::CreateCompatibleDC(hdc);
					hbmOld=SelectBitmap(hdcMem,m_hbmIcons);
				}
				Theme::DrawStyleBackground(hdc,&rc,&Style);
				DrawUtil::DrawMonoColorDIB(hdc,
					rc.left+((rc.right-rc.left)-TITLE_BUTTON_ICON_WIDTH)/2,
					rc.top+((rc.bottom-rc.top)-TITLE_BUTTON_ICON_HEIGHT)/2,
					hdcMem,
					(i!=ITEM_MAXIMIZE || !m_fMaximized?
						(i-1):4)*TITLE_BUTTON_ICON_WIDTH,0,
					TITLE_BUTTON_ICON_WIDTH,TITLE_BUTTON_ICON_HEIGHT,
					Style.TextColor);
			}
		}
	}
	if (hdcMem!=NULL) {
		::SelectObject(hdcMem,hbmOld);
		::DeleteDC(hdcMem);
	}
	if (rc.right<PaintRect.right) {
		rc.left=rc.right;
		rc.right=PaintRect.right;
		Theme::FillGradient(hdc,&rc,&m_Theme.CaptionStyle.Gradient);
	}
	GetClientRect(&rc);
	Theme::DrawBorder(hdc,rc,&m_Theme.Border);
	::SetBkColor(hdc,crOldBkColor);
	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hfontOld);
}




CTitleBar::CEventHandler::CEventHandler()
	: m_pTitleBar(NULL)
{
}


CTitleBar::CEventHandler::~CEventHandler()
{
	if (m_pTitleBar!=NULL)
		m_pTitleBar->SetEventHandler(NULL);
}
