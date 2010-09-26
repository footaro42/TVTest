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
			::EndMenu();
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
			::EndMenu();
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


HMENU CMainMenu::GetMenuHandle() const
{
	if (m_hmenu)
		return ::GetSubMenu(m_hmenu,0);
	return NULL;
}


HMENU CMainMenu::GetSubMenu(int SubMenu) const
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




class CChannelMenuItem
{
	const CChannelInfo *m_pChannelInfo;
	struct Event {
		bool fValid;
		CEventInfoData EventInfo;
		Event() : fValid(false) {}
	};
	Event m_EventList[2];
public:
	CChannelMenuItem(const CChannelInfo *pChannelInfo)
		: m_pChannelInfo(pChannelInfo) {}
	const CEventInfoData *GetEventInfo(CEpgProgramList *pProgramList,
									   int Index,const SYSTEMTIME *pCurTime=NULL);
	const CEventInfoData *GetEventInfo(int Index) const;
	const CChannelInfo *GetChannelInfo() const { return m_pChannelInfo; }
};

const CEventInfoData *CChannelMenuItem::GetEventInfo(CEpgProgramList *pProgramList,
													 int Index,const SYSTEMTIME *pCurTime)
{
	if (Index<0 || Index>=lengthof(m_EventList)
			|| (Index>0 && !m_EventList[Index-1].fValid)
			|| m_pChannelInfo->GetServiceID()==0)
		return NULL;
	if (!m_EventList[Index].fValid) {
		SYSTEMTIME st;

		if (Index==0) {
			if (pCurTime!=NULL)
				st=*pCurTime;
			else
				::GetLocalTime(&st);
		} else {
			if (!m_EventList[Index-1].EventInfo.GetEndTime(&st))
				return NULL;
		}
		if (!pProgramList->GetEventInfo(m_pChannelInfo->GetTransportStreamID(),
										m_pChannelInfo->GetServiceID(),
										&st,&m_EventList[Index].EventInfo))
			return NULL;
		m_EventList[Index].fValid=true;
	}
	return &m_EventList[Index].EventInfo;
}

const CEventInfoData *CChannelMenuItem::GetEventInfo(int Index) const
{
	if (!m_EventList[Index].fValid)
		return NULL;
	return &m_EventList[Index].EventInfo;
}


CChannelMenu::CChannelMenu(CEpgProgramList *pProgramList,CLogoManager *pLogoManager)
	: m_Flags(0)
	, m_hwnd(NULL)
	, m_hmenu(NULL)
	, m_pProgramList(pProgramList)
	, m_pLogoManager(pLogoManager)
	, m_pChannelList(NULL)
	, m_TextHeight(0)
	, m_ChannelNameWidth(0)
	, m_EventNameWidth(0)
	, m_LogoWidth(24)
	, m_LogoHeight(14)
	, m_MenuLogoMargin(3)
{
}


CChannelMenu::~CChannelMenu()
{
	Destroy();
}


