#include "stdafx.h"
#include <commctrl.h>
#include "TVTest.h"
#include "AppMain.h"
#include "StreamInfo.h"
#include "DialogUtil.h"
#include "resource.h"




CStreamInfo::CStreamInfo()
	: m_pEventHandler(NULL)
{
	m_WindowPosition.x=0;
	m_WindowPosition.y=0;
	m_WindowPosition.Width=0;
	m_WindowPosition.Height=0;
}


CStreamInfo::~CStreamInfo()
{
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

		if (m_WindowPosition.Width==0)
			m_WindowPosition.Width=m_MinSize.cx;
		if (m_WindowPosition.Height==0)
			m_WindowPosition.Height=m_MinSize.cy;
		::MoveWindow(hDlg,m_WindowPosition.x,m_WindowPosition.y,
					 m_WindowPosition.Width,m_WindowPosition.Height,FALSE);
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
				::GetDlgItemText(hDlg,IDC_STREAMINFO_NETWORK,p,Length-(p-pszText));
				p+=::lstrlen(p);
				*p++='\r';
				*p++='\n';
				CopyTreeViewText(hwndTree,TreeView_GetChild(hwndTree,TreeView_GetRoot(hwndTree)),
								 p,Length-(p-pszText));
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

	case WM_DESTROY:
		{
			RECT rc;

			::GetWindowRect(hDlg,&rc);
			m_WindowPosition.Set(&rc);
		}
		return TRUE;
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
	tvis.item.cChildren=1;
	hItem=TreeView_InsertItem(hwndTree,&tvis);
	if (hItem!=NULL) {
		int NumServices;
		int i,j;

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
						StreamType==0x02?TEXT("MPEG-2"):StreamType==0x1B?TEXT("H.264"):TEXT("Unknown"),
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
}


int CStreamInfo::CopyTreeViewText(HWND hwndTree,HTREEITEM hItem,LPTSTR pszText,int MaxText,int Level)
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
			int Length=CopyTreeViewText(hwndTree,hChild,p,MaxText,Level+1);
			p+=Length;
			MaxText-=Length;
		}
		tvi.hItem=TreeView_GetNextSibling(hwndTree,tvi.hItem);
	}
	*p='\0';
	return p-pszText;
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
