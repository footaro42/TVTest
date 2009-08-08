#include "stdafx.h"
#include <commctrl.h>
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "ChannelScan.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define SCAN_INTERVAL	5		// スキャン時のチャンネル切り替え間隔(秒)
#define RETRY_COUNT		4
#define RETRY_INTERVAL	1000

#define WM_APP_BEGINSCAN	WM_APP
#define WM_APP_CHANNELFOUND	(WM_APP+1)
#define WM_APP_ENDSCAN		(WM_APP+2)




class CChannelListSort {
	static int CALLBACK CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort);
public:
	enum SortType {
		SORT_NAME,
		SORT_CHANNEL,
		//SORT_SERVICE,
		SORT_CHANNELNAME,
		SORT_SERVICEID,
		SORT_REMOTECONTROLKEYID
	};
	CChannelListSort();
	CChannelListSort(SortType Type,bool fDescending=false);
	void Sort(HWND hwndList);
	bool UpdateChannelList(HWND hwndList,CChannelList *pList);
	SortType m_Type;
	bool m_fDescending;
};


CChannelListSort::CChannelListSort()
{
	m_Type=SORT_CHANNEL;
	m_fDescending=false;
}


CChannelListSort::CChannelListSort(SortType Type,bool fDescending)
{
	m_Type=Type;
	m_fDescending=fDescending;
}


int CALLBACK CChannelListSort::CompareFunc(LPARAM lParam1,LPARAM lParam2,LPARAM lParamSort)
{
	CChannelListSort *pThis=reinterpret_cast<CChannelListSort*>(lParamSort);
	CChannelInfo *pChInfo1=reinterpret_cast<CChannelInfo*>(lParam1);
	CChannelInfo *pChInfo2=reinterpret_cast<CChannelInfo*>(lParam2);
	int Cmp;

	switch (pThis->m_Type) {
	case SORT_NAME:
		Cmp=::lstrcmpi(pChInfo1->GetName(),pChInfo2->GetName());
		if (Cmp==0)
			Cmp=::lstrcmp(pChInfo1->GetName(),pChInfo2->GetName());
		break;
	case SORT_CHANNEL:
		Cmp=pChInfo1->GetChannelIndex()-pChInfo2->GetChannelIndex();
		break;
	/*
	case SORT_SERVICE:
		Cmp=pChInfo1->GetChannelIndex()-pChInfo2->GetChannelIndex();
		if (Cmp==0)
			Cmp=pChInfo1->GetService()-pChInfo2->GetService();
		break;
	*/
	case SORT_CHANNELNAME:
		Cmp=pChInfo1->GetChannelIndex()-pChInfo2->GetChannelIndex();
		break;
	case SORT_SERVICEID:
		Cmp=pChInfo1->GetServiceID()-pChInfo2->GetServiceID();
		break;
	case SORT_REMOTECONTROLKEYID:
		Cmp=pChInfo1->GetChannelNo()-pChInfo2->GetChannelNo();
		break;
	default:
		Cmp=0;
	}
	return pThis->m_fDescending?-Cmp:Cmp;
}


void CChannelListSort::Sort(HWND hwndList)
{
	ListView_SortItems(hwndList,CompareFunc,reinterpret_cast<LPARAM>(this));
}


bool CChannelListSort::UpdateChannelList(HWND hwndList,CChannelList *pList)
{
	int Count=ListView_GetItemCount(hwndList);
	LV_ITEM lvi;
	CChannelList ChannelList;
	int i;

	lvi.mask=LVIF_PARAM;
	lvi.iSubItem=0;
	for (i=0;i<Count;i++) {
		lvi.iItem=i;
		ListView_GetItem(hwndList,&lvi);
		ChannelList.AddChannel(*reinterpret_cast<CChannelInfo*>(lvi.lParam));
	}
	*pList=ChannelList;
	for (i=0;i<Count;i++) {
		lvi.iItem=i;
		lvi.lParam=reinterpret_cast<LPARAM>(pList->GetChannelInfo(i));
		ListView_SetItem(hwndList,&lvi);
	}
	return true;
}




CChannelScan::CChannelScan(CCoreEngine *pCoreEngine)
{
	m_pCoreEngine=pCoreEngine;
	m_pOriginalTuningSpaceList=NULL;
	m_fIgnoreSignalLevel=false;
	m_hScanDlg=NULL;
	m_fChanging=false;
}


