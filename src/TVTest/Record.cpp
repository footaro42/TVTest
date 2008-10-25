#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "Record.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CRecordTask::CRecordTask()
{
	m_State=STATE_STOP;
	m_pDtvEngine=NULL;
}


CRecordTask::~CRecordTask()
{
	Stop();
}


bool CRecordTask::Start(CDtvEngine *pDtvEngine,LPCTSTR pszFileName)
{
	if (m_State!=STATE_STOP)
		return false;
	if (!pDtvEngine->m_FileWriter.OpenFile(pszFileName,
												CNCachedFile::CNF_SHAREREAD))
		return false;
	m_State=STATE_RECORDING;
	m_pDtvEngine=pDtvEngine;
	m_StartTime=::GetTickCount();
	m_TotalPauseTime=0;
	return true;
}


bool CRecordTask::Stop()
{
	if (m_State==STATE_RECORDING || m_State==STATE_PAUSE) {
		m_pDtvEngine->m_FileWriter.CloseFile();
		m_State=STATE_STOP;
		m_pDtvEngine=NULL;
	} else
		return false;
	return true;
}


bool CRecordTask::Pause()
{
	if (m_State==STATE_RECORDING) {
		m_pDtvEngine->m_FileWriter.Pause();
		m_State=STATE_PAUSE;
		m_PauseStartTime=::GetTickCount();
	} else if (m_State==STATE_PAUSE) {
		m_pDtvEngine->m_FileWriter.Resume();
		m_State=STATE_RECORDING;
		m_TotalPauseTime+=DiffTime(m_PauseStartTime,::GetTickCount());
	} else
		return false;
	return true;
}


CRecordTask::State CRecordTask::GetState() const
{
	return m_State;
}


DWORD CRecordTask::GetStartTime() const
{
	if (m_State==STATE_STOP)
		return 0;
	return m_StartTime;
}


DWORD CRecordTask::GetRecordTime() const
{
	DWORD Time;

	if (m_State==STATE_RECORDING) {
		Time=DiffTime(m_StartTime,::GetTickCount());
	} else if (m_State==STATE_PAUSE) {
		Time=DiffTime(m_StartTime,m_PauseStartTime);
	} else
		return 0;
	return Time-m_TotalPauseTime;
}


LONGLONG CRecordTask::GetWroteSize() const
{
	if (m_State==STATE_STOP)
		return 0;
	return m_pDtvEngine->m_FileWriter.GetWriteSize();
}


LPCTSTR CRecordTask::GetFileName() const
{
	if (m_State==STATE_STOP)
		return NULL;
	return m_pDtvEngine->m_FileWriter.GetFileName();
}




CRecordOptions::CRecordOptions()
{
	m_szSaveFolder[0]='\0';
	::lstrcpy(m_szFileName,TEXT("Record.ts"));
	m_fAddTime=true;
	m_fConfirmChannelChange=true;
	m_fConfirmExit=true;
	m_fDescrambleCurServiceOnly=false;
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
	pSettings->Read(TEXT("AddRecordTime"),&m_fAddTime);
	pSettings->Read(TEXT("ConfirmRecChChange"),&m_fConfirmChannelChange);
	pSettings->Read(TEXT("ConfrimRecordingExit"),&m_fConfirmExit);
	pSettings->Read(TEXT("RecordDescrambleCurServiceOnly"),&m_fDescrambleCurServiceOnly);
	return true;
}


bool CRecordOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("RecordFolder"),m_szSaveFolder);
	pSettings->Write(TEXT("RecordFileName"),m_szFileName);
	pSettings->Write(TEXT("AddRecordTime"),m_fAddTime);
	pSettings->Write(TEXT("ConfirmRecChChange"),m_fConfirmChannelChange);
	pSettings->Write(TEXT("ConfrimRecordingExit"),m_fConfirmExit);
	pSettings->Write(TEXT("RecordDescrambleCurServiceOnly"),m_fDescrambleCurServiceOnly);
	return true;
}


