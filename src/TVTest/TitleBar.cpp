#include "stdafx.h"
#include <commctrl.h>
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
{
	NONCLIENTMETRICS ncm;

	ncm.cbSize=sizeof(NONCLIENTMETRICS);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0);
	m_hfont=CreateFontIndirect(&ncm.lfCaptionFont);
	m_FontHeight=abs(ncm.lfCaptionFont.lfHeight);
	m_BackGradient.Type=Theme::GRADIENT_NORMAL;
	m_BackGradient.Direction=Theme::DIRECTION_VERT;
	m_BackGradient.Color1=RGB(128,192,160);
	m_BackGradient.Color2=RGB(128,192,160);
	m_crTextColor=RGB(64,96,80);
	m_HighlightBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_HighlightBackGradient.Direction=Theme::DIRECTION_VERT;
	m_HighlightBackGradient.Color1=RGB(64,96,80);
	m_HighlightBackGradient.Color2=RGB(64,96,80);
	m_crHighlightTextColor=RGB(128,192,160);
	m_BorderType=Theme::BORDER_RAISED;
	m_hbmIcons=NULL;
	m_hwndToolTip=NULL;
	m_pszLabel=NULL;
	m_hIcon=NULL;
	m_HotItem=-1;
	m_fTrackMouseEvent=false;
	m_fMaximized=false;
	m_pEventHandler=NULL;
}


