#include "stdafx.h"
#include "TVTest.h"
#include "StatusView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define STATUS_WINDOW_CLASS	APP_NAME TEXT(" Status")

#define ITEM_MARGIN	4




CStatusItem::CStatusItem(int ID,int DefaultWidth)
	: m_pStatus(NULL)
	, m_ID(ID)
	, m_DefaultWidth(DefaultWidth)
	, m_Width(DefaultWidth)
	, m_MinWidth(8)
	, m_fVisible(true)
	, m_fBreak(false)
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
	, m_ItemHeight(m_FontHeight+ITEM_MARGIN*2)
	, m_fMultiRow(false)
	, m_MaxRows(2)
	, m_Rows(1)
	, m_fSingleMode(false)
	, m_pszSingleText(NULL)
	, m_HotItem(-1)
	, m_fTrackMouseEvent(false)
	, m_fOnButtonDown(false)
	, m_pEventHandler(NULL)
	, m_fBufferedPaint(false)
	, m_fAdjustSize(true)
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
	if (Index<0 || Index>=m_ItemList.Length())
		return NULL;
	return m_ItemList[Index];
}


CStatusItem *CStatusView::GetItem(int Index)
{
	if (Index<0 || Index>=m_ItemList.Length())
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
	pItem->m_pStatus=this;
	return true;
}


int CStatusView::IDToIndex(int ID) const
{
	int i;

	for (i=0;i<m_ItemList.Length();i++) {
		if (m_ItemList[i]->GetID()==ID)
			return i;
	}
	return -1;
}