CChannelScan::~CChannelScan()
{
}


bool CChannelScan::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();

	if ((Flags&UPDATE_CHANNELLIST)!=0) {
		AppMain.UpdateChannelList(&m_TuningSpaceList);
	}

	if ((Flags&UPDATE_PREVIEW)!=0) {
		if (m_fRestorePreview)
			AppMain.GetMainWindow()->EnablePreview(true);
	}

	return true;
}


bool CChannelScan::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("ChannelScanIgnoreSignalLevel"),&m_fIgnoreSignalLevel);
	return true;
}


bool CChannelScan::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("ChannelScanIgnoreSignalLevel"),m_fIgnoreSignalLevel);
	return true;
}


bool CChannelScan::SetTuningSpaceList(const CTuningSpaceList *pTuningSpaceList)
{
	m_pOriginalTuningSpaceList=pTuningSpaceList;
	return true;
}


void CChannelScan::InsertChannelInfo(int Index,const CChannelInfo *pChInfo)
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST);
	LV_ITEM lvi;
	TCHAR szText[16];

	lvi.mask=LVIF_TEXT | LVIF_PARAM;
	lvi.iItem=Index;
	lvi.iSubItem=0;
	lvi.pszText=const_cast<LPTSTR>(pChInfo->GetName());
	lvi.lParam=reinterpret_cast<LPARAM>(pChInfo);
	lvi.iItem=ListView_InsertItem(hwndList,&lvi);
	lvi.mask=LVIF_TEXT;
	lvi.iSubItem=1;
	::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelIndex());
	lvi.pszText=szText;
	ListView_SetItem(hwndList,&lvi);
	lvi.iSubItem=2;
	//::wsprintf(szText,TEXT("%d"),pChInfo->GetService());
	LPCTSTR pszChannelName=m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetChannelName(pChInfo->GetSpace(),pChInfo->GetChannelIndex());
	::lstrcpy(szText,pszChannelName!=NULL?pszChannelName:TEXT("???"));
	ListView_SetItem(hwndList,&lvi);
	lvi.iSubItem=3;
	::wsprintf(szText,TEXT("%d"),pChInfo->GetServiceID());
	ListView_SetItem(hwndList,&lvi);
	if (pChInfo->GetChannelNo()>0) {
		lvi.iSubItem=4;
		::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelNo());
		lvi.pszText=szText;
		ListView_SetItem(hwndList,&lvi);
	}
	ListView_SetCheckState(hwndList,lvi.iItem,pChInfo->IsEnabled());
}


void CChannelScan::SetChannelList(int Space)
{
	const CChannelList *pChannelList=m_TuningSpaceList.GetChannelList(Space);

	ListView_DeleteAllItems(::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST));
	if (pChannelList==NULL)
		return;
	m_fChanging=true;
	for (int i=0;i<pChannelList->NumChannels();i++)
		InsertChannelInfo(i,pChannelList->GetChannelInfo(i));
	m_fChanging=false;
}


CChannelInfo *CChannelScan::GetSelectedChannelInfo() const
{
	HWND hwndList=::GetDlgItem(m_hDlg,IDC_CHANNELSCAN_CHANNELLIST);
	LV_ITEM lvi;

	lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
	if (lvi.iItem>=0) {
		lvi.mask=LVIF_PARAM;
		lvi.iSubItem=0;
		if (ListView_GetItem(hwndList,&lvi))
			return reinterpret_cast<CChannelInfo*>(lvi.lParam);
	}
	return NULL;
}


