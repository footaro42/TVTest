#include "stdafx.h"
#include "TVTest.h"
#include "SideBar.h"
#include "DrawUtil.h"
#include "resource.h"


#define SIDE_BAR_CLASS APP_NAME TEXT(" Side Bar")
#define ICON_WIDTH			16
#define ICON_HEIGHT			16
#define PADDING_WIDTH		2
#define BUTTON_MARGIN		3
#define SEPARATOR_WIDTH		8




HINSTANCE CSideBar::m_hinst=NULL;


bool CSideBar::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=SIDE_BAR_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CSideBar::CSideBar(const CCommandList *pCommandList)
	: m_hwndToolTip(NULL)
	, m_fShowToolTips(true)
	, m_hbmIcons(NULL)
	, m_fVertical(true)
	, m_BackColor1(RGB(128,192,160))
	, m_BackColor2(RGB(128,192,160))
	, m_ForeColor(RGB(64,96,80))
	, m_HighlightBackColor1(RGB(64,96,80))
	, m_HighlightBackColor2(RGB(64,96,80))
	, m_HighlightForeColor(RGB(128,192,160))
	, m_HotItem(-1)
	, m_ClickItem(-1)
	, m_fTrackMouseEvent(false)
	, m_pEventHandler(NULL)
	, m_pCommandList(pCommandList)
{
}


CSideBar::~CSideBar()
{
	if (m_hbmIcons!=NULL)
		::DeleteObject(m_hbmIcons);
}


bool CSideBar::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 SIDE_BAR_CLASS,NULL,m_hinst);
}


int CSideBar::GetBarWidth() const
{
	return (m_fVertical?ICON_WIDTH:ICON_HEIGHT)+BUTTON_MARGIN*2+PADDING_WIDTH*2;
}


bool CSideBar::SetIconImage(HBITMAP hbm,COLORREF crTransparent)
{
	if (m_hbmIcons!=NULL)
		::DeleteObject(m_hbmIcons);
	m_hbmIcons=hbm;
	m_IconTransparentColor=crTransparent;
	return true;
}


void CSideBar::DeleteAllItems()
{
	if (m_hwndToolTip!=NULL) {
		TOOLINFO ti;

		ti.cbSize=TTTOOLINFO_V1_SIZE;
		ti.hwnd=m_hwnd;
		for (size_t i=0;i<m_ItemList.size();i++) {
			if (m_ItemList[i].Command!=ITEM_SEPARATOR) {
				ti.uId=i;
				::SendMessage(m_hwndToolTip,TTM_DELTOOL,0,(LPARAM)&ti);
			}
		}
	}
	m_ItemList.clear();
}


bool CSideBar::AddItem(const SideBarItem *pItem)
{
	return AddItems(pItem,1);
}


