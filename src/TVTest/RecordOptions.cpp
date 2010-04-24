#include "stdafx.h"
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


// 書き出しバッファサイズの制限(バイト単位)
#define MIN_BUFFER_SIZE	1024
#define MAX_BUFFER_SIZE	(32*1024*1024)

// さかのぼり録画バッファサイズの制限(MiB単位)
#define MIN_TIMESHIFT_BUFFER_SIZE	1
#define MAX_TIMESHIFT_BUFFER_SIZE	1024




CRecordOptions::CRecordOptions()
	: m_fConfirmChannelChange(true)
	, m_fConfirmExit(true)
	, m_fConfirmStop(false)
	, m_fConfirmStopStatusBarOnly(false)
	, m_fCurServiceOnly(false)
	, m_fSaveSubtitle(true)
	, m_fSaveDataCarrousel(true)
	, m_fDescrambleCurServiceOnly(false)
	, m_fAlertLowFreeSpace(true)
	, m_LowFreeSpaceThreshold(2048)
	, m_BufferSize(0x100000)
	, m_TimeShiftBufferSize(32)
	, m_fEnableTimeShiftRecording(false)
{
	m_szSaveFolder[0]='\0';
	::lstrcpy(m_szFileName,TEXT("Record_%date%-%time%.ts"));
}


CRecordOptions::~CRecordOptions()
{
}


bool CRecordOptions::Apply(DWORD Flags)
{
	CDtvEngine &DtvEngine=GetAppClass().GetCoreEngine()->m_DtvEngine;

	if ((Flags&UPDATE_RECORDSTREAM)!=0
			&& !GetAppClass().GetRecordManager()->IsRecording()) {
		DWORD Stream=CTsSelector::STREAM_ALL;
		if (!m_fSaveSubtitle)
			Stream^=CTsSelector::STREAM_CAPTION;
		if (!m_fSaveDataCarrousel)
			Stream^=CTsSelector::STREAM_DATACARROUSEL;
		DtvEngine.SetWriteCurServiceOnly(m_fCurServiceOnly,Stream);
		DtvEngine.m_FileWriter.ClearQueue();
	}
	if ((Flags&UPDATE_TIMESHIFTBUFFER)!=0)
		DtvEngine.m_FileWriter.SetQueueSize(0x100000,m_TimeShiftBufferSize);
	if ((Flags&UPDATE_ENABLETIMESHIFT)!=0)
		DtvEngine.m_FileWriter.EnableQueueing(m_fEnableTimeShiftRecording);
	return true;
}


bool CRecordOptions::Read(CSettings *pSettings)
{
	TCHAR szPath[MAX_PATH];
	unsigned int Value;

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
#if 0
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
#endif
	pSettings->Read(TEXT("ConfirmRecChChange"),&m_fConfirmChannelChange);
	pSettings->Read(TEXT("ConfrimRecordingExit"),&m_fConfirmExit);
	pSettings->Read(TEXT("ConfrimRecordStop"),&m_fConfirmStop);
	pSettings->Read(TEXT("ConfrimRecordStopStatusBarOnly"),&m_fConfirmStopStatusBarOnly);
	pSettings->Read(TEXT("RecordCurServiceOnly"),&m_fCurServiceOnly);
	pSettings->Read(TEXT("RecordSubtitle"),&m_fSaveSubtitle);
	pSettings->Read(TEXT("RecordDataCarrousel"),&m_fSaveDataCarrousel);
	pSettings->Read(TEXT("RecordDescrambleCurServiceOnly"),&m_fDescrambleCurServiceOnly);
	if (pSettings->Read(TEXT("RecordBufferSize"),&Value))
		m_BufferSize=CLAMP(Value,MIN_BUFFER_SIZE,MAX_BUFFER_SIZE);
	pSettings->Read(TEXT("AlertLowFreeSpace"),&m_fAlertLowFreeSpace);
	pSettings->Read(TEXT("LowFreeSpaceThreshold"),&m_LowFreeSpaceThreshold);
	if (pSettings->Read(TEXT("TimeShiftRecBufferSize"),&Value))
		m_TimeShiftBufferSize=CLAMP(Value,MIN_TIMESHIFT_BUFFER_SIZE,MAX_TIMESHIFT_BUFFER_SIZE);
	pSettings->Read(TEXT("TimeShiftRecording"),&m_fEnableTimeShiftRecording);
	return true;
}


