#include "stdafx.h"
#include <shlwapi.h>
#include <shlobj.h>
#include "TVTest.h"
#include "AppMain.h"
#include "RecordOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CRecordOptions::CRecordOptions()
{
	m_szSaveFolder[0]='\0';
	::lstrcpy(m_szFileName,TEXT("Record.ts"));
	m_fConfirmChannelChange=true;
	m_fConfirmExit=true;
	m_fConfirmStop=false;
	m_fConfirmStopStatusBarOnly=false;
	m_fCurServiceOnly=false;
	m_fSaveSubtitle=true;
	m_fSaveDataCarrousel=true;
	m_fDescrambleCurServiceOnly=false;
	m_BufferSize=0x100000;
}


CRecordOptions::~CRecordOptions()
{
}


bool CRecordOptions::Read(CSettings *pSettings)
{
	TCHAR szPath[MAX_PATH];

	// Backward compatibility
	if (pSettings->Read(TEXT("RecordFile"),szPath,lengthof(szPath))
			&& szPath[0]!='\0') {
		LPTSTR pszFileName=::PathFindFileName(szPath);

		if (pszFileName!=szPath) {
			*(pszFileName-1)='\0';
			::lstrcpy(m_szSaveFolder,szPath);
			::lstrcpy(m_szFileName,pszFileName);
		}
	}
	if (pSettings->Read(TEXT("RecordFolder"),szPath,lengthof(szPath))
			&& szPath[0]!='\0')
		::lstrcpy(m_szSaveFolder,szPath);
	if (pSettings->Read(TEXT("RecordFileName"),szPath,lengthof(szPath))
			&& szPath[0]!='\0')
		::lstrcpy(m_szFileName,szPath);
	// Backward compatibility
	bool fAddTime;
	if (pSettings->Read(TEXT("AddRecordTime"),&fAddTime) && fAddTime) {
		TCHAR szFormat[] = TEXT("%date%_%time%");
		if (::lstrlen(m_szFileName)+lengthof(szFormat)<=lengthof(m_szFileName)) {
			LPTSTR pszExt=::PathFindExtension(m_szFileName);
			TCHAR szExtension[MAX_PATH];
			::lstrcpy(szExtension,pszExt);
			::wsprintf(pszExt,TEXT("%s%s"),szFormat,szExtension);
		}
	}
	pSettings->Read(TEXT("ConfirmRecChChange"),&m_fConfirmChannelChange);
	pSettings->Read(TEXT("ConfrimRecordingExit"),&m_fConfirmExit);
	pSettings->Read(TEXT("ConfrimRecordStop"),&m_fConfirmStop);
	pSettings->Read(TEXT("ConfrimRecordStopStatusBarOnly"),&m_fConfirmStopStatusBarOnly);
	pSettings->Read(TEXT("RecordCurServiceOnly"),&m_fCurServiceOnly);
	pSettings->Read(TEXT("RecordSubtitle"),&m_fSaveSubtitle);
	pSettings->Read(TEXT("RecordDataCarrousel"),&m_fSaveDataCarrousel);
	pSettings->Read(TEXT("RecordDescrambleCurServiceOnly"),&m_fDescrambleCurServiceOnly);
	pSettings->Read(TEXT("RecordBufferSize"),&m_BufferSize);
	return true;
}


bool CRecordOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("RecordFolder"),m_szSaveFolder);
	pSettings->Write(TEXT("RecordFileName"),m_szFileName);
	pSettings->Write(TEXT("AddRecordTime"),false);	// Backward compatibility
	pSettings->Write(TEXT("ConfirmRecChChange"),m_fConfirmChannelChange);
	pSettings->Write(TEXT("ConfrimRecordingExit"),m_fConfirmExit);
	pSettings->Write(TEXT("ConfrimRecordStop"),m_fConfirmStop);
	pSettings->Write(TEXT("ConfrimRecordStopStatusBarOnly"),m_fConfirmStopStatusBarOnly);
	pSettings->Write(TEXT("RecordCurServiceOnly"),m_fCurServiceOnly);
	pSettings->Write(TEXT("RecordSubtitle"),m_fSaveSubtitle);
	pSettings->Write(TEXT("RecordDataCarrousel"),m_fSaveDataCarrousel);
	pSettings->Write(TEXT("RecordDescrambleCurServiceOnly"),m_fDescrambleCurServiceOnly);
	pSettings->Write(TEXT("RecordBufferSize"),m_BufferSize);
	return true;
}


bool CRecordOptions::SetSaveFolder(LPCTSTR pszFolder)
{
	::lstrcpy(m_szSaveFolder,pszFolder);
	return true;
}


