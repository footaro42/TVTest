#include "stdafx.h"
#include "TVTest.h"
#include "Logger.h"
#include "DialogUtil.h"
#include "StdUtil.h"
#include "resource.h"




CLogItem::CLogItem(LPCTSTR pszText)
{
	::GetSystemTimeAsFileTime(&m_Time);
	m_pszText=DuplicateString(pszText);
}


CLogItem::~CLogItem()
{
	delete [] m_pszText;
}


void CLogItem::GetTime(SYSTEMTIME *pTime) const
{
	SYSTEMTIME stUTC;

	::FileTimeToSystemTime(&m_Time,&stUTC);
	::SystemTimeToTzSpecificLocalTime(NULL,&stUTC,pTime);
}


int CLogItem::Format(char *pszText,int MaxLength) const
{
	SYSTEMTIME st;
	int Length;

	GetTime(&st);
	Length=::GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,
											NULL,pszText,MaxLength-1);
	pszText[Length-1]=' ';
	Length+=::GetTimeFormatA(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,
								NULL,pszText+Length,MaxLength-Length);
	pszText[Length-1]='>';
	Length+=::WideCharToMultiByte(CP_ACP,0,m_pszText,::lstrlen(m_pszText),
									pszText+Length,MaxLength-Length-1,NULL,NULL);
	pszText[Length]='\0';
	return Length;
}




CLogger::CLogger()
{
	m_NumLogItems=0;
	m_ppList=NULL;
	m_ListLength=0;
	m_fOutputToFile=false;
}


CLogger::~CLogger()
{
	Clear();
}


bool CLogger::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("OutputLogToFile"),&m_fOutputToFile);
	if (m_fOutputToFile && m_NumLogItems>0) {
		TCHAR szFileName[MAX_PATH];

		GetDefaultLogFileName(szFileName);
		SaveToFile(szFileName,true);
	}
	return true;
}


bool CLogger::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("OutputLogToFile"),m_fOutputToFile);
	return true;
}


bool CLogger::AddLog(LPCTSTR pszText, ...)
{
	if (pszText==NULL)
		return false;

	va_list Args;
	va_start(Args,pszText);
	AddLogV(pszText,Args);
	va_end(Args);
	return true;
}