bool CRecordOptions::GenerateFileName(LPTSTR pszFileName,int MaxLength,SYSTEMTIME *pTime,LPCTSTR *ppszErrorMessage) const
{
	if (m_szSaveFolder[0]=='\0') {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("設定で保存先フォルダを指定してください。");
		return false;
	}
	if (!::PathIsDirectory(m_szSaveFolder)) {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("保存先フォルダが見付かりません。");
		return false;
	}
	if (m_szFileName[0]=='\0') {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("設定でファイル名を指定してください。");
		return false;
	}
	if (::lstrlen(m_szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength) {
		if (ppszErrorMessage)
			*ppszErrorMessage=TEXT("ファイル名が長すぎます。");
		return false;
	}
	::PathCombine(pszFileName,m_szSaveFolder,m_szFileName);
	if (m_fAddTime) {
		SYSTEMTIME st;

		if (pTime==NULL) {
			::GetLocalTime(&st);
			pTime=&st;
		}
		if (::lstrlen(pszFileName)+15>=MaxLength) {
			if (ppszErrorMessage)
				*ppszErrorMessage=TEXT("ファイル名が長すぎて日時を付加できません。");
			return false;
		}
		::wsprintf(::PathFindExtension(pszFileName),
			TEXT("%04d%02d%02d_%02d%02d%02d%s"),
			pTime->wYear,pTime->wMonth,pTime->wDay,
			pTime->wHour,pTime->wMinute,pTime->wSecond,
			::PathFindExtension(m_szFileName));
	}
	if (::PathFileExists(pszFileName)) {
		LPTSTR p=::PathFindExtension(pszFileName);
		LPTSTR pszExtension=::PathFindExtension(m_szFileName);

		for (int i=0;;i++) {
			::wsprintf(p,TEXT("-%d%s"),i+1,pszExtension);
			if (!::PathFileExists(pszFileName))
				break;
		}
	}
	return true;
}


bool CRecordOptions::ConfirmChannelChange(HWND hwndOwner)
{
	if (m_fConfirmChannelChange) {
		if (::MessageBox(hwndOwner,TEXT("録画中です。チャンネル変更しますか?"),
				TEXT("チャンネル変更の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
	return true;
}


bool CRecordOptions::ConfirmExit(HWND hwndOwner)
{
	if (m_fConfirmExit) {
		if (::MessageBox(hwndOwner,
				TEXT("現在録画中です。\r\n終了してもいいですか?"),
				TEXT("終了の確認"),
				MB_OKCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION)!=IDOK)
			return false;
	}
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
			if (pThis->m_szSaveFolder[0]=='\0') {
				::GetModuleFileName(NULL,pThis->m_szSaveFolder,lengthof(pThis->m_szSaveFolder));
				*(::PathFindFileName(pThis->m_szSaveFolder)-1)='\0';
			}
			::SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,pThis->m_szSaveFolder);
			::SendDlgItemMessage(hDlg,IDC_RECORDOPTIONS_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,pThis->m_szFileName);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_ADDTIME,
				pThis->m_fAddTime?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE,
				pThis->m_fConfirmChannelChange?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CONFIRMEXIT,
				pThis->m_fConfirmExit?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_DESCRAMBLECURSERVICEONLY,
				pThis->m_fDescrambleCurServiceOnly?BST_CHECKED:BST_UNCHECKED);
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
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CRecordOptions *pThis=GetThis(hDlg);
				TCHAR szPath[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_SAVEFOLDER,szPath,MAX_PATH);
				if (!::PathIsDirectory(szPath)) {
					TCHAR szMessage[MAX_PATH+64];

					::wsprintf(szMessage,
						TEXT("録画ファイルの保存先フォルダ \"%s\" がありません。\r\n")
						TEXT("作成しますか?"),szPath);
					if (::MessageBox(hDlg,szMessage,TEXT("フォルダ作成の確認"),
										MB_YESNO | MB_ICONQUESTION)==IDYES) {
						int Result;

						Result=::SHCreateDirectoryEx(hDlg,szPath,NULL);
						if (Result!=ERROR_SUCCESS && Result!=ERROR_ALREADY_EXISTS)
							::MessageBox(hDlg,TEXT("フォルダが作成できません。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
					}
				}
				::lstrcpy(pThis->m_szSaveFolder,szPath);
				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,
					pThis->m_szFileName,lengthof(pThis->m_szFileName));
				pThis->m_fAddTime=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_ADDTIME)==BST_CHECKED;
				pThis->m_fConfirmChannelChange=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE)==BST_CHECKED;
				pThis->m_fConfirmExit=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMEXIT)==BST_CHECKED;
				pThis->m_fDescrambleCurServiceOnly=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_DESCRAMBLECURSERVICEONLY)==BST_CHECKED;
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




CRecordManager::CRecordManager()
{
	m_szFileName[0]='\0';
	m_ExistsOperation=EXISTS_CONFIRM;
	m_fStopTimeSpec=false;
	m_fStopDateTime=false;
	m_StopTime=60*60*1000;
	m_fRecording=false;
	m_pRecordTask=NULL;
	m_fDescrambleCurServiceOnly=false;
}


CRecordManager::~CRecordManager()
{
	StopRecord();
}


bool CRecordManager::SetFileName(LPCTSTR pszFileName)
{
	if (lstrlen(pszFileName)>=MAX_PATH)
		return false;
	lstrcpy(m_szFileName,pszFileName);
	return true;
}


bool CRecordManager::SetFileExistsOperation(FileExistsOperation Operation)
{
	m_ExistsOperation=Operation;
	return true;
}


bool CRecordManager::SetStopTimeSpec(bool fSpec)
{
	m_fStopTimeSpec=fSpec;
	return true;
}


bool CRecordManager::SetStopTime(unsigned int Time)
{
	m_fStopDateTime=false;
	m_StopTime=Time;
	return true;
}


bool CRecordManager::StartRecord(CDtvEngine *pDtvEngine,LPCTSTR pszFileName)
{
	SYSTEMTIME st;
	FILETIME ft;

	if (m_fRecording)
		return false;
	m_pRecordTask=new CRecordTask;
	if (!m_pRecordTask->Start(pDtvEngine,pszFileName)) {
		delete m_pRecordTask;
		m_pRecordTask=NULL;
		return false;
	}
	pDtvEngine->SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
	::GetLocalTime(&st);
	::SystemTimeToFileTime(&st,&m_ftStartTime);
	m_fRecording=true;
	return true;
}


void CRecordManager::StopRecord()
{
	if (m_fRecording) {
		m_pRecordTask->Stop();
		m_fRecording=false;
		delete m_pRecordTask;
		m_pRecordTask=NULL;
	}
}


bool CRecordManager::PauseRecord()
{
	if (!m_fRecording)
		return false;
	return m_pRecordTask->Pause();
}


bool CRecordManager::IsPaused() const
{
	return m_fRecording && m_pRecordTask->IsPaused();
}


DWORD CRecordManager::GetRecordTime() const
{
	if (!m_fRecording)
		return 0;
	return m_pRecordTask->GetRecordTime();
}


bool CRecordManager::QueryStop() const
{
	if (!m_fRecording || !m_fStopTimeSpec)
		return false;
	if (m_fStopDateTime) {
		SYSTEMTIME st;
		FILETIME ft;

		GetLocalTime(&st);
		SystemTimeToFileTime(&st,&ft);
		if (CompareFileTime(&ft,&m_ftStopTime)>=0)
			return true;
	} else {
		if (DiffTime(m_pRecordTask->GetStartTime(),GetTickCount())>=m_StopTime)
			return true;
	}
	return false;
}


static void SetDateTimeFormat(HWND hDlg,UINT ID)
{
	int Length;
	TCHAR szText[256];

	GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_SSHORTDATE,szText,
														lengthof(szText)-1);
	Length=lstrlen(szText);
	szText[Length++]=' ';
	GetLocaleInfo(LOCALE_USER_DEFAULT,LOCALE_STIMEFORMAT,
										szText+Length,lengthof(szText)-Length);
	DateTime_SetFormat(GetDlgItem(hDlg,ID),szText);
}


