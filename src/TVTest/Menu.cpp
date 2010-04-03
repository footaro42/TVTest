#include "stdafx.h"
#include "TVTest.h"
#include "Menu.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CMainMenu::CMainMenu()
	: m_hmenu(NULL)
	, m_fPopup(false)
{
}


CMainMenu::~CMainMenu()
{
	Destroy();
}


bool CMainMenu::Create(HINSTANCE hinst)
{
	if (m_hmenu)
		return false;
	m_hmenu=::LoadMenu(hinst,MAKEINTRESOURCE(IDM_MENU));
	if (m_hmenu==NULL)
		return false;
	return true;
}


void CMainMenu::Destroy()
{
	if (m_hmenu) {
		::DestroyMenu(m_hmenu);
		m_hmenu=NULL;
	}
}


bool CMainMenu::Popup(UINT Flags,int x,int y,HWND hwnd,bool fToggle)
{
	if (m_hmenu==NULL)
		return false;
	if (!m_fPopup || !fToggle || m_PopupMenu>=0) {
		if (m_fPopup)
			EndMenu();
		m_fPopup=true;
		m_PopupMenu=-1;
		::TrackPopupMenu(::GetSubMenu(m_hmenu,0),Flags,x,y,0,hwnd,NULL);
		m_fPopup=false;
	} else {
		::EndMenu();
	}
	return true;
}


bool CMainMenu::PopupSubMenu(int SubMenu,UINT Flags,int x,int y,HWND hwnd,bool fToggle)
{
	HMENU hmenu=GetSubMenu(SubMenu);

	if (hmenu==NULL)
		return false;
	if (!m_fPopup || !fToggle || m_PopupMenu!=SubMenu) {
		if (m_fPopup)
			EndMenu();
		m_fPopup=true;
		m_PopupMenu=SubMenu;
		::TrackPopupMenu(hmenu,Flags,x,y,0,hwnd,NULL);
		m_fPopup=false;
	} else {
		::EndMenu();
	}
	return true;
}


void CMainMenu::EnableItem(int ID,bool fEnable)
{
	if (m_hmenu)
		::EnableMenuItem(m_hmenu,ID,MF_BYCOMMAND | (fEnable?MFS_ENABLED:MFS_GRAYED));
}


void CMainMenu::CheckItem(int ID,bool fCheck)
{
	if (m_hmenu)
		::CheckMenuItem(m_hmenu,ID,MF_BYCOMMAND | (fCheck?MFS_CHECKED:MFS_UNCHECKED));
}


void CMainMenu::CheckRadioItem(int FirstID,int LastID,int CheckID)
{
	if (m_hmenu)
		::CheckMenuRadioItem(m_hmenu,FirstID,LastID,CheckID,MF_BYCOMMAND);
}


HMENU CMainMenu::GetSubMenu(int SubMenu)
{
	if (m_hmenu)
		return ::GetSubMenu(::GetSubMenu(m_hmenu,0),SubMenu);
	return NULL;
}


bool CMainMenu::SetAccelerator(CAccelerator *pAccelerator)
{
	if (m_hmenu==NULL)
		return false;
	pAccelerator->SetMenuAccel(m_hmenu);
	return true;
}




CChannelMenu::CChannelMenu(CEpgProgramList *pProgramList,CLogoManager *pLogoManager)
	: m_hmenu(NULL)
	, m_pProgramList(pProgramList)
	, m_pLogoManager(pLogoManager)
	, m_pChannelList(NULL)
	, m_hfont(NULL)
	, m_hfontCurrent(NULL)
	, m_TextHeight(0)
	, m_ChannelNameWidth(0)
	, m_EventNameWidth(0)
	, m_LogoWidth(24)
	, m_LogoHeight(14)
{
}


CChannelMenu::~CChannelMenu()
{
	Destroy();
	if (m_hfont!=NULL)
		::DeleteObject(m_hfont);
	if (m_hfontCurrent!=NULL)
		::DeleteObject(m_hfontCurrent);
}


