#include "stdafx.h"
#include <shlwapi.h>
#include <shlobj.h>
#include "TVTest.h"
#include "AppMain.h"
#include "Record.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CRecordTime::CRecordTime()
{
	Clear();
}


bool CRecordTime::SetCurrentTime()
{
	SYSTEMTIME st;

	::GetLocalTime(&st);
	::SystemTimeToFileTime(&st,&m_Time);
	m_TickTime=::GetTickCount();
	return true;
}


bool CRecordTime::GetTime(FILETIME *pTime) const
{
	if (!IsValid())
		return false;
	*pTime=m_Time;
	return true;
}


void CRecordTime::Clear()
{
	::ZeroMemory(&m_Time,sizeof(m_Time));
	m_TickTime=0;
}


bool CRecordTime::IsValid() const
{
	if (m_Time.dwLowDateTime==0 && m_Time.dwHighDateTime==0)
		return false;
	return true;
}




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
	m_StartTime.SetCurrentTime();
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
	return m_StartTime.GetTickTime();
}


bool CRecordTask::GetStartTime(FILETIME *pTime) const
{
	if (m_State==STATE_STOP)
		return false;
	return m_StartTime.GetTime(pTime);
}


bool CRecordTask::GetStartTime(CRecordTime *pTime) const
{
	if (m_State==STATE_STOP)
		return false;
	*pTime=m_StartTime;
	return true;
}


DWORD CRecordTask::GetRecordTime() const
{
	DWORD Time;

	if (m_State==STATE_RECORDING) {
		Time=DiffTime(m_StartTime.GetTickTime(),::GetTickCount());
	} else if (m_State==STATE_PAUSE) {
		Time=DiffTime(m_StartTime.GetTickTime(),m_PauseStartTime);
	} else
		return 0;
	return Time-m_TotalPauseTime;
}


