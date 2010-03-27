#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "StreamInfo.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CStreamInfo::CStreamInfo()
	: m_pEventHandler(NULL)
{
}


CStreamInfo::~CStreamInfo()
{
}


static void CopyText(LPCTSTR pszText)
{
	HGLOBAL hData=::GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,(::lstrlen(pszText)+1)*sizeof(TCHAR));
	if (hData!=NULL) {
		::lstrcpy(static_cast<LPTSTR>(::GlobalLock(hData)),pszText);
		::GlobalUnlock(hData);
		if (::OpenClipboard(GetAppClass().GetMainWindow()->GetHandle())) {
			::EmptyClipboard();
			::SetClipboardData(
#ifdef UNICODE
				CF_UNICODETEXT,
#else
				CF_TEXT,
#endif
				hData);
			::CloseClipboard();
		}
	}
}


INT_PTR CStreamInfo::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	INT_PTR Result=CResizableDialog::DlgProc(hDlg,uMsg,wParam,lParam);

	switch (uMsg) {
	case WM_INITDIALOG:
		SetService();

		AddControl(IDC_STREAMINFO_STREAM,ALIGN_HORZ);
		AddControl(IDC_STREAMINFO_NETWORK,ALIGN_HORZ);
		AddControl(IDC_STREAMINFO_SERVICE,ALIGN_ALL);
		AddControl(IDC_STREAMINFO_UPDATE,ALIGN_BOTTOM);
		AddControl(IDC_STREAMINFO_COPY,ALIGN_BOTTOM);
		//AddControl(IDOK,ALIGN_BOTTOM_RIGHT);

		ApplyPosition();
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_STREAMINFO_UPDATE:
			SetService();
			return TRUE;

		case IDC_STREAMINFO_COPY:
			{
				int Length;
				LPTSTR pszText,p;
				HWND hwndTree=::GetDlgItem(hDlg,IDC_STREAMINFO_SERVICE);

				Length=0x8000;
				pszText=new TCHAR[Length];
				p=pszText;
				::GetDlgItemText(hDlg,IDC_STREAMINFO_STREAM,p,Length);
				p+=::lstrlen(p);
				*p++='\r';
				*p++='\n';
				::GetDlgItemText(hDlg,IDC_STREAMINFO_NETWORK,p,Length-(int)(p-pszText));
				p+=::lstrlen(p);
				*p++='\r';
				*p++='\n';
				GetTreeViewText(hwndTree,TreeView_GetChild(hwndTree,TreeView_GetRoot(hwndTree)),true,
								p,Length-(int)(p-pszText));
				CopyText(pszText);
				delete [] pszText;
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			if (m_pEventHandler==NULL || m_pEventHandler->OnClose())
				::DestroyWindow(hDlg);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (reinterpret_cast<NMHDR*>(lParam)->code) {
		case NM_RCLICK:
			{
				NMHDR *pnmhdr=reinterpret_cast<NMHDR*>(lParam);
				//HTREEITEM hItem=TreeView_GetSelection(pnmhdr->hwndFrom);
				DWORD Pos=::GetMessagePos();
				TVHITTESTINFO tvhti;
				tvhti.pt.x=(SHORT)LOWORD(Pos);
				tvhti.pt.y=(SHORT)HIWORD(Pos);
				::ScreenToClient(pnmhdr->hwndFrom,&tvhti.pt);
				HTREEITEM hItem=TreeView_HitTest(pnmhdr->hwndFrom,&tvhti);
				if (hItem!=NULL) {
					HMENU hmenu=::CreatePopupMenu();
					::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("コピー(&C)"));
					POINT pt;
					::GetCursorPos(&pt);
					switch (::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hDlg,NULL)) {
					case 1:
						{
							int Length=0x8000;
							LPTSTR pszText=new TCHAR[Length];

							if (GetTreeViewText(pnmhdr->hwndFrom,hItem,false,pszText,Length)>0)
								CopyText(pszText);
							delete [] pszText;
						}
						break;
					}
				}
			}
			return TRUE;
		}
		break;
	}
	return Result;
}