bool CChannelMenu::Create(const CChannelList *pChannelList,int CurChannel,UINT Command,
						  HMENU hmenu,HWND hwnd,unsigned int Flags,int MaxRows)
{
	Destroy();

	m_pChannelList=pChannelList;
	m_CurChannel=CurChannel;
	m_FirstCommand=Command;
	m_LastCommand=Command+pChannelList->NumChannels()-1;
	m_Flags=Flags;
	m_hwnd=hwnd;

	m_Margins.cxLeftWidth=2;
	m_Margins.cxRightWidth=2;
	m_Margins.cyTopHeight=2;
	m_Margins.cyBottomHeight=2;
	if (m_UxTheme.Initialize() && m_UxTheme.IsActive()
			&& m_UxTheme.Open(hwnd,VSCLASS_MENU)) {
		m_UxTheme.GetMargins(MENU_POPUPITEM,0,TMT_CONTENTMARGINS,&m_Margins);
		if (m_Margins.cxLeftWidth<2)
			m_Margins.cxLeftWidth=2;
		if (m_Margins.cxRightWidth<2)
			m_Margins.cxRightWidth=2;
	}

	HDC hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);

	CreateFont(hdc);
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);

	SYSTEMTIME st;
	GetBaseTime(&st);
	m_ChannelNameWidth=0;
	m_EventNameWidth=0;
	if (hmenu==NULL) {
		m_hmenu=::CreatePopupMenu();
	} else {
		m_hmenu=hmenu;
		m_Flags|=FLAG_SHARED;
		ClearMenu(hmenu);
	}
	MENUITEMINFO mii;
	mii.cbSize=sizeof(MENUITEMINFO);
	mii.fMask=MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA;
	int PrevSpace=-1;
	for (int i=0,j=0;i<pChannelList->NumChannels();i++) {
		const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);
		if (!pChInfo->IsEnabled())
			continue;

		TCHAR szText[256];
		int Length;
		SIZE sz;

		if (i==CurChannel)
			DrawUtil::SelectObject(hdc,m_FontCurrent);
		Length=::wsprintf(szText,TEXT("%d: %s"),pChInfo->GetChannelNo(),pChInfo->GetName());
		::GetTextExtentPoint32(hdc,szText,Length,&sz);
		if (sz.cx>m_ChannelNameWidth)
			m_ChannelNameWidth=sz.cx;
		mii.wID=m_FirstCommand+i;
		mii.fType=MFT_OWNERDRAW;
		if ((MaxRows>0 && j==MaxRows)
				|| ((Flags&FLAG_SPACEBREAK)!=0 && pChInfo->GetSpace()!=PrevSpace)) {
			mii.fType|=MFT_MENUBREAK;
			j=0;
		}
		mii.fState=MFS_ENABLED;
		if (i==CurChannel)
			mii.fState|=MFS_CHECKED;
		CChannelMenuItem *pItem=new CChannelMenuItem(pChInfo);
		mii.dwItemData=reinterpret_cast<ULONG_PTR>(pItem);
		if ((Flags&FLAG_SHOWEVENTINFO)!=0) {
			const CEventInfoData *pEventInfo=pItem->GetEventInfo(m_pProgramList,0,&st);

			if (pEventInfo!=NULL) {
				Length=GetEventText(pEventInfo,szText,lengthof(szText));
				::GetTextExtentPoint32(hdc,szText,Length,&sz);
				if (sz.cx>m_EventNameWidth)
					m_EventNameWidth=sz.cx;
			}
		}
		::InsertMenuItem(m_hmenu,i,TRUE,&mii);
		if (i==CurChannel)
			DrawUtil::SelectObject(hdc,m_Font);
		PrevSpace=pChInfo->GetSpace();
		j++;
	}
	::SelectObject(hdc,hfontOld);
	::DeleteDC(hdc);

	if ((Flags&FLAG_SHOWTOOLTIP)!=0) {
		m_Tooltip.Create(hwnd);
		m_Tooltip.SetMaxWidth(480);
		m_Tooltip.SetPopDelay(30*1000);
		m_Tooltip.AddTrackingTip(1,TEXT(""));
	}
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
				delete reinterpret_cast<CChannelMenuItem*>(mii.dwItemData);
		}
		if ((m_Flags&FLAG_SHARED)==0)
			::DestroyMenu(m_hmenu);
		else
			ClearMenu(m_hmenu);
		m_hmenu=NULL;
	}
	m_UxTheme.Close();
	m_Tooltip.Destroy();
	m_hwnd=NULL;
}


bool CChannelMenu::Popup(UINT Flags,int x,int y)
{
	if (m_hmenu==NULL)
		return false;
	::TrackPopupMenu(m_hmenu,Flags,x,y,0,m_hwnd,NULL);
	return true;
}