bool CChannelMenu::Create(const CChannelList *pChannelList,int CurChannel,bool fUpdateProgramList)
{
	FILETIME ft;
	SYSTEMTIME st;
	int i;
	MENUITEMINFO mii;
	HDC hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);

	CreateFont(hdc);
	HFONT hfontOld=SelectFont(hdc,m_hfont);

	Destroy();
	m_pChannelList=pChannelList;
	m_CurChannel=CurChannel;
	::GetLocalTime(&st);
	::SystemTimeToFileTime(&st,&ft);
	ft+=120LL*FILETIME_SECOND;
	::FileTimeToSystemTime(&ft,&st);
	m_ChannelNameWidth=0;
	m_EventNameWidth=0;
	m_hmenu=::CreatePopupMenu();
	mii.cbSize=sizeof(MENUITEMINFO);
	mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
	mii.fType=MFT_OWNERDRAW;
	for (i=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);
		if (!pChInfo->IsEnabled())
			continue;

		TCHAR szText[256];
		int Length;
		SIZE sz;

		if (i==CurChannel)
			::SelectObject(hdc,m_hfontCurrent);
		Length=::wsprintf(szText,TEXT("%d: %s"),pChInfo->GetChannelNo(),pChInfo->GetName());
		::GetTextExtentPoint32(hdc,szText,Length,&sz);
		if (sz.cx>m_ChannelNameWidth)
			m_ChannelNameWidth=sz.cx;
		mii.wID=CM_CHANNEL_FIRST+i;
		mii.fState=MFS_ENABLED;
		if (i==CurChannel)
			mii.fState|=MFS_CHECKED;
		mii.dwItemData=reinterpret_cast<ULONG_PTR>((LPVOID)NULL);
		if (pChInfo->GetServiceID()!=0) {
			WORD TransportStreamID=pChInfo->GetTransportStreamID();
			WORD ServiceID=pChInfo->GetServiceID();
			bool fOK=false;
			CEventInfoData EventInfo;

			if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
				fOK=true;
			} else if (fUpdateProgramList) {
				if (m_pProgramList->UpdateProgramList(TransportStreamID,ServiceID)
						&& m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo))
					fOK=true;
			}
			if (fOK) {
				if (EventInfo.GetEventName()!=NULL) {
					SYSTEMTIME stStart,stEnd;

					EventInfo.GetStartTime(&stStart);
					EventInfo.GetEndTime(&stEnd);
					Length=::wnsprintf(szText,lengthof(szText),
								L"%d:%02d`%d:%02d %s",
								stStart.wHour,stStart.wMinute,
								stEnd.wHour,stEnd.wMinute,
								EventInfo.GetEventName());
					::GetTextExtentPoint32(hdc,szText,Length,&sz);
					if (sz.cx>m_EventNameWidth)
						m_EventNameWidth=sz.cx;
				}
				mii.dwItemData=reinterpret_cast<ULONG_PTR>(new CEventInfoData(EventInfo));
			}
		}
		::InsertMenuItem(m_hmenu,i,TRUE,&mii);
		if (i==CurChannel)
			::SelectObject(hdc,m_hfont);
	}
	::SelectObject(hdc,hfontOld);
	::DeleteDC(hdc);
	return true;
}


void CChannelMenu::Destroy()
{
	if (m_hmenu) {
		MENUITEMINFO mii;
		int i;

		mii.cbSize=sizeof(MENUITEMINFO);
		mii.fMask=MIIM_DATA;
		for (i=::GetMenuItemCount(m_hmenu)-1;i>=0;i--) {
			if (::GetMenuItemInfo(m_hmenu,i,TRUE,&mii))
				delete reinterpret_cast<CEventInfoData*>(mii.dwItemData);
		}
		::DestroyMenu(m_hmenu);
		m_hmenu=NULL;
	}
}


