#include "stdafx.h"
#include <commctrl.h>
#include "TVTest.h"
#include "AppMain.h"
#include "StreamInfo.h"
#include "DialogUtil.h"
#include "resource.h"




CStreamInfo::CStreamInfo()
{
	m_hDlg=NULL;
	m_WindowPos.x=0;
	m_WindowPos.y=0;
}


CStreamInfo::~CStreamInfo()
{
}


CStreamInfo *CStreamInfo::GetThis(HWND hDlg)
{
	return static_cast<CStreamInfo*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CStreamInfo::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CStreamInfo *pThis=reinterpret_cast<CStreamInfo*>(lParam);
			CTsAnalyzer *pAnalyzer = &GetAppClass().GetCoreEngine()->m_DtvEngine.m_TsAnalyzer;
			TCHAR szText[256];

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hDlg=hDlg;

			WORD TSID=pAnalyzer->GetTransportStreamID();
			if (TSID!=0) {
				::wsprintf(szText,TEXT("TSID 0x%04x (%d)"),TSID,TSID);
				TCHAR szTsName[64];
				if (pAnalyzer->GetTsName(szTsName,lengthof(szTsName))>0) {
					::wsprintf(szText+::lstrlen(szText),TEXT(" %s"),szTsName);
				}
				::SetDlgItemText(hDlg,IDC_STREAMINFO_STREAM,szText);
			}

			WORD NID=pAnalyzer->GetNetworkID();
			if (NID!=0) {
				::wsprintf(szText,TEXT("NID 0x%04x (%d)"),NID,NID);
				TCHAR szName[64];
				if (pAnalyzer->GetNetworkName(szName,lengthof(szName))>0) {
					::wsprintf(szText+::lstrlen(szText),TEXT(" %s"),szName);
				}
				::SetDlgItemText(hDlg,IDC_STREAMINFO_NETWORK,szText);
			}

			HWND hwndTree=::GetDlgItem(hDlg,IDC_STREAMINFO_SERVICE);
			TVINSERTSTRUCT tvis;
			HTREEITEM hItem;
			int i,j;

			tvis.hParent=TVI_ROOT;
			tvis.hInsertAfter=TVI_LAST;
			tvis.item.mask=TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
			tvis.item.state=TVIS_EXPANDED;
			tvis.item.stateMask=(UINT)-1;
			tvis.item.pszText=TEXT("サービス");
			tvis.item.cChildren=1;
			hItem=TreeView_InsertItem(hwndTree,&tvis);
			if (hItem!=NULL) {
				int NumServices;

				NumServices=pAnalyzer->GetServiceNum();
				for (i=0;i<NumServices;i++) {
					TCHAR szServiceName[64];
					WORD ServiceID,PID;

					tvis.hParent=hItem;
					tvis.item.state=0;
					tvis.item.cChildren=1;
					if (!pAnalyzer->GetServiceName(i,szServiceName,lengthof(szServiceName)))
						::lstrcpy(szServiceName,TEXT("???"));
					if (!pAnalyzer->GetServiceID(i,&ServiceID))
						ServiceID=0;
					::wsprintf(szText,TEXT("サービス%d (%s) SID 0x%04x (%d)"),i+1,szServiceName,ServiceID,ServiceID);
					tvis.item.pszText=szText;
					tvis.hParent=TreeView_InsertItem(hwndTree,&tvis);
					tvis.item.cChildren=0;
					if (pAnalyzer->GetPmtPID(i,&PID)) {
						::wsprintf(szText,TEXT("PMT PID 0x%04x (%d)"),PID,PID);
						TreeView_InsertItem(hwndTree,&tvis);
					}
					if (pAnalyzer->GetVideoEsPID(i,&PID)) {
						::wsprintf(szText,TEXT("映像 PID 0x%04x (%d)"),PID,PID);
						BYTE StreamType;
						if (pAnalyzer->GetVideoStreamType(i,&StreamType))
							::wsprintf(szText+::lstrlen(szText),TEXT(" Type %s (0x%02x)"),
								StreamType==0x02?TEXT("MPEG-2"):StreamType==0x1B?TEXT("H.264"):TEXT("???"),
								StreamType);
						TreeView_InsertItem(hwndTree,&tvis);
					}
					int NumAudioStream=pAnalyzer->GetAudioEsNum(i);
					for (j=0;j<NumAudioStream;j++) {
						if (pAnalyzer->GetAudioEsPID(i,j,&PID)) {
							::wsprintf(szText,TEXT("音声%d PID 0x%04x (%d)"),j+1,PID,PID);
							TreeView_InsertItem(hwndTree,&tvis);
						}
					}
					int NumSubtitleStream=pAnalyzer->GetSubtitleEsNum(i);
					for (j=0;j<NumSubtitleStream;j++) {
						if (pAnalyzer->GetSubtitleEsPID(i,j,&PID)) {
							::wsprintf(szText,TEXT("字幕%d PID 0x%04x (%d)"),j+1,PID,PID);
							TreeView_InsertItem(hwndTree,&tvis);
						}
					}
					int NumDataStream=pAnalyzer->GetDataCarrouselEsNum(i);
					for (j=0;j<NumDataStream;j++) {
						if (pAnalyzer->GetDataCarrouselEsPID(i,j,&PID)) {
							::wsprintf(szText,TEXT("データ%d PID 0x%04x (%d)"),j+1,PID,PID);
							TreeView_InsertItem(hwndTree,&tvis);
						}
					}
					if (pAnalyzer->GetPcrPID(i,&PID)) {
						::wsprintf(szText,TEXT("PCR PID 0x%04x (%d)"),PID,PID);
						TreeView_InsertItem(hwndTree,&tvis);
					}
					if (pAnalyzer->GetEcmPID(i,&PID)) {
						::wsprintf(szText,TEXT("ECM PID 0x%04x (%d)"),PID,PID);
						TreeView_InsertItem(hwndTree,&tvis);
					}
				}
			}

			::SetWindowPos(hDlg,NULL,pThis->m_WindowPos.x,pThis->m_WindowPos.y,
						   0,0,SWP_NOSIZE | SWP_NOZORDER);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg,IDOK);
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		{
			CStreamInfo *pThis=GetThis(hDlg);
			RECT rc;

			::GetWindowRect(hDlg,&rc);
			pThis->m_WindowPos.x=rc.left;
			pThis->m_WindowPos.y=rc.top;
			::RemoveProp(hDlg,TEXT("This"));
			pThis->m_hDlg=NULL;
		}
		return TRUE;
	}
	return FALSE;
}


bool CStreamInfo::Show(HWND hwndOwner)
{
	if (m_hDlg!=NULL)
		return false;
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_STREAMINFO),
							   hwndOwner,DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}