void CStreamInfo::SetService()
{
	CTsAnalyzer *pAnalyzer = &GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsAnalyzer;
	TCHAR szText[256];

	WORD TSID=pAnalyzer->GetTransportStreamID();
	if (TSID!=0) {
		::wsprintf(szText,TEXT("TSID 0x%04x (%d)"),TSID,TSID);
		TCHAR szTsName[64];
		if (pAnalyzer->GetTsName(szTsName,lengthof(szTsName))>0) {
			::wsprintf(szText+::lstrlen(szText),TEXT(" %s"),szTsName);
		}
	} else {
		szText[0]='\0';
	}
	::SetDlgItemText(m_hDlg,IDC_STREAMINFO_STREAM,szText);

	WORD NID=pAnalyzer->GetNetworkID();
	if (NID!=0) {
		::wsprintf(szText,TEXT("NID 0x%04x (%d)"),NID,NID);
		TCHAR szName[64];
		if (pAnalyzer->GetNetworkName(szName,lengthof(szName))>0) {
			::wsprintf(szText+::lstrlen(szText),TEXT(" %s"),szName);
		}
	} else {
		szText[0]='\0';
	}
	::SetDlgItemText(m_hDlg,IDC_STREAMINFO_NETWORK,szText);

	CTsAnalyzer::CServiceList ServiceList;
	pAnalyzer->GetServiceList(&ServiceList);

	HWND hwndTree=::GetDlgItem(m_hDlg,IDC_STREAMINFO_SERVICE);
	TVINSERTSTRUCT tvis;
	HTREEITEM hItem;

	TreeView_DeleteAllItems(hwndTree);
	tvis.hParent=TVI_ROOT;
	tvis.hInsertAfter=TVI_LAST;
	tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
	tvis.item.state=TVIS_EXPANDED;
	tvis.item.stateMask=(UINT)-1;
	tvis.item.pszText=TEXT("サービス");
	tvis.item.cChildren=ServiceList.NumServices()>0?1:0;
	hItem=TreeView_InsertItem(hwndTree,&tvis);
	if (hItem!=NULL) {
		int i,j;

		for (i=0;i<ServiceList.NumServices();i++) {
			const CTsAnalyzer::ServiceInfo *pServiceInfo=ServiceList.GetServiceInfo(i);
			TCHAR szServiceName[64];
			WORD ServiceID,PID;

			tvis.hParent=hItem;
			tvis.item.state=0;
			tvis.item.cChildren=1;
			if (pServiceInfo->szServiceName[0]!='\0')
				::lstrcpyn(szServiceName,pServiceInfo->szServiceName,lengthof(szServiceName));
			else
				::lstrcpy(szServiceName,TEXT("???"));
			ServiceID=pServiceInfo->ServiceID;
			::wsprintf(szText,TEXT("サービス%d (%s) SID 0x%04x (%d)"),i+1,szServiceName,ServiceID,ServiceID);
			if (pServiceInfo->ServiceType!=0xFF) {
				::wsprintf(szText+::lstrlen(szText),TEXT(" Type %#02x"),pServiceInfo->ServiceType);
			}
			tvis.item.pszText=szText;
			tvis.hParent=TreeView_InsertItem(hwndTree,&tvis);
			tvis.item.cChildren=0;
			PID=pServiceInfo->PmtPID;
			::wsprintf(szText,TEXT("PMT PID %#04x (%d)"),PID,PID);
			TreeView_InsertItem(hwndTree,&tvis);
			PID=pServiceInfo->VideoEs.PID;
			if (PID!=CTsAnalyzer::PID_INVALID) {
				BYTE StreamType=pServiceInfo->VideoStreamType;
				::wsprintf(szText,TEXT("映像 PID %#04x (%d) Type %#02x (%s) / Component tag %#02x"),
					PID,PID,StreamType,
					StreamType==0x02?TEXT("MPEG-2"):StreamType==0x1B?TEXT("H.264"):TEXT("Unknown"),
					pServiceInfo->VideoEs.ComponentTag);
				TreeView_InsertItem(hwndTree,&tvis);
			}
			int NumAudioStreams=(int)pServiceInfo->AudioEsList.size();
			for (j=0;j<NumAudioStreams;j++) {
				PID=pServiceInfo->AudioEsList[j].PID;
				::wsprintf(szText,TEXT("音声%d PID %#04x (%d) Component tag %#02x"),
						   j+1,PID,PID,pServiceInfo->AudioEsList[j].ComponentTag);
				TreeView_InsertItem(hwndTree,&tvis);
			}
			int NumCaptionStreams=(int)pServiceInfo->CaptionEsList.size();
			for (j=0;j<NumCaptionStreams;j++) {
				PID=pServiceInfo->CaptionEsList[j].PID;
				::wsprintf(szText,TEXT("字幕%d PID %#04x (%d) Component tag %#02x"),
						   j+1,PID,PID,pServiceInfo->CaptionEsList[j].ComponentTag);
				TreeView_InsertItem(hwndTree,&tvis);
			}
			int NumDataStreams=(int)pServiceInfo->DataCarrouselEsList.size();
			for (j=0;j<NumDataStreams;j++) {
				PID=pServiceInfo->DataCarrouselEsList[j].PID;
				::wsprintf(szText,TEXT("データ%d PID %#04x (%d) Component tag %#02x"),
						   j+1,PID,PID,pServiceInfo->DataCarrouselEsList[j].ComponentTag);
				TreeView_InsertItem(hwndTree,&tvis);
			}
			PID=pServiceInfo->PcrPID;
			if (PID!=CTsAnalyzer::PID_INVALID) {
				::wsprintf(szText,TEXT("PCR PID %#04x (%d)"),PID,PID);
				TreeView_InsertItem(hwndTree,&tvis);
			}
			PID=pServiceInfo->EcmPID;
			if (PID!=CTsAnalyzer::PID_INVALID) {
				::wsprintf(szText,TEXT("ECM PID %#04x (%d)"),PID,PID);
				TreeView_InsertItem(hwndTree,&tvis);
			}
		}
	}

	const CChannelInfo *pChannelInfo=GetAppClass().GetChannelManager()->GetCurrentChannelInfo();
	if (pChannelInfo!=NULL) {
		tvis.hParent=TVI_ROOT;
		tvis.hInsertAfter=TVI_LAST;
		tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
		tvis.item.state=TVIS_EXPANDED;
		tvis.item.stateMask=(UINT)-1;
		tvis.item.pszText=TEXT("チャンネルファイル用フォーマット");
		tvis.item.cChildren=ServiceList.NumServices()>0?1:0;;
		hItem=TreeView_InsertItem(hwndTree,&tvis);
		if (hItem!=NULL) {
			const int RemoteControlKeyID=pAnalyzer->GetRemoteControlKeyID();

			for (int i=0;i<ServiceList.NumServices();i++) {
				const CTsAnalyzer::ServiceInfo *pServiceInfo=ServiceList.GetServiceInfo(i);

				tvis.hParent=hItem;
				tvis.item.state=0;
				tvis.item.cChildren=0;
				if (pServiceInfo->szServiceName[0]!='\0')
					::lstrcpy(szText,pServiceInfo->szServiceName);
				else
					::wsprintf(szText,TEXT("サービス%d"),i+1);
				::wsprintf(szText+::lstrlen(szText),TEXT(",%d,%d,%d,%d,%d,%d,%d"),
						   pChannelInfo->GetSpace(),
						   pChannelInfo->GetChannelIndex(),
						   RemoteControlKeyID,
						   i,
						   pServiceInfo->ServiceID,NID,TSID);
				tvis.item.pszText=szText;
				TreeView_InsertItem(hwndTree,&tvis);
			}
		}
	}
}