bool CChannelMenu::Popup(UINT Flags,int x,int y,HWND hwnd)
{
	if (m_hmenu==NULL)
		return false;
	::TrackPopupMenu(m_hmenu,Flags,x,y,0,hwnd,NULL);
	return true;
}


bool CChannelMenu::OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

	if (m_hmenu!=NULL && pmis->CtlType==ODT_MENU
			&& pmis->itemID>=CM_CHANNEL_FIRST && pmis->itemID<=CM_CHANNEL_LAST) {
		pmis->itemWidth=m_ChannelNameWidth+MENU_MARGIN*2+m_LogoWidth+MENU_LOGO_MARGIN;
		if (m_EventNameWidth>0)
			pmis->itemWidth+=m_TextHeight+m_EventNameWidth;
		pmis->itemHeight=m_TextHeight+MENU_MARGIN*2;
		return true;
	}
	return false;
}


bool CChannelMenu::OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	if (m_hmenu!=NULL && pdis->CtlType==ODT_MENU
			&& pdis->itemID>=CM_CHANNEL_FIRST && pdis->itemID<=CM_CHANNEL_LAST) {
		const CChannelInfo *pChInfo=m_pChannelList->GetChannelInfo(pdis->itemID-CM_CHANNEL_FIRST);
		CEventInfoData *pEventInfo=reinterpret_cast<CEventInfoData*>(pdis->itemData);
		HFONT hfontOld=SelectFont(pdis->hDC,(pdis->itemState&ODS_CHECKED)==0?m_hfont:m_hfontCurrent);
		int OldBkMode;
		COLORREF crOldTextColor;
		RECT rc;
		TCHAR szText[256];

		::FillRect(pdis->hDC,&pdis->rcItem,
			reinterpret_cast<HBRUSH>((pdis->itemState&ODS_SELECTED)!=0?
											COLOR_HIGHLIGHT+1:COLOR_MENU+1));
		OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
		crOldTextColor=::SetTextColor(pdis->hDC,::GetSysColor(
			(pdis->itemState&ODS_SELECTED)!=0?COLOR_HIGHLIGHTTEXT:COLOR_MENUTEXT));
		rc.left=pdis->rcItem.left+MENU_MARGIN;
		rc.top=pdis->rcItem.top+MENU_MARGIN;
		rc.bottom=rc.top+m_TextHeight;
		HBITMAP hbmLogo=m_pLogoManager->GetAssociatedLogoBitmap(
			pChInfo->GetNetworkID(),pChInfo->GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
		if (hbmLogo!=NULL) {
			HDC hdcMemory=::CreateCompatibleDC(pdis->hDC);
			HBITMAP hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,hbmLogo));
			int OldStretchMode=::SetStretchBltMode(pdis->hDC,STRETCH_HALFTONE);
			BITMAP bm;
			::GetObject(hbmLogo,sizeof(BITMAP),&bm);
			::StretchBlt(pdis->hDC,rc.left,rc.top+(rc.bottom-rc.top-m_LogoHeight)/2,
						 m_LogoWidth,m_LogoHeight,
						 hdcMemory,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
			::SetStretchBltMode(pdis->hDC,OldStretchMode);
			::SelectObject(hdcMemory,hbmOld);
			::DeleteDC(hdcMemory);
		}
		rc.left+=m_LogoWidth+MENU_LOGO_MARGIN;
		rc.right=rc.left+m_ChannelNameWidth;
		::wsprintf(szText,TEXT("%d: %s"),pChInfo->GetChannelNo(),pChInfo->GetName());
		::DrawText(pdis->hDC,szText,-1,&rc,
					DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		if (pEventInfo!=NULL && pEventInfo->GetEventName()!=NULL) {
			SYSTEMTIME stStart,stEnd;

			pEventInfo->GetStartTime(&stStart);
			pEventInfo->GetEndTime(&stEnd);
			::wnsprintf(szText,lengthof(szText),L"%d:%02d`%d:%02d %s",
				stStart.wHour,stStart.wMinute,stEnd.wHour,stEnd.wMinute,
				pEventInfo->GetEventName());
			rc.left=rc.right+m_TextHeight;
			rc.right=pdis->rcItem.right-MENU_MARGIN;
			::DrawText(pdis->hDC,szText,-1,&rc,
					DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
		::SetTextColor(pdis->hDC,crOldTextColor);
		::SetBkMode(pdis->hDC,OldBkMode);
		::SelectObject(pdis->hDC,hfontOld);
		return true;
	}
	return false;
}


void CChannelMenu::CreateFont(HDC hdc)
{
	if (m_hfont!=NULL)
		return;

	NONCLIENTMETRICS ncm;

#if WINVER<0x0600
	ncm.cbSize=sizeof(ncm);
#else
	ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
	m_hfont=::CreateFontIndirect(&ncm.lfMenuFont);
	ncm.lfMenuFont.lfWeight=FW_BOLD;
	m_hfontCurrent=::CreateFontIndirect(&ncm.lfMenuFont);

	if (hdc!=NULL) {
		HFONT hfontOld;
		TEXTMETRIC tm;

		hfontOld=static_cast<HFONT>(::SelectObject(hdc,m_hfont));
		::GetTextMetrics(hdc,&tm);
		m_TextHeight=tm.tmHeight;//+tm.tmInternalLeading;
		::SelectObject(hdc,hfontOld);
	} else {
		m_TextHeight=abs(ncm.lfMenuFont.lfHeight);
	}
	m_LogoHeight=min(m_TextHeight,14);
	m_LogoWidth=m_LogoHeight*16/9;
}




CPopupMenu::CPopupMenu()
	: m_hmenu(NULL)
{
}


CPopupMenu::CPopupMenu(HINSTANCE hinst,LPCTSTR pszName)
{
	m_hmenu=::LoadMenu(hinst,pszName);
}


CPopupMenu::CPopupMenu(HINSTANCE hinst,int ID)
{
	m_hmenu=::LoadMenu(hinst,MAKEINTRESOURCE(ID));
}


CPopupMenu::~CPopupMenu()
{
	if (m_hmenu!=NULL)
		::DestroyMenu(m_hmenu);
}


HMENU CPopupMenu::GetPopupHandle()
{
	if (m_hmenu==NULL)
		return NULL;
	return ::GetSubMenu(m_hmenu,0);
}


bool CPopupMenu::EnableItem(int ID,bool fEnable)
{
	if (m_hmenu==NULL)
		return false;
	return ::EnableMenuItem(m_hmenu,ID,MF_BYCOMMAND | (fEnable?MFS_ENABLED:MFS_GRAYED))>=0;
}


bool CPopupMenu::CheckItem(int ID,bool fCheck)
{
	if (m_hmenu==NULL)
		return false;
	return ::CheckMenuItem(m_hmenu,ID,MF_BYCOMMAND | (fCheck?MFS_CHECKED:MFS_UNCHECKED))>=0;
}


bool CPopupMenu::CheckRadioItem(int FirstID,int LastID,int CheckID)
{
	if (m_hmenu==NULL)
		return false;
	return ::CheckMenuRadioItem(m_hmenu,FirstID,LastID,CheckID,MF_BYCOMMAND)!=FALSE;
}


bool CPopupMenu::Popup(HWND hwnd,const POINT *pPos,UINT Flags)
{
	if (m_hmenu==NULL)
		return false;
	POINT pt;
	if (pPos!=NULL)
		pt=*pPos;
	else
		::GetCursorPos(&pt);
	::TrackPopupMenu(GetPopupHandle(),Flags,pt.x,pt.y,0,hwnd,NULL);
	return true;
}


bool CPopupMenu::Popup(HMENU hmenu,HWND hwnd,const POINT *pPos,UINT Flags,bool fToggle)
{
	if (m_hmenu==NULL) {
		m_hmenu=hmenu;
		POINT pt;
		if (pPos!=NULL)
			pt=*pPos;
		else
			::GetCursorPos(&pt);
		::TrackPopupMenu(m_hmenu,Flags,pt.x,pt.y,0,hwnd,NULL);
		m_hmenu=NULL;
	} else {
		if (fToggle)
			EndMenu();
	}
	return true;
}


bool CPopupMenu::Popup(HINSTANCE hinst,LPCTSTR pszName,HWND hwnd,const POINT *pPos,UINT Flags,bool fToggle)
{
	if (m_hmenu==NULL) {
		m_hmenu=::LoadMenu(hinst,pszName);
		if (m_hmenu==NULL)
			return false;
		POINT pt;
		if (pPos!=NULL)
			pt=*pPos;
		else
			::GetCursorPos(&pt);
		::TrackPopupMenu(GetPopupHandle(),Flags,pt.x,pt.y,0,hwnd,NULL);
		::DestroyMenu(m_hmenu);
		m_hmenu=NULL;
	} else {
		if (fToggle)
			EndMenu();
	}
	return true;
}




#define DROPDOWNMENU_WINDOW_CLASS APP_NAME TEXT(" Drop Down Menu")


HINSTANCE CDropDownMenu::m_hinst=NULL;


bool CDropDownMenu::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		OSVERSIONINFO osvi;
		WNDCLASS wc;

		osvi.dwOSVersionInfoSize=sizeof(osvi);
		if (::GetVersionEx(&osvi)
				&& (osvi.dwMajorVersion>5
					|| (osvi.dwMajorVersion==5 && osvi.dwMinorVersion>=1))
				&& osvi.dwPlatformId==VER_PLATFORM_WIN32_NT) {
			// XP or later
			wc.style=CS_DROPSHADOW;
		} else {
			wc.style=0;
		}
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=DROPDOWNMENU_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CDropDownMenu::CDropDownMenu()
	: m_hwnd(NULL)
	, m_hwndMessage(NULL)
	, m_TextColor(::GetSysColor(COLOR_MENUTEXT))
	, m_BackColor(::GetSysColor(COLOR_MENU))
	, m_HighlightTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT))
	, m_HighlightBackColor(::GetSysColor(COLOR_HIGHLIGHT))
	, m_BorderType(Theme::BORDER_RAISED)
{
	::SetRect(&m_ItemMargin,8,2,8,2);
	::SetRect(&m_WindowMargin,1,1,1,1);
	NONCLIENTMETRICS ncm;
#if WINVER<0x0600
	ncm.cbSize=sizeof(ncm);
#else
	ncm.cbSize=offsetof(NONCLIENTMETRICS,iPaddedBorderWidth);
#endif
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,ncm.cbSize,&ncm,0);
	m_hfont=::CreateFontIndirect(&ncm.lfMenuFont);
}


