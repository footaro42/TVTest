#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "Menu.h"
#include "resource.h"




CMainMenu::CMainMenu()
{
	m_hmenu=NULL;
	m_fPopup=false;
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
	if (!m_fPopup || !fToggle) {
		m_fPopup=true;
		::TrackPopupMenu(::GetSubMenu(m_hmenu,0),Flags,x,y,0,hwnd,NULL);
		m_fPopup=false;
	} else {
		::EndMenu();
	}
	return true;
}


bool CMainMenu::PopupSubMenu(int SubMenu,UINT Flags,int x,int y,HWND hwnd)
{
	HMENU hmenu=GetSubMenu(SubMenu);

	if (hmenu==NULL)
		return false;
	::TrackPopupMenu(hmenu,Flags,x,y,0,hwnd,NULL);
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




CChannelMenu::CChannelMenu(CEpgProgramList *pProgramList)
{
	m_hmenu=NULL;
	m_pProgramList=pProgramList;
	m_pChannelList=NULL;
	NONCLIENTMETRICS ncm;
	ncm.cbSize=sizeof(NONCLIENTMETRICS);
	::SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&ncm,0);
	m_hfont=::CreateFontIndirect(&ncm.lfMenuFont);
	m_TextHeight=abs(ncm.lfMenuFont.lfHeight);
}


CChannelMenu::~CChannelMenu()
{
	Destroy();
	::DeleteObject(m_hfont);
}


bool CChannelMenu::Create(const CChannelList *pChannelList)
{
	FILETIME ft;
	SYSTEMTIME st;
	int i;
	MENUITEMINFO mii;
	HDC hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
	HFONT hfontOld=SelectFont(hdc,m_hfont);
	SIZE sz;

	Destroy();
	m_pChannelList=pChannelList;
	::GetLocalTime(&st);
	::SystemTimeToFileTime(&st,&ft);
	ft+=(LONGLONG)120*FILETIME_SECOND;
	::FileTimeToSystemTime(&ft,&st);
	m_ChannelNameWidth=0;
	m_EventNameWidth=0;
	m_hmenu=::CreatePopupMenu();
	mii.cbSize=sizeof(MENUITEMINFO);
	mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
	mii.fType=MFT_OWNERDRAW;
	mii.fState=MFS_ENABLED;
	for (i=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);
		if (!pChInfo->IsEnabled())
			continue;

		TCHAR szText[256];
		int Length;

		Length=::wsprintf(szText,TEXT("%d: %s"),pChInfo->GetChannelNo(),pChInfo->GetName());
		::GetTextExtentPoint32(hdc,szText,Length,&sz);
		if (sz.cx>m_ChannelNameWidth)
			m_ChannelNameWidth=sz.cx;
		mii.wID=CM_CHANNEL_FIRST+i;
		mii.dwItemData=reinterpret_cast<ULONG_PTR>((LPVOID)NULL);
		if (pChInfo->GetServiceID()!=0) {
			WORD TransportStreamID=pChInfo->GetTransportStreamID();
			WORD ServiceID=pChInfo->GetServiceID();
			bool fOK=false;
			CEventInfoData EventInfo;

			if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
				fOK=true;
			} else {
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
		pmis->itemWidth=m_ChannelNameWidth+MENU_MARGIN*2;
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
		HFONT hfontOld=SelectFont(pdis->hDC,m_hfont);
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
		rc.right=rc.left+m_ChannelNameWidth;
		rc.top=pdis->rcItem.top+MENU_MARGIN;
		rc.bottom=rc.top+m_TextHeight;
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