bool CChannelMenu::OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

	if (m_hmenu!=NULL && pmis->CtlType==ODT_MENU
			&& pmis->itemID>=m_FirstCommand && pmis->itemID<=m_LastCommand) {
		pmis->itemWidth=m_ChannelNameWidth+
						m_Margins.cxLeftWidth+m_Margins.cxRightWidth;
		if ((m_Flags&FLAG_SHOWLOGO)!=0)
			pmis->itemWidth+=m_LogoWidth+m_MenuLogoMargin;
		if (m_EventNameWidth>0)
			pmis->itemWidth+=m_TextHeight+m_EventNameWidth;
		pmis->itemHeight=max(m_TextHeight,m_LogoHeight)+
							m_Margins.cyTopHeight+m_Margins.cyBottomHeight;
		return true;
	}
	return false;
}


bool CChannelMenu::OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	if (m_hmenu==NULL || hwnd!=m_hwnd || pdis->CtlType!=ODT_MENU
			|| pdis->itemID<m_FirstCommand || pdis->itemID>m_LastCommand)
		return false;

	const CChannelMenuItem *pItem=reinterpret_cast<CChannelMenuItem*>(pdis->itemData);
	const CChannelInfo *pChInfo=pItem->GetChannelInfo();
	const bool fSelected=(pdis->itemState&ODS_SELECTED)!=0;
	COLORREF crTextColor,crOldTextColor;
	RECT rc;
	TCHAR szText[256];

	if (m_UxTheme.IsOpen()) {
		const int StateID=fSelected?MPI_HOT:MPI_NORMAL;

		m_UxTheme.DrawBackground(pdis->hDC,MENU_POPUPITEM,StateID,
								 MENU_POPUPBACKGROUND,0,&pdis->rcItem);
		m_UxTheme.GetColor(MENU_POPUPITEM,StateID,TMT_TEXTCOLOR,&crTextColor);
	} else {
		::FillRect(pdis->hDC,&pdis->rcItem,
			reinterpret_cast<HBRUSH>(fSelected?COLOR_HIGHLIGHT+1:COLOR_MENU+1));
		crTextColor=::GetSysColor(fSelected?COLOR_HIGHLIGHTTEXT:COLOR_MENUTEXT);
	}

	HFONT hfontOld=DrawUtil::SelectObject(pdis->hDC,
						(pdis->itemState&ODS_CHECKED)==0?m_Font:m_FontCurrent);
	int OldBkMode=::SetBkMode(pdis->hDC,TRANSPARENT);
	crOldTextColor=::SetTextColor(pdis->hDC,crTextColor);
	rc.left=pdis->rcItem.left+m_Margins.cxLeftWidth;
	rc.top=pdis->rcItem.top+m_Margins.cyTopHeight;
	rc.bottom=pdis->rcItem.bottom-m_Margins.cyBottomHeight;

	if ((m_Flags&FLAG_SHOWLOGO)!=0) {
		HBITMAP hbmLogo=m_pLogoManager->GetAssociatedLogoBitmap(
			pChInfo->GetNetworkID(),pChInfo->GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
		if (hbmLogo!=NULL) {
			DrawUtil::DrawBitmap(pdis->hDC,
								 rc.left,rc.top+(rc.bottom-rc.top-m_LogoHeight)/2,
								 m_LogoWidth,m_LogoHeight,hbmLogo);
		}
		rc.left+=m_LogoWidth+m_MenuLogoMargin;
	}

	rc.right=rc.left+m_ChannelNameWidth;
	::wsprintf(szText,TEXT("%d: %s"),pChInfo->GetChannelNo(),pChInfo->GetName());
	::DrawText(pdis->hDC,szText,-1,&rc,
			   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	if ((m_Flags&FLAG_SHOWEVENTINFO)!=0) {
		const CEventInfoData *pEventInfo=pItem->GetEventInfo(0);
		if (pEventInfo!=NULL) {
			int Length=GetEventText(pEventInfo,szText,lengthof(szText));
			rc.left=rc.right+m_TextHeight;
			rc.right=pdis->rcItem.right-m_Margins.cxRightWidth;
			::DrawText(pdis->hDC,szText,Length,&rc,
					   DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
	}

	::SetTextColor(pdis->hDC,crOldTextColor);
	::SetBkMode(pdis->hDC,OldBkMode);
	::SelectObject(pdis->hDC,hfontOld);
	return true;
}


bool CChannelMenu::OnMenuSelect(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	HMENU hmenu=reinterpret_cast<HMENU>(lParam);
	UINT Command=LOWORD(wParam);

	if (hmenu==NULL || hmenu!=m_hmenu || hwnd!=m_hwnd || HIWORD(wParam)==0xFFFF
			|| Command<m_FirstCommand || Command>m_LastCommand) {
		if (m_Tooltip.IsVisible())
			m_Tooltip.TrackActivate(1,false);
		return false;
	}

	if ((m_Flags&FLAG_SHOWTOOLTIP)!=0) {
		MENUITEMINFO mii;

		mii.cbSize=sizeof(mii);
		mii.fMask=MIIM_DATA;
		if (::GetMenuItemInfo(hmenu,Command,FALSE,&mii)) {
			CChannelMenuItem *pItem=reinterpret_cast<CChannelMenuItem*>(mii.dwItemData);
			if (pItem==NULL)
				return false;

			const CEventInfoData *pEventInfo1,*pEventInfo2;
			pEventInfo1=pItem->GetEventInfo(0);
			if (pEventInfo1==NULL) {
				SYSTEMTIME st;

				::GetLocalTime(&st);
				pEventInfo1=pItem->GetEventInfo(m_pProgramList,0,&st);
			}
			if (pEventInfo1!=NULL) {
				TCHAR szText[256*2+1];
				int Length;
				POINT pt;

				Length=GetEventText(pEventInfo1,szText,lengthof(szText)/2);
				pEventInfo2=pItem->GetEventInfo(m_pProgramList,1);
				if (pEventInfo2!=NULL) {
					szText[Length++]='\r';
					szText[Length++]='\n';
					GetEventText(pEventInfo2,szText+Length,lengthof(szText)/2);
				}
				m_Tooltip.SetText(1,szText);
				::GetCursorPos(&pt);
				pt.x+=16;
				pt.y+=max(m_TextHeight,m_LogoHeight)+
							m_Margins.cyTopHeight+m_Margins.cyBottomHeight;
				m_Tooltip.TrackPosition(pt.x,pt.y);
				m_Tooltip.TrackActivate(1,true);
			} else {
				m_Tooltip.TrackActivate(1,false);
			}
		}
	}
	return true;
}


bool CChannelMenu::OnUninitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	if (hwnd==m_hwnd && reinterpret_cast<HMENU>(wParam)==m_hmenu) {
		Destroy();
		return true;
	}
	return false;
}


int CChannelMenu::GetEventText(const CEventInfoData *pEventInfo,
							   LPTSTR pszText,int MaxLength) const
{
	SYSTEMTIME stStart,stEnd;
	TCHAR szEnd[16];
	int Length;

	pEventInfo->GetStartTime(&stStart);
	if (pEventInfo->GetEndTime(&stEnd))
		::wsprintf(szEnd,TEXT("%02d:%02d"),stEnd.wHour,stEnd.wMinute);
	else
		szEnd[0]='\0';
	Length=::wnsprintf(pszText,MaxLength-1,TEXT("%02d:%02d`%s %ls"),
					   stStart.wHour,stStart.wMinute,szEnd,
					   NullToEmptyString(pEventInfo->GetEventName()));
	pszText[Length]='\0';
	return Length;
}


void CChannelMenu::CreateFont(HDC hdc)
{
	if (m_Font.IsCreated())
		return;

	LOGFONT lf;
	if (!m_UxTheme.IsActive()
			|| !m_UxTheme.GetFont(MENU_POPUPITEM,0,TMT_GLYPHFONT,&lf))
		DrawUtil::GetSystemFont(DrawUtil::FONT_MENU,&lf);
	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_FontCurrent.Create(&lf);

	if (hdc!=NULL)
		m_TextHeight=m_Font.GetHeight(hdc);
	else
		m_TextHeight=abs(lf.lfHeight);
	m_LogoHeight=min(m_TextHeight,14);
	m_LogoWidth=m_LogoHeight*16/9;
}


void CChannelMenu::GetBaseTime(SYSTEMTIME *pTime)
{
	FILETIME ft;

	::GetLocalTime(pTime);
	::SystemTimeToFileTime(pTime,&ft);
	ft+=120LL*FILETIME_SECOND;
	::FileTimeToSystemTime(&ft,pTime);
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
			::EndMenu();
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
			::EndMenu();
	}
	return true;
}




CIconMenu::CIconMenu()
	: m_hmenu(NULL)
	, m_hImageList(NULL)
	, m_CheckItem(0)
{
}


CIconMenu::~CIconMenu()
{
	Finalize();
}


bool CIconMenu::Initialize(HMENU hmenu,HINSTANCE hinst,LPCTSTR pszImageName,
						   int IconWidth,COLORREF crMask,
						   UINT FirstID,UINT LastID)
{
	Finalize();
	m_hImageList=::ImageList_LoadBitmap(hinst,pszImageName,IconWidth,1,crMask);
	if (m_hImageList==NULL)
		return false;
	m_hmenu=hmenu;
	m_FirstID=FirstID;
	m_LastID=LastID;
	return true;
}


void CIconMenu::Finalize()
{
	m_hmenu=NULL;
	if (m_hImageList!=NULL) {
		::ImageList_Destroy(m_hImageList);
		m_hImageList=NULL;
	}
	m_CheckItem=0;
}


bool CIconMenu::OnInitMenuPopup(HWND hwnd,HMENU hmenu)
{
	if (hmenu!=m_hmenu)
		return false;

	MENUITEMINFO mii;
	mii.cbSize=sizeof(mii);
	int Count=::GetMenuItemCount(hmenu);
	for (int i=0;i<Count;i++) {
		mii.fMask=MIIM_ID;
		::GetMenuItemInfo(hmenu,i,TRUE,&mii);
		if (mii.wID>=m_FirstID && mii.wID<=m_LastID) {
			mii.fMask=MIIM_BITMAP | MIIM_DATA;
			mii.hbmpItem=HBMMENU_CALLBACK;
			mii.dwItemData=i;
			::SetMenuItemInfo(hmenu,i,TRUE,&mii);
		}
	}

	MENUINFO mi;
	mi.cbSize=sizeof(mi);
	mi.fMask=MIM_STYLE;
	::GetMenuInfo(hmenu,&mi);
	if ((mi.dwStyle&MNS_CHECKORBMP)==0) {
		mi.dwStyle|=MNS_CHECKORBMP;
		::SetMenuInfo(hmenu,&mi);
	}

	return true;
}


bool CIconMenu::OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPMEASUREITEMSTRUCT pmis=reinterpret_cast<LPMEASUREITEMSTRUCT>(lParam);

	if (pmis->CtlType!=ODT_MENU)
		return false;

	int cx,cy;
	::ImageList_GetIconSize(m_hImageList,&cx,&cy);
	pmis->itemHeight=max(pmis->itemHeight+3,(UINT)cy+ICON_MARGIN*2);
	pmis->itemWidth=cx+ICON_MARGIN*2+TEXT_MARGIN;

	return true;
}