CDropDownMenu::~CDropDownMenu()
{
	Clear();
	if (m_hfont!=NULL)
		::DeleteObject(m_hfont);
}


void CDropDownMenu::Clear()
{
	m_ItemList.DeleteAll();
	m_ItemList.Clear();
}


bool CDropDownMenu::AppendItem(int Command,LPCTSTR pszText)
{
	CItem *pItem=new CItem(Command,pszText);

	if (!m_ItemList.Add(pItem)) {
		delete pItem;
		return false;
	}
	return true;
}


bool CDropDownMenu::InsertItem(int Index,int Command,LPCTSTR pszText)
{
	CItem *pItem=new CItem(Command,pszText);

	if (!m_ItemList.Insert(Index,pItem)) {
		delete pItem;
		return false;
	}
	return true;
}


bool CDropDownMenu::DeleteItem(int Command)
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	return m_ItemList.Delete(Index);
}


bool CDropDownMenu::SetItemText(int Command,LPCTSTR pszText)
{
	int Index=CommandToIndex(Command);

	if (Index<0)
		return false;
	return m_ItemList[Index]->SetText(pszText);
}


int CDropDownMenu::CommandToIndex(int Command) const
{
	if (Command<0)
		return -1;
	for (int i=0;i<m_ItemList.Length();i++) {
		if (m_ItemList[i]->GetCommand()==Command)
			return i;
	}
	return -1;
}


