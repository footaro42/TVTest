#ifndef RECORD_OPTIONS_H
#define RECORD_OPTIONS_H


#include "Options.h"
#include "Record.h"


class CRecordOptions : public COptions
{
	TCHAR m_szSaveFolder[MAX_PATH];
	TCHAR m_szFileName[MAX_PATH];
	bool m_fConfirmChannelChange;
	bool m_fConfirmExit;
	bool m_fConfirmStop;
	bool m_fConfirmStopStatusBarOnly;
	bool m_fCurServiceOnly;
	bool m_fSaveSubtitle;
	bool m_fSaveDataCarrousel;
	bool m_fDescrambleCurServiceOnly;
	bool m_fAlertLowFreeSpace;
	unsigned int m_LowFreeSpaceThreshold;
	unsigned int m_BufferSize;
	unsigned int m_TimeShiftBufferSize;
	bool m_fEnableTimeShiftRecording;
	bool m_fShowRemainTime;
	int m_StatusBarRecordCommand;

	static CRecordOptions *GetThis(HWND hDlg);

public:
	enum {
		UPDATE_RECORDSTREAM		=0x00000001UL,
		UPDATE_TIMESHIFTBUFFER	=0x00000002UL,
		UPDATE_ENABLETIMESHIFT	=0x00000004UL
	};

	CRecordOptions();
	~CRecordOptions();
// COptions
	bool Apply(DWORD Flags);
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CRecordOptions
	bool SetSaveFolder(LPCTSTR pszFolder);
	LPCTSTR GetSaveFolder() const { return m_szSaveFolder; }
	bool GetFilePath(LPTSTR pszFileName,int MaxLength) const;
	bool GenerateFilePath(LPTSTR pszFileName,int MaxLength,LPCTSTR *ppszErrorMessage=NULL) const;
	bool ConfirmChannelChange(HWND hwndOwner) const;
	bool ConfirmServiceChange(HWND hwndOwner,const CRecordManager *pRecordManager) const;
	bool ConfirmStop(HWND hwndOwner) const;
	bool ConfirmStatusBarStop(HWND hwndOwner) const;
	bool ConfirmExit(HWND hwndOwner,const CRecordManager *pRecordManager) const;
	bool ApplyOptions(CRecordManager *pManager);
	bool GetAlertLowFreeSpace() const { return m_fAlertLowFreeSpace; }
	ULONGLONG GetLowFreeSpaceThresholdBytes() const {
		return (ULONGLONG)m_LowFreeSpaceThreshold*(1024*1024);
	}
	bool IsTimeShiftRecordingEnabled() const { return m_fEnableTimeShiftRecording; }
	bool EnableTimeShiftRecording(bool fEnable);
	void SetShowRemainTime(bool fShow) { m_fShowRemainTime=fShow; }
	bool GetShowRemainTime() const { return m_fShowRemainTime; }
	int GetStatusBarRecordCommand() const { return m_StatusBarRecordCommand; }
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
