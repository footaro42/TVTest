#include "stdafx.h"
#include "TVTest.h"
#include "SideBarOptions.h"
#include "AppMain.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




static const CSideBar::SideBarItem ItemList[] = {
	{CM_ZOOM_20,				0},
	{CM_ZOOM_25,				1},
	{CM_ZOOM_33,				2},
	{CM_ZOOM_50,				3},
	{CM_ZOOM_66,				4},
	{CM_ZOOM_75,				5},
	{CM_ZOOM_100,				6},
	{CM_ZOOM_150,				7},
	{CM_ZOOM_200,				8},
	{CM_ASPECTRATIO_DEFAULT,	9},
	{CM_ASPECTRATIO_16x9,		10},
	{CM_ASPECTRATIO_LETTERBOX,	11},
	{CM_ASPECTRATIO_SUPERFRAME,	12},
	{CM_ASPECTRATIO_SIDECUT,	13},
	{CM_ASPECTRATIO_4x3,		14},
	{CM_FULLSCREEN,				15},
	{CM_ALWAYSONTOP,			16},
	{CM_DISABLEVIEWER,			17},
	{CM_CAPTURE,				18},
	{CM_SAVEIMAGE,				19},
	{CM_COPY,					20},
	{CM_CAPTUREPREVIEW,			21},
	{CM_RESET,					22},
	{CM_RESETVIEWER,			23},
	{CM_INFORMATION,			24},
	{CM_PROGRAMGUIDE,			25},
	{CM_STATUSBAR,				26},
	{CM_VIDEODECODERPROPERTY,	27},
	{CM_OPTIONS,				28},
	{CM_STREAMINFO,				29},
	{CM_CHANNELDISPLAYMENU,		30},
	{CM_CHANNELNO_1,			31},
	{CM_CHANNELNO_2,			32},
	{CM_CHANNELNO_3,			33},
	{CM_CHANNELNO_4,			34},
	{CM_CHANNELNO_5,			35},
	{CM_CHANNELNO_6,			36},
	{CM_CHANNELNO_7,			37},
	{CM_CHANNELNO_8,			38},
	{CM_CHANNELNO_9,			39},
	{CM_CHANNELNO_10,			40},
	{CM_CHANNELNO_11,			41},
	{CM_CHANNELNO_12,			42},
};

static const int DefaultItemList[] = {
#ifndef TVH264_FOR_1SEG
	CM_ZOOM_25,
	CM_ZOOM_33,
#endif
	CM_ZOOM_50,
	CM_ZOOM_100,
#ifdef TVH264_FOR_1SEG
	CM_ZOOM_150,
	CM_ZOOM_200,
#endif
	0,
	CM_FULLSCREEN,
	CM_ALWAYSONTOP,
	CM_DISABLEVIEWER,
	0,
	CM_INFORMATION,
	CM_PROGRAMGUIDE,
	CM_CHANNELDISPLAYMENU,
	CM_OPTIONS,
};


CSideBarOptions::CSideBarOptions(CSideBar *pSideBar)
	: m_pSideBar(pSideBar)
	, m_fShowPopup(true)
	, m_fShowToolTips(true)
	, m_Place(PLACE_LEFT)
	, m_himlIcons(NULL)
{
	m_ItemList.resize(lengthof(DefaultItemList));
	for (int i=0;i<lengthof(DefaultItemList);i++)
		m_ItemList[i]=DefaultItemList[i];
}


CSideBarOptions::~CSideBarOptions()
{
}


bool CSideBarOptions::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("SideBar"),CSettings::OPEN_READ)) {
		int Value,NumItems;

		Settings.Read(TEXT("ShowPopup"),&m_fShowPopup);
		Settings.Read(TEXT("ShowToolTips"),&m_fShowToolTips);
		if (Settings.Read(TEXT("Place"),&Value)
				&& Value>=PLACE_FIRST && Value<=PLACE_LAST)
			m_Place=(PlaceType)Value;
		if (Settings.Read(TEXT("ItemCount"),&NumItems) && NumItems>0) {
			m_ItemList.clear();
			for (int i=0;i<NumItems;i++) {
				TCHAR szName[32],szCommand[CCommandList::MAX_COMMAND_TEXT];

				::wsprintf(szName,TEXT("Item%d"),i);
				if (Settings.Read(szName,szCommand,lengthof(szCommand))) {
					if (szCommand[0]=='\0') {
						m_ItemList.push_back(ITEM_SEPARATOR);
					} else {
						int Command=m_pSideBar->GetCommandList()->ParseText(szCommand);

						if (Command!=0) {
							for (int j=0;j<lengthof(ItemList);j++) {
								if (ItemList[j].Command==Command) {
									m_ItemList.push_back(Command);
									break;
								}
							}
						}
					}
				}
			}
		}
		Settings.Close();
	}
	return true;
}