bool CDropDownMenu::Show(HWND hwndOwner,HWND hwndMessage,const POINT *pPos,int CurItem,UINT Flags)
{
	if (m_ItemList.IsEmpty() || m_hwnd!=NULL)
		return false;

	m_HotItem=CommandToIndex(CurItem);
	HWND hwnd=::CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TOPMOST,DROPDOWNMENU_WINDOW_CLASS,
							   NULL,WS_POPUP,0,0,0,0,hwndOwner,NULL,m_hinst,this);
	if (hwnd==NULL)
		return false;
	m_hwndMessage=hwndMessage;

	HDC hdc=::GetDC(hwnd);
	HFONT hfontOld=SelectFont(hdc,m_hfont);
	int MaxWidth=0;
	for (int i=0;i<m_ItemList.Length();i++) {
		int Width=m_ItemList[i]->GetWidth(hdc);
		if (Width>MaxWidth)
			MaxWidth=Width;
	}
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	m_ItemHeight=tm.tmHeight+//tm.tmInternalLeading+
		m_ItemMargin.top+m_ItemMargin.bottom;
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(hwnd,hdc);

	::MoveWindow(hwnd,pPos->x,pPos->y,
				 MaxWidth+m_ItemMargin.left+m_ItemMargin.right+m_WindowMargin.left+m_WindowMargin.right,
				 m_ItemHeight*m_ItemList.Length()+m_WindowMargin.top+m_WindowMargin.bottom,
				 FALSE);
	::ShowWindow(hwnd,SW_SHOWNA);

	return true;
}