int CStatusView::IndexToID(int Index) const
{
	if (Index<0 || Index>=m_ItemList.Length())
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
			CStatusView *pThis=static_cast<CStatusView*>(OnCreate(hwnd,lParam));
			RECT rc;

			::SetRectEmpty(&rc);
			rc.bottom=pThis->m_ItemHeight;
			Theme::AddBorderRect(&pThis->m_Theme.Border,&rc);
			::AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
			::SetWindowPos(hwnd,NULL,0,0,pcs->cx,rc.bottom-rc.top,
						   SWP_NOZORDER | SWP_NOMOVE);

			pThis->m_HotItem=-1;
			pThis->m_fTrackMouseEvent=false;
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

	case WM_SIZE:
		{
			CStatusView *pThis=GetStatusView(hwnd);

			if (pThis->m_fMultiRow)
				pThis->AdjustSize();
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CStatusView *pThis=GetStatusView(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			if (::GetCapture()==hwnd) {
				pThis->GetItemRectByIndex(pThis->m_HotItem,&rc);
				x-=rc.left;
				pThis->m_ItemList[pThis->m_HotItem]->OnMouseMove(x,y);
			} else {
				CStatusView *pThis=GetStatusView(hwnd);

				if (pThis->m_fSingleMode)
					break;

				POINT pt;
				pt.x=x;
				pt.y=y;
				int i;
				for (i=0;i<pThis->m_ItemList.Length();i++) {
					if (!pThis->m_ItemList[i]->GetVisible())
						continue;
					pThis->GetItemRectByIndex(i,&rc);
					if (::PtInRect(&rc,pt))
						break;
				}
				if (i==pThis->m_ItemList.Length())
					i=-1;
				if (i!=pThis->m_HotItem)
					pThis->SetHotItem(i);
				if (!pThis->m_fTrackMouseEvent) {
					TRACKMOUSEEVENT tme;

					tme.cbSize=sizeof(TRACKMOUSEEVENT);
					tme.dwFlags=TME_LEAVE;
					tme.hwndTrack=hwnd;
					if (TrackMouseEvent(&tme))
						pThis->m_fTrackMouseEvent=true;
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			CStatusView *pThis=GetStatusView(hwnd);

			pThis->m_fTrackMouseEvent=false;
			if (!pThis->m_fOnButtonDown) {
				if (pThis->m_HotItem>=0)
					pThis->SetHotItem(-1);
				if (pThis->m_pEventHandler)
					pThis->m_pEventHandler->OnMouseLeave();
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		{
			CStatusView *pThis=GetStatusView(hwnd);

			if (pThis->m_HotItem>=0) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				pThis->GetItemRectByIndex(pThis->m_HotItem,&rc);
				x-=rc.left;
				pThis->m_fOnButtonDown=true;
				switch (uMsg) {
				case WM_LBUTTONDOWN:
					pThis->m_ItemList[pThis->m_HotItem]->OnLButtonDown(x,y);
					break;
				case WM_RBUTTONDOWN:
					pThis->m_ItemList[pThis->m_HotItem]->OnRButtonDown(x,y);
					break;
				case WM_LBUTTONDBLCLK:
					pThis->m_ItemList[pThis->m_HotItem]->OnLButtonDoubleClick(x,y);
					break;
				}
				pThis->m_fOnButtonDown=false;
				if (!pThis->m_fTrackMouseEvent) {
					POINT pt;

					::GetCursorPos(&pt);
					::ScreenToClient(hwnd,&pt);
					::GetClientRect(hwnd,&rc);
					if (::PtInRect(&rc,pt)) {
						::SendMessage(hwnd,WM_MOUSEMOVE,0,MAKELPARAM(pt.x,pt.y));
					} else {
						pThis->SetHotItem(-1);
						if (pThis->m_pEventHandler)
							pThis->m_pEventHandler->OnMouseLeave();
					}
				}
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd) {
			ReleaseCapture();
		}
		return 0;

	case WM_MOUSEHOVER:
		{
			CStatusView *pThis=GetStatusView(hwnd);

			if (pThis->m_HotItem>=0) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				pThis->GetItemRectByIndex(pThis->m_HotItem,&rc);
				x-=rc.left;
				if (pThis->m_ItemList[pThis->m_HotItem]->OnMouseHover(x,y)) {
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
			CStatusView *pThis=GetStatusView(hwnd);

			if (pThis->m_HotItem>=0) {
				SetCursor(LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_NOTIFY:
		{
			CStatusView *pThis=GetStatusView(hwnd);

			if (pThis->m_HotItem>=0)
				return pThis->m_ItemList[pThis->m_HotItem]->OnNotifyMessage(reinterpret_cast<LPNMHDR>(lParam));
		}
		break;

	case WM_DISPLAYCHANGE:
		{
			CStatusView *pThis=GetStatusView(hwnd);

			pThis->m_Offscreen.Destroy();
		}
		return 0;

	case WM_DESTROY:
		{
			CStatusView *pThis=GetStatusView(hwnd);

			pThis->m_Offscreen.Destroy();
			pThis->OnDestroy();
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
			Invalidate(&rc);
	}
}


bool CStatusView::GetItemRect(int ID,RECT *pRect) const
{
	int Index;

	Index=IDToIndex(ID);
	if (Index<0)
		return false;
	return GetItemRectByIndex(Index,pRect);
}


bool CStatusView::GetItemRectByIndex(int Index,RECT *pRect) const
{
	if (Index<0 || Index>=m_ItemList.Length())
		return false;

	RECT rc;
	GetClientRect(&rc);
	Theme::SubtractBorderRect(&m_Theme.Border,&rc);
	if (m_fMultiRow)
		rc.bottom=rc.top+m_ItemHeight;
	int Left=rc.left;
	const CStatusItem *pItem;
	for (int i=0;i<Index;i++) {
		pItem=m_ItemList[i];
		if (m_fMultiRow && pItem->m_fBreak) {
			rc.left=Left;
			rc.top=rc.bottom;
			rc.bottom+=m_ItemHeight;
		} else if (pItem->GetVisible()) {
			rc.left+=pItem->GetWidth()+ITEM_MARGIN*2;
		}
	}
	rc.right=rc.left;
	pItem=m_ItemList[Index];
	if (pItem->GetVisible())
		rc.right+=pItem->GetWidth()+ITEM_MARGIN*2;
	*pRect=rc;
	return true;
}


bool CStatusView::GetItemClientRect(int ID,RECT *pRect) const
{
	RECT rc;

	if (!GetItemRect(ID,&rc))
		return false;
	if (rc.left<rc.right) {
		rc.left+=ITEM_MARGIN;
		rc.top+=ITEM_MARGIN;
		rc.right-=ITEM_MARGIN;
		rc.bottom-=ITEM_MARGIN;
	}
	*pRect=rc;
	return true;
}


int CStatusView::GetItemHeight() const
{
	RECT rc;

	if (m_fMultiRow)
		return m_ItemHeight;
	GetClientRect(&rc);
	Theme::SubtractBorderRect(&m_Theme.Border,&rc);
	return rc.bottom-rc.top;
}


void CStatusView::GetItemMargin(RECT *pRect) const
{
	pRect->left=ITEM_MARGIN;
	pRect->top=ITEM_MARGIN;
	pRect->right=ITEM_MARGIN;
	pRect->bottom=ITEM_MARGIN;
}


int CStatusView::GetIntegralWidth() const
{
	int Width;

	Width=0;
	for (int i=0;i<m_ItemList.Length();i++) {
		if (m_ItemList[i]->GetVisible())
			Width+=m_ItemList[i]->GetWidth()+ITEM_MARGIN*2;
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
	for (i=0;i<m_ItemList.Length();i++)
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
	if (m_hwnd!=NULL)
		Redraw(NULL,RDW_INVALIDATE | RDW_UPDATENOW);
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
	m_ItemHeight=m_FontHeight+ITEM_MARGIN*2;
	if (m_hwnd!=NULL)
		AdjustSize();
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


bool CStatusView::SetMultiRow(bool fMultiRow)
{
	if (m_fMultiRow!=fMultiRow) {
		m_fMultiRow=fMultiRow;
		if (m_hwnd!=NULL)
			AdjustSize();
	}
	return true;
}


bool CStatusView::SetMaxRows(int MaxRows)
{
	if (MaxRows<1)
		return false;
	if (m_MaxRows!=MaxRows) {
		m_MaxRows=MaxRows;
		if (m_hwnd!=NULL && m_fMultiRow && m_Rows>MaxRows)
			AdjustSize();
	}
	return true;
}


int CStatusView::CalcHeight(int Width) const
{
	int Rows=1;

	if (m_fMultiRow) {
		RECT rc={0,0,Width,0};
		Theme::SubtractBorderRect(&m_Theme.Border,&rc);
		int RowWidth=0;
		for (int i=0;i<m_ItemList.Length();i++) {
			const CStatusItem *pItem=m_ItemList[i];

			if (pItem->GetVisible()) {
				const int ItemWidth=pItem->GetWidth()+ITEM_MARGIN*2;

				if (RowWidth==0) {
					RowWidth=ItemWidth;
				} else {
					if (RowWidth+ItemWidth>rc.right-rc.left && Rows<m_MaxRows) {
						Rows++;
						RowWidth=ItemWidth;
					} else {
						RowWidth+=ItemWidth;
					}
				}
			}
		}
	}

	RECT rcBorder={0,0,0,m_ItemHeight*Rows};
	Theme::AddBorderRect(&m_Theme.Border,&rcBorder);
	return rcBorder.bottom-rcBorder.top;
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

	for (i=0;i<m_ItemList.Length();i++) {
		CStatusItem *pItem=GetItem(IDToIndex(pOrderList[i]));

		if (pItem==NULL)
			return false;
		NewList.Add(pItem);
		for (j=0;j<i;j++) {
			if (NewList[i]==NewList[j])
				return false;
		}
	}
	for (i=0;i<m_ItemList.Length();i++)
		m_ItemList.Set(i,NewList[i]);
	if (m_hwnd!=NULL && !m_fSingleMode) {
		if (m_fMultiRow)
			AdjustSize();
		else
			Invalidate();
	}
	return true;
}


bool CStatusView::DrawItemPreview(CStatusItem *pItem,HDC hdc,const RECT *pRect,
								  bool fHighlight,HFONT hfont) const
{
	HFONT hfontOld;
	int OldBkMode;
	COLORREF crOldTextColor,crOldBkColor;
	const Theme::Style &Style=
		fHighlight?m_Theme.HighlightItemStyle:m_Theme.ItemStyle;

	if (hfont!=NULL)
		hfontOld=SelectFont(hdc,hfont);
	else
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


void CStatusView::EnableSizeAdjustment(bool fEnable)
{
	if (m_fAdjustSize!=fEnable) {
		m_fAdjustSize=fEnable;
		if (fEnable && m_hwnd!=NULL)
			AdjustSize();
	}
}


void CStatusView::OnTrace(LPCTSTR pszOutput)
{
	SetSingleText(pszOutput);
}


void CStatusView::SetHotItem(int Item)
{
	if (Item<0 || Item>=m_ItemList.Length())
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
	const int ItemHeight=m_fMultiRow?m_ItemHeight:rc.bottom-rc.top;

	if (!m_fSingleMode) {
		int MaxWidth=0;
		for (int i=0;i<m_ItemList.Length();i++) {
			const CStatusItem *pItem=m_ItemList[i];
			if (pItem->GetVisible() && pItem->GetWidth()>MaxWidth)
				MaxWidth=pItem->GetWidth();
		}
		if (MaxWidth>m_Offscreen.GetWidth()
				|| ItemHeight>m_Offscreen.GetHeight())
			m_Offscreen.Create(MaxWidth+ITEM_MARGIN*2,ItemHeight);
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

	if (m_fMultiRow)
		rc.bottom=rc.top+ItemHeight;

	if (m_fSingleMode) {
		RECT rcRow=rc;

		for (int i=0;i<m_Rows;i++) {
			const Theme::Style &Style=i==0?m_Theme.ItemStyle:m_Theme.BottomItemStyle;

			Theme::FillGradient(hdcDst,&rcRow,&Style.Gradient);
			rcRow.top=rcRow.bottom;
			rcRow.bottom+=ItemHeight;
		}
		::SetTextColor(hdcDst,m_Theme.ItemStyle.TextColor);
		rc.left+=ITEM_MARGIN;
		rc.right-=ITEM_MARGIN;
		::DrawText(hdcDst,m_pszSingleText,-1,&rc,
				   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	} else {
		const int Left=rc.left;
		int Row=0;

		rc.right=Left;
		for (int i=0;i<m_ItemList.Length();i++) {
			CStatusItem *pItem=m_ItemList[i];

			if (pItem->GetVisible()) {
				rc.left=rc.right;
				rc.right=rc.left+pItem->GetWidth()+ITEM_MARGIN*2;
				if (rc.right>pPaintRect->left && rc.left<pPaintRect->right) {
					const bool fHighlight=i==m_HotItem;
					const Theme::Style &Style=
						fHighlight?m_Theme.HighlightItemStyle:
						Row==0?m_Theme.ItemStyle:m_Theme.BottomItemStyle;
					RECT rcDraw=rc;
					if (hdcDst!=hdc)
						::OffsetRect(&rcDraw,-rcDraw.left,-rcDraw.top);
					Theme::DrawStyleBackground(hdcDst,&rcDraw,&Style);
					::SetTextColor(hdcDst,Style.TextColor);
					::SetBkColor(hdcDst,MixColor(Style.Gradient.Color1,Style.Gradient.Color2));
					rcDraw.left+=ITEM_MARGIN;
					rcDraw.right-=ITEM_MARGIN;
					pItem->Draw(hdcDst,&rcDraw);
					if (hdcDst!=hdc)
						m_Offscreen.CopyTo(hdc,&rc);
				}
			}
			if (m_fMultiRow && pItem->m_fBreak) {
				if (rc.right<pPaintRect->right) {
					rc.left=max(rc.right,pPaintRect->left);
					rc.right=pPaintRect->right;
					Theme::FillGradient(hdc,&rc,
										Row==0?&m_Theme.ItemStyle.Gradient:&m_Theme.BottomItemStyle.Gradient);
				}
				rc.right=Left;
				rc.top=rc.bottom;
				rc.bottom+=ItemHeight;
				Row++;
			}
		}
		if (rc.right<pPaintRect->right) {
			rc.left=max(rc.right,pPaintRect->left);
			rc.right=pPaintRect->right;
			Theme::FillGradient(hdc,&rc,
								Row==0?&m_Theme.ItemStyle.Gradient:&m_Theme.BottomItemStyle.Gradient);
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
	if (!m_fAdjustSize)
		return;

	int OldRows=m_Rows;
	RECT rcWindow,rc;

	CalcRows();
	GetPosition(&rcWindow);
	::SetRectEmpty(&rc);
	rc.bottom=m_ItemHeight*m_Rows;
	Theme::AddBorderRect(&m_Theme.Border,&rc);
	CalcPositionFromClientRect(&rc);
	int Height=rc.bottom-rc.top;
	if (Height!=rcWindow.bottom-rcWindow.top) {
		::SetWindowPos(m_hwnd,NULL,0,0,rcWindow.right-rcWindow.left,Height,
					   SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
		Invalidate();
		if (m_pEventHandler!=NULL)
			m_pEventHandler->OnHeightChanged(Height);
	} else if (m_Rows!=OldRows) {
		Invalidate();
	}
}


void CStatusView::CalcRows()
{
	if (m_fMultiRow) {
		RECT rc;
		int Rows,RowWidth;

		GetClientRect(&rc);
		Theme::SubtractBorderRect(&m_Theme.Border,&rc);
		Rows=1;
		RowWidth=0;
		for (int i=0;i<m_ItemList.Length();i++) {
			CStatusItem *pItem=m_ItemList[i];

			pItem->m_fBreak=false;
			if (pItem->GetVisible()) {
				const int ItemWidth=pItem->GetWidth()+ITEM_MARGIN*2;

				if (RowWidth==0) {
					RowWidth=ItemWidth;
				} else {
					if (RowWidth+ItemWidth>rc.right-rc.left && Rows<m_MaxRows) {
						m_ItemList[i-1]->m_fBreak=true;
						Rows++;
						RowWidth=ItemWidth;
					} else {
						RowWidth+=ItemWidth;
					}
				}
			}
		}
		m_Rows=Rows;
	} else {
		for (int i=0;i<m_ItemList.Length();i++)
			m_ItemList[i]->m_fBreak=false;
		m_Rows=1;
	}
}
