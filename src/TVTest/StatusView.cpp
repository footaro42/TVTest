#include "stdafx.h"
#include "TVTest.h"
#include "StatusView.h"
#include "DrawUtil.h"


#define STATUS_WINDOW_CLASS	APP_NAME TEXT(" Status")

#define STATUS_MARGIN	4




CStatusItem::CStatusItem(int ID,int DefaultWidth)
{
	m_pStatus=NULL;
	m_ID=ID;
	m_DefaultWidth=DefaultWidth;
	m_Width=DefaultWidth;
	m_MinWidth=8;
	m_fVisible=true;
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


bool CStatusItem::GetMenuPos(POINT *pPos)
{
	if (m_pStatus==NULL)
		return false;

	RECT rc;

	if (!GetRect(&rc))
		return false;
	pPos->x=rc.left;
	pPos->y=rc.bottom;
	::ClientToScreen(m_pStatus->GetHandle(),pPos);
	return true;
}


void CStatusItem::DrawText(HDC hdc,const RECT *pRect,LPCTSTR pszText) const
{
	::DrawText(hdc,pszText,-1,const_cast<LPRECT>(pRect),
			   DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
}


void CStatusItem::DrawIcon(HDC hdc,const RECT *pRect,HBITMAP hbm,int SrcX,int SrcY,
							int IconWidth,int IconHeight,bool fEnabled) const
{
	HDC hdcMem;
	HBITMAP hbmOld;
	RGBQUAD Palette[2];
	COLORREF cr,crTrans;

	if (hbm==NULL)
		return;
	hdcMem=::CreateCompatibleDC(hdc);
	hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,hbm));
	cr=::GetTextColor(hdc);
	if (!fEnabled)
		cr=MixColor(cr,::GetBkColor(hdc),128);
	Palette[0].rgbBlue=GetBValue(cr);
	Palette[0].rgbGreen=GetGValue(cr);
	Palette[0].rgbRed=GetRValue(cr);
	crTrans=cr^0x00FFFFFF;
	Palette[1].rgbBlue=GetBValue(crTrans);
	Palette[1].rgbGreen=GetGValue(crTrans);
	Palette[1].rgbRed=GetRValue(crTrans);
	::SetDIBColorTable(hdcMem,0,2,Palette);
	::TransparentBlt(hdc,
					 pRect->left+(pRect->right-pRect->left-16)/2,
					 pRect->top+(pRect->bottom-pRect->top-16)/2,
					 16,16,hdcMem,SrcX,SrcY,IconWidth,IconHeight,crTrans);
	::SelectObject(hdcMem,hbmOld);
	::DeleteDC(hdcMem);
}




CStatusViewEventHandler::CStatusViewEventHandler()
{
	m_pStatusView=NULL;
}


CStatusViewEventHandler::~CStatusViewEventHandler()
{
}




HINSTANCE CStatusView::m_hinst=NULL;


bool CStatusView::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=STATUS_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CStatusView::CStatusView()
{
	LOGFONT lf;
	GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	m_hfontStatus=CreateFontIndirect(&lf);
	m_FontHeight=abs(lf.lfHeight);
	m_crBackColor1=RGB(128,192,160);
	m_crBackColor2=RGB(128,192,160);
	m_crTextColor=RGB(64,96,80);
	m_crHighlightBackColor1=RGB(64,96,80);
	m_crHighlightBackColor2=RGB(64,96,80);
	m_crHighlightTextColor=RGB(128,192,160);
	m_NumItems=0;
	m_fSingleMode=false;
	m_pszSingleText=NULL;
	m_HotItem=-1;
	m_fTrackMouseEvent=false;
	m_fOnButtonDown=false;
	m_pEventHandler=NULL;
}