bool CDropDownMenu::Hide()
{
	if (m_hwnd!=NULL)
		::DestroyWindow(m_hwnd);
	return true;
}


bool CDropDownMenu::GetPosition(RECT *pRect)
{
	if (m_hwnd==NULL)
		return false;
	return ::GetWindowRect(m_hwnd,pRect)!=FALSE;
}


bool CDropDownMenu::GetItemRect(int Index,RECT *pRect) const
{
	if (Index<0 || Index>=m_ItemList.Length())
		return false;
	::GetClientRect(m_hwnd,pRect);
	pRect->top=m_ItemHeight*Index;
	pRect->bottom=pRect->top+m_ItemHeight;
	::OffsetRect(pRect,m_WindowMargin.left,m_WindowMargin.top);
	return true;
}


int CDropDownMenu::HitTest(int x,int y) const
{
	POINT pt;

	pt.x=x;
	pt.y=y;
	for (int i=0;i<m_ItemList.Length();i++) {
		RECT rc;

		GetItemRect(i,&rc);
		if (::PtInRect(&rc,pt)) {
			if (m_ItemList[i]->IsSeparator())
				return -1;
			return i;
		}
	}
	return -1;
}


void CDropDownMenu::UpdateItem(int Index) const
{
	RECT rc;
	if (GetItemRect(Index,&rc))
		::InvalidateRect(m_hwnd,&rc,TRUE);
}