DWORD CRecordTask::GetPauseTime() const
{
	if (m_State==STATE_RECORDING)
		return m_TotalPauseTime;
	if (m_State==STATE_PAUSE)
		return DiffTime(m_PauseStartTime,::GetTickCount())+m_TotalPauseTime;
	return 0;
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




CRecordManager::CRecordManager()
{
	m_fRecording=false;
	m_fReserved=false;
	m_pszFileName=NULL;
	m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	m_StopTimeSpec.Type=TIME_NOTSPECIFIED;
	m_pDtvEngine=NULL;
	m_ExistsOperation=EXISTS_CONFIRM;
	m_fCurServiceOnly=false;
	m_SaveStream=CTsSelector::STREAM_MPEG2VIDEO | CTsSelector::STREAM_AAC |
				 CTsSelector::STREAM_SUBTITLE;
	m_fDescrambleCurServiceOnly=false;
}


CRecordManager::~CRecordManager()
{
	StopRecord();
	delete [] m_pszFileName;
}


bool CRecordManager::SetFileName(LPCTSTR pszFileName)
{
	if (m_fRecording)
		return false;
	return ReplaceString(&m_pszFileName,pszFileName);
}


bool CRecordManager::SetFileExistsOperation(FileExistsOperation Operation)
{
	if (m_fRecording)
		return false;
	m_ExistsOperation=Operation;
	return true;
}


bool CRecordManager::GetStartTime(FILETIME *pTime) const
{
	if (!m_fRecording)
		return false;
	return m_RecordTask.GetStartTime(pTime);
}


bool CRecordManager::GetReserveTime(FILETIME *pTime) const
{
	if (!m_fReserved)
		return false;
	return m_ReserveTime.GetTime(pTime);
}


bool CRecordManager::SetStartTimeSpec(const TimeSpecInfo *pInfo)
{
	if (m_fRecording)
		return false;
	if (pInfo!=NULL && pInfo->Type!=TIME_NOTSPECIFIED) {
		m_fReserved=true;
		m_ReserveTime.SetCurrentTime();
		m_StartTimeSpec=*pInfo;
	} else {
		m_fReserved=false;
		m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	}
	return true;
}


bool CRecordManager::GetStartTimeSpec(TimeSpecInfo *pInfo) const
{
	*pInfo=m_StartTimeSpec;
	return true;
}


bool CRecordManager::SetStopTimeSpec(const TimeSpecInfo *pInfo)
{
	if (pInfo!=NULL)
		m_StopTimeSpec=*pInfo;
	else
		m_StopTimeSpec.Type=TIME_NOTSPECIFIED;
	return true;
}


bool CRecordManager::GetStopTimeSpec(TimeSpecInfo *pInfo) const
{
	*pInfo=m_StopTimeSpec;
	return true;
}


bool CRecordManager::StartRecord(CDtvEngine *pDtvEngine,LPCTSTR pszFileName)
{
	if (m_fRecording)
		return false;
	pDtvEngine->SetWriteCurServiceOnly(m_fCurServiceOnly,m_SaveStream);
	bool fDescrambleCurOnly=pDtvEngine->GetDescrambleCurServiceOnly();
	pDtvEngine->SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
	if (!m_RecordTask.Start(pDtvEngine,pszFileName)) {
		pDtvEngine->SetWriteCurServiceOnly(false);
		pDtvEngine->SetDescrambleCurServiceOnly(fDescrambleCurOnly);
		return false;
	}
	m_pDtvEngine=pDtvEngine;
	m_fRecording=true;
	m_fReserved=false;
	m_StartTimeSpec.Type=TIME_NOTSPECIFIED;
	return true;
}


void CRecordManager::StopRecord()
{
	if (m_fRecording) {
		m_RecordTask.Stop();
		m_fRecording=false;
		if (m_fCurServiceOnly)
			m_pDtvEngine->SetWriteCurServiceOnly(false);
		//SAFE_DELETE(m_pszFileName);
		m_pDtvEngine=NULL;
	}
}


bool CRecordManager::PauseRecord()
{
	if (!m_fRecording)
		return false;
	return m_RecordTask.Pause();
}


bool CRecordManager::IsPaused() const
{
	return m_fRecording && m_RecordTask.IsPaused();
}


bool CRecordManager::CancelReserve()
{
	if (!m_fReserved)
		return false;
	m_fReserved=false;
	return true;
}


DWORD CRecordManager::GetRecordTime() const
{
	if (!m_fRecording)
		return 0;
	return m_RecordTask.GetRecordTime();
}


DWORD CRecordManager::GetPauseTime() const
{
	if (!m_fRecording)
		return 0;
	return m_RecordTask.GetPauseTime();
}


bool CRecordManager::QueryStart(int Offset) const
{
	if (!m_fReserved)
		return false;
	switch (m_StartTimeSpec.Type) {
	case TIME_DATETIME:
		{
			SYSTEMTIME st;
			FILETIME ft;

			::GetLocalTime(&st);
			::SystemTimeToFileTime(&st,&ft);
			if (Offset!=0)
				ft+=(LONGLONG)Offset*(FILETIME_SECOND/1000);
			if (::CompareFileTime(&ft,&m_StartTimeSpec.Time.DateTime)>=0)
				return true;
		}
		break;
	case TIME_DURATION:
		{
			DWORD Diff;

			Diff=DiffTime(m_ReserveTime.GetTickTime(),::GetTickCount());
			if ((LONGLONG)Offset<=-(LONGLONG)Diff)
				return true;
			Diff+=Offset;
			if (Diff>=m_StartTimeSpec.Time.Duration)
				return true;
		}
		break;
	}
	return false;
}


bool CRecordManager::QueryStop(int Offset) const
{
	if (!m_fRecording)
		return false;
	switch (m_StopTimeSpec.Type) {
	case TIME_DATETIME:
		{
			SYSTEMTIME st;
			FILETIME ft;

			::GetLocalTime(&st);
			::SystemTimeToFileTime(&st,&ft);
			if (Offset!=0)
				ft+=(LONGLONG)Offset*FILETIME_MILLISECOND;
			if (::CompareFileTime(&ft,&m_StopTimeSpec.Time.DateTime)>=0)
				return true;
		}
		break;
	case TIME_DURATION:
		{
			DWORD Diff;

			Diff=DiffTime(m_RecordTask.GetStartTime(),::GetTickCount());
			if ((LONGLONG)Offset<=-(LONGLONG)Diff)
				return true;
			Diff+=Offset;
			if (Diff>=m_StopTimeSpec.Time.Duration)
				return true;
		}
		break;
	}
	return false;
}


CRecordManager *CRecordManager::GetThis(HWND hDlg)
{
	return static_cast<CRecordManager*>(::GetProp(hDlg,TEXT("This")));
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


static BOOL DateTime_SetFiletime(HWND hwndDT,DWORD flag,FILETIME *pFileTime)
{
	SYSTEMTIME st;

	FileTimeToSystemTime(pFileTime,&st);
	return DateTime_SetSystemtime(hwndDT,flag,&st);
}


BOOL CALLBACK CRecordManager::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CRecordManager *pThis=reinterpret_cast<CRecordManager*>(lParam);
			int i;
			SYSTEMTIME st;
			FILETIME ftTime;

			::SetProp(hDlg,TEXT("This"),pThis);
			::SendDlgItemMessage(hDlg,IDC_RECORD_FILENAME,EM_LIMITTEXT,MAX_PATH-1,0);
			if (pThis->m_pszFileName!=NULL)
				::SetDlgItemText(hDlg,IDC_RECORD_FILENAME,pThis->m_pszFileName);
			static const LPCTSTR pszExistsOperation[] = {
				TEXT("上書きする"),TEXT("確認を取る"),TEXT("連番を付加する")
			};
			for (i=0;i<3;i++)
				::SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_ADDSTRING,0,
							reinterpret_cast<LPARAM>(pszExistsOperation[i]));
			::SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_SETCURSEL,
										(WPARAM)pThis->m_ExistsOperation,0);
			::EnableDlgItems(hDlg,IDC_RECORD_FILENAME_LABEL,IDC_RECORD_FILEEXISTS,
														!pThis->m_fRecording);
			// 開始時間
			DWORD Delay;
			::CheckRadioButton(hDlg,IDC_RECORD_START_NOW,IDC_RECORD_START_DELAY,
							 IDC_RECORD_START_NOW+pThis->m_StartTimeSpec.Type);
			switch (pThis->m_StartTimeSpec.Type) {
			case TIME_NOTSPECIFIED:
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				Delay=60*1000;
				break;
			case TIME_DATETIME:
				{
					FILETIME ft;
					ULARGE_INTEGER Time1,Time2;

					::GetLocalTime(&st);
					::SystemTimeToFileTime(&st,&ft);
					ftTime=pThis->m_StartTimeSpec.Time.DateTime;
					Time1.LowPart=ft.dwLowDateTime;
					Time1.HighPart=ft.dwHighDateTime;
					Time2.LowPart=ftTime.dwLowDateTime;
					Time2.HighPart=ftTime.dwHighDateTime;
					if (Time1.QuadPart<Time2.QuadPart) {
						Delay=
							(DWORD)((Time2.QuadPart-Time1.QuadPart)/FILETIME_MILLISECOND);
					} else {
						Delay=0;
					}
				}
				break;
			case TIME_DURATION:
				Delay=(DWORD)pThis->m_StartTimeSpec.Time.Duration;
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				ftTime+=(LONGLONG)Delay*FILETIME_MILLISECOND;
				break;
			}
			SetDateTimeFormat(hDlg,IDC_RECORD_STARTTIME_TIME);
			DateTime_SetFiletime(GetDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME),
															GDT_VALID,&ftTime);
			Delay/=1000;
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_HOUR,Delay/(60*60),FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STARTTIME_HOUR_UD,UDM_SETRANGE,
														0,MAKELPARAM(100,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_MINUTE,Delay/60%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STARTTIME_MINUTE_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_SECOND,Delay%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STARTTIME_SECOND_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			if (pThis->m_fRecording) {
				::EnableDlgItems(hDlg,IDC_RECORD_STARTTIME,IDC_RECORD_STARTTIME_SECOND_LABEL,false);
			} else {
				EnableDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME,
									pThis->m_StartTimeSpec.Type==TIME_DATETIME);
				EnableDlgItems(hDlg,IDC_RECORD_STARTTIME_HOUR,
									IDC_RECORD_STARTTIME_SECOND_LABEL,
									pThis->m_StartTimeSpec.Type==TIME_DURATION);
			}
			// 終了時間
			DWORD Duration;
			::CheckDlgButton(hDlg,IDC_RECORD_STOPSPECTIME,
				pThis->m_StopTimeSpec.Type!=TIME_NOTSPECIFIED?BST_CHECKED:BST_UNCHECKED);
			::CheckRadioButton(hDlg,IDC_RECORD_STOPDATETIME,
													IDC_RECORD_STOPREMAINTIME,
				pThis->m_StopTimeSpec.Type==TIME_DATETIME?
							IDC_RECORD_STOPDATETIME:IDC_RECORD_STOPREMAINTIME);
			switch (pThis->m_StopTimeSpec.Type) {
			case TIME_NOTSPECIFIED:
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				ftTime+=60*60*FILETIME_SECOND;
				Duration=60*60*1000;
				break;
			case TIME_DATETIME:
				{
					FILETIME ft;
					ULARGE_INTEGER Time1,Time2;

					::GetLocalTime(&st);
					::SystemTimeToFileTime(&st,&ft);
					ftTime=pThis->m_StopTimeSpec.Time.DateTime;
					Time1.LowPart=ft.dwLowDateTime;
					Time1.HighPart=ft.dwHighDateTime;
					Time2.LowPart=ftTime.dwLowDateTime;
					Time2.HighPart=ftTime.dwHighDateTime;
					if (Time1.QuadPart<Time2.QuadPart) {
						Duration=
							(DWORD)((Time2.QuadPart-Time1.QuadPart)/FILETIME_MILLISECOND);
					} else {
						Duration=0;
					}
				}
				break;
			case TIME_DURATION:
				Duration=(DWORD)pThis->m_StopTimeSpec.Time.Duration;
				::GetLocalTime(&st);
				st.wSecond=0;
				st.wMilliseconds=0;
				::SystemTimeToFileTime(&st,&ftTime);
				ftTime+=(LONGLONG)Duration*FILETIME_MILLISECOND;
				break;
			}
			SetDateTimeFormat(hDlg,IDC_RECORD_STOPTIME_TIME);
			DateTime_SetFiletime(GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),
															GDT_VALID,&ftTime);
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,
												Duration/(60*60*1000),FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_HOUR_UD,UDM_SETRANGE,
														0,MAKELPARAM(100,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,
												Duration/(60*1000)%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_MINUTE_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			::SetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,
												Duration/1000%60,FALSE);
			::SendDlgItemMessage(hDlg,IDC_RECORD_STOPTIME_SECOND_UD,UDM_SETRANGE,
														0,MAKELPARAM(60,0));
			if (pThis->m_StopTimeSpec.Type==TIME_NOTSPECIFIED) {
				EnableDlgItems(hDlg,IDC_RECORD_STOPDATETIME,
									IDC_RECORD_STOPTIME_SECOND_LABEL,false);
			} else {
				EnableDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME,
									pThis->m_StopTimeSpec.Type==TIME_DATETIME);
				EnableDlgItems(hDlg,IDC_RECORD_STOPTIME_HOUR,
									IDC_RECORD_STOPTIME_SECOND_LABEL,
									pThis->m_StopTimeSpec.Type==TIME_DURATION);
			}
			::DlgCheckBox_Check(hDlg,IDC_RECORD_CURSERVICEONLY,
								pThis->m_fCurServiceOnly);
			::DlgCheckBox_Check(hDlg,IDC_RECORD_SAVESUBTITLE,
				(pThis->m_SaveStream&CTsSelector::STREAM_SUBTITLE)!=0);
			::DlgCheckBox_Check(hDlg,IDC_RECORD_SAVEDATACARROUSEL,
				(pThis->m_SaveStream&CTsSelector::STREAM_DATACARROUSEL)!=0);
			if (pThis->m_fRecording) {
				EnableDlgItems(hDlg,IDC_RECORD_CURSERVICEONLY,IDC_RECORD_SAVEDATACARROUSEL,false);
			} else {
				EnableDlgItems(hDlg,IDC_RECORD_SAVESUBTITLE,IDC_RECORD_SAVEDATACARROUSEL,
							   pThis->m_fCurServiceOnly);
			}
			EnableDlgItem(hDlg,IDC_RECORD_CANCEL,pThis->m_fReserved);
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

		case IDC_RECORD_START_NOW:
		case IDC_RECORD_START_DATETIME:
		case IDC_RECORD_START_DELAY:
			EnableDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME,
				IsDlgButtonChecked(hDlg,IDC_RECORD_START_DATETIME)==BST_CHECKED);
			EnableDlgItems(hDlg,IDC_RECORD_STARTTIME_HOUR,
								IDC_RECORD_STARTTIME_SECOND_LABEL,
				IsDlgButtonChecked(hDlg,IDC_RECORD_START_DELAY)==BST_CHECKED);
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

		case IDC_RECORD_CURSERVICEONLY:
			EnableDlgItems(hDlg,IDC_RECORD_SAVESUBTITLE,IDC_RECORD_SAVEDATACARROUSEL,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORD_CURSERVICEONLY));
			return TRUE;

		case IDOK:
			{
				CRecordManager *pThis=GetThis(hDlg);
				SYSTEMTIME st;
				FILETIME ftCur,ftStart,ftStop;

				if (!pThis->m_fRecording) {
					GetLocalTime(&st);
					SystemTimeToFileTime(&st,&ftCur);
					DateTime_GetSystemtime(
								GetDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME),&st);
					SystemTimeToFileTime(&st,&ftStart);
					if (CompareFileTime(&ftStart,&ftCur)<=0
							&& IsDlgButtonChecked(hDlg,
									IDC_RECORD_START_DATETIME)==BST_CHECKED) {
						MessageBox(hDlg,
							TEXT("指定された開始時間を既に過ぎています。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_RECORD_STARTTIME_TIME));
						return TRUE;
					}
				}
				DateTime_GetSystemtime(
								GetDlgItem(hDlg,IDC_RECORD_STOPTIME_TIME),&st);
				SystemTimeToFileTime(&st,&ftStop);
				if (CompareFileTime(&ftStop,&ftCur)<=0
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
				if (!pThis->m_fRecording) {
					TCHAR szFileName[MAX_PATH];

					GetDlgItemText(hDlg,IDC_RECORD_FILENAME,szFileName,MAX_PATH);
					CFilePath FilePath(szFileName);
					if (szFileName[0]=='\0' || *FilePath.GetFileName()=='\0') {
						MessageBox(hDlg,TEXT("ファイル名を入力してください。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_RECORD_FILENAME));
						return TRUE;
					}
					if (!FilePath.IsValid()) {
						MessageBox(hDlg,
							TEXT("ファイル名に使用できない文字が含まれています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_RECORD_FILENAME));
						return TRUE;
					}
					if (!FilePath.HasDirectory()) {
						MessageBox(hDlg,TEXT("保存先フォルダを入力してください。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_RECORD_FILENAME));
						return TRUE;
					}
					FilePath.GetDirectory(szFileName);
					if (!::PathIsDirectory(szFileName)) {
						MessageBox(hDlg,TEXT("保存先フォルダが見付かりません。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_RECORD_FILENAME));
						return TRUE;
					}
					pThis->SetFileName(FilePath.GetPath());
					pThis->m_ExistsOperation=(FileExistsOperation)
						SendDlgItemMessage(hDlg,IDC_RECORD_FILEEXISTS,CB_GETCURSEL,0,0);
					switch (GetCheckedRadioButton(hDlg,IDC_RECORD_START_NOW,IDC_RECORD_START_DELAY)) {
					case IDC_RECORD_START_NOW:
					default:
						pThis->SetStartTimeSpec(NULL);
						break;
					case IDC_RECORD_START_DATETIME:
						{
							TimeSpecInfo TimeSpec;

							TimeSpec.Type=TIME_DATETIME;
							TimeSpec.Time.DateTime=ftStart;
							pThis->SetStartTimeSpec(&TimeSpec);
						}
						break;
					case IDC_RECORD_START_DELAY:
						{
							TimeSpecInfo TimeSpec;
							unsigned int Hour,Minute,Second;

							TimeSpec.Type=TIME_DURATION;
							Hour=GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_HOUR,NULL,FALSE);
							Minute=GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_MINUTE,NULL,FALSE);
						Second=GetDlgItemInt(hDlg,IDC_RECORD_STARTTIME_SECOND,NULL,FALSE);
							TimeSpec.Time.Duration=(Hour*(60*60)+Minute*60+Second)*1000;
							pThis->SetStartTimeSpec(&TimeSpec);
						}
						break;
					}
					pThis->m_fCurServiceOnly=DlgCheckBox_IsChecked(hDlg,IDC_RECORD_CURSERVICEONLY);
					pThis->m_SaveStream=CTsSelector::STREAM_MPEG2VIDEO | CTsSelector::STREAM_AAC;
					if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_SAVESUBTITLE))
						pThis->m_SaveStream|=CTsSelector::STREAM_SUBTITLE;
					if (DlgCheckBox_IsChecked(hDlg,IDC_RECORD_SAVEDATACARROUSEL))
						pThis->m_SaveStream|=CTsSelector::STREAM_DATACARROUSEL;
				}
				if (IsDlgButtonChecked(hDlg,IDC_RECORD_STOPSPECTIME)==BST_CHECKED) {
					TimeSpecInfo TimeSpec;

					if (IsDlgButtonChecked(hDlg,IDC_RECORD_STOPDATETIME)==BST_CHECKED) {
						TimeSpec.Type=TIME_DATETIME;
						TimeSpec.Time.DateTime=ftStop;
					} else {
						unsigned int Hour,Minute,Second;

						TimeSpec.Type=TIME_DURATION;
						Hour=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_HOUR,NULL,FALSE);
						Minute=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_MINUTE,NULL,FALSE);
						Second=GetDlgItemInt(hDlg,IDC_RECORD_STOPTIME_SECOND,NULL,FALSE);
						TimeSpec.Time.Duration=(Hour*(60*60)+Minute*60+Second)*1000;
					}
					pThis->SetStopTimeSpec(&TimeSpec);
				} else {
					pThis->SetStopTimeSpec(NULL);
				}
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
			return TRUE;

		case IDC_RECORD_CANCEL:
			{
				CRecordManager *pThis=GetThis(hDlg);

				pThis->CancelReserve();
				EndDialog(hDlg,IDCANCEL);
			}
			return TRUE;
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


bool CRecordManager::RecordDialog(HWND hwndOwner)
{
	return DialogBoxParam(GetAppClass().GetResourceInstance(),
						  MAKEINTRESOURCE(IDD_RECORDOPTION),hwndOwner,
						  DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}


BOOL CALLBACK CRecordManager::StopTimeDlgProc(HWND hDlg,UINT uMsg,
												WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CRecordManager *pThis=reinterpret_cast<CRecordManager*>(lParam);
			FILETIME ft;;

			SetProp(hDlg,TEXT("This"),pThis);
			CheckDlgButton(hDlg,IDC_RECORDSTOPTIME_ENABLE,
				pThis->m_StopTimeSpec.Type!=TIME_NOTSPECIFIED?BST_CHECKED:BST_UNCHECKED);
			EnableDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME,
								pThis->m_StopTimeSpec.Type!=TIME_NOTSPECIFIED);
			SetDateTimeFormat(hDlg,IDC_RECORDSTOPTIME_TIME);
			if (pThis->m_StopTimeSpec.Type==TIME_DATETIME) {
				ft=pThis->m_StopTimeSpec.Time.DateTime;
			} else if (pThis->m_StopTimeSpec.Type==TIME_DURATION) {
				pThis->GetStartTime(&ft);
				ft+=(LONGLONG)pThis->m_StopTimeSpec.Time.Duration*FILETIME_MILLISECOND;
			}
			DateTime_SetFiletime(GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME),
																GDT_VALID,&ft);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RECORDSTOPTIME_ENABLE:
			EnableDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME,
				IsDlgButtonChecked(hDlg,IDC_RECORDSTOPTIME_ENABLE)==BST_CHECKED);
			return TRUE;

		case IDOK:
			{
				CRecordManager *pThis=GetThis(hDlg);
				bool fStopTimeSpec;
				SYSTEMTIME st;

				fStopTimeSpec=IsDlgButtonChecked(hDlg,
									IDC_RECORDSTOPTIME_ENABLE)==BST_CHECKED;
				if (fStopTimeSpec) {
					FILETIME ft,ftCur;
					ULARGE_INTEGER Time1,Time2;
					TimeSpecInfo TimeSpec;

					if (DateTime_GetSystemtime(
							GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME),&st)!=
																GDT_VALID) {
						MessageBox(hDlg,TEXT("時間の取得エラー。"),NULL,
												MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					SystemTimeToFileTime(&st,&ft);
					GetLocalTime(&st);
					SystemTimeToFileTime(&st,&ftCur);
					if (CompareFileTime(&ft,&ftCur)<=0) {
						MessageBox(hDlg,
								TEXT("指定された停止時間を既に過ぎています。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_RECORDSTOPTIME_TIME));
						return TRUE;
					}
					TimeSpec.Type=TIME_DATETIME;
					TimeSpec.Time.DateTime=ft;
					pThis->SetStopTimeSpec(&TimeSpec);
				} else {
					pThis->SetStopTimeSpec(NULL);
				}
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;

	case WM_DESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


bool CRecordManager::ChangeStopTimeDialog(HWND hwndOwner)
{
	return DialogBoxParam(GetAppClass().GetResourceInstance(),
						  MAKEINTRESOURCE(IDD_RECORDSTOPTIME),hwndOwner,
						  StopTimeDlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}


bool CRecordManager::DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName)
{
	lstrcpy(pszFileName,m_pszFileName);
	switch (m_ExistsOperation) {
	case EXISTS_CONFIRM:
		if (PathFileExists(m_pszFileName)
				&& MessageBox(hwndOwner,
					TEXT("ファイルが既に存在します。\n上書きしますか?"),
					TEXT("上書きの確認"),MB_OKCANCEL | MB_ICONQUESTION)!=IDOK)
			return false;
		break;
	case EXISTS_SEQUENCIALNUMBER:
		if (PathFileExists(m_pszFileName)) {
			int i;
			TCHAR szFileName[MAX_PATH];
			LPTSTR pszExtension,p;
			int ExtensionOffset;

			pszExtension=PathFindExtension(m_pszFileName);
			lstrcpy(szFileName,m_pszFileName);
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


bool CRecordManager::SetCurServiceOnly(bool fOnly)
{
	m_fCurServiceOnly=fOnly;
	return true;
}


bool CRecordManager::SetSaveStream(DWORD Stream)
{
	m_SaveStream=Stream;
	return true;
}


bool CRecordManager::SetDescrambleCurServiceOnly(bool fOnly)
{
	m_fDescrambleCurServiceOnly=fOnly;
	return true;
}




CRecordOptions::CRecordOptions()
{
	m_szSaveFolder[0]='\0';
	::lstrcpy(m_szFileName,TEXT("Record.ts"));
	m_fAddTime=true;
	m_fConfirmChannelChange=true;
	m_fConfirmExit=true;
	m_fCurServiceOnly=false;
	m_fSaveSubtitle=true;
	m_fSaveDataCarrousel=false;
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
	pSettings->Read(TEXT("RecordCurServiceOnly"),&m_fCurServiceOnly);
	pSettings->Read(TEXT("RecordSubtitle"),&m_fSaveSubtitle);
	pSettings->Read(TEXT("RecordDataCarrousel"),&m_fSaveDataCarrousel);
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
	pSettings->Write(TEXT("RecordCurServiceOnly"),m_fCurServiceOnly);
	pSettings->Write(TEXT("RecordSubtitle"),m_fSaveSubtitle);
	pSettings->Write(TEXT("RecordDataCarrousel"),m_fSaveDataCarrousel);
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


bool CRecordOptions::GetFilePath(LPTSTR pszFileName,int MaxLength) const
{
	if (m_szSaveFolder[0]=='\0' || m_szFileName[0]=='\0')
		return false;
	if (::lstrlen(m_szSaveFolder)+1+::lstrlen(m_szFileName)>=MaxLength)
		return false;
	::PathCombine(pszFileName,m_szSaveFolder,m_szFileName);
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
	DWORD Stream=CTsSelector::STREAM_MPEG2VIDEO | CTsSelector::STREAM_AAC;
	if (m_fSaveSubtitle)
		Stream|=CTsSelector::STREAM_SUBTITLE;
	if (m_fSaveDataCarrousel)
		Stream|=CTsSelector::STREAM_DATACARROUSEL;
	pManager->SetSaveStream(Stream);
	pManager->SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
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
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_CURSERVICEONLY,
				pThis->m_fCurServiceOnly?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE,
				pThis->m_fSaveSubtitle?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
				pThis->m_fSaveDataCarrousel?BST_CHECKED:BST_UNCHECKED);
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE,
								IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
						   pThis->m_fCurServiceOnly);
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

		case IDC_RECORDOPTIONS_CURSERVICEONLY:
			EnableDlgItems(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE,IDC_RECORDOPTIONS_SAVEDATACARROUSEL,
				DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_CURSERVICEONLY));
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
				if (!::PathIsDirectory(szSaveFolder)) {
					TCHAR szMessage[MAX_PATH+64];

					::wsprintf(szMessage,
						TEXT("録画ファイルの保存先フォルダ \"%s\" がありません。\r\n")
						TEXT("作成しますか?"),szSaveFolder);
					if (::MessageBox(hDlg,szMessage,TEXT("フォルダ作成の確認"),
										MB_YESNO | MB_ICONQUESTION)==IDYES) {
						int Result;

						Result=::SHCreateDirectoryEx(hDlg,szSaveFolder,NULL);
						if (Result!=ERROR_SUCCESS
								&& Result!=ERROR_ALREADY_EXISTS) {
							::MessageBox(hDlg,TEXT("フォルダが作成できません。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						}
					}
				}
				::GetDlgItemText(hDlg,IDC_RECORDOPTIONS_FILENAME,szFileName,lengthof(szFileName));
				if (szFileName[0]!='\0') {
					CFilePath FilePath(szFileName);
					if (!FilePath.IsValid()) {
						::MessageBox(hDlg,
							TEXT("録画ファイル名に、ファイル名に使用できない文字が含まれています。"),
							NULL,MB_OK | MB_ICONEXCLAMATION);
					}
				}
				::lstrcpy(pThis->m_szSaveFolder,szSaveFolder);
				::lstrcpy(pThis->m_szFileName,szFileName);
				pThis->m_fAddTime=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_ADDTIME)==BST_CHECKED;
				pThis->m_fConfirmChannelChange=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMCHANNELCHANGE)==BST_CHECKED;
				pThis->m_fConfirmExit=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CONFIRMEXIT)==BST_CHECKED;
				pThis->m_fCurServiceOnly=::IsDlgButtonChecked(hDlg,
					IDC_RECORDOPTIONS_CURSERVICEONLY)==BST_CHECKED;
				pThis->m_fSaveSubtitle=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVESUBTITLE);
				pThis->m_fSaveDataCarrousel=
					DlgCheckBox_IsChecked(hDlg,IDC_RECORDOPTIONS_SAVEDATACARROUSEL);
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