CStatusView::~CStatusView()
{
	Destroy();
	DeleteObject(m_hfontStatus);
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
			CStatusView *pStatus=dynamic_cast<CStatusView*>(OnCreate(hwnd,lParam));
			/*
			HDC hdc;
			HFONT hfontOld;
			TEXTMETRIC tm;

			hdc=GetDC(hwnd);
			hfontOld=(HFONT)SelectObject(hdc,pStatus->m_hfontStatus);
			GetTextMetrics(hdc,&tm);
			pThis->m_FontHeight=tm.tmHeight;
			SelectObject(hdc,hfontOld);
			ReleaseDC(hwnd,hdc);
			*/

			RECT rc;

			rc.left=0;
			rc.top=0;
			rc.right=0;
			rc.bottom=pStatus->m_FontHeight+STATUS_MARGIN*2;
			AdjustWindowRectEx(&rc,pcs->style,FALSE,pcs->dwExStyle);
			MoveWindow(hwnd,0,0,0,rc.bottom-rc.top,FALSE);
			pStatus->m_HotItem=-1;
			pStatus->m_fTrackMouseEvent=false;
		}
		return 0;

	case WM_PAINT:
		{
			CStatusView *pStatus=GetStatusView(hwnd);
			PAINTSTRUCT ps;
			HFONT hfontOld;
			COLORREF crBkColor1,crBkColor2,crOldTextColor,crOldBkColor;
			int OldBkMode;
			RECT rc;

			::BeginPaint(hwnd,&ps);
			hfontOld=SelectFont(ps.hdc,pStatus->m_hfontStatus);
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			crOldTextColor=::GetTextColor(ps.hdc);
			crOldBkColor=::GetBkColor(ps.hdc);
			pStatus->GetClientRect(&rc);
			if (pStatus->m_fSingleMode) {
				DrawUtil::FillGradient(ps.hdc,&rc,pStatus->m_crBackColor1,pStatus->m_crBackColor2,
													DrawUtil::DIRECTION_VERT);
				::SetTextColor(ps.hdc,pStatus->m_crTextColor);
				rc.left+=STATUS_MARGIN;
				::DrawText(ps.hdc,pStatus->m_pszSingleText,-1,&rc,
						DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
			} else {
				int x;

				for (int i=0;i<pStatus->m_NumItems;i++) {
					if (!pStatus->m_ItemList[i]->GetVisible())
						continue;
					bool fHighlight=i==pStatus->m_HotItem;
					::SetTextColor(ps.hdc,fHighlight?
						pStatus->m_crHighlightTextColor:pStatus->m_crTextColor);
					crBkColor1=fHighlight?pStatus->m_crHighlightBackColor1:pStatus->m_crBackColor1;
					crBkColor2=fHighlight?pStatus->m_crHighlightBackColor2:pStatus->m_crBackColor2;
					::SetBkColor(ps.hdc,MixColor(crBkColor1,crBkColor2,128));
					rc.right=rc.left+pStatus->m_ItemList[i]->GetWidth()+
															STATUS_MARGIN*2;
					DrawUtil::FillGradient(ps.hdc,&rc,crBkColor1,crBkColor2,
													DrawUtil::DIRECTION_VERT);
					rc.left+=STATUS_MARGIN;
					rc.right-=STATUS_MARGIN;
					pStatus->m_ItemList[i]->Draw(ps.hdc,&rc);
					rc.left=rc.right+STATUS_MARGIN;
				}
				rc.right=ps.rcPaint.right;
				if (rc.right>rc.left)
					DrawUtil::FillGradient(ps.hdc,&rc,
							pStatus->m_crBackColor1,pStatus->m_crBackColor2,
							DrawUtil::DIRECTION_VERT);
			}
			::SetBkColor(ps.hdc,crOldBkColor);
			::SetTextColor(ps.hdc,crOldTextColor);
			::SetBkMode(ps.hdc,OldBkMode);
			::SelectObject(ps.hdc,hfontOld);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		if (GetCapture()==hwnd) {
			CStatusView *pStatus=GetStatusView(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			pStatus->GetItemRect(pStatus->IndexToID(pStatus->m_HotItem),&rc);
			x-=rc.left;
			pStatus->m_ItemList[pStatus->m_HotItem]->OnMouseMove(x,y);
		} else {
			CStatusView *pStatus=GetStatusView(hwnd);

			if (pStatus->m_fSingleMode)
				break;

			int x=GET_X_LPARAM(lParam);
			int Left,Right;
			int i;

			Left=0;
			for (i=0;i<pStatus->m_NumItems;i++) {
				if (!pStatus->m_ItemList[i]->GetVisible())
					continue;
				Right=Left+STATUS_MARGIN*2+pStatus->m_ItemList[i]->GetWidth();
				if (x>=Left && x<Right)
					break;
				Left=Right;
			}
			if (i==pStatus->m_NumItems)
				i=-1;
			if (i!=pStatus->m_HotItem) {
				int OldHotItem;

				OldHotItem=pStatus->m_HotItem;
				pStatus->m_HotItem=i;
				if (OldHotItem>=0)
					pStatus->UpdateItem(pStatus->IndexToID(OldHotItem));
				if (pStatus->m_HotItem>=0)
					pStatus->UpdateItem(pStatus->IndexToID(pStatus->m_HotItem));
			}
			if (!pStatus->m_fTrackMouseEvent) {
				TRACKMOUSEEVENT tme;

				tme.cbSize=sizeof(TRACKMOUSEEVENT);
				tme.dwFlags=TME_LEAVE;
				tme.hwndTrack=hwnd;
				if (TrackMouseEvent(&tme))
					pStatus->m_fTrackMouseEvent=true;
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			pStatus->m_fTrackMouseEvent=false;
			if (!pStatus->m_fOnButtonDown) {
				if (pStatus->m_HotItem>=0) {
					int i=pStatus->m_HotItem;

					pStatus->m_HotItem=-1;
					pStatus->UpdateItem(pStatus->IndexToID(i));
				}
				if (pStatus->m_pEventHandler)
					pStatus->m_pEventHandler->OnMouseLeave();
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

			if (pStatus->m_HotItem>=0) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
				RECT rc;

				pStatus->GetItemRect(pStatus->IndexToID(pStatus->m_HotItem),&rc);
				x-=rc.left;
				pStatus->m_fOnButtonDown=true;
				if (uMsg==WM_LBUTTONDOWN)
					pStatus->m_ItemList[pStatus->m_HotItem]->OnLButtonDown(x,y);
				else
					pStatus->m_ItemList[pStatus->m_HotItem]->OnRButtonDown(x,y);
				pStatus->m_fOnButtonDown=false;
				if (!pStatus->m_fTrackMouseEvent) {
					POINT pt;

					::GetCursorPos(&pt);
					::GetWindowRect(hwnd,&rc);
					if (::PtInRect(&rc,pt)) {
						::ScreenToClient(hwnd,&pt);
						::SendMessage(hwnd,WM_MOUSEMOVE,0,MAKELPARAM(pt.x,pt.y));
					} else {
						int i=pStatus->m_HotItem;

						pStatus->m_HotItem=-1;
						pStatus->UpdateItem(pStatus->IndexToID(i));
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

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CStatusView *pStatus=GetStatusView(hwnd);

			if (pStatus->m_HotItem>=0) {
				SetCursor(LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_DESTROY:
		{
			CStatusView *pStatus=GetStatusView(hwnd);

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
			InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


bool CStatusView::GetItemRect(int ID,RECT *pRect) const
{
	int Index,i;
	RECT rc;

	Index=IDToIndex(ID);
	if (Index<0)
		return false;
	GetClientRect(&rc);
	for (i=0;i<Index;i++) {
		if (m_ItemList[i]->GetVisible())
			rc.left+=m_ItemList[i]->GetWidth()+STATUS_MARGIN*2;
	}
	rc.right=rc.left;
	if (m_ItemList[Index]->GetVisible())
		rc.right+=m_ItemList[i]->GetWidth()+STATUS_MARGIN*2;
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
	return rc.bottom-rc.top;
}


void CStatusView::SetVisible(bool fVisible)
{
	int i;

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
		m_HotItem=-1;
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


void CStatusView::SetColor(COLORREF crBack1,COLORREF crBack2,COLORREF crText,
	COLORREF crHighlightBack1,COLORREF crHighlightBack2,COLORREF crHighlightText)
{
	m_crBackColor1=crBack1;
	m_crBackColor2=crBack2;
	m_crTextColor=crText;
	m_crHighlightBackColor1=crHighlightBack1;
	m_crHighlightBackColor2=crHighlightBack2;
	m_crHighlightTextColor=crHighlightText;
	if (m_hwnd!=NULL)
		Invalidate();
}


bool CStatusView::SetFont(HFONT hfont)
{
	DeleteObject(m_hfontStatus);
	m_hfontStatus=hfont;
	if (m_hwnd!=NULL) {
		HDC hdc;
		HFONT hfontOld;
		TEXTMETRIC tm;
		RECT rc;

		hdc=GetDC(m_hwnd);
		hfontOld=SelectFont(hdc,m_hfontStatus);
		GetTextMetrics(hdc,&tm);
		SelectFont(hdc,hfontOld);
		ReleaseDC(m_hwnd,hdc);
		GetClientRect(&rc);
		rc.bottom=tm.tmHeight+STATUS_MARGIN*2;
		AdjustWindowRectEx(&rc,GetWindowStyle(m_hwnd),FALSE,GetWindowExStyle(m_hwnd));
		SetWindowPos(m_hwnd,NULL,0,0,rc.right-rc.left,rc.bottom-rc.top,
			SWP_NOZORDER | SWP_NOMOVE);
		InvalidateRect(m_hwnd,NULL,TRUE);
	}
	return true;
}


int CStatusView::GetCurItem() const
{
	if (m_HotItem<0)
		return -1;
	return m_ItemList[m_HotItem]->GetID();
}


bool CStatusView::SetEventHandler(CStatusViewEventHandler *pEventHandler)
{
	if (m_pEventHandler)
		m_pEventHandler->m_pStatusView=NULL;
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
	COLORREF crOldTextColor,crBkColor1,crBkColor2,crOldBkColor;

	hfontOld=SelectFont(hdc,m_hfontStatus);
	OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	crOldTextColor=::SetTextColor(hdc,fHighlight?m_crHighlightTextColor:m_crTextColor);
	crBkColor1=fHighlight?m_crHighlightBackColor1:m_crBackColor1;
	crBkColor2=fHighlight?m_crHighlightBackColor2:m_crBackColor2;
	crOldBkColor=::SetBkColor(hdc,MixColor(crBkColor1,crBkColor2,128));
	DrawUtil::FillGradient(hdc,pRect,crBkColor1,crBkColor2,DrawUtil::DIRECTION_VERT);
	pItem->DrawPreview(hdc,pRect);
	::SetBkColor(hdc,crOldBkColor);
	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	SelectFont(hdc,hfontOld);
	return true;
}


void CStatusView::OnTrace(LPCTSTR pszOutput)
{
	SetSingleText(pszOutput);
}
