#include "stdafx.h"
#include "TVTest.h"
#include "ControlPanel.h"
#include "resource.h"


#define CONTROL_PANEL_WINDOW_CLASS APP_NAME TEXT(" Control Panel")




HINSTANCE CControlPanel::m_hinst=NULL;


bool CControlPanel::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=CONTROL_PANEL_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CControlPanel::CControlPanel()
{
	m_NumItems=0;
	LOGFONT lf;
	GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	m_hfont=CreateFontIndirect(&lf);
	m_FontHeight=abs(lf.lfHeight);
	m_crBackColor=RGB(0,0,0);
	m_crTextColor=RGB(255,255,255);
	m_crOverBackColor=RGB(255,255,255);
	m_crOverTextColor=RGB(0,0,0);
	m_hwndMessage=NULL;
	m_HotItem=-1;
}


CControlPanel::~CControlPanel()
{
	int i;

	for (i=0;i<m_NumItems;i++)
		delete m_pItemList[i];
	DeleteObject(m_hfont);
}


bool CControlPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CONTROL_PANEL_WINDOW_CLASS,TEXT("‘€ì"),m_hinst);
}


bool CControlPanel::AddItem(CControlPanelItem *pItem)
{
	if (m_NumItems==MAX_ITEMS)
		return false;
	m_pItemList[m_NumItems++]=pItem;
	pItem->m_pControlPanel=this;
	return true;
}


bool CControlPanel::UpdateItem(int Index)
{
	RECT rc;

	if (Index<0 || Index>=m_NumItems || m_hwnd==NULL)
		return false;
	m_pItemList[Index]->GetPosition(&rc);
	InvalidateRect(m_hwnd,&rc,TRUE);
	return true;
}


bool CControlPanel::GetItemPosition(int Index,RECT *pRect) const
{
	if (Index<0 || Index>=m_NumItems)
		return false;
	m_pItemList[Index]->GetPosition(pRect);
	return true;
}


void CControlPanel::SetColors(COLORREF crBack,COLORREF crText,COLORREF crOverBack,COLORREF crOverText)
{
	m_crBackColor=crBack;
	m_crTextColor=crText;
	m_crOverBackColor=crOverBack;
	m_crOverTextColor=crOverText;
	if (m_hwnd!=NULL)
		InvalidateRect(m_hwnd,NULL,TRUE);
}


void CControlPanel::SetSendMessageWindow(HWND hwnd)
{
	m_hwndMessage=hwnd;
}


void CControlPanel::SendCommand(int Command)
{
	if (m_hwndMessage!=NULL)
		::SendMessage(m_hwndMessage,WM_COMMAND,Command,(LPARAM)GetHandle());
}


bool CControlPanel::CheckRadioItem(int FirstID,int LastID,int CheckID)
{
	int i;

	for (i=0;i<m_NumItems;i++) {
		if (m_pItemList[i]->m_Command>=FirstID
				&& m_pItemList[i]->m_Command<=LastID)
			m_pItemList[i]->m_fCheck=m_pItemList[i]->m_Command==CheckID;
	}
	if (m_hwnd!=NULL)
		InvalidateRect(m_hwnd,NULL,TRUE);
	return true;
}