bool CSideBar::AddItems(const SideBarItem *pItemList,int NumItems)
{
	if (pItemList==NULL || NumItems<=0)
		return false;
	size_t OldSize=m_ItemList.size();
	m_ItemList.resize(OldSize+NumItems);
	::CopyMemory(&m_ItemList[OldSize],pItemList,NumItems*sizeof(SideBarItem));
	if (m_hwndToolTip!=NULL) {
		TOOLINFO ti;

		ti.cbSize=TTTOOLINFO_V1_SIZE;
		ti.uFlags=TTF_SUBCLASS;
		ti.hwnd=m_hwnd;
		ti.hinst=NULL;
		ti.lpszText=LPSTR_TEXTCALLBACK;
		for (int i=0;i<NumItems;i++) {
			if (pItemList[i].Command!=ITEM_SEPARATOR) {
				ti.uId=OldSize+i;
				GetItemRect(OldSize+i,&ti.rect);
				::SendMessage(m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
			}
		}
	}
	return true;
}


void CSideBar::SetColor(COLORREF crBack1,COLORREF crBack2,COLORREF crFore,COLORREF crHighlightBack1,COLORREF crHighlightBack2,COLORREF crHighlightFore)
{
	m_BackColor1=crBack1;
	m_BackColor2=crBack2;
	m_ForeColor=crFore;
	m_HighlightBackColor1=crHighlightBack1;
	m_HighlightBackColor2=crHighlightBack2;
	m_HighlightForeColor=crHighlightFore;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CSideBar::ShowToolTips(bool fShow)
{
	if (m_fShowToolTips!=fShow) {
		m_fShowToolTips=fShow;
		if (m_hwndToolTip!=NULL)
			::SendMessage(m_hwndToolTip,TTM_ACTIVATE,fShow,0);
	}
}


void CSideBar::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pSideBar=NULL;
	if (pHandler!=NULL)
		pHandler->m_pSideBar=this;
	m_pEventHandler=pHandler;
}


CSideBar *CSideBar::GetThis(HWND hwnd)
{
	return static_cast<CSideBar*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CSideBar::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CSideBar *pThis=static_cast<CSideBar*>(OnCreate(hwnd,lParam));
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);

			pThis->m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,
				NULL,WS_POPUP | TTS_ALWAYSTIP,0,0,0,0,hwnd,NULL,m_hinst,NULL);
			TOOLINFO ti;
			ti.cbSize=TTTOOLINFO_V1_SIZE;
			ti.uFlags=TTF_SUBCLASS;
			ti.hwnd=hwnd;
			ti.hinst=NULL;
			ti.lpszText=LPSTR_TEXTCALLBACK;
			for (int i=0;i<(int)pThis->m_ItemList.size();i++) {
				if (pThis->m_ItemList[i].Command!=ITEM_SEPARATOR) {
					ti.uId=i;
					pThis->GetItemRect(i,&ti.rect);
					::SendMessage(pThis->m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
				}
			}
			::SendMessage(pThis->m_hwndToolTip,TTM_ACTIVATE,pThis->m_fShowToolTips,0);
			pThis->m_HotItem=-1;
			pThis->m_fTrackMouseEvent=false;
		}
		return 0;

	case WM_SIZE:
		{
			CSideBar *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				pThis->UpdateItem(pThis->m_HotItem);
				pThis->m_HotItem=-1;
			}
			pThis->SetToolTip();
		}
		return 0;

	case WM_PAINT:
		{
			CSideBar *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			RECT rc;
			DrawUtil::FillDirection FillDir;
			HDC hdcMemory;
			HBITMAP hbmOld;

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rc);
			if (pThis->m_fVertical) {
				rc.top=ps.rcPaint.top;
				rc.bottom=ps.rcPaint.bottom;
				FillDir=DrawUtil::DIRECTION_HORZ;
			} else {
				rc.left=ps.rcPaint.left;
				rc.right=ps.rcPaint.right;
				FillDir=DrawUtil::DIRECTION_VERT;
			}
			DrawUtil::FillGradient(ps.hdc,&rc,
								   pThis->m_BackColor1,pThis->m_BackColor2,FillDir);
			hdcMemory=::CreateCompatibleDC(ps.hdc);
			hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,pThis->m_hbmIcons));
			for (int i=0;i<(int)pThis->m_ItemList.size();i++) {
				pThis->GetItemRect(i,&rc);
				if (pThis->m_ItemList[i].Command!=ITEM_SEPARATOR
						&& rc.left<ps.rcPaint.right && rc.right>ps.rcPaint.left
						&& rc.top<ps.rcPaint.bottom && rc.bottom>ps.rcPaint.top) {
					bool fDisabled=(pThis->m_ItemList[i].Flags&ITEM_FLAG_DISABLED)!=0;
					bool fHotItem=pThis->m_HotItem==i;
					RGBQUAD ForeColor;
					if (fHotItem) {
						DrawUtil::FillGradient(ps.hdc,&rc,
											   pThis->m_HighlightBackColor1,
											   pThis->m_HighlightBackColor2,
											   FillDir);
						ForeColor.rgbBlue=GetBValue(pThis->m_HighlightForeColor);
						ForeColor.rgbGreen=GetGValue(pThis->m_HighlightForeColor);
						ForeColor.rgbRed=GetRValue(pThis->m_HighlightForeColor);
					} else {
						ForeColor.rgbBlue=GetBValue(pThis->m_ForeColor);
						ForeColor.rgbGreen=GetGValue(pThis->m_ForeColor);
						ForeColor.rgbRed=GetRValue(pThis->m_ForeColor);
					}
					::SetDIBColorTable(hdcMemory,0,1,&ForeColor);
					::TransparentBlt(ps.hdc,
									 rc.left+BUTTON_MARGIN,rc.top+BUTTON_MARGIN,
									 ICON_WIDTH,ICON_HEIGHT,
									 hdcMemory,
									 pThis->m_ItemList[i].Icon*ICON_WIDTH,0,
									 ICON_WIDTH,ICON_HEIGHT,
									 pThis->m_IconTransparentColor);
				}
			}
			::SelectObject(hdcMemory,hbmOld);
			::DeleteDC(hdcMemory);
			::GetClientRect(hwnd,&rc);
			::DrawEdge(ps.hdc,&rc,BDR_RAISEDOUTER,BF_RECT);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CSideBar *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int HotItem=pThis->HitTest(x,y);

			if (HotItem>=0
					&& (pThis->m_ItemList[HotItem].Command==ITEM_SEPARATOR
						|| (pThis->m_ItemList[HotItem].Flags&ITEM_FLAG_DISABLED)!=0))
				HotItem=-1;
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
			CSideBar *pThis=GetThis(hwnd);

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
			CSideBar *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				pThis->m_ClickItem=pThis->m_HotItem;
				::SetCapture(hwnd);
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd) {
			CSideBar *pThis=GetThis(hwnd);

			::ReleaseCapture();
			if (pThis->m_HotItem>=0) {
				if (pThis->m_pEventHandler!=NULL)
					pThis->m_pEventHandler->OnCommand(
						pThis->m_ItemList[pThis->m_HotItem].Command);
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CSideBar *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnRButtonDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CSideBar *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				CSideBar *pThis=GetThis(hwnd);
				LPNMTTDISPINFO pnmttdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);

				pThis->m_pCommandList->GetCommandName(
					pThis->m_pCommandList->IDToIndex(pThis->m_ItemList[pnmttdi->hdr.idFrom].Command),
					pnmttdi->szText,lengthof(pnmttdi->szText));
				pnmttdi->lpszText=pnmttdi->szText;
				pnmttdi->hinst=NULL;
			}
			return 0;

		case TTN_SHOW:
			{
				LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);
				RECT rcBar,rcTip;

				::GetWindowRect(hwnd,&rcBar);
				::GetWindowRect(pnmh->hwndFrom,&rcTip);
				::SetWindowPos(pnmh->hwndFrom,NULL,rcBar.right-PADDING_WIDTH,rcTip.top,0,0,
							   SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CSideBar *pThis=GetThis(hwnd);

			pThis->OnDestroy();
			pThis->m_hwndToolTip=NULL;
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CSideBar::GetItemRect(int Item,RECT *pRect) const
{
	int Offset=PADDING_WIDTH;

	for (int i=0;i<Item;i++) {
		if (m_ItemList[i].Command==ITEM_SEPARATOR) {
			Offset+=SEPARATOR_WIDTH;
		} else {
			if (m_fVertical)
				Offset+=ICON_HEIGHT+BUTTON_MARGIN*2;
			else
				Offset+=ICON_WIDTH+BUTTON_MARGIN*2;
		}
	}
	if (m_fVertical) {
		pRect->left=PADDING_WIDTH;
		pRect->right=PADDING_WIDTH+ICON_WIDTH+BUTTON_MARGIN*2;
		pRect->top=Offset;
		pRect->bottom=Offset+(m_ItemList[Item].Command==ITEM_SEPARATOR?SEPARATOR_WIDTH:ICON_HEIGHT+BUTTON_MARGIN*2);
	} else {
		pRect->top=PADDING_WIDTH;
		pRect->bottom=PADDING_WIDTH+ICON_HEIGHT+BUTTON_MARGIN*2;
		pRect->left=Offset;
		pRect->right=Offset+(m_ItemList[Item].Command==ITEM_SEPARATOR?SEPARATOR_WIDTH:ICON_WIDTH+BUTTON_MARGIN*2);
	}
}


void CSideBar::UpdateItem(int Item) const
{
	RECT rc;

	GetItemRect(Item,&rc);
	::InvalidateRect(m_hwnd,&rc,TRUE);
}


int CSideBar::HitTest(int x,int y) const
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	for (size_t i=0;i<m_ItemList.size();i++) {
		RECT rc;
		GetItemRect(i,&rc);
		if (::PtInRect(&rc,pt))
			return i;
	}
	return -1;
}


void CSideBar::SetToolTip()
{
	TOOLINFO ti;

	if (m_hwndToolTip==NULL)
		return;
	ti.cbSize=TTTOOLINFO_V1_SIZE;
	ti.hwnd=m_hwnd;
	for (int i=0;i<(int)m_ItemList.size();i++) {
		ti.uId=i;
		GetItemRect(i,&ti.rect);
		::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,reinterpret_cast<LPARAM>(&ti));
	}
}




CSideBar::CEventHandler::CEventHandler()
	: m_pSideBar(NULL)
{
}


CSideBar::CEventHandler::~CEventHandler()
{
}
