#ifndef RECORD_H
#define RECORD_H


#include "DtvEngine.h"
#include "Options.h"


class CRecordTime {
	FILETIME m_Time;
	DWORD m_TickTime;
public:
	CRecordTime();
	bool SetCurrentTime();
	bool GetTime(FILETIME *pTime) const;
	DWORD GetTickTime() const { return m_TickTime; }
	void Clear();
	bool IsValid() const;
};

class CRecordTask {
public:
	enum State {
		STATE_STOP,
		STATE_RECORDING,
		STATE_PAUSE
	};
protected:
	State m_State;
	CDtvEngine *m_pDtvEngine;
	CRecordTime m_StartTime;
	DWORD m_PauseStartTime;
	DWORD m_TotalPauseTime;
public:
	CRecordTask();
	virtual ~CRecordTask();
	bool Start(CDtvEngine *pDtvEngine,LPCTSTR pszFileName);
	bool Stop();
	bool Pause();
	State GetState() const;
	bool IsStopped() const { return m_State==STATE_STOP; }
	bool IsRecording() const { return m_State==STATE_RECORDING; }
	bool IsPaused() const { return m_State==STATE_PAUSE; }
	DWORD GetStartTime() const;
	bool GetStartTime(FILETIME *pTime) const;
	bool GetStartTime(CRecordTime *pTime) const;
	DWORD GetRecordTime() const;
	DWORD GetPauseTime() const;
	LONGLONG GetWroteSize() const;
	LPCTSTR GetFileName() const;
};

class CRecordManager {
public:
	enum TimeSpecType {
		TIME_NOTSPECIFIED,
		TIME_DATETIME,
		TIME_DURATION
	};
	struct TimeSpecInfo {
		TimeSpecType Type;
		union {
			FILETIME DateTime;
			ULONGLONG Duration;
		} Time;
	};
	enum FileExistsOperation {
		EXISTS_OVERWRITE,
		EXISTS_CONFIRM,
		EXISTS_SEQUENCIALNUMBER
	};
private:
	bool m_fRecording;
	bool m_fReserved;
	LPTSTR m_pszFileName;
	CRecordTime m_ReserveTime;
	TimeSpecInfo m_StartTimeSpec;
	TimeSpecInfo m_StopTimeSpec;
	CRecordTask m_RecordTask;
	CDtvEngine *m_pDtvEngine;
	FileExistsOperation m_ExistsOperation;
	bool m_fCurServiceOnly;
	DWORD m_SaveStream;
	bool m_fDescrambleCurServiceOnly;
	static CRecordManager *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK StopTimeDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CRecordManager();
	~CRecordManager();
	bool SetFileName(LPCTSTR pszFileName);
	LPCTSTR GetFileName() const { return m_pszFileName; }
	bool SetFileExistsOperation(FileExistsOperation Operation);
	FileExistsOperation GetFileExistsOperation() const { return m_ExistsOperation; }
	bool GetStartTime(FILETIME *pTime) const;
	bool GetReserveTime(FILETIME *pTime) const;
	bool SetStartTimeSpec(const TimeSpecInfo *pInfo);
	bool GetStartTimeSpec(TimeSpecInfo *pInfo) const;
	bool SetStopTimeSpec(const TimeSpecInfo *pInfo);
	bool GetStopTimeSpec(TimeSpecInfo *pInfo) const;
	bool StartRecord(CDtvEngine *pDtvEngine,LPCTSTR pszFileName);
	void StopRecord();
	bool PauseRecord();
	bool IsRecording() const { return m_fRecording; }
	bool IsPaused() const;
	bool IsReserved() const { return m_fReserved; }
	bool CancelReserve();
	DWORD GetRecordTime() const;
	DWORD GetPauseTime() const;
	const CRecordTask *GetRecordTask() const { return &m_RecordTask; }
	bool QueryStart(int Offset=0) const;
	bool QueryStop(int Offset=0) const;
	bool RecordDialog(HWND hwndOwner);
	bool ChangeStopTimeDialog(HWND hwndOwner);
	bool DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName);
	bool SetCurServiceOnly(bool fOnly);
	bool GetCurServiceOnly() const { return m_fCurServiceOnly; }
	bool SetSaveStream(DWORD Stream);
	DWORD GetSaveStream() const { return m_SaveStream; }
	bool SetDescrambleCurServiceOnly(bool fOnly);
	bool GetDescrambleCurServiceOnly() const { return m_fDescrambleCurServiceOnly; }
};

class CRecordOptions : public COptions {
	TCHAR m_szSaveFolder[MAX_PATH];
	TCHAR m_szFileName[MAX_PATH];
	bool m_fAddTime;
	bool m_fConfirmChannelChange;
	bool m_fConfirmExit;
	bool m_fCurServiceOnly;
	bool m_fSaveSubtitle;
	bool m_fSaveDataCarrousel;
	bool m_fDescrambleCurServiceOnly;
	static CRecordOptions *GetThis(HWND hDlg);
public:
	CRecordOptions();
	~CRecordOptions();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool GenerateFileName(LPTSTR pszFileName,int MaxLength,SYSTEMTIME *pTime=NULL,LPCTSTR *ppszErrorMessage=NULL) const;
	bool GetFilePath(LPTSTR pszFileName,int MaxLength) const;
	bool ConfirmChannelChange(HWND hwndOwner) const;
	bool ConfirmServiceChange(HWND hwndOwner,const CRecordManager *pRecordManager) const;
	bool ConfirmExit(HWND hwndOwner,const CRecordManager *pRecordManager) const;
	bool ApplyOptions(CRecordManager *pManager);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