CTitleBar::~CTitleBar()
{
	delete [] m_pszLabel;
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
	if (!ReplaceString(&m_pszLabel,pszLabel))
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


bool CTitleBar::SetEventHandler(CTitleBarEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pTitleBar=NULL;
	if (pHandler!=NULL)
		pHandler->m_pTitleBar=this;
	m_pEventHandler=pHandler;
	return true;
}


void CTitleBar::SetColor(const Theme::GradientInfo *pBackGradient,COLORREF crText,
						 const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightText)
{
	m_BackGradient=*pBackGradient;
	m_crTextColor=crText;
	m_HighlightBackGradient=*pHighlightBackGradient;
	m_crHighlightTextColor=crHighlightText;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CTitleBar::SetBorderType(Theme::BorderType Type)
{
	if (m_BorderType!=Type) {
		m_BorderType=Type;
		if (m_hwnd!=NULL)
			Invalidate();
	}
}


/*
bool CTitleBar::SetFont(const LOGFONT *pFont)
{
	if (pFont==NULL)
		return false;
	HFONT hfont=::CreateFontIndirect(pFont);
	if (hfont==NULL)
		return false;
	if (m_hfont!=NULL)
		::DeleteObject(m_hfont);
	m_hfont=hfont;


	return true;
}
*/


void CTitleBar::SetIcon(HICON hIcon)
{
	m_hIcon=hIcon;
	if (m_hwnd!=NULL)
		Invalidate();
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
			pThis->m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,
				NULL,WS_POPUP | TTS_ALWAYSTIP,0,0,0,0,hwnd,NULL,m_hinst,NULL);
			TOOLINFO ti;
			ti.cbSize=TTTOOLINFO_V1_SIZE;
			ti.uFlags=TTF_SUBCLASS;
			ti.hwnd=hwnd;
			ti.hinst=NULL;
			ti.lpszText=LPSTR_TEXTCALLBACK;
			for (int i=ITEM_BUTTON_FIRST;i<=ITEM_LAST;i++) {
				ti.uId=i;
				pThis->GetItemRect(i,&ti.rect);
				::SendMessage(pThis->m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
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
			pThis->SetToolTip();
		}
		return 0;

	case WM_PAINT:
		{
			CTitleBar *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HFONT hfontOld;
			COLORREF crOldTextColor,crOldBkColor;
			int OldBkMode;
			RECT rc,rcDraw;

			::BeginPaint(hwnd,&ps);
			hfontOld=SelectFont(ps.hdc,pThis->m_hfont);
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			crOldTextColor=::GetTextColor(ps.hdc);
			crOldBkColor=::GetBkColor(ps.hdc);
			for (int i=0;i<=ITEM_LAST;i++) {
				pThis->GetItemRect(i,&rc);
				if (rc.right>rc.left
						&& rc.left<ps.rcPaint.right && rc.right>ps.rcPaint.left
						&& rc.top<ps.rcPaint.bottom && rc.bottom>ps.rcPaint.top) {
					bool fHighlight=i==pThis->m_HotItem && i!=ITEM_LABEL;

					::SetTextColor(ps.hdc,fHighlight?
						pThis->m_crHighlightTextColor:pThis->m_crTextColor);
					Theme::FillGradient(ps.hdc,&rc,
						fHighlight?&pThis->m_HighlightBackGradient:&pThis->m_BackGradient);
					rcDraw.left=rc.left+TITLE_MARGIN;
					rcDraw.top=rc.top+TITLE_MARGIN;
					rcDraw.right=rc.right-TITLE_MARGIN;
					rcDraw.bottom=rc.bottom-TITLE_MARGIN;
					if (i==ITEM_LABEL) {
						if (pThis->m_hIcon!=NULL) {
							::DrawIconEx(ps.hdc,
										 rcDraw.left,
										 rc.top+((rc.bottom-rc.top)-TITLE_ICON_HEIGHT)/2,
										 pThis->m_hIcon,
										 TITLE_ICON_WIDTH,TITLE_ICON_HEIGHT,
										 0,NULL,DI_NORMAL);
							rcDraw.left+=TITLE_ICON_WIDTH+ICON_TEXT_MARGIN;
						}
						if (pThis->m_pszLabel!=NULL) {
							::DrawText(ps.hdc,pThis->m_pszLabel,-1,&rcDraw,
								DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
						}
					} else {
						HDC hdcMem;
						HBITMAP hbmOld;
						RGBQUAD Palette[2];
						COLORREF cr,crTrans;

						hdcMem=::CreateCompatibleDC(ps.hdc);
						hbmOld=SelectBitmap(hdcMem,pThis->m_hbmIcons);
						cr=::GetTextColor(ps.hdc);
						Palette[0].rgbBlue=GetBValue(cr);
						Palette[0].rgbGreen=GetGValue(cr);
						Palette[0].rgbRed=GetRValue(cr);
						crTrans=cr^0x00FFFFFF;
						Palette[1].rgbBlue=GetBValue(crTrans);
						Palette[1].rgbGreen=GetGValue(crTrans);
						Palette[1].rgbRed=GetRValue(crTrans);
						::SetDIBColorTable(hdcMem,0,2,Palette);
						::TransparentBlt(ps.hdc,
							rc.left+((rc.right-rc.left)-TITLE_BUTTON_ICON_WIDTH)/2,
							rc.top+((rc.bottom-rc.top)-TITLE_BUTTON_ICON_HEIGHT)/2,
							TITLE_BUTTON_ICON_WIDTH,TITLE_BUTTON_ICON_HEIGHT,
							hdcMem,
							(i!=ITEM_MAXIMIZE || !pThis->m_fMaximized?
								(i-1):4)*TITLE_BUTTON_ICON_WIDTH,0,
							TITLE_BUTTON_ICON_WIDTH,TITLE_BUTTON_ICON_HEIGHT,crTrans);
						::SelectObject(hdcMem,hbmOld);
						::DeleteDC(hdcMem);
					}
				}
			}
			if (rc.right<ps.rcPaint.right) {
				rc.left=rc.right;
				rc.right=ps.rcPaint.right;
				Theme::FillGradient(ps.hdc,&rc,&pThis->m_BackGradient);
			}
			::GetClientRect(hwnd,&rc);
			Theme::DrawBorder(ps.hdc,&rc,pThis->m_BorderType);
			::SetBkColor(ps.hdc,crOldBkColor);
			::SetTextColor(ps.hdc,crOldTextColor);
			::SetBkMode(ps.hdc,OldBkMode);
			::SelectObject(ps.hdc,hfontOld);
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

			pThis->OnDestroy();
			pThis->m_hwndToolTip=NULL;
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


void CTitleBar::SetToolTip()
{
	int i;
	TOOLINFO ti;

	if (m_hwndToolTip==NULL)
		return;
	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.hwnd=m_hwnd;
	for (i=ITEM_BUTTON_FIRST;i<=ITEM_LAST;i++) {
		ti.uId=i;
		GetItemRect(i,&ti.rect);
		::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,reinterpret_cast<LPARAM>(&ti));
	}
}




CTitleBarEventHandler::CTitleBarEventHandler()
{
	m_pTitleBar=NULL;
}


CTitleBarEventHandler::~CTitleBarEventHandler()
{
}