bool CLogger::AddLogV(LPCTSTR pszText,va_list Args)
{
	if (pszText==NULL)
		return false;

	CBlockLock Lock(&m_Lock);

	if (m_NumLogItems==m_ListLength) {
		CLogItem **ppNewList;
		int ListLength;

		if (m_ListLength==0)
			ListLength=64;
		else
			ListLength=m_ListLength*2;
		ppNewList=static_cast<CLogItem**>(realloc(m_ppList,ListLength*sizeof(CLogItem*)));
		if (ppNewList==NULL)
			return false;
		m_ppList=ppNewList;
		m_ListLength=ListLength;
	}

	TCHAR szText[1024];
	StdUtil::vsnprintf(szText,lengthof(szText),pszText,Args);
	m_ppList[m_NumLogItems++]=new CLogItem(szText);

	if (m_fOutputToFile) {
		TCHAR szFileName[MAX_PATH];
		HANDLE hFile;

		GetDefaultLogFileName(szFileName);
		hFile=::CreateFile(szFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
						   OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if (hFile!=INVALID_HANDLE_VALUE) {
			char szText[1024];
			DWORD Length,Write;

			::SetFilePointer(hFile,0,NULL,FILE_END);
			Length=m_ppList[m_NumLogItems-1]->Format(szText,lengthof(szText)-1);
			szText[Length++]='\r';
			szText[Length++]='\n';
			::WriteFile(hFile,szText,Length,&Write,NULL);
			::CloseHandle(hFile);
		}
	}

	return true;
}


void CLogger::Clear()
{
	CBlockLock Lock(&m_Lock);

	if (m_ppList!=NULL) {
		int i;

		for (i=m_NumLogItems-1;i>=0;i--)
			delete m_ppList[i];
		free(m_ppList);
		m_ppList=NULL;
		m_NumLogItems=0;
		m_ListLength=0;
	}
}


bool CLogger::SetOutputToFile(bool fOutput)
{
	CBlockLock Lock(&m_Lock);

	m_fOutputToFile=fOutput;
	return true;
}


bool CLogger::SaveToFile(LPCTSTR pszFileName,bool fAppend)
{
	HANDLE hFile;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
					   fAppend?OPEN_ALWAYS:CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (fAppend)
		::SetFilePointer(hFile,0,NULL,FILE_END);

	m_Lock.Lock();

	for (int i=0;i<m_NumLogItems;i++) {
		char szText[1024];
		DWORD Length,Write;

		Length=m_ppList[i]->Format(szText,lengthof(szText)-1);
		szText[Length++]='\r';
		szText[Length++]='\n';
		::WriteFile(hFile,szText,Length,&Write,NULL);
	}

	m_Lock.Unlock();

	::CloseHandle(hFile);
	return true;
}


void CLogger::GetDefaultLogFileName(LPTSTR pszFileName) const
{
	::GetModuleFileName(NULL,pszFileName,MAX_PATH);
	::PathRenameExtension(pszFileName,TEXT(".log"));
}


CLogger *CLogger::GetThis(HWND hDlg)
{
	return static_cast<CLogger*>(GetOptions(hDlg));
}


INT_PTR CALLBACK CLogger::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CLogger *pThis=static_cast<CLogger*>(OnInitDialog(hDlg,lParam));
			HWND hwndList=GetDlgItem(hDlg,IDC_LOG_LIST);
			LV_COLUMN lvc;
			LV_ITEM lvi;
			int i;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=80;
			lvc.pszText=TEXT("日時");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("内容");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvi.mask=LVIF_TEXT;
			pThis->m_Lock.Lock();
			for (i=0;i<pThis->m_NumLogItems;i++) {
				SYSTEMTIME st;
				int Length;
				TCHAR szTime[64];

				lvi.iItem=i;
				lvi.iSubItem=0;
				pThis->m_ppList[i]->GetTime(&st);
				Length=::GetDateFormat(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,
											NULL,szTime,lengthof(szTime)-1);
				szTime[Length-1]=' ';
				::GetTimeFormat(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,
								NULL,szTime+Length,lengthof(szTime)-Length);
				lvi.pszText=szTime;
				ListView_InsertItem(hwndList,&lvi);
				lvi.iSubItem=1;
				lvi.pszText=(LPTSTR)pThis->m_ppList[i]->GetText();
				ListView_SetItem(hwndList,&lvi);
			}
			for (i=0;i<2;i++)
				ListView_SetColumnWidth(hwndList,i,LVSCW_AUTOSIZE_USEHEADER);
			if (pThis->m_NumLogItems>0)
				ListView_EnsureVisible(hwndList,pThis->m_NumLogItems-1,FALSE);
			pThis->m_Lock.Unlock();

			DlgCheckBox_Check(hDlg,IDC_LOG_OUTPUTTOFILE,pThis->m_fOutputToFile);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_LOG_CLEAR:
			{
				CLogger *pThis=GetThis(hDlg);

				ListView_DeleteAllItems(GetDlgItem(hDlg,IDC_LOG_LIST));
				pThis->Clear();
			}
			return TRUE;

		case IDC_LOG_SAVE:
			{
				CLogger *pThis=GetThis(hDlg);
				TCHAR szFileName[MAX_PATH];

				pThis->GetDefaultLogFileName(szFileName);
				if (!pThis->SaveToFile(szFileName,false)) {
					::MessageBox(hDlg,TEXT("保存ができません。"),NULL,MB_OK | MB_ICONEXCLAMATION);
				} else {
					TCHAR szMessage[MAX_PATH+64];

					::wsprintf(szMessage,TEXT("ログを \"%s\" に保存しました。"),szFileName);
					::MessageBox(hDlg,szMessage,TEXT("ログ保存"),MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CLogger *pThis=GetThis(hDlg);

				bool fOutput=DlgCheckBox_IsChecked(hDlg,IDC_LOG_OUTPUTTOFILE);

				if (fOutput!=pThis->m_fOutputToFile) {
					CBlockLock Lock(&pThis->m_Lock);

					if (fOutput && pThis->m_NumLogItems>0) {
						TCHAR szFileName[MAX_PATH];

						pThis->GetDefaultLogFileName(szFileName);
						pThis->SaveToFile(szFileName,true);
					}
					pThis->m_fOutputToFile=fOutput;
				}
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CLogger *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


void CLogger::OnTrace(LPCTSTR pszOutput)
{
	AddLog(pszOutput);
}
