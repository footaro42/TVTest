#ifndef RECORD_H
#define RECORD_H


#include "DtvEngine.h"
#include "Options.h"


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
	DWORD m_StartTime;
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
	DWORD GetRecordTime() const;
	LONGLONG GetWroteSize() const;
	LPCTSTR GetFileName() const;
};

class CRecordOptions : public COptions {
	TCHAR m_szSaveFolder[MAX_PATH];
	TCHAR m_szFileName[MAX_PATH];
	bool m_fAddTime;
	bool m_fConfirmChannelChange;
	bool m_fConfirmExit;
	bool m_fDescrambleCurServiceOnly;
	static CRecordOptions *GetThis(HWND hDlg);
public:
	CRecordOptions();
	~CRecordOptions();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool GenerateFileName(LPTSTR pszFileName,int MaxLength,SYSTEMTIME *pTime=NULL,LPCTSTR *ppszErrorMessage=NULL) const;
	bool ConfirmChannelChange(HWND hwndOwner);
	bool ConfirmExit(HWND hwndOwner);
	bool GetDescrambleCurServiceOnly() const { return m_fDescrambleCurServiceOnly; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

class CRecordManager {
public:
	enum FileExistsOperation {
		EXISTS_OVERWRITE,
		EXISTS_CONFIRM,
		EXISTS_SEQUENCIALNUMBER
	};
private:
	TCHAR m_szFileName[MAX_PATH];
	FileExistsOperation m_ExistsOperation;
	bool m_fStopTimeSpec;
	bool m_fStopDateTime;
	DWORD m_StopTime;
	FILETIME m_ftStopTime;
	FILETIME m_ftStartTime;
	bool m_fRecording;
	CRecordTask *m_pRecordTask;
	bool m_fDescrambleCurServiceOnly;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK StopTimeDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CRecordManager();
	~CRecordManager();
	bool SetFileName(LPCTSTR pszFileName);
	LPCTSTR GetFileName() const { return m_szFileName; }
	bool SetFileExistsOperation(FileExistsOperation Operation);
	FileExistsOperation GetFileExistsOperation() const { return m_ExistsOperation; }
	bool SetStopTimeSpec(bool fSpec);
	bool GetStopTimeSpec() const {return m_fStopTimeSpec; }
	bool SetStopTime(unsigned int Time);
	DWORD GetStopTime() const { return m_StopTime; }
	bool StartRecord(CDtvEngine *pDtvEngine,LPCTSTR pszFileName);
	void StopRecord();
	bool PauseRecord();
	bool IsRecording() const { return m_fRecording; }
	bool IsPaused() const;
	DWORD GetRecordTime() const;
	const CRecordTask *GetRecordTask() const { return m_pRecordTask; }
	bool QueryStop() const;
	bool RecordDialog(HWND hwndOwner,HINSTANCE hinst);
	bool DoFileExistsOperation(HWND hwndOwner,LPTSTR pszFileName);
	bool SetDescrambleCurServiceOnly(bool fOnly);
	DWORD CalcStopTime(DWORD StartTime);
	bool ChangeStopTimeDialog(HWND hwndOwner,HINSTANCE hinst);
};


#endif