bool CSideBarOptions::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (!Settings.Open(pszFileName,TEXT("SideBar"),CSettings::OPEN_WRITE))
		return false;

	Settings.Clear();
	Settings.Write(TEXT("ShowPopup"),m_fShowPopup);
	Settings.Write(TEXT("ShowToolTips"),m_fShowToolTips);
	Settings.Write(TEXT("Place"),(int)m_Place);
	Settings.Write(TEXT("ItemCount"),(int)m_ItemList.size());
	const CCommandList *pCommandList=m_pSideBar->GetCommandList();
	for (int i=0;i<(int)m_ItemList.size();i++) {
		TCHAR szName[32];

		::wsprintf(szName,TEXT("Item%d"),i);
		if (m_ItemList[i]==0)
			Settings.Write(szName,TEXT(""));
		else
			Settings.Write(szName,pCommandList->GetCommandText(pCommandList->IDToIndex(m_ItemList[i])));
	}
	Settings.Close();
	return true;
}


bool CSideBarOptions::ApplySideBarOptions()
{
	m_pSideBar->SetIconImage((HBITMAP)::LoadImage(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDB_SIDEBAR),IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION),RGB(255,255,255));
	ApplyItemList();
	m_pSideBar->ShowToolTips(m_fShowToolTips);
	m_pSideBar->SetVertical(m_Place==PLACE_LEFT || m_Place==PLACE_RIGHT);
	return true;
}


void CSideBarOptions::ApplyItemList() const
{
	m_pSideBar->DeleteAllItems();
	for (size_t i=0;i<m_ItemList.size();i++) {
		if (m_ItemList[i]==0) {
			CSideBar::SideBarItem Item;

			Item.Command=CSideBar::ITEM_SEPARATOR;
			Item.Icon=-1;
			Item.Flags=0;
			m_pSideBar->AddItem(&Item);
		} else {
			int j;

			for (j=0;j<lengthof(ItemList);j++) {
				if (ItemList[j].Command==m_ItemList[i])
					break;
			}
			m_pSideBar->AddItem(&ItemList[j]);
		}
	}
	m_pSideBar->Invalidate();
}


bool CSideBarOptions::SetPlace(PlaceType Place)
{
	if (Place<PLACE_FIRST || Place>PLACE_LAST)
		return false;
	m_Place=Place;
	return true;
}


CSideBarOptions *CSideBarOptions::GetThis(HWND hDlg)
{
	return static_cast<CSideBarOptions*>(GetOptions(hDlg));
}