BOOL CALLBACK CRecordManager::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static CRecordManager *pThis;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			int i;

			pThis=reinterpret_cast<CRecordManager*>(lParam);
			SendDlgItemMessage(hDlg,IDC_RECORD_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			SetDlgItemText(hDlg,IDC_RECORD_FILENAME,pThis->m_szFileName);
			static const LPCTSTR pszExistsOperation[] = {
				TEXT("上書きする"),TEXT("確認を取る"),TEXT("連番を付加する")
			};
			for (i=0;i<3;i++)
				SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_ADDSTRING,0,
							reinterpret_cast<LPARAM>(pszExistsOperation[i]));
			SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_SETCURSEL,
										(WPARAM)pThis->m_ExistsOperation,0);
			CheckDlgButton(hDlg,IDC_RECORD_STOPSPECTIME,
							pThis->m_fStopTimeSpec?BST_CHECKED:BST_UNCHECKED);
			CheckRadioButton(hDlg,IDC_RECORD_STOPDATETIME,
													IDC_RECORD_STOPREMAINTIME,
				pThis->m_fStopDateTime?
							IDC_RECORD_STOPDATETIME:IDC_RECORD_STOPREMAINTIME);
			if (pThis->m_fStopDateTime) {
				SYSTEMTIME st;
				FILETIME ft;
				ULARGE_INTEGER Time1,Time2;

				GetLocalTime(&st);
				SystemTimeToFileTime(&st,&ft);
				Time1.LowPart=pThis->m_ftStopTime.dwLowDateTime;
				Time1.HighPart=pThis->m_ftStopTime.dwHighDateTime;
				Time2.LowPart=ft.dwLowDateTime;
				Time2.HighPart=ft.dwHighDateTime;
				if (Time1.QuadPart>Time2.QuadPart)
					pThis->m_StopTime=
						(DWORD)((Time1.QuadPart-Time2.QuadPart)/10000);
				else
					pThis->m_StopTime=0;
			} else {
				SYSTEMTIME st;
				FILETIME ft;
				ULARGE_INTEGER Time;

				GetLocalTime(&st);
				st.wSecond=0;
				SystemTimeToFileTime(&st,&ft);
				Time.LowPart=ft.dwLowDateTime;
				Time.HighPart=ft.dwHighDateTime;
				Time.QuadPart+=(ULONGLONG)10000*pThis->m_StopTime;
				pThis->m_ftStopTime.dwLowDateTime=Time.LowPart;
				pThis->m_ftStopTime.dwHighDateTime=Time.HighPart;
			}
			SetDateTimeFormat(hDlg,IDC_RECORD_STOPTIME_TIME);
			SYSTEMTIME st;
			FileTimeToSystemTime(&pThis->m_ftStopTime,&st);
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),
																GDT_VALID,&st);
			SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,
										pThis->m_StopTime/(60*60*1000),FALSE);
			SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_HOUR_UD,UDM_SETRANGE,
														0,MAKELPARAM(100,0));
			SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,
										pThis->m_StopTime/(60*1000)%60,FALSE);
			SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_MINUTE_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,
											pThis->m_StopTime/1000%60,FALSE);
			SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_SECOND_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			if (!pThis->m_fStopTimeSpec) {
				EnableDlgItems(hDlg,IDC_RECORD_STOPDATETIME,
									IDC_RECORD_STOPTIME_SECOND_LABEL,false);
			} else {
				EnableDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME,
													pThis->m_fStopDateTime);
				EnableDlgItems(hDlg,IDC_RECORD_STOPTIME_HOUR,
					IDC_RECORD_STOPTIME_SECOND_LABEL,!pThis->m_fStopDateTime);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORD_FILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH];

				GetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFileName,MAX_PATH);
				ofn.lStructSize=sizeof(OPENFILENAME);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("TSファイル(*.ts)\0*.ts\0すべてのファイル\0*.*\0");
				ofn.lpstrCustomFilter=NULL;
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=MAX_PATH;
				ofn.lpstrFileTitle=NULL;
				ofn.lpstrInitialDir=NULL;
				ofn.lpstrTitle=TEXT("保存ファイル名");
				ofn.Flags=OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				ofn.lpstrDefExt=TEXT("ts");
