#include "stdafx.h"
#include "TVTest.h"
#include "StatusView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define STATUS_WINDOW_CLASS	APP_NAME TEXT(" Status")

#define STATUS_MARGIN	4




CStatusItem::CStatusItem(int ID,int DefaultWidth)
	: m_pStatus(NULL)
	, m_ID(ID)
	, m_DefaultWidth(DefaultWidth)
	, m_Width(DefaultWidth)
	, m_MinWidth(8)
	, m_fVisible(true)
{
}


int CStatusItem::GetIndex() const
{
	if (m_pStatus!=NULL) {
		for (int i=0;i<m_pStatus->NumItems();i++) {
			if (m_pStatus->GetItem(i)==this)
				return i;
		}
	}
	return -1;
}


bool CStatusItem::GetRect(RECT *pRect) const
{
	if (m_pStatus==NULL)
		return false;
	return m_pStatus->GetItemRect(m_ID,pRect);
}


bool CStatusItem::GetClientRect(RECT *pRect) const
{
	if (m_pStatus==NULL)
		return false;
	return m_pStatus->GetItemClientRect(m_ID,pRect);
}


bool CStatusItem::SetWidth(int Width)
{
	if (Width<0)
		return false;
	if (Width<m_MinWidth)
		Width=m_MinWidth;
	else
		m_Width=Width;
	return true;
}


void CStatusItem::SetVisible(bool fVisible)
{
	m_fVisible=fVisible;
	OnVisibleChange(fVisible && m_pStatus!=NULL && m_pStatus->GetVisible());
}


bool CStatusItem::Update()
{
	if (m_pStatus==NULL)
		return false;
	m_pStatus->UpdateItem(m_ID);
	return true;
}


bool CStatusItem::GetMenuPos(POINT *pPos,UINT *pFlags)
{
	if (m_pStatus==NULL)
		return false;

	RECT rc;
	POINT pt;

	if (!GetRect(&rc))
		return false;
	if (pFlags!=NULL)
		*pFlags=0;
	pt.x=rc.left;
	pt.y=rc.bottom;
	::ClientToScreen(m_pStatus->GetHandle(),&pt);
	HMONITOR hMonitor=::MonitorFromPoint(pt,MONITOR_DEFAULTTONULL);
	if (hMonitor!=NULL) {
		MONITORINFO mi;

		mi.cbSize=sizeof(mi);
		if (::GetMonitorInfo(hMonitor,&mi)) {
			if (pt.y>=mi.rcMonitor.bottom-32) {
				pt.x=rc.left;
				pt.y=rc.top;
				::ClientToScreen(m_pStatus->GetHandle(),&pt);
				if (pFlags!=NULL)
					*pFlags=TPM_BOTTOMALIGN;
			}
		}
	}
	*pPos=pt;
	return true;
}


void CStatusItem::DrawText(HDC hdc,const RECT *pRect,LPCTSTR pszText,DWORD Flags) const
{
	
	::DrawText(hdc,pszText,-1,const_cast<LPRECT>(pRect),
			   ((Flags&DRAWTEXT_HCENTER)!=0?DT_CENTER:DT_LEFT) |
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}


void CStatusItem::DrawIcon(HDC hdc,const RECT *pRect,HBITMAP hbm,int SrcX,int SrcY,
							int IconWidth,int IconHeight,bool fEnabled) const
{
	HDC hdcMem;
	HBITMAP hbmOld;
	COLORREF cr;

	if (hbm==NULL)
		return;
	hdcMem=::CreateCompatibleDC(hdc);
	if (hdcMem==NULL)
		return;
	hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,hbm));
	cr=::GetTextColor(hdc);
	if (!fEnabled)
		cr=MixColor(cr,::GetBkColor(hdc));
	DrawUtil::DrawMonoColorDIB(hdc,
							   pRect->left+(pRect->right-pRect->left-16)/2,
							   pRect->top+(pRect->bottom-pRect->top-16)/2,
							   hdcMem,SrcX,SrcY,IconWidth,IconHeight,cr);
	::SelectObject(hdcMem,hbmOld);
	::DeleteDC(hdcMem);
}