bool CIconMenu::OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	LPDRAWITEMSTRUCT pdis=reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	if (pdis->CtlType!=ODT_MENU || (HMENU)pdis->hwndItem!=m_hmenu)
		return false;

	int cx,cy;
	::ImageList_GetIconSize(m_hImageList,&cx,&cy);
	int x=pdis->rcItem.left+ICON_MARGIN;
	int y=pdis->rcItem.top+((pdis->rcItem.bottom-pdis->rcItem.top)-cy)/2;

	if (pdis->itemID==m_CheckItem) {
		RECT rc;
		COLORREF cr;
		int r,g,b;
		HPEN hpen1,hpen2,hpenOld;

		::SetRect(&rc,x,y,x+cx,y+cy);
		::FillRect(pdis->hDC,&rc,reinterpret_cast<HBRUSH>(COLOR_MENU+1));
		cr=::GetSysColor(COLOR_MENU);
		r=GetRValue(cr);
		g=GetGValue(cr);
		b=GetBValue(cr);
		hpen1=::CreatePen(PS_SOLID,1,RGB(r/2,g/2,b/2));
		hpen2=::CreatePen(PS_SOLID,1,RGB(r+(255-r)/2,g+(255-g)/2,b+(255-b)/2));
		hpenOld=static_cast<HPEN>(::SelectObject(pdis->hDC,hpen1));
		::MoveToEx(pdis->hDC,x+cx+ICON_MARGIN,y-ICON_MARGIN,NULL);
		::LineTo(pdis->hDC,x-ICON_MARGIN,y-ICON_MARGIN);
		::LineTo(pdis->hDC,x-ICON_MARGIN,y+cy+ICON_MARGIN-1);
		::SelectObject(pdis->hDC,hpen2);
		::LineTo(pdis->hDC,x+cx+ICON_MARGIN-1,y+cy+ICON_MARGIN-1);
		::LineTo(pdis->hDC,x+cx+ICON_MARGIN-1,y-ICON_MARGIN);
		::SelectObject(pdis->hDC,hpenOld);
		::DeleteObject(hpen1);
		::DeleteObject(hpen2);
	}

	::ImageList_Draw(m_hImageList,(int)pdis->itemData,pdis->hDC,x,y,ILD_NORMAL);

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
	, m_Font(DrawUtil::FONT_MENU)
	, m_TextColor(::GetSysColor(COLOR_MENUTEXT))
	, m_BackColor(::GetSysColor(COLOR_MENU))
	, m_HighlightTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT))
	, m_HighlightBackColor(::GetSysColor(COLOR_HIGHLIGHT))
	, m_BorderType(Theme::BORDER_RAISED)
{
	::SetRect(&m_ItemMargin,8,2,8,2);
	::SetRect(&m_WindowMargin,1,1,1,1);
}


