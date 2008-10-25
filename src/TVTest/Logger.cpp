#include "stdafx.h"
#include <commctrl.h>
#include <shlwapi.h>
#include "TVTest.h"
#include "Logger.h"
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




CLogger::CLogger()
{
	m_NumLogItems=0;
	m_ppList=NULL;
	m_ListLength=0;
}


CLogger::~CLogger()
{
	Clear();
}


bool CLogger::AddLog(LPCTSTR pszText, ...)
{
	if (m_NumLogItems==m_ListLength) {
		if (m_ListLength==0)
			m_ListLength=64;
		else
			m_ListLength*=2;
		m_ppList=static_cast<CLogItem**>(realloc(m_ppList,m_ListLength*sizeof(CLogItem*)));
	}

	va_list Args;
	TCHAR szText[1024];

	va_start(Args,pszText);
	_vsntprintf(szText,lengthof(szText),pszText,Args);
	m_ppList[m_NumLogItems++]=new CLogItem(szText);
	va_end(Args);
	return true;
}


void CLogger::Clear()
{
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


bool CLogger::SaveToFile(LPCTSTR pszFileName) const
{
	HANDLE hFile;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
					   FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	for (int i=0;i<m_NumLogItems;i++) {
		char szText[1024];
		SYSTEMTIME st;
		LPCTSTR pszText;
		DWORD Length,Write;

		m_ppList[i]->GetTime(&st);
		Length=::GetDateFormatA(LOCALE_USER_DEFAULT,DATE_SHORTDATE,&st,
											NULL,szText,lengthof(szText)-1);
		szText[Length-1]=' ';
		Length+=::GetTimeFormatA(LOCALE_USER_DEFAULT,TIME_FORCE24HOURFORMAT,&st,
								NULL,szText+Length,lengthof(szText)-Length);
		szText[Length-1]='>';
		pszText=m_ppList[i]->GetText();
		Length+=::WideCharToMultiByte(CP_ACP,0,pszText,::lstrlen(pszText),
							szText+Length,lengthof(szText)-Length-1,NULL,NULL);
		szText[Length++]='\r';
		szText[Length++]='\n';
		::WriteFile(hFile,szText,Length,&Write,NULL);
	}
	::CloseHandle(hFile);
	return true;
}


CLogger *CLogger::GetThis(HWND hDlg)
{
	return static_cast<CLogger*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CLogger::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CLogger *pThis=dynamic_cast<CLogger*>(OnInitDialog(hDlg,lParam));
			HWND hwndList=GetDlgItem(hDlg,IDC_LOG_LIST);
			LV_COLUMN lvc;
			LV_ITEM lvi;
			int i;

			ListView_SetExtendedListViewStyle(hwndList,LVS_EX_FULLROWSELECT);
			lvc.mask=LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
			lvc.fmt=LVCFMT_LEFT;
			lvc.cx=80;
			lvc.pszText=TEXT("時間");
			ListView_InsertColumn(hwndList,0,&lvc);
			lvc.pszText=TEXT("内容");
			ListView_InsertColumn(hwndList,1,&lvc);
			lvi.mask=LVIF_TEXT;
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

				::GetModuleFileName(NULL,szFileName,lengthof(szFileName));
				::PathRenameExtension(szFileName,TEXT(".log"));
				if (!pThis->SaveToFile(szFileName)) {
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

	case WM_DESTROY:
		{
			CLogger *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
