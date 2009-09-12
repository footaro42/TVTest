#ifndef APP_MAIN_H
#define APP_MAIN_H


#include "CoreEngine.h"
#include "MainWindow.h"
#include "ChannelManager.h"
#include "Record.h"
#include "DriverManager.h"


class CAppMain {
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szDefaultChannelFileName[MAX_PATH];
	TCHAR m_szChannelSettingFileName[MAX_PATH];
	bool m_fFirstExecute;
	bool m_fChannelScanning;
	bool SetService(int Service);
	bool GenerateRecordFileName(LPTSTR pszFileName,int MaxFileName) const;

public:
	bool Initialize();
	bool Finalize();
	HINSTANCE GetInstance() const;
	HINSTANCE GetResourceInstance() const;
	bool GetAppDirectory(LPTSTR pszDirectory) const;
	LPCTSTR GetIniFileName() const { return m_szIniFileName; }
	bool AddLog(LPCTSTR pszText, ...);
	void OnError(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle=NULL);
	bool LoadSettings();
	bool SaveSettings();
	bool SaveCurrentChannel();
	bool IsFirstExecute() const;
	bool SaveChannelSettings();
	bool InitializeChannel();
	bool UpdateChannelList(const CTuningSpaceList *pList);
	const CChannelInfo *GetCurrentChannelInfo() const;
	bool SetChannel(int Space,int Channel,int ServiceID=-1);
	bool FollowChannelChange(WORD TransportStreamID,WORD ServiceID);
	bool SetServiceByIndex(int Service);
	bool SetServiceByID(WORD ServiceID,int *pServiceIndex=NULL);
	bool SetDriver(LPCTSTR pszFileName);
	bool OpenTuner();
	bool CloseTuner();
	bool OpenBcasCard(bool fRetry=false);
	bool ShowHelpContent(int ID);
	bool StartRecord(LPCTSTR pszFileName=NULL,
					 const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					 const CRecordManager::TimeSpecInfo *pStopTime=NULL);
	bool ModifyRecord(LPCTSTR pszFileName=NULL,
					  const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					  const CRecordManager::TimeSpecInfo *pStopTime=NULL);
	bool StartReservedRecord();
	bool CancelReservedRecord();
	bool StopRecord();
	void BeginChannelScan();
	void EndChannelScan();
	bool IsChannelScanning() const { return m_fChannelScanning; }
	bool IsDriverNoSignalLevel(LPCTSTR pszFileName) const;
	COLORREF GetColor(LPCTSTR pszText) const;
	CCoreEngine *GetCoreEngine();
	const CCoreEngine *GetCoreEngine() const;
	CMainWindow *GetMainWindow();
	const CChannelManager *GetChannelManager() const;
	const CRecordManager *GetRecordManager() const;
	const CDriverManager *GetDriverManager() const;
};


CAppMain &GetAppClass();


#endif