int CStreamInfo::GetTreeViewText(HWND hwndTree,HTREEITEM hItem,bool fSiblings,LPTSTR pszText,int MaxText,int Level)
{
	if (MaxText<=0)
		return 0;

	TV_ITEM tvi;
	LPTSTR p=pszText;

	tvi.mask=TVIF_TEXT;
	tvi.hItem=hItem;
	while (tvi.hItem!=NULL && MaxText>2) {
		if (Level>0) {
			if (MaxText<=Level)
				break;
			for (int i=0;i<Level;i++)
				*p++='\t';
			MaxText-=Level;
		}
		tvi.pszText=p;
		tvi.cchTextMax=MaxText;
		if (TreeView_GetItem(hwndTree,&tvi)) {
			int Len=::lstrlen(p);
			p+=Len;
			*p++='\r';
			*p++='\n';
			MaxText-=Len+2;
		}
		HTREEITEM hChild=TreeView_GetChild(hwndTree,tvi.hItem);
		if (hChild!=NULL) {
			int Length=GetTreeViewText(hwndTree,hChild,true,p,MaxText,Level+1);
			p+=Length;
			MaxText-=Length;
		}
		if (!fSiblings)
			break;
		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
	*p='\0';
	return (int)(p-pszText);
}


bool CStreamInfo::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_STREAMINFO));
}


bool CStreamInfo::SetEventHandler(CEventHandler *pHandler)
{
	m_pEventHandler=pHandler;
	return true;
}