bool CRecordOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("RecordFolder"),m_szSaveFolder);
	pSettings->Write(TEXT("RecordFileName"),m_szFileName);
	pSettings->Write(TEXT("ConfirmRecChChange"),m_fConfirmChannelChange);
	pSettings->Write(TEXT("ConfrimRecordingExit"),m_fConfirmExit);
	pSettings->Write(TEXT("ConfrimRecordStop"),m_fConfirmStop);
	pSettings->Write(TEXT("ConfrimRecordStopStatusBarOnly"),m_fConfirmStopStatusBarOnly);
	pSettings->Write(TEXT("RecordCurServiceOnly"),m_fCurServiceOnly);
	pSettings->Write(TEXT("RecordSubtitle"),m_fSaveSubtitle);
	pSettings->Write(TEXT("RecordDataCarrousel"),m_fSaveDataCarrousel);
	pSettings->Write(TEXT("RecordDescrambleCurServiceOnly"),m_fDescrambleCurServiceOnly);
	pSettings->Write(TEXT("RecordBufferSize"),m_BufferSize);
	pSettings->Write(TEXT("AlertLowFreeSpace"),m_fAlertLowFreeSpace);
	pSettings->Write(TEXT("LowFreeSpaceThreshold"),m_LowFreeSpaceThreshold);
	pSettings->Write(TEXT("TimeShiftRecBufferSize"),m_TimeShiftBufferSize);
	pSettings->Write(TEXT("TimeShiftRecording"),m_fEnableTimeShiftRecording);
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


bool CRecordOptions::EnableTimeShiftRecording(bool fEnable)
{
	if (m_fEnableTimeShiftRecording!=fEnable) {
		if (!GetAppClass().GetCoreEngine()->m_DtvEngine.m_FileWriter.EnableQueueing(fEnable))
			return false;
		m_fEnableTimeShiftRecording=fEnable;
	}
	return true;
}


bool CRecordOptions::ApplyOptions(CRecordManager *pManager)
{
	pManager->SetCurServiceOnly(m_fCurServiceOnly);
	DWORD Stream=CTsSelector::STREAM_ALL;
	if (!m_fSaveSubtitle)
		Stream^=CTsSelector::STREAM_CAPTION;
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


INT_PTR CALLBACK CRecordOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_DESCRAMBLECURSERVICEONLY,
				pThis->m_fDescrambleCurServiceOnly?BST_CHECKED:BST_UNCHECKED);
			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE,
							pThis->m_BufferSize/1024,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE_UD,
							   MIN_BUFFER_SIZE/1024,MAX_BUFFER_SIZE/1024);

			DlgCheckBox_Check(hDlg,IDC_RECORDOPTIONS_ALERTLOWFREESPACE,pThis->m_fAlertLowFreeSpace);
			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD,pThis->m_LowFreeSpaceThreshold,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_SPIN,
							   1,0x7FFFFFFF);
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_LABEL,
								IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_UNIT,
						   pThis->m_fAlertLowFreeSpace);

			::SetDlgItemInt(hDlg,IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE,pThis->m_TimeShiftBufferSize,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE_SPIN,
							   MIN_TIMESHIFT_BUFFER_SIZE,MAX_TIMESHIFT_BUFFER_SIZE);
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

		case IDC_RECORDOPTIONS_CONFIRMSTOP:
			EnableDlgItem(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMSTOP));
			return TRUE;

		case IDC_RECORDOPTIONS_ALERTLOWFREESPACE:
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_LABEL,
								IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD_UNIT,
						   DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_ALERTLOWFREESPACE));
			return TRUE;
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
				pThis->m_fConfirmChannelChange=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE);
				pThis->m_fConfirmExit=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMEXIT);
				pThis->m_fConfirmStop=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMSTOP);
				pThis->m_fConfirmStopStatusBarOnly=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CONFIRMSTATUSBARSTOP);

				bool fOptionChanged=false;
				bool f=DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CURSERVICEONLY);
				if (pThis->m_fCurServiceOnly!=f) {
					pThis->m_fCurServiceOnly=f;
					fOptionChanged=true;
				}
				f=DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE);
				if (pThis->m_fSaveSubtitle!=f) {
					pThis->m_fSaveSubtitle=f;
					fOptionChanged=true;
				}
				f=DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVEDATACARROUSEL);
				if (pThis->m_fSaveDataCarrousel!=f) {
					pThis->m_fSaveDataCarrousel=f;
					fOptionChanged=true;
				}
				if (fOptionChanged)
					pThis->SetUpdateFlag(UPDATE_RECORDSTREAM);

				pThis->m_fDescrambleCurServiceOnly=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_DESCRAMBLECURSERVICEONLY);
				unsigned int BufferSize=
					::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_BUFFERSIZE,NULL,FALSE)*1024;
				pThis->m_BufferSize=CLAMP(BufferSize,MIN_BUFFER_SIZE,MAX_BUFFER_SIZE);

				pThis->m_fAlertLowFreeSpace=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_ALERTLOWFREESPACE);
				pThis->m_LowFreeSpaceThreshold=
					::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_LOWFREESPACETHRESHOLD,NULL,FALSE);

				BufferSize=::GetDlgItemInt(hDlg,IDC_RECORDOPTIONS_TIMESHIFTBUFFERSIZE,NULL,FALSE);
				BufferSize=CLAMP(BufferSize,MIN_TIMESHIFT_BUFFER_SIZE,MAX_TIMESHIFT_BUFFER_SIZE);
				if (BufferSize!=pThis->m_TimeShiftBufferSize) {
					pThis->m_TimeShiftBufferSize=BufferSize;
					pThis->SetUpdateFlag(UPDATE_TIMESHIFTBUFFER);
				}
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