#if _WIN32_WINNT>=0x500
				ofn.pvReserved=NULL;
				ofn.dwReserved=0;
				ofn.FlagsEx=0;
#endif
				if (GetSaveFileName(&ofn))
					SetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFileName);
			}
			return TRUE;

		case IDC_RECORD_STOPSPECTIME:
		case IDC_RECORD_STOPDATETIME:
		case IDC_RECORD_STOPREMAINTIME:
			if (IsDlgButtonChecked(hDlg,IDC_RECORD_STOPSPECTIME)==BST_CHECKED) {
				bool fDateTime=IsDlgButtonChecked(hDlg,
										IDC_RECORD_STOPDATETIME)==BST_CHECKED;

				EnableDlgItems(hDlg,IDC_RECORD_STOPDATETIME,
									IDC_RECORD_STOPREMAINTIME,TRUE);
				EnableDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME,fDateTime);
				EnableDlgItems(hDlg,IDC_RECORD_STOPTIME_HOUR,
									IDC_RECORD_STOPTIME_SECOND_LABEL,!fDateTime);
			} else {
				EnableDlgItems(hDlg,IDC_RECORD_STOPDATETIME,
									IDC_RECORD_STOPTIME_SECOND_LABEL,false);
			}
			return TRUE;

		case IDOK:
		case IDCANCEL:
			if (GetWindowTextLength(GetDlgItem(hDlg,IDC_RECORD_FILENAME))==0) {
				MessageBox(hDlg,TEXT("ファイル名を入力してください。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(hDlg,IDC_RECORD_FILENAME));
				return TRUE;
			}
			{
				SYSTEMTIME st,stUTC,stLocal;
				FILETIME ft,ftCur;

				DateTime_GetSystemtime(
								GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),&st);
				SystemTimeToFileTime(&st,&ft);
				GetSystemTime(&stUTC);
				SystemTimeToTzSpecificLocalTime(NULL,&stUTC,&stLocal);
				SystemTimeToFileTime(&stLocal,&ftCur);
				if (CompareFileTime(&ft,&ftCur)<=0
						&& IsDlgButtonChecked(hDlg,
									IDC_RECORD_STOPSPECTIME)==BST_CHECKED
						&& IsDlgButtonChecked(hDlg,
									IDC_RECORD_STOPDATETIME)==BST_CHECKED) {
					MessageBox(hDlg,
						TEXT("指定された停止時間を既に過ぎています。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
					SetFocus(GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME));
					return TRUE;
				}
				pThis->m_ftStopTime=ft;
			}
			GetDlgItemText(hDlg,IDC_RECORD_FILENAME,pThis->m_szFileName,
																	MAX_PATH);
			pThis->m_ExistsOperation=(FileExistsOperation)
				SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_GETCURSEL,0,0);
			pThis->m_fStopTimeSpec=IsDlgButtonChecked(hDlg,
										IDC_RECORD_STOPSPECTIME)==BST_CHECKED;
			pThis->m_fStopDateTime=IsDlgButtonChecked(hDlg,
										IDC_RECORD_STOPDATETIME)==BST_CHECKED;
			{
				unsigned int Hour,Minute,Second;

				Hour=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,NULL,FALSE);
				Minute=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,NULL,FALSE);
				Second=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,NULL,FALSE);
				pThis->m_StopTime=Hour*(60*60*1000)+Minute*(60*1000)+Second*1000;
			}
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;
	}
	return FALSE;
}