bool CRecordOptions::GetFilePath(LPTSTR pszFileName,int MaxLength) const
{
	if (m_szSaveFolder[0]=='\0' || m_szFileName[0]=='\0')
		return false;
	if (::lstrlen(m_szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength)
		return false;
	::PathCombine(pszFileName,m_szSaveFolder,m_szFileName);
	return true;
}


bool CRecordOptions::GenerateFilePath(LPTSTR pszFileName,int MaxLength,LPCTSTR *ppszErrorMessage) const
{
	if (m_szSaveFolder[0]=='\0') {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("設定で保存先フォルダを指定してください。");
		return false;
	}
	if (!::PathIsDirectory(m_szSaveFolder)) {
		int Result=::SHCreateDirectoryEx(NULL,m_szSaveFolder,NULL);
		if (Result!=ERROR_SUCCESS && Result!=ERROR_ALREADY_EXISTS) {
			if (ppszErrorMessage)
				*ppszErrorMessage=TEXT("保存先フォルダが作成できません。");
			return false;
		}
	}
	if (m_szFileName[0]=='\0') {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("設定でファイル名を指定してください。");
		return false;
	}
	if (::lstrlen(m_szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength) {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("ファイルパスが長すぎます。");
		return false;
	}
	::lstrcpy(pszFileName,m_szSaveFolder);
	::PathAddBackslash(pszFileName);
	::lstrcat(pszFileName,m_szFileName);
	return true;
}


bool CRecordOptions::ConfirmChannelChange(HWND hwndOwner) const
{
	if (m_fConfirmChannelChange) {
		if (::MessageBox(hwndOwner,TEXT("録画中です。チャンネル変更しますか?"),
				TEXT("チャンネル変更の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmServiceChange(HWND hwndOwner,const CRecordManager *pRecordManager) const
{
	if (pRecordManager->GetCurServiceOnly()) {
		if (::MessageBox(hwndOwner,
				TEXT("現在のサービスのみ録画中です。\r\n")
				TEXT("サービスの変更をすると正常に再生できなくなるかも知れません。\r\n")
				TEXT("サービスを変更しますか?"),
				TEXT("変更の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmStop(HWND hwndOwner) const
{
	if (m_fConfirmStop && !m_fConfirmStopStatusBarOnly) {
		if (::MessageBox(hwndOwner,
				TEXT("録画を停止しますか?"),TEXT("停止の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmStatusBarStop(HWND hwndOwner) const
{
	if (m_fConfirmStop && m_fConfirmStopStatusBarOnly) {
		if (::MessageBox(hwndOwner,
				TEXT("録画を停止しますか?"),TEXT("停止の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmExit(HWND hwndOwner,const CRecordManager *pRecordManager) const
{
	if (m_fConfirmExit && pRecordManager->IsRecording()) {
		if (::MessageBox(hwndOwner,
				TEXT("現在録画中です。\r\n終了してもいいですか?"),
				TEXT("終了の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	if (pRecordManager->IsReserved()) {
		if (::MessageBox(hwndOwner,
				TEXT("録画の設定がされています。\r\n")
				TEXT("終了すると録画は行われません。\r\n")
				TEXT("終了してもいいですか?"),
				TEXT("終了の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ApplyOptions(CRecordManager *pManager)
{
	pManager->SetCurServiceOnly(m_fCurServiceOnly);
	DWORD Stream=CTsSelector::STREAM_ALL;
	if (!m_fSaveSubtitle)
		Stream^=CTsSelector::STREAM_SUBTITLE;
	if (!m_fSaveDataCarrousel)
		Stream^=CTsSelector::STREAM_DATACARROUSEL;
	pManager->SetSaveStream(Stream);
	pManager->SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
	pManager->SetBufferSize(m_BufferSize);
	return true;
}


CRecordOptions *CRecordOptions::GetThis(HWND hDlg)
{
	return static_cast<CRecordOptions*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CRecordOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CRecordOptions *pThis=dynamic_cast<CRecordOptions*>(OnInitDialog(hDlg,lParam));

			::SetProp(hDlg,TEXT("This"),pThis);
			/*
			if (pThis->m_szSaveFolder[0]=='\0') {
				if (!::SHGetSpecialFolderPath(hDlg,pThis->m_szSaveFolder,
											  CSIDL_MYVIDEO,FALSE)
						&& !::SHGetSpecialFolderPath(hDlg,pThis->m_szSaveFolder,
													 CSIDL_PERSONAL,FALSE))
					pThis->m_szSaveFolder[0]='\0';
			}
			*/
			::SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,pThis->m_szSaveFolder);
			::SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,pThis->m_szFileName);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE,
				pThis->m_fConfirmChannelChange?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMEXIT,
				pThis->m_fConfirmExit?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMSTOP,
				pThis->m_fConfirmStop?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,
				pThis->m_fConfirmStopStatusBarOnly?BST_CHECKED:BST_UNCHECKED);
			EnableDlgItem(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,pThis->m_fConfirmStop);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CURSERVICEONLY,
				pThis->m_fCurServiceOnly?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE,
				pThis->m_fSaveSubtitle?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
				pThis->m_fSaveDataCarrousel?BST_CHECKED:BST_UNCHECKED);
			/*
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE,
								IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
						   pThis->m_fCurServiceOnly);
			*/
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_DESCRAMBLECURSERVICEONLY,
				pThis->m_fDescrambleCurServiceOnly?BST_CHECKED:BST_UNCHECKED);
			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE,
							pThis->m_BufferSize/1024,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE_UD,
								 UDM_SETRANGE32,1,0x7FFFFFFF);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORDOPTIONS_SAVEFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,szFolder,MAX_PATH);
				if (BrowseFolderDialog(hDlg,szFolder,
										TEXT("録画ファイルの保存先フォルダ:")))
					::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,szFolder);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_FILENAME:
			if (HIWORD(wParam)==EN_CHANGE) {
				TCHAR szFormat[MAX_PATH],szFileName[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,szFormat,lengthof(szFormat));
				szFileName[0]='\0';
				if (szFormat[0]!='\0') {
					CRecordManager::EventInfo EventInfo;

					CRecordManager::GetEventInfoSample(&EventInfo);
					if (!GetAppClass().GetRecordManager()->GenerateFileName(
							szFileName,lengthof(szFileName),&EventInfo,szFormat))
						szFileName[0]='\0';
				}
				::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAMEPREVIEW,szFileName);
			}
			return TRUE;

		case IDC_RECORDOPTIONS_FILENAMEFORMAT:
			{
				RECT rc;
				POINT pt;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_RECORDOPTIONS_FILENAMEFORMAT),&rc);
				pt.x=rc.left;
				pt.y=rc.bottom;
				CRecordManager::InsertFileNameParameter(hDlg,IDC_RECORDOPTIONS_FILENAME,&pt);
			}
			return TRUE;

		/*
		case IDC_RECORDOPTIONS_CURSERVICEONLY:
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE,IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CURSERVICEONLY));
			return TRUE;
		*/

		case IDC_RECORDOPTIONS_CONFIRMSTOP:
			EnableDlgItem(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMSTOP));
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CRecordOptions *pThis=GetThis(hDlg);
				TCHAR szSaveFolder[MAX_PATH],szFileName[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,szSaveFolder,MAX_PATH);
				if (szSaveFolder[0]!='\0' && !::PathIsDirectory(szSaveFolder)) {
					TCHAR szMessage[MAX_PATH+80];

					::wsprintf(szMessage,
						TEXT("録画ファイルの保存先フォルダ \"%s\" がありません。\n")
						TEXT("作成しますか?"),szSaveFolder);
					if (::MessageBox(hDlg,szMessage,TEXT("フォルダ作成の確認"),
										MB_YESNO | MB_ICONQUESTION)==IDYES) {
						int Result;

						Result=::SHCreateDirectoryEx(hDlg,szSaveFolder,NULL);
						if (Result!=ERROR_SUCCESS
								&& Result!=ERROR_ALREADY_EXISTS) {
							pThis->SettingError();
							::MessageBox(hDlg,TEXT("フォルダが作成できません。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
							SetDlgItemFocus(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER);
							return TRUE;
						}
					}
				}
				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,szFileName,lengthof(szFileName));
				if (szFileName[0]!='\0') {
#if 0
					CFilePath FilePath(szFileName);
					if (!FilePath.IsValid()) {
						::MessageBox(hDlg,
							TEXT("録画ファイル名に、ファイル名に使用できない文字が含まれています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
					}
#else
					TCHAR szMessage[256];
					if (!IsValidFileName(szFileName,false,szMessage,lengthof(szMessage))) {
						pThis->SettingError();
						SetDlgItemFocus(hDlg,IDC_RECORDOPTIONS_FILENAME);
						SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_FILENAME,EM_SETSEL,0,-1);
						::MessageBox(hDlg,szMessage,NULL,MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
#endif
				}
				::lstrcpy(pThis->m_szSaveFolder,szSaveFolder);
				::lstrcpy(pThis->m_szFileName,szFileName);
				pThis->m_fConfirmChannelChange=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE)==BST_CHECKED;
				pThis->m_fConfirmExit=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMEXIT)==BST_CHECKED;
				pThis->m_fConfirmStop=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMSTOP)==BST_CHECKED;
				pThis->m_fConfirmStopStatusBarOnly=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP)==BST_CHECKED;
				pThis->m_fCurServiceOnly=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CURSERVICEONLY)==BST_CHECKED;
				pThis->m_fSaveSubtitle=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE);
				pThis->m_fSaveDataCarrousel=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVEDATACARROUSEL);
				pThis->m_fDescrambleCurServiceOnly=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_DESCRAMBLECURSERVICEONLY)==BST_CHECKED;
				pThis->m_BufferSize=::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE,NULL,FALSE)*1024;
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CRecordOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