CChannelScan *CChannelScan::GetThis(HWND hDlg)
{
	return static_cast<CChannelScan*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CChannelScan::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CChannelScan *pThis=dynamic_cast<CChannelScan*>(OnInitDialog(hDlg,lParam));
			int NumSpaces;
			LPCTSTR pszName;

			pThis->m_TuningSpaceList=*pThis->m_pOriginalTuningSpaceList;
			for (NumSpaces=0;(pszName=pThis->m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(NumSpaces))!=NULL;NumSpaces++) {
				::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_ADDSTRING,0,
									 reinterpret_cast<LPARAM>(pszName));
			}
			if (NumSpaces>0) {
				pThis->m_ScanSpace=0;
				::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_SETCURSEL,pThis->m_ScanSpace,0);
				pThis->m_TuningSpaceList.Reserve(NumSpaces);
			} else {
				pThis->m_ScanSpace=-1;
				EnableDlgItems(hDlg,IDC_CHANNELSCAN_SPACE,IDC_CHANNELSCAN_START,false);
			}
			::CheckDlgButton(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL,
							pThis->m_fIgnoreSignalLevel?BST_CHECKED:BST_UNCHECKED);
			pThis->m_fUpdated=false;
			pThis->m_fScaned=false;
			pThis->m_fRestorePreview=false;
			pThis->m_SortColumn=-1;

			HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);

			ListView_SetExtendedListViewStyle(hwndList,
				LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

			LV_COLUMN lvc;

			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=128;
			lvc.pszText=TEXT("名前");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.fmt=LVCFMT_RIGHT;
			lvc.cx=40;
			lvc.pszText=TEXT("番号");
			ListView_InsertColumn(hwndList,1,&lvc);
			/*
			lvc.cx=64;
			lvc.pszText=TEXT("サービス");
			ListView_InsertColumn(hwndList,2,&lvc);
			*/
			lvc.cx=72;
			lvc.pszText=TEXT("チャンネル");
			ListView_InsertColumn(hwndList,2,&lvc);
			lvc.cx=72;
			lvc.pszText=TEXT("サービスID");
			ListView_InsertColumn(hwndList,3,&lvc);
			lvc.cx=80;
			lvc.pszText=TEXT("リモコンキー");
			ListView_InsertColumn(hwndList,4,&lvc);
			if (NumSpaces>0) {
				//pThis->SetChannelList(hDlg,pThis->m_ScanSpace);
				SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_CHANNELSCAN_SPACE,CBN_SELCHANGE),0);
				/*
				for (i=0;i<4;i++)
					ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
				*/
			}
			if (GetAppClass().GetCoreEngine()->IsNetworkDriver())
				EnableDlgItems(hDlg,IDC_CHANNELSCAN_FIRST,IDC_CHANNELSCAN_LAST,false);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_CHANNELSCAN_SPACE:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				CChannelScan *pThis=GetThis(hDlg);
				bool fBS=false,fCS=false;
				bool fPresetLoaded=false;

				pThis->m_ScanSpace=::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_GETCURSEL,0,0);
				LPCTSTR pszName=pThis->m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(pThis->m_ScanSpace);
				if (pszName!=NULL) {
					fBS=::StrStrI(pszName,TEXT("BS"))!=NULL;
					fCS=::StrStrI(pszName,TEXT("CS"))!=NULL;
				}
				if (fBS || fCS) {
					CTuningSpaceInfo *pTuningSpaceInfo=pThis->m_TuningSpaceList.GetTuningSpaceInfo(pThis->m_ScanSpace);
					CChannelList *pChannelList;

					if (pTuningSpaceInfo!=NULL
							&& (pChannelList=pTuningSpaceInfo->GetChannelList())!=NULL
							&& pChannelList->NumChannels()==0) {
						::SendMessage(hDlg,WM_COMMAND,IDC_CHANNELSCAN_LOADPRESET,0);
						fPresetLoaded=true;
					}
				}
				if (!fPresetLoaded)
					pThis->SetChannelList(pThis->m_ScanSpace);
				DlgCheckBox_Check(hDlg,IDC_CHANNELSCAN_SCANSERVICE,fBS || fCS);
				EnableDlgItem(hDlg,IDC_CHANNELSCAN_LOADPRESET,fBS || fCS);
				pThis->m_SortColumn=-1;
			}
			return TRUE;

		case IDC_CHANNELSCAN_LOADPRESET:
			{
				CChannelScan *pThis=GetThis(hDlg);
				bool fBS=false,fCS=false;
				LPCTSTR pszName=pThis->m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetSpaceName(pThis->m_ScanSpace);

				if (pszName!=NULL) {
					fBS=::StrStrI(pszName,TEXT("BS"))!=NULL;
					fCS=::StrStrI(pszName,TEXT("CS"))!=NULL;
				}
				if (fBS || fCS) {
					CTuningSpaceInfo *pTuningSpaceInfo=pThis->m_TuningSpaceList.GetTuningSpaceInfo(pThis->m_ScanSpace);
					CChannelList *pChannelList;

					if (pTuningSpaceInfo!=NULL
							&& (pChannelList=pTuningSpaceInfo->GetChannelList())!=NULL) {
						TCHAR szPresetFileName[MAX_PATH];
						CTuningSpaceList PresetList;

						GetAppClass().GetAppDirectory(szPresetFileName);
						::PathAppend(szPresetFileName,
							fBS?TEXT("Preset_BS.ch2"):TEXT("Preset_CS.ch2"));
						if (PresetList.LoadFromFile(szPresetFileName)) {
							CChannelList *pPresetChannelList=PresetList.GetAllChannelList();

							pChannelList->Clear();
							for (int i=0;i<pPresetChannelList->NumChannels();i++) {
								CChannelInfo ChannelInfo(*pPresetChannelList->GetChannelInfo(i));

								ChannelInfo.SetSpace(pThis->m_ScanSpace);
								pChannelList->AddChannel(ChannelInfo);
							}
							pThis->m_fUpdated=true;
							pThis->SetChannelList(pThis->m_ScanSpace);
						}
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_START:
			{
				int Space=::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_SPACE,CB_GETCURSEL,0,0);

				if (Space>=0) {
					CChannelScan *pThis=GetThis(hDlg);

					if (GetAppClass().GetMainWindow()->IsPreview()) {
						GetAppClass().GetMainWindow()->EnablePreview(false);
						pThis->m_fRestorePreview=true;
					}
					pThis->m_ScanSpace=Space;
					pThis->m_ScanChannel=0;
					pThis->m_fScanService=
						::IsDlgButtonChecked(hDlg,IDC_CHANNELSCAN_SCANSERVICE)==BST_CHECKED;
					pThis->m_fIgnoreSignalLevel=
						::IsDlgButtonChecked(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL)==BST_CHECKED;
					ListView_DeleteAllItems(::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST));
					if (::DialogBoxParam(GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDD_CHANNELSCAN),::GetParent(hDlg),
							ScanDlgProc,reinterpret_cast<LPARAM>(pThis))==IDOK) {
						HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
						CChannelListSort ListSort(CChannelListSort::SORT_REMOTECONTROLKEYID);

						ListSort.Sort(hwndList);
						ListSort.UpdateChannelList(hwndList,&pThis->m_ScanningChannelList);
						pThis->m_SortColumn=(int)CChannelListSort::SORT_REMOTECONTROLKEYID;
						pThis->m_fSortDescending=false;
						*pThis->m_TuningSpaceList.GetChannelList(Space)=
												pThis->m_ScanningChannelList;
						pThis->m_fUpdated=true;
					} else {
						pThis->SetChannelList(Space);
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_PROPERTIES:
			{
				CChannelScan *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
				LV_ITEM lvi;

				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem>=0) {
					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					ListView_GetItem(hwndList,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					if (::DialogBoxParam(GetAppClass().GetResourceInstance(),
										 MAKEINTRESOURCE(IDD_CHANNELPROPERTIES),
										 hDlg,ChannelPropDlgProc,
										 reinterpret_cast<LPARAM>(pChInfo))==IDOK) {
						lvi.mask=LVIF_TEXT;
						lvi.pszText=const_cast<LPTSTR>(pChInfo->GetName());
						ListView_SetItem(hwndList,&lvi);
						lvi.iSubItem=4;
						TCHAR szText[16];
						if (pChInfo->GetChannelNo()>0)
							::wsprintf(szText,TEXT("%d"),pChInfo->GetChannelNo());
						else
							szText[0]='\0';
						lvi.pszText=szText;
						ListView_SetItem(hwndList,&lvi);
						pThis->m_fUpdated=true;
					}
				}
			}
			return TRUE;

		case IDC_CHANNELSCAN_DELETE:
			{
				CChannelScan *pThis=GetThis(hDlg);
				HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);
				LV_ITEM lvi;

				lvi.iItem=ListView_GetNextItem(hwndList,-1,LVNI_SELECTED);
				if (lvi.iItem>=0) {
					lvi.mask=LVIF_PARAM;
					lvi.iSubItem=0;
					ListView_GetItem(hwndList,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					CChannelList *pList=pThis->m_TuningSpaceList.GetChannelList(pThis->m_ScanSpace);
					if (pList!=NULL) {
						int Index=pList->Find(pChInfo);
						if (Index>=0) {
							ListView_DeleteItem(hwndList,lvi.iItem);
							pList->DeleteChannel(Index);
							pThis->m_fUpdated=true;
						}
					}
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case LVN_COLUMNCLICK:
			{
				CChannelScan *pThis=GetThis(hDlg);
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (pThis->m_SortColumn==pnmlv->iSubItem) {
					pThis->m_fSortDescending=!pThis->m_fSortDescending;
				} else {
					pThis->m_SortColumn=pnmlv->iSubItem;
					pThis->m_fSortDescending=false;
				}
				CChannelListSort ListSort(
								(CChannelListSort::SortType)pnmlv->iSubItem,
								pThis->m_fSortDescending);
				ListSort.Sort(pnmlv->hdr.hwndFrom);
				ListSort.UpdateChannelList(pnmlv->hdr.hwndFrom,
					pThis->m_TuningSpaceList.GetChannelList(pThis->m_ScanSpace));
			}
			return TRUE;

		case NM_RCLICK:
			if (((LPNMHDR)lParam)->hwndFrom==::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST)) {
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0) {
					HMENU hmenu=::LoadMenu(GetAppClass().GetResourceInstance(),
											MAKEINTRESOURCE(IDM_CHANNELSCAN));
					POINT pt;

					::GetCursorPos(&pt);
					::TrackPopupMenu(GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,
														pt.x,pt.y,0,hDlg,NULL);
					::DestroyMenu(hmenu);
				}
			}
			break;

		case NM_DBLCLK:
			if (((LPNMHDR)lParam)->hwndFrom==::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST)) {
				LPNMITEMACTIVATE pnmia=reinterpret_cast<LPNMITEMACTIVATE>(lParam);

				if (pnmia->iItem>=0)
					::SendMessage(hDlg,WM_COMMAND,IDC_CHANNELSCAN_PROPERTIES,0);
			}
			break;

		case LVN_ENDLABELEDIT:
			{
				CChannelScan *pThis=GetThis(hDlg);
				NMLVDISPINFO *plvdi=reinterpret_cast<NMLVDISPINFO*>(lParam);
				BOOL fResult;

				if (plvdi->item.pszText!=NULL && plvdi->item.pszText[0]!='\0') {
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=plvdi->item.iItem;
					lvi.iSubItem=0;
					ListView_GetItem(plvdi->hdr.hwndFrom,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->SetName(plvdi->item.pszText);
					pThis->m_fUpdated=true;
					fResult=TRUE;
				} else {
					fResult=FALSE;
				}
				::SetWindowLongPtr(hDlg,DWLP_MSGRESULT,fResult);
			}
			return TRUE;

		case LVN_ITEMCHANGED:
			if (!GetThis(hDlg)->m_fChanging) {
				LPNMLISTVIEW pnmlv=reinterpret_cast<LPNMLISTVIEW>(lParam);

				if (pnmlv->iItem>=0
						&& (pnmlv->uNewState&LVIS_STATEIMAGEMASK)!=
						   (pnmlv->uOldState&LVIS_STATEIMAGEMASK)) {
					CChannelScan *pThis=GetThis(hDlg);
					LV_ITEM lvi;

					lvi.mask=LVIF_PARAM;
					lvi.iItem=pnmlv->iItem;
					lvi.iSubItem=0;
					ListView_GetItem(pnmlv->hdr.hwndFrom,&lvi);
					CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lvi.lParam);
					pChInfo->Enable((pnmlv->uNewState&LVIS_STATEIMAGEMASK)==INDEXTOSTATEIMAGEMASK(2));
					pThis->m_fUpdated=true;
				}
			}
			return TRUE;

		case PSN_APPLY:
			{
				CChannelScan *pThis=GetThis(hDlg);

				if (pThis->m_fUpdated) {
					CFilePath FilePath(pThis->m_pCoreEngine->GetDriverFileName());
					TCHAR szAppDir[MAX_PATH];

					pThis->m_TuningSpaceList.MakeAllChannelList();
					GetAppClass().GetAppDirectory(szAppDir);
					if (!FilePath.HasDirectory())
						FilePath.SetDirectory(szAppDir);
					FilePath.SetExtension(TEXT(".ch2"));
					pThis->m_TuningSpaceList.SaveToFile(FilePath.GetPath());
					pThis->SetUpdateFlag(UPDATE_CHANNELLIST);
				}
				if (pThis->m_fRestorePreview)
					pThis->SetUpdateFlag(UPDATE_PREVIEW);
				pThis->m_fIgnoreSignalLevel=
					::IsDlgButtonChecked(hDlg,IDC_CHANNELSCAN_IGNORESIGNALLEVEL)==BST_CHECKED;
			}
			return TRUE;

		case PSN_RESET:
			{
				CChannelScan *pThis=GetThis(hDlg);

				if (pThis->m_fRestorePreview)
					//pThis->SetUpdateFlag(UPDATE_PREVIEW);
					GetAppClass().GetMainWindow()->EnablePreview(true);
			}
			return TRUE;
		}
		break;

	case WM_APP_CHANNELFOUND:
		{
			CChannelScan *pThis=GetThis(hDlg);
			const CChannelInfo *pChInfo=reinterpret_cast<const CChannelInfo*>(lParam);
			HWND hwndList=::GetDlgItem(hDlg,IDC_CHANNELSCAN_CHANNELLIST);

			pThis->m_fChanging=true;
			pThis->InsertChannelInfo(ListView_GetItemCount(hwndList),pChInfo);
			pThis->m_fChanging=false;
			::UpdateWindow(hwndList);
		}
		return TRUE;

	case WM_DESTROY:
		{
			CChannelScan *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


BOOL CALLBACK CChannelScan::ScanDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CChannelScan *pThis=reinterpret_cast<CChannelScan*>(lParam);
			int i;

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hScanDlg=hDlg;
			for (i=pThis->m_ScanChannel;pThis->m_pCoreEngine->m_DtvEngine.m_BonSrcDecoder.GetChannelName(pThis->m_ScanSpace,i)!=NULL;i++);
			pThis->m_NumChannels=i;
			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETRANGE32,
								 pThis->m_ScanChannel,i);
			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETPOS,
								 pThis->m_ScanChannel,0);
			pThis->m_hCancelEvent=::CreateEvent(NULL,FALSE,FALSE,NULL);
			GetAppClass().BeginChannelScan();
			pThis->m_hScanThread=::CreateThread(NULL,0,ScanProc,pThis,0,NULL);
			pThis->m_fScaned=true;
			::SetTimer(hDlg,1,500,NULL);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			{
				CChannelScan *pThis=GetThis(hDlg);

				pThis->m_fOK=LOWORD(wParam)==IDOK;
				::SetEvent(pThis->m_hCancelEvent);
				::EnableDlgItem(hDlg,IDOK,false);
				::EnableDlgItem(hDlg,IDCANCEL,false);
			}
			return TRUE;
		}
		return TRUE;

	case WM_TIMER:
		{
			const CCoreEngine *pCoreEngine=GetAppClass().GetCoreEngine();
			int Level=(int)(pCoreEngine->GetSignalLevel()*100.0f);
			int BitRate=(int)(pCoreEngine->GetBitRateFloat()*100.0f);
			TCHAR szText[64];

			::wsprintf(szText,TEXT("%d.%02d dB / %d.%02d Mbps"),
								Level/100,Level%100,BitRate/100,BitRate%100);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_LEVEL,szText);
		}
		return TRUE;

	case WM_APP_BEGINSCAN:
		{
			CChannelScan *pThis=GetThis(hDlg);
			unsigned int EstimateRemain=(pThis->m_NumChannels-wParam)*SCAN_INTERVAL;
			TCHAR szText[64];

			if (pThis->m_fIgnoreSignalLevel)
				EstimateRemain+=(pThis->m_NumChannels-wParam)*RETRY_COUNT*RETRY_INTERVAL/1000;
			::wsprintf(szText,
				TEXT("チャンネル %d/%d をスキャン中... (残り時間 %d:%02d)"),
				(int)wParam+1,pThis->m_NumChannels,
				EstimateRemain/60,EstimateRemain%60);
			::SetDlgItemText(hDlg,IDC_CHANNELSCAN_INFO,szText);
			::SendDlgItemMessage(hDlg,IDC_CHANNELSCAN_PROGRESS,PBM_SETPOS,wParam,0);
		}
		return TRUE;

	case WM_APP_CHANNELFOUND:
		{
			CChannelScan *pThis=GetThis(hDlg);
			//int ScanChannel=LOWORD(wParam),Service=HIWORD(wParam);
			const CChannelInfo *pChInfo=pThis->m_ScanningChannelList.GetChannelInfo((int)lParam);

			::SendMessage(pThis->m_hDlg,WM_APP_CHANNELFOUND,0,reinterpret_cast<LPARAM>(pChInfo));
		}
		return TRUE;

	case WM_APP_ENDSCAN:
		{
			CChannelScan *pThis=GetThis(hDlg);

			::WaitForSingleObject(pThis->m_hScanThread,INFINITE);
			GetAppClass().EndChannelScan();
			::EndDialog(hDlg,wParam!=0 || pThis->m_fOK?IDOK:IDCANCEL);
		}
		return TRUE;

	case WM_DESTROY:
		{
			CChannelScan *pThis=GetThis(hDlg);

			::CloseHandle(pThis->m_hCancelEvent);
			pThis->m_hCancelEvent=NULL;
			::CloseHandle(pThis->m_hScanThread);
			pThis->m_hScanThread=NULL;
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}