CStatusView::CEventHandler::CEventHandler()
	: m_pStatusView(NULL)
{
}


CStatusView::CEventHandler::~CEventHandler()
{
	if (m_pStatusView!=NULL)
		m_pStatusView->SetEventHandler(NULL);
}




HINSTANCE CStatusView::m_hinst=NULL;


bool CStatusView::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=STATUS_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CStatusView::CStatusView()
	: m_Font(/*DrawUtil::FONT_STATUS*/DrawUtil::FONT_DEFAULT)
	, m_FontHeight(m_Font.GetHeight(false))
	, m_NumItems(0)
	, m_fSingleMode(false)
	, m_pszSingleText(NULL)
	, m_HotItem(-1)
	, m_fTrackMouseEvent(false)
	, m_fOnButtonDown(false)
	, m_pEventHandler(NULL)
	, m_fBufferedPaint(false)
{
	m_Theme.ItemStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.ItemStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.ItemStyle.Gradient.Color1=RGB(192,192,192);
	m_Theme.ItemStyle.Gradient.Color2=RGB(192,192,192);
	m_Theme.ItemStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.ItemStyle.TextColor=RGB(0,0,0);
	m_Theme.HighlightItemStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.HighlightItemStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.HighlightItemStyle.Gradient.Color1=RGB(128,128,128);
	m_Theme.HighlightItemStyle.Gradient.Color2=RGB(128,128,128);
	m_Theme.HighlightItemStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.HighlightItemStyle.TextColor=RGB(255,255,255);
	m_Theme.Border.Type=Theme::BORDER_RAISED;
	m_Theme.Border.Color=RGB(192,192,192);
}


CStatusView::~CStatusView()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pStatusView=NULL;
	m_ItemList.DeleteAll();
	delete [] m_pszSingleText;
}


bool CStatusView::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 STATUS_WINDOW_CLASS,NULL,m_hinst);
}


const CStatusItem *CStatusView::GetItem(int Index) const
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ItemList[Index];
}


CStatusItem *CStatusView::GetItem(int Index)
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ItemList[Index];
}


const CStatusItem *CStatusView::GetItemByID(int ID) const
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_ItemList[Index];
}


CStatusItem *CStatusView::GetItemByID(int ID)
{
	int Index=IDToIndex(ID);

	if (Index<0)
		return NULL;
	return m_ItemList[Index];
}


bool CStatusView::AddItem(CStatusItem *pItem)
{
	m_ItemList.Add(pItem);
	m_NumItems++;
	pItem->m_pStatus=this;
	return true;
}


int CStatusView::IDToIndex(int ID) const
{
	int i;

	for (i=0;i<m_NumItems;i++) {
		if (m_ItemList[i]->GetID()==ID)
			return i;
	}
	return -1;
}


int CStatusView::IndexToID(int Index) const
{
	if (Index<0 || Index>=m_NumItems)
		return -1;
	return m_ItemList[Index]->GetID();
}


