#include "stdafx.h"
#include "IconMenu.h"




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


bool CIconMenu::Initialize(HMENU hmenu,HINSTANCE hinst,LPCTSTR pszImageName,int IconWidth,COLORREF crMask)
{
	Finalize();
	m_hImageList=::ImageList_LoadBitmap(hinst,pszImageName,IconWidth,1,crMask);
	if (m_hImageList==NULL)
		return false;
	m_hmenu=hmenu;
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


bool CIconMenu::OnInitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
	HMENU hmenu=reinterpret_cast<HMENU>(wParam);
	if (hmenu!=m_hmenu)
		return false;

	MENUITEMINFO mii;
	mii.cbSize=sizeof(mii);
	mii.fMask=MIIM_BITMAP | MIIM_DATA;
	mii.hbmpItem=HBMMENU_CALLBACK;
	int Count=::GetMenuItemCount(hmenu);
	for (int i=0;i<Count;i++) {
		mii.dwItemData=i;
		::SetMenuItemInfo(hmenu,i,TRUE,&mii);
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
	int x=pdis->rcItem.left+ICON_MARGIN,y=pdis->rcItem.top+((pdis->rcItem.bottom-pdis->rcItem.top)-cy)/2;

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