DWORD WINAPI CChannelScan::ScanProc(LPVOID lpParameter)
{
	CChannelScan *pThis=static_cast<CChannelScan*>(lpParameter);
	CDtvEngine *pDtvEngine=&pThis->m_pCoreEngine->m_DtvEngine;
	bool fComplete=false;

	pThis->m_ScanningChannelList.Clear();
	while (true) {
		LPCTSTR pszName=pDtvEngine->m_BonSrcDecoder.GetChannelName(pThis->m_ScanSpace,pThis->m_ScanChannel);

		if (pszName==NULL) {
			fComplete=true;
			break;
		}
		::PostMessage(pThis->m_hScanDlg,WM_APP_BEGINSCAN,pThis->m_ScanChannel,0);
		pDtvEngine->SetChannel(pThis->m_ScanSpace,pThis->m_ScanChannel);
		if (::WaitForSingleObject(pThis->m_hCancelEvent,SCAN_INTERVAL*1000)==WAIT_OBJECT_0)
			break;
		bool fFound=false;
		int NumServices;
		TCHAR szName[32];
		if (pThis->m_fIgnoreSignalLevel
				|| pDtvEngine->m_BonSrcDecoder.GetSignalLevel()>7.0) {
			for (int i=0;i<=RETRY_COUNT;i++) {
				if (i>0 && ::WaitForSingleObject(pThis->m_hCancelEvent,RETRY_INTERVAL)==WAIT_OBJECT_0)
					goto End;
				NumServices=pDtvEngine->m_ProgManager.GetServiceNum();
				if (NumServices>0) {
					if (pThis->m_fScanService) {
						int j;

						for (j=0;j<NumServices;j++) {
							if (pDtvEngine->m_ProgManager.GetServiceName(szName,i)==0)
								break;
						}
						if (j==NumServices)
							fFound=true;
					} else {
						if (pDtvEngine->m_ProgManager.GetServiceName(szName,0)>0
								&& pDtvEngine->m_ProgManager.GetTSName(szName,lengthof(szName))>0)
							fFound=true;
					}
					if (fFound)
						break;
				}
			}
		}
		if (!fFound) {
			pThis->m_ScanChannel++;
			continue;
		}
		WORD NetworkID=pDtvEngine->m_ProgManager.GetNetworkID();
		WORD TransportStreamID=pDtvEngine->m_ProgManager.GetTransportStreamID();
		if (pThis->m_fScanService) {
			for (int i=0;i<NumServices;i++) {
				pDtvEngine->m_ProgManager.GetServiceName(szName,i);
				CChannelInfo ChInfo(pThis->m_ScanSpace,0,pThis->m_ScanChannel,
					pDtvEngine->m_ProgManager.GetRemoteControlKeyID(),
					i,szName);
				ChInfo.SetNetworkID(NetworkID);
				ChInfo.SetTransportStreamID(TransportStreamID);
				WORD ServiceID=0;
				if (pDtvEngine->m_ProgManager.GetServiceID(&ServiceID,i))
					ChInfo.SetServiceID(ServiceID);
				TRACE(TEXT("Channel found : %s %d %d %d %d\n"),
					szName,pThis->m_ScanChannel,ChInfo.GetChannelNo(),NetworkID,ServiceID);
				pThis->m_ScanningChannelList.AddChannel(ChInfo);
				::PostMessage(pThis->m_hScanDlg,WM_APP_CHANNELFOUND,
						MAKEWPARAM(pThis->m_ScanChannel,i),
						pThis->m_ScanningChannelList.NumChannels()-1);
			}
		} else {
			CChannelInfo ChInfo(pThis->m_ScanSpace,0,pThis->m_ScanChannel,
				pDtvEngine->m_ProgManager.GetRemoteControlKeyID(),
				0,szName);
			ChInfo.SetNetworkID(NetworkID);
			ChInfo.SetTransportStreamID(TransportStreamID);
			WORD ServiceID=0;
			if (pDtvEngine->m_ProgManager.GetServiceID(&ServiceID,0))
				ChInfo.SetServiceID(ServiceID);
			TRACE(TEXT("Channel found : %s %d %d %d %d\n"),
				szName,pThis->m_ScanChannel,ChInfo.GetChannelNo(),NetworkID,ServiceID);
			pThis->m_ScanningChannelList.AddChannel(ChInfo);
			::PostMessage(pThis->m_hScanDlg,WM_APP_CHANNELFOUND,
				pThis->m_ScanChannel,
				pThis->m_ScanningChannelList.NumChannels()-1);
		}
		pThis->m_ScanChannel++;
	}
End:
	::PostMessage(pThis->m_hScanDlg,WM_APP_ENDSCAN,fComplete,0);
	return 0;
}