bool CRecordManager::RecordDialog(HWND hwndOwner,HINSTANCE hinst)
{
	return DialogBoxParam(hinst,MAKEINTRESOURCE(IDD_RECORDOPTION),hwndOwner,
								DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}


bool CRecordManager::DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName)
{
	lstrcpy(pszFileName,m_szFileName);
	switch (m_ExistsOperation) {
	case EXISTS_CONFIRM:
		if (PathFileExists(m_szFileName)
				&& MessageBox(hwndOwner,
					TEXT("ファイルが既に存在します。\n上書きしますか?"),
					TEXT("上書きの確認"),MB_OKCANCEL | MB_ICONQUESTION)!=IDOK)
			return false;
		break;
	case EXISTS_SEQUENCIALNUMBER:
		if (PathFileExists(m_szFileName)) {
			int i;
			TCHAR szFileName[MAX_PATH];
			LPTSTR pszExtension,p;
			int ExtensionOffset;

			pszExtension=PathFindExtension(m_szFileName);
			lstrcpy(szFileName,m_szFileName);
			p=PathFindExtension(szFileName);
			for (i=0;;i++) {
				wsprintf(p,TEXT("%d%s"),i+1,pszExtension);
				if (!PathFileExists(szFileName))
					break;
			}
			lstrcpy(pszFileName,szFileName);
		}
		break;
	}
	return true;
}