void CDropDownMenu::Draw(HDC hdc,const RECT *pPaintRect)
{
	HBRUSH hbrBack=::CreateSolidBrush(m_BackColor);
FillRect(hdc,pPaintRect,hbrBack);

	HFONT hfontOld=SelectFont(hdc,m_hfont);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF OldTextColor=::GetTextColor(hdc);
	RECT rc;

	for (int i=0;i<m_ItemList.Length();i++) {
		GetItemRect(i,&rc);
		if (rc.bottom>pPaintRect->top && rc.top<pPaintRect->bottom) {
			if (i!=m_HotItem) {
				::FillRect(hdc,&rc,hbrBack);
				::SetTextColor(hdc,m_TextColor);
			} else {
				HBRUSH hbr=::CreateSolidBrush(m_HighlightBackColor);
				::FillRect(hdc,&rc,hbr);
				::DeleteObject(hbr);
				::SetTextColor(hdc,m_HighlightTextColor);
			}
			rc.left+=m_ItemMargin.left;
			rc.right-=m_ItemMargin.right;
			m_ItemList[i]->Draw(hdc,&rc);
		}
	}
	::SetTextColor(hdc,OldTextColor);
	::SetBkMode(hdc,OldBkMode);
	::SelectObject(hdc,hfontOld);
	::DeleteObject(hbrBack);
	::GetClientRect(m_hwnd,&rc);
	Theme::DrawBorder(hdc,&rc,m_BorderType);
}


CDropDownMenu *CDropDownMenu::GetThis(HWND hwnd)
{
	return reinterpret_cast<CDropDownMenu*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CDropDownMenu::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CDropDownMenu *pThis=static_cast<CDropDownMenu*>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams);

			pThis->m_hwnd=hwnd;
			pThis->m_fTrackMouseEvent=false;
			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		}
		return 0;

	case WM_PAINT:
		{
			CDropDownMenu *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			pThis->Draw(ps.hdc,&ps.rcPaint);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CDropDownMenu *pThis=GetThis(hwnd);
			int Item=pThis->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));

			if (Item!=pThis->m_HotItem) {
				int OldHotItem=pThis->m_HotItem;

				pThis->m_HotItem=Item;
				if (OldHotItem>=0)
					pThis->UpdateItem(OldHotItem);
				if (Item>=0)
					pThis->UpdateItem(Item);
			}
			if (!pThis->m_fTrackMouseEvent) {
				TRACKMOUSEEVENT tme;

				tme.cbSize=sizeof(tme);
				tme.dwFlags=TME_LEAVE;
				tme.hwndTrack=hwnd;
				if (::TrackMouseEvent(&tme))
					pThis->m_fTrackMouseEvent=true;
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CDropDownMenu *pThis=GetThis(hwnd);

			if (pThis->m_HotItem>=0) {
				::SendMessage(pThis->m_hwndMessage,WM_COMMAND,pThis->m_ItemList[pThis->m_HotItem]->GetCommand(),0);
				::DestroyWindow(hwnd);
			}
		}
		return 0;

	case WM_MOUSELEAVE:
		::DestroyWindow(hwnd);
		return 0;

	case WM_MOUSEACTIVATE:
		return MA_NOACTIVATE;

	case WM_DESTROY:
		{
			CDropDownMenu *pThis=GetThis(hwnd);

			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CDropDownMenu::CItem::CItem(int Command,LPCTSTR pszText)
	: m_Command(Command)
	, m_pszText(DuplicateString(pszText))
	, m_Width(0)
{
}


CDropDownMenu::CItem::~CItem()
{
	delete [] m_pszText;
}


bool CDropDownMenu::CItem::SetText(LPCTSTR pszText)
{
	return ReplaceString(&m_pszText,pszText);
}


int CDropDownMenu::CItem::GetWidth(HDC hdc)
{
	if (m_pszText!=NULL && m_Width==0) {
		SIZE sz;

		::GetTextExtentPoint32(hdc,m_pszText,::lstrlen(m_pszText),&sz);
		m_Width=sz.cx;
	}
	return m_Width;
}


void CDropDownMenu::CItem::Draw(HDC hdc,const RECT *pRect)
{
	if (m_pszText!=NULL) {
		RECT rc=*pRect;

		::DrawText(hdc,m_pszText,-1,&rc,DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
	} else if (m_Command<0) {
		RECT rc=*pRect;

		rc.top=(rc.top+rc.bottom)/2-1;
		rc.bottom=rc.top+2;
		::DrawEdge(hdc,&rc,BDR_SUNKENOUTER,BF_RECT);
	}
}