CControlPanel *CControlPanel::GetThis(HWND hwnd)
{
	return reinterpret_cast<CControlPanel*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CControlPanel::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,
																LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CControlPanel *pThis=dynamic_cast<CControlPanel*>(OnCreate(hwnd,lParam));

			pThis->m_HotItem=-1;
			pThis->m_fTrackMouseEvent=false;
		}
		return 0;

	case WM_SIZE:
		{
			CControlPanel *pThis=GetThis(hwnd);
			int Width=LOWORD(lParam),Height=HIWORD(lParam);
			int i;
			bool fRayoutChanged;

			fRayoutChanged=false;
			for (i=0;i<pThis->m_NumItems;i++) {
				if (pThis->m_pItemList[i]->Rayout(Width,Height))
					fRayoutChanged=true;
			}
			if (fRayoutChanged)
				InvalidateRect(hwnd,NULL,TRUE);
		}
		return 0;

	case WM_PAINT:
		{
			CControlPanel *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			HBRUSH hbrBack;
			HFONT hfontOld;
			COLORREF crOldTextColor;
			int OldBkMode;
			RECT rc;

			BeginPaint(hwnd,&ps);
			hbrBack=CreateSolidBrush(pThis->m_crBackColor);
			FillRect(ps.hdc,&ps.rcPaint,hbrBack);
			DeleteObject(hbrBack);
			hfontOld=SelectFont(ps.hdc,pThis->m_hfont);
			crOldTextColor=GetTextColor(ps.hdc);
			OldBkMode=SetBkMode(ps.hdc,TRANSPARENT);
			for (int i=0;i<pThis->m_NumItems;i++) {
				pThis->m_pItemList[i]->GetPosition(&rc);
				if (rc.left<ps.rcPaint.right && rc.right>ps.rcPaint.left
						&& rc.top<ps.rcPaint.bottom && rc.bottom>ps.rcPaint.top) {
					COLORREF crText,crBack;

					if (i==pThis->m_HotItem) {
						crText=pThis->m_crOverTextColor;
						crBack=pThis->m_crOverBackColor;
					} else {
						crText=pThis->m_crTextColor;
						crBack=pThis->m_crBackColor;
						if (!pThis->m_pItemList[i]->GetEnable())
							crText=RGB((GetRValue(crText)+GetRValue(crBack))/2,
									   (GetGValue(crText)+GetGValue(crBack))/2,
									   (GetBValue(crText)+GetBValue(crBack))/2);
						if (pThis->m_pItemList[i]->GetCheck())
							crBack=RGB((GetRValue(crText)+GetRValue(crBack))/2,
									   (GetGValue(crText)+GetGValue(crBack))/2,
									   (GetBValue(crText)+GetBValue(crBack))/2);
					}
					SetTextColor(ps.hdc,crText);
					SetBkColor(ps.hdc,crBack);
					hbrBack=CreateSolidBrush(crBack);
					FillRect(ps.hdc,&rc,hbrBack);
					DeleteObject(hbrBack);
					pThis->m_pItemList[i]->Draw(ps.hdc);
				}
			}
			SetBkMode(ps.hdc,OldBkMode);
			SetTextColor(ps.hdc,crOldTextColor);
			SelectFont(ps.hdc,hfontOld);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CControlPanel *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			RECT rc;

			if (GetCapture()==hwnd) {
				pThis->m_pItemList[pThis->m_HotItem]->GetPosition(&rc);
				x-=rc.left;
				y-=rc.top;
				pThis->m_pItemList[pThis->m_HotItem]->OnMouseMove(x,y);
			} else {
				int i;
				POINT pt;

				pt.x=x;
				pt.y=y;
				for (i=pThis->m_NumItems-1;i>=0;i--) {
					if (pThis->m_pItemList[i]->GetVisible()
							&& pThis->m_pItemList[i]->GetEnable()) {
						pThis->m_pItemList[i]->GetPosition(&rc);
						if (PtInRect(&rc,pt))
							break;
					}
				}
				if (i!=pThis->m_HotItem) {
					int OldHotItem;

					OldHotItem=pThis->m_HotItem;
					pThis->m_HotItem=i;
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
					if (TrackMouseEvent(&tme))
						pThis->m_fTrackMouseEvent=true;
				}
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		{
			CControlPanel *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				int i=pThis->m_HotItem;

				pThis->m_HotItem=-1;
				pThis->UpdateItem(i);
			}
			pThis->m_fTrackMouseEvent=false;
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		{
			CControlPanel *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (pThis->m_HotItem>=0) {
				RECT rc;

				pThis->m_pItemList[pThis->m_HotItem]->GetPosition(&rc);
				x-=rc.left;
				y-=rc.top;
				if (uMsg==WM_LBUTTONDOWN)
					pThis->m_pItemList[pThis->m_HotItem]->OnLButtonDown(x,y);
				else
					pThis->m_pItemList[pThis->m_HotItem]->OnRButtonDown(x,y);
			} else if (uMsg==WM_RBUTTONDOWN) {
				POINT pt;

				pt.x=x;
				pt.y=y;
				MapWindowPoints(hwnd,pThis->GetParent(),&pt,1);
				::SendMessage(pThis->GetParent(),uMsg,wParam,MAKELPARAM(pt.x,pt.y));
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
			CControlPanel *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				SetCursor(LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_DESTROY:
		{
			CControlPanel *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CControlPanelItem::CControlPanelItem()
{
	m_Position.Left=0;
	m_Position.Top=0;
	m_Position.Width=0;
	m_Position.Height=0;
	m_Command=0;
	m_fVisible=true;
	m_fEnable=true;
	m_fCheck=false;
	m_pControlPanel=NULL;
}


CControlPanelItem::~CControlPanelItem()
{
}


void CControlPanelItem::GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const
{
	if (pLeft)
		*pLeft=m_Position.Left;
	if (pTop)
		*pTop=m_Position.Top;
	if (pWidth)
		*pWidth=m_Position.Width;
	if (pHeight)
		*pHeight=m_Position.Height;
}


bool CControlPanelItem::SetPosition(int Left,int Top,int Width,int Height)
{
	if (Width<0 || Height<0)
		return false;
	m_Position.Left=Left;
	m_Position.Top=Top;
	m_Position.Width=Width;
	m_Position.Height=Height;
	return true;
}


void CControlPanelItem::GetPosition(RECT *pRect) const
{
	pRect->left=m_Position.Left;
	pRect->top=m_Position.Top;
	pRect->right=m_Position.Left+m_Position.Width;
	pRect->bottom=m_Position.Top+m_Position.Height;
}


void CControlPanelItem::SetVisible(bool fVisible)
{
	m_fVisible=fVisible;
}


void CControlPanelItem::SetEnable(bool fEnable)
{
	m_fEnable=fEnable;
}


void CControlPanelItem::SetCheck(bool fCheck)
{
	m_fCheck=fCheck;
}


void CControlPanelItem::OnLButtonDown(int x,int y)
{
	m_pControlPanel->SendCommand(m_Command);
}




CControlPanelButton::CControlPanelButton(int Command,LPCTSTR pszText,
										int Left,int Top,int Width,int Height)
{
	m_Command=Command;
	m_pszText=DuplicateString(pszText);
	m_Position.Left=Left;
	m_Position.Top=Top;
	m_Position.Width=Width;
	m_Position.Height=Height;
}


CControlPanelButton::~CControlPanelButton()
{
	delete [] m_pszText;
}


void CControlPanelButton::Draw(HDC hdc)
{
	RECT rc;

	GetPosition(&rc);
	DrawText(hdc,m_pszText,-1,&rc,
						DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
}