BOOL CALLBACK CChannelScan::ChannelPropDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CChannelInfo *pChInfo=reinterpret_cast<CChannelInfo*>(lParam);
			TCHAR szText[64];

			::SetProp(hDlg,TEXT("ChannelInfo"),pChInfo);
			::SendDlgItemMessage(hDlg,IDC_CHANNELPROP_NAME,EM_LIMITTEXT,MAX_CHANNEL_NAME-1,0);
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_NAME,pChInfo->GetName());
			for (int i=1;i<=12;i++) {
				TCHAR szText[4];

				::wsprintf(szText,TEXT("%d"),i);
				DlgComboBox_AddString(hDlg,IDC_CHANNELPROP_CONTROLKEY,szText);
			}
			if (pChInfo->GetChannelNo()>0)
				::SetDlgItemInt(hDlg,IDC_CHANNELPROP_CONTROLKEY,pChInfo->GetChannelNo(),TRUE);
			::wsprintf(szText,TEXT("%d (0x%04x)"),
				pChInfo->GetTransportStreamID(),pChInfo->GetTransportStreamID());
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_TSID,szText);
			::wsprintf(szText,TEXT("%d (0x%04x)"),
						pChInfo->GetServiceID(),pChInfo->GetServiceID());
			::SetDlgItemText(hDlg,IDC_CHANNELPROP_SERVICEID,szText);
			::SetDlgItemInt(hDlg,IDC_CHANNELPROP_TUNINGSPACE,pChInfo->GetSpace(),TRUE);
			::SetDlgItemInt(hDlg,IDC_CHANNELPROP_CHANNELINDEX,pChInfo->GetChannelIndex(),TRUE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				CChannelInfo *pChInfo=static_cast<CChannelInfo*>(::GetProp(hDlg,TEXT("ChannelInfo")));
				bool fModified=false;
				TCHAR szName[MAX_CHANNEL_NAME];
				int ControlKey;

				::GetDlgItemText(hDlg,IDC_CHANNELPROP_NAME,szName,lengthof(szName));
				if (szName[0]=='\0') {
					::MessageBox(hDlg,TEXT("名前を入力してください。"),TEXT("お願い"),MB_OK | MB_ICONINFORMATION);
					::SetFocus(::GetDlgItem(hDlg,IDC_CHANNELPROP_NAME));
					return TRUE;
				}
				if (::lstrcmp(pChInfo->GetName(),szName)!=0) {
					pChInfo->SetName(szName);
					fModified=true;
				}
				ControlKey=::GetDlgItemInt(hDlg,IDC_CHANNELPROP_CONTROLKEY,NULL,TRUE);
				if (ControlKey!=pChInfo->GetChannelNo()) {
					pChInfo->SetChannelNo(ControlKey);
					fModified=true;
				}
				if (!fModified)
					wParam=IDCANCEL;
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg,TEXT("ChannelInfo"));
		return TRUE;
	}
	return FALSE;
}