CStatusView *CStatusView::GetStatusView(HWND hwnd)
{
	return reinterpret_cast<CStatusView*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CStatusView::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,
																LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CStatusView *pStatus=static_cast<CStatusView*>(OnCreate(hwnd,lParam));
			RECT rc;

			::SetRectEmpty(&rc);
			rc.bottom=pStatus->m_FontHeight+STATUS_MARGIN*2;
			Theme::AddBorderRect(&pStatus->m_Theme.Border,&rc);
			::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
			::SetWindowPos(hwnd,NULL,0,0,pcs->cx,rc.bottom-rc.top,
						   SWP_NOZORDER | SWP_NOMOVE);

			pStatus->m_HotItem=-1;
			pStatus->m_fTrackMouseEvent=false;
		}
		return 0;

	case WM_PAINT:
		{
			CStatusView *pThis=GetStatusView(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			if (pThis->m_fBufferedPaint) {
				HDC hdc=pThis->m_BufferedPaint.Begin(ps.hdc,&ps.rcPaint,true);

				if (hdc!=NULL) {
					pThis->Draw(hdc,&ps.rcPaint);
					pThis->m_BufferedPaint.SetOpaque();
					pThis->m_BufferedPaint.End();
				} else {
					pThis->Draw(ps.hdc,&ps.rcPaint);
				}
			} else {
				pThis->Draw(ps.hdc,&ps.rcPaint);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CStatusView *pStatus=GetStatusView(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			if (GetCapture()==hwnd) {
				pStatus->GetItemRect(pStatus->IndexToID(pStatus->m_HotItem),&rc);
				x-=rc.left;
				pStatus->m_ItemList[pStatus->m_HotItem]->OnMouseMove(x,y);
			} else {
				CStatusView *pStatus=GetStatusView(hwnd);

				if (pStatus->m_fSingleMode)
					break;

				POINT pt;
				pt.x=x;
				pt.y=y;
				int i;
				for (i=0;i<pStatus->m_NumItems;i++) {
					if (!pStatus->m_ItemList[i]->GetVisible())
						continue;
					pStatus->GetItemRect(pStatus->IndexToID(i),&rc);
					if (::PtInRect(&rc,pt))
						break;
				}
				if (i==pStatus->m_NumItems)
					i=-1;
				if (i!=pStatus->m_HotItem)
					pStatus->SetHotItem(i);
				if (!pStatus->m_fTrackMouseEvent) {
					TRACKMOUSEEVENT tme;

					tme.cbSize=sizeof(TRACKMOUSEEVENT);
					tme.dwFlags=TME_LEAVE;
					tme.hwndTrack=hwnd;
					if (TrackMouseEvent(&tme))
						pStatus->m_fTrackMouseEvent=true;
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			pStatus->m_fTrackMouseEvent=false;
			if (!pStatus->m_fOnButtonDown) {
				if (pStatus->m_HotItem>=0)
					pStatus->SetHotItem(-1);
				if (pStatus->m_pEventHandler)
					pStatus->m_pEventHandler->OnMouseLeave();
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			if (pStatus->m_HotItem>=0) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				pStatus->GetItemRect(pStatus->IndexToID(pStatus->m_HotItem),&rc);
				x-=rc.left;
				pStatus->m_fOnButtonDown=true;
				switch (uMsg) {
				case WM_LBUTTONDOWN:
					pStatus->m_ItemList[pStatus->m_HotItem]->OnLButtonDown(x,y);
					break;
				case WM_RBUTTONDOWN:
					pStatus->m_ItemList[pStatus->m_HotItem]->OnRButtonDown(x,y);
					break;
				case WM_LBUTTONDBLCLK:
					pStatus->m_ItemList[pStatus->m_HotItem]->OnLButtonDoubleClick(x,y);
					break;
				}
				pStatus->m_fOnButtonDown=false;
				if (!pStatus->m_fTrackMouseEvent) {
					POINT pt;

					::GetCursorPos(&pt);
					::ScreenToClient(hwnd,&pt);
					::GetClientRect(hwnd,&rc);
					if (::PtInRect(&rc,pt)) {
						::SendMessage(hwnd,WM_MOUSEMOVE,0,MAKELPARAM(pt.x,pt.y));
					} else {
						pStatus->SetHotItem(-1);
						if (pStatus->m_pEventHandler)
							pStatus->m_pEventHandler->OnMouseLeave();
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd) {
			ReleaseCapture();
		}
		return 0;

	case WM_MOUSEHOVER:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			if (pStatus->m_HotItem>=0) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				pStatus->GetItemRect(pStatus->IndexToID(pStatus->m_HotItem),&rc);
				x-=rc.left;
				if (pStatus->m_ItemList[pStatus->m_HotItem]->OnMouseHover(x,y)) {
					TRACKMOUSEEVENT tme;
					tme.cbSize=sizeof(TRACKMOUSEEVENT);
					tme.dwFlags=TME_HOVER;
					tme.hwndTrack=hwnd;
					tme.dwHoverTime=HOVER_DEFAULT;
					::TrackMouseEvent(&tme);
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CStatusView *pStatus=GetStatusView(hwnd);

			if (pStatus->m_HotItem>=0) {
				SetCursor(LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			if (pStatus->m_HotItem>=0)
				return pStatus->m_ItemList[pStatus->m_HotItem]->OnNotifyMessage(reinterpret_cast<LPNMHDR>(lParam));
		}
		break;

	case WM_DISPLAYCHANGE:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			pStatus->m_Offscreen.Destroy();
		}
		return 0;

	case WM_DESTROY:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			pStatus->m_Offscreen.Destroy();
			pStatus->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CStatusView::UpdateItem(int ID)
{
	if (m_hwnd!=NULL) {
		RECT rc;

		GetItemRect(ID,&rc);
		if (rc.right>rc.left)
			::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


bool CStatusView::GetItemRect(int ID,RECT *pRect) const
{
	int Index;
	RECT rc;

	Index=IDToIndex(ID);
	if (Index<0)
		return false;
	GetClientRect(&rc);
	Theme::SubtractBorderRect(&m_Theme.Border,&rc);
	for (int i=0;i<Index;i++) {
		if (m_ItemList[i]->GetVisible())
			rc.left+=m_ItemList[i]->GetWidth()+STATUS_MARGIN*2;
	}
	rc.right=rc.left;
	if (m_ItemList[Index]->GetVisible())
		rc.right+=m_ItemList[Index]->GetWidth()+STATUS_MARGIN*2;
	*pRect=rc;
	return true;
}


bool CStatusView::GetItemClientRect(int ID,RECT *pRect) const
{
	RECT rc;

	if (!GetItemRect(ID,&rc))
		return false;
	if (rc.left<rc.right) {
		rc.left+=STATUS_MARGIN;
		rc.top+=STATUS_MARGIN;
		rc.right-=STATUS_MARGIN;
		rc.bottom-=STATUS_MARGIN;
	}
	*pRect=rc;
	return true;
}


int CStatusView::GetItemHeight() const
{
	RECT rc;

	GetClientRect(&rc);
	Theme::SubtractBorderRect(&m_Theme.Border,&rc);
	return rc.bottom-rc.top;
}


int CStatusView::GetIntegralWidth() const
{
	int Width;

	Width=0;
	for (int i=0;i<m_NumItems;i++) {
		if (m_ItemList[i]->GetVisible())
			Width+=m_ItemList[i]->GetWidth()+STATUS_MARGIN*2;
	}
	RECT rc={0,0,Width,0};
	Theme::AddBorderRect(&m_Theme.Border,&rc);
	return rc.right-rc.left;
}


void CStatusView::SetVisible(bool fVisible)
{
	int i;

	if (m_HotItem>=0)
		SetHotItem(-1);
	CBasicWindow::SetVisible(fVisible);
	for (i=0;i<m_NumItems;i++)
		m_ItemList[i]->OnVisibleChange(fVisible && m_ItemList[i]->GetVisible());
}


void CStatusView::SetSingleText(LPCTSTR pszText)
{
	delete [] m_pszSingleText;
	if (pszText!=NULL) {
		m_pszSingleText=DuplicateString(pszText);
		m_fSingleMode=true;
		SetHotItem(-1);
	} else {
		if (!m_fSingleMode)
			return;
		m_pszSingleText=NULL;
		m_fSingleMode=false;
	}
	if (m_hwnd!=NULL) {
		Invalidate();
		Update();
	}
}


bool CStatusView::SetTheme(const ThemeInfo *pTheme)
{
	if (pTheme==NULL)
		return false;
	m_Theme=*pTheme;
	if (m_hwnd!=NULL) {
		AdjustSize();
		Invalidate();
	}
	return true;
}


bool CStatusView::GetTheme(ThemeInfo *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


bool CStatusView::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	m_FontHeight=m_Font.GetHeight(false);
	if (m_hwnd!=NULL) {
		AdjustSize();
		Invalidate();
	}
	return true;
}


bool CStatusView::GetFont(LOGFONT *pFont) const
{
	return m_Font.GetLogFont(pFont);
}


int CStatusView::GetCurItem() const
{
	if (m_HotItem<0)
		return -1;
	return m_ItemList[m_HotItem]->GetID();
}


bool CStatusView::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pStatusView=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pStatusView=this;
	m_pEventHandler=pEventHandler;
	return true;
}


bool CStatusView::SetItemOrder(const int *pOrderList)
{
	int i,j;
	CPointerVector<CStatusItem> NewList;

	for (i=0;i<m_NumItems;i++) {
		CStatusItem *pItem=GetItem(IDToIndex(pOrderList[i]));

		if (pItem==NULL)
			return false;
		NewList.Add(pItem);
		for (j=0;j<i;j++) {
			if (NewList[i]==NewList[j])
				return false;
		}
	}
	for (i=0;i<m_NumItems;i++)
		m_ItemList.Set(i,NewList[i]);
	if (m_hwnd!=NULL && !m_fSingleMode)
		Invalidate();
	return true;
}


bool CStatusView::DrawItemPreview(CStatusItem *pItem,HDC hdc,const RECT *pRect,bool fHighlight) const
{
	HFONT hfontOld;
	int OldBkMode;
	COLORREF crOldTextColor,crOldBkColor;
	const Theme::Style &Style=
		fHighlight?m_Theme.HighlightItemStyle:m_Theme.ItemStyle;

	hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	crOldTextColor=::SetTextColor(hdc,Style.TextColor);
	crOldBkColor=::SetBkColor(hdc,MixColor(Style.Gradient.Color1,Style.Gradient.Color2,128));
	Theme::DrawStyleBackground(hdc,pRect,&Style);
	pItem->DrawPreview(hdc,pRect);
	::SetBkColor(hdc,crOldBkColor);
	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	SelectFont(hdc,hfontOld);
	return true;
}


bool CStatusView::EnableBufferedPaint(bool fEnable)
{
	m_fBufferedPaint=fEnable;
	return true;
}


void CStatusView::OnTrace(LPCTSTR pszOutput)
{
	SetSingleText(pszOutput);
}


void CStatusView::SetHotItem(int Item)
{
	if (Item<0 || Item>=m_NumItems)
		Item=-1;
	if (m_HotItem!=Item) {
		int OldHotItem=m_HotItem;

		m_HotItem=Item;
		if (OldHotItem>=0) {
			m_ItemList[OldHotItem]->OnFocus(false);
			UpdateItem(IndexToID(OldHotItem));
		}
		if (m_HotItem>=0) {
			m_ItemList[m_HotItem]->OnFocus(true);
			UpdateItem(IndexToID(m_HotItem));
		}

		TRACKMOUSEEVENT tme;
		tme.cbSize=sizeof(TRACKMOUSEEVENT);
		tme.dwFlags=TME_HOVER;
		if (m_HotItem<0)
			tme.dwFlags|=TME_CANCEL;
		tme.hwndTrack=m_hwnd;
		tme.dwHoverTime=HOVER_DEFAULT;
		::TrackMouseEvent(&tme);
	}
}


void CStatusView::Draw(HDC hdc,const RECT *pPaintRect)
{
	RECT rcClient,rc;
	HDC hdcDst;
	HFONT hfontOld;
	COLORREF crOldTextColor,crOldBkColor;
	int OldBkMode;

	GetClientRect(&rcClient);
	rc=rcClient;
	Theme::SubtractBorderRect(&m_Theme.Border,&rc);

	if (!m_fSingleMode) {
		const int Height=rc.bottom-rc.top;
		int MaxWidth=0;
		for (int i=0;i<m_NumItems;i++) {
			const CStatusItem *pItem=m_ItemList[i];
			if (pItem->GetVisible() && pItem->GetWidth()>MaxWidth)
				MaxWidth=pItem->GetWidth();
		}
		if (MaxWidth>m_Offscreen.GetWidth()
				|| Height>m_Offscreen.GetHeight())
			m_Offscreen.Create(MaxWidth+STATUS_MARGIN*2,Height);
		hdcDst=m_Offscreen.GetDC();
		if (hdcDst==NULL)
			hdcDst=hdc;
	} else {
		hdcDst=hdc;
	}

	hfontOld=DrawUtil::SelectObject(hdcDst,m_Font);
	OldBkMode=::SetBkMode(hdcDst,TRANSPARENT);
	crOldTextColor=::GetTextColor(hdcDst);
	crOldBkColor=::GetBkColor(hdcDst);
	if (m_fSingleMode) {
		Theme::FillGradient(hdcDst,&rc,&m_Theme.ItemStyle.Gradient);
		::SetTextColor(hdcDst,m_Theme.ItemStyle.TextColor);
		rc.left+=STATUS_MARGIN;
		::DrawText(hdcDst,m_pszSingleText,-1,&rc,
				   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
	} else {
		RECT rcDraw;

		rc.right=rc.left;
		for (int i=0;i<m_NumItems;i++) {
			CStatusItem *pItem=m_ItemList[i];
			if (!pItem->GetVisible())
				continue;
			rc.left=rc.right;
			rc.right=rc.left+pItem->GetWidth()+STATUS_MARGIN*2;
			if (rc.right<=pPaintRect->left || rc.left>=pPaintRect->right)
				continue;
			bool fHighlight=i==m_HotItem;
			const Theme::Style &Style=
				fHighlight?m_Theme.HighlightItemStyle:m_Theme.ItemStyle;
			rcDraw=rc;
			if (hdcDst!=hdc)
				::OffsetRect(&rcDraw,-rcDraw.left,-rcDraw.top);
			Theme::DrawStyleBackground(hdcDst,&rcDraw,&Style);
			::SetTextColor(hdcDst,Style.TextColor);
			::SetBkColor(hdcDst,MixColor(Style.Gradient.Color1,Style.Gradient.Color2));
			rcDraw.left+=STATUS_MARGIN;
			rcDraw.right-=STATUS_MARGIN;
			pItem->Draw(hdcDst,&rcDraw);
			if (hdcDst!=hdc)
				m_Offscreen.CopyTo(hdc,&rc);
		}
		if (rc.right<pPaintRect->right) {
			rc.left=max(rc.right,pPaintRect->left);
			rc.right=pPaintRect->right;
			Theme::FillGradient(hdc,&rc,&m_Theme.ItemStyle.Gradient);
		}
	}
	Theme::DrawBorder(hdc,rcClient,&m_Theme.Border);
	::SetBkColor(hdcDst,crOldBkColor);
	::SetTextColor(hdcDst,crOldTextColor);
	::SetBkMode(hdcDst,OldBkMode);
	::SelectObject(hdcDst,hfontOld);
}


void CStatusView::AdjustSize()
{
	RECT rcWindow,rc;

	GetPosition(&rcWindow);
	::SetRectEmpty(&rc);
	rc.bottom=m_FontHeight+STATUS_MARGIN*2;
	Theme::AddBorderRect(&m_Theme.Border,&rc);
	CalcPositionFromClientRect(&rc);
	int Height=rc.bottom-rc.top;
	if (Height!=rcWindow.bottom-rcWindow.top) {
		::SetWindowPos(m_hwnd,NULL,0,0,rcWindow.right-rcWindow.left,Height,
					   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnHeightChanged(Height);
	}
}