BOOL CALLBACK CSideBarOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSideBarOptions *pThis=static_cast<CSideBarOptions*>(OnInitDialog(hDlg,lParam));

			DlgCheckBox_Check(hDlg,IDC_SIDEBAR_SHOW,pThis->m_fShowPopup);
			DlgCheckBox_Check(hDlg,IDC_SIDEBAR_SHOWTOOLTIPS,pThis->m_fShowToolTips);

			pThis->m_himlIcons=::ImageList_LoadBitmap(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDB_SIDEBAR),16,1,RGB(255,255,255));

			HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			ListView_SetImageList(hwndList,pThis->m_himlIcons,LVSIL_SMALL);
			RECT rc;
			::GetClientRect(hwndList,&rc);
			rc.right-=::GetSystemMetrics(SM_CXVSCROLL);
			LVCOLUMN lvc;
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=rc.right;
			lvc.iSubItem=0;
			ListView_InsertColumn(hwndList,0,&lvc);
			pThis->SetItemList(hwndList,&pThis->m_ItemList[0],pThis->m_ItemList.size());
			hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_COMMANDLIST);
			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			ListView_SetImageList(hwndList,pThis->m_himlIcons,LVSIL_SMALL);
			::GetClientRect(hwndList,&rc);
			rc.right-=::GetSystemMetrics(SM_CXVSCROLL);
			lvc.cx=rc.right;
			ListView_InsertColumn(hwndList,0,&lvc);
			int List[lengthof(ItemList)];
			for (int i=0;i<lengthof(ItemList);i++)
				List[i]=ItemList[i].Command;
			pThis->SetItemList(hwndList,List,lengthof(List));
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_SIDEBAR_UP:
		case IDC_SIDEBAR_DOWN:
			{
				bool fUp=LOWORD(wParam)==IDC_SIDEBAR_UP;
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if ((fUp && Sel>0) || (!fUp && Sel<ListView_GetItemCount(hwndList)-1)) {
					LVITEM lvi;
					TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

					lvi.mask=LVIF_STATE | LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					lvi.stateMask=(UINT)-1;
					lvi.pszText=szText;
					lvi.cchTextMax=lengthof(szText);
					ListView_GetItem(hwndList,&lvi);
					ListView_DeleteItem(hwndList,Sel);
					if (fUp)
						lvi.iItem--;
					else
						lvi.iItem++;
					ListView_InsertItem(hwndList,&lvi);
				}
			}
			return TRUE;

		case IDC_SIDEBAR_REMOVE:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0)
					ListView_DeleteItem(hwndList,Sel);
			}
			return TRUE;

		case IDC_SIDEBAR_DEFAULT:
			{
				CSideBarOptions *pThis=GetThis(hDlg);

				pThis->SetItemList(::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST),
								   DefaultItemList,lengthof(DefaultItemList));
			}
			return TRUE;

		case IDC_SIDEBAR_ADD:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_COMMANDLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);

				if (Sel>=0) {
					LVITEM lvi;
					TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

					lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
					lvi.iItem=Sel;
					lvi.iSubItem=0;
					lvi.pszText=szText;
					lvi.cchTextMax=lengthof(szText);
					ListView_GetItem(hwndList,&lvi);
					hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
					Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
					lvi.iItem=Sel>=0?Sel:ListView_GetItemCount(hwndList);
					lvi.state=LVIS_SELECTED | LVIS_FOCUSED;
					lvi.stateMask=LVIS_SELECTED | LVIS_FOCUSED;
					ListView_InsertItem(hwndList,&lvi);
				}
			}
			return TRUE;

		case IDC_SIDEBAR_SEPARATOR:
			{
				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				int Sel=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				LVITEM lvi;

				lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
				lvi.iItem=Sel>=0?Sel:ListView_GetItemCount(hwndList);
				lvi.iSubItem=0;
				lvi.pszText=TEXT("(‹æØ‚è)");
				lvi.iImage=-1;
				lvi.lParam=0;
				ListView_InsertItem(hwndList,&lvi);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_ITEMCHANGED:
			{
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);
				int Sel=ListView_GetNextItem(pnmlv->hdr.hwndFrom,-1,LVNI_SELECTED);

				if (pnmlv->hdr.idFrom==IDC_SIDEBAR_ITEMLIST) {
					EnableDlgItem(hDlg,IDC_SIDEBAR_UP,Sel>0);
					EnableDlgItem(hDlg,IDC_SIDEBAR_DOWN,Sel>=0 && Sel<ListView_GetItemCount(pnmlv->hdr.hwndFrom)-1);
					EnableDlgItem(hDlg,IDC_SIDEBAR_REMOVE,Sel>=0);
				} else {
					EnableDlgItem(hDlg,IDC_SIDEBAR_ADD,Sel>=0);
				}
			}
			break;

		case NM_RCLICK:
			{
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->hdr.idFrom==IDC_SIDEBAR_ITEMLIST) {
					if (pnmia->iItem>=0) {
						static const int MenuIDs[] = {
							IDC_SIDEBAR_UP,
							IDC_SIDEBAR_DOWN,
							0,
							IDC_SIDEBAR_REMOVE,
						};
						PopupMenuFromControls(hDlg,MenuIDs,lengthof(MenuIDs),TPM_RIGHTBUTTON);
					}
				} else {
					if (pnmia->iItem>=0) {
						static const int MenuIDs[] = {
							IDC_SIDEBAR_ADD,
						};
						PopupMenuFromControls(hDlg,MenuIDs,lengthof(MenuIDs),TPM_RIGHTBUTTON);
					}
				}
			}
			break;

		case PSN_APPLY:
			{
				CSideBarOptions *pThis=GetThis(hDlg);

				pThis->m_fShowPopup=DlgCheckBox_IsChecked(hDlg,IDC_SIDEBAR_SHOW);
				pThis->m_fShowToolTips=DlgCheckBox_IsChecked(hDlg,IDC_SIDEBAR_SHOWTOOLTIPS);
				pThis->m_pSideBar->ShowToolTips(pThis->m_fShowToolTips);

				HWND hwndList=::GetDlgItem(hDlg,IDC_SIDEBAR_ITEMLIST);
				int i,Count;
				LVITEM lvi;

				pThis->m_ItemList.clear();
				Count=ListView_GetItemCount(hwndList);
				lvi.mask=LVIF_PARAM;
				lvi.iSubItem=0;
				for (i=0;i<Count;i++) {
					lvi.iItem=i;
					ListView_GetItem(hwndList,&lvi);
					pThis->m_ItemList.push_back(lvi.lParam);
				}
				pThis->ApplyItemList();
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CSideBarOptions *pThis=GetThis(hDlg);

			if (pThis->m_himlIcons!=NULL) {
				::ImageList_Destroy(pThis->m_himlIcons);
				pThis->m_himlIcons=NULL;
			}
			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


void CSideBarOptions::SetItemList(HWND hwndList,const int *pList,int NumItems)
{
	const CCommandList *pCommandList=m_pSideBar->GetCommandList();
	LVITEM lvi;
	TCHAR szText[CCommandList::MAX_COMMAND_TEXT];

	ListView_DeleteAllItems(hwndList);
	lvi.mask=LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
	lvi.iSubItem=0;
	lvi.pszText=szText;
	for (int i=0;i<NumItems;i++) {
		if (pList[i]==0) {
			::lstrcpy(szText,TEXT("(‹æØ‚è)"));
			lvi.iImage=-1;
		} else {
			pCommandList->GetCommandName(pCommandList->IDToIndex(pList[i]),szText,lengthof(szText));
			for (int j=0;j<lengthof(ItemList);j++) {
				if (ItemList[j].Command==pList[i]) {
					lvi.iImage=ItemList[j].Icon;
					break;
				}
			}
		}
		lvi.iItem=i;
		lvi.lParam=pList[i];
		ListView_InsertItem(hwndList,&lvi);
	}
}