bool CRecordManager::SetDescrambleCurServiceOnly(bool fOnly)
{
	m_fDescrambleCurServiceOnly=fOnly;
	return true;
}


BOOL CALLBACK CRecordManager::StopTimeDlgProc(HWND hDlg,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	static CRecordManager *pThis;

	switch (uMsg) {
	case WM_INITDIALOG:
		{
			SYSTEMTIME st;

			pThis=reinterpret_cast<CRecordManager*>(lParam);
			CheckDlgButton(hDlg,IDC_RECORDSTOPTIME_ENABLE,
							pThis->m_fStopTimeSpec?BST_CHECKED:BST_UNCHECKED);
			EnableDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME,pThis->m_fStopTimeSpec);
			SetDateTimeFormat(hDlg,IDC_RECORDSTOPTIME_TIME);
			if (pThis->m_fStopDateTime) {
				FileTimeToSystemTime(&pThis->m_ftStopTime,&st);
			} else {
				ULARGE_INTEGER Time;
				FILETIME ft;

				Time.LowPart=pThis->m_ftStartTime.dwLowDateTime;
				Time.HighPart=pThis->m_ftStartTime.dwHighDateTime;
				Time.QuadPart+=(ULONGLONG)10000*pThis->m_StopTime;
				ft.dwLowDateTime=Time.LowPart;
				ft.dwHighDateTime=Time.HighPart;
				FileTimeToSystemTime(&ft,&st);
			}
			DateTime_SetSystemtime(GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME),
																GDT_VALID,&st);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORDSTOPTIME_ENABLE:
			EnableWindow(GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME),
				IsDlgButtonChecked(hDlg,IDC_RECORDSTOPTIME_ENABLE)==
																BST_CHECKED);
			return TRUE;

		case IDOK:
			{
				bool fStopTimeSpec;
				SYSTEMTIME st;

				fStopTimeSpec=IsDlgButtonChecked(hDlg,
									IDC_RECORDSTOPTIME_ENABLE)==BST_CHECKED;
				if (DateTime_GetSystemtime(
						GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME),&st)!=
																GDT_VALID) {
					MessageBox(hDlg,TEXT("時間の取得エラー。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				FILETIME ft,ftCur;
				ULARGE_INTEGER Time1,Time2;

				SystemTimeToFileTime(&st,&ft);
				GetLocalTime(&st);
				SystemTimeToFileTime(&st,&ftCur);
				if (pThis->m_fStopTimeSpec) {
					if (CompareFileTime(&ft,&ftCur)<=0) {
						MessageBox(hDlg,
								TEXT("指定された停止時間を既に過ぎています。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
				}
				pThis->m_fStopTimeSpec=fStopTimeSpec;
				pThis->m_fStopDateTime=true;
				pThis->m_ftStopTime=ft;
				Time1.LowPart=ftCur.dwLowDateTime;
				Time1.HighPart=ftCur.dwHighDateTime;
				Time2.LowPart=ft.dwLowDateTime;
				Time2.HighPart=ft.dwHighDateTime;
				pThis->m_StopTime=GetTickCount()+
										(DWORD)(Time2.QuadPart-Time1.QuadPart);
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;
	}
	return FALSE;
}


bool CRecordManager::ChangeStopTimeDialog(HWND hwndOwner,HINSTANCE hinst)
{
	return DialogBoxParam(hinst,MAKEINTRESOURCE(IDD_RECORDSTOPTIME),hwndOwner,
						StopTimeDlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}