CDropDownMenu::~CDropDownMenu()
{
	Clear();
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
	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	int MaxWidth=0;
	for (int i=0;i<m_ItemList.Length();i++) {
		int Width=m_ItemList[i]->GetWidth(hdc);
		if (Width>MaxWidth)
			MaxWidth=Width;
	}
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	m_ItemHeight=tm.tmHeight/*-tm.tmInternalLeading*/+
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
	if (m_hwnd!=NULL) {
		RECT rc;
		if (GetItemRect(Index,&rc))
			::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


void CDropDownMenu::Draw(HDC hdc,const RECT *pPaintRect)
{
	HBRUSH hbrBack=::CreateSolidBrush(m_BackColor);
	FillRect(hdc,pPaintRect,hbrBack);

	HFONT hfontOld=DrawUtil::SelectObject(hdc,m_Font);
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	COLORREF OldTextColor=::GetTextColor(hdc);
	RECT rc;

	for (int i=0;i<m_ItemList.Length();i++) {
		GetItemRect(i,&rc);
		if (rc.bottom>pPaintRect->top && rc.top<pPaintRect->bottom) {
			if (i!=m_HotItem) {
				//::FillRect(hdc,&rc,hbrBack);
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
	Theme::DrawBorder(hdc,rc,m_BorderType);
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
				::DestroyWindow(hwnd);
				::SendMessage(pThis->m_hwndMessage,WM_COMMAND,
							  pThis->m_ItemList[pThis->m_HotItem]->GetCommand(),0);
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
