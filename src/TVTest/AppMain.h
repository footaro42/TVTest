#ifndef APP_MAIN_H
#define APP_MAIN_H


#include "CoreEngine.h"
#include "MainWindow.h"
#include "ChannelManager.h"
#include "Record.h"


class CAppMain {
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szDefaultChannelFileName[MAX_PATH];
	TCHAR m_szChannelSettingFileName[MAX_PATH];
	bool m_fFirstExecute;
	bool m_fChannelScanning;
	void SetTuningSpaceMenu(HMENU hmenu);
	void SetTuningSpaceMenu();
	void SetChannelMenu(HMENU hmenu);
	void SetChannelMenu();
	void SetNetworkRemoconChannelMenu(HMENU hmenu);
	bool SetService(int Service);
public:
	bool Initialize();
	bool Finalize();
	HINSTANCE GetInstance() const;
	HINSTANCE GetResourceInstance() const;
	bool GetAppDirectory(LPTSTR pszDirectory) const;
	LPCTSTR GetIniFileName() const { return m_szIniFileName; }
	bool AddLog(LPCTSTR pszText, ...);
	bool LoadSettings();
	bool SaveSettings();
	bool SaveCurrentChannel();
	bool IsFirstExecute() const;
	bool SaveChannelSettings();
	bool InitializeChannel();
	bool UpdateChannelList(const CTuningSpaceList *pList);
	bool UpdateChannelMenu();
	const CChannelInfo *GetCurrentChannelInfo() const;
	bool SetChannel(int Space,int Channel,int Service=-1);
	bool FollowChannelChange(WORD TransportStreamID,WORD ServiceID);
	bool SetServiceByIndex(int Service);
	bool SetServiceByID(WORD ServiceID,int *pServiceIndex=NULL);
	bool SetDriver(LPCTSTR pszFileName);
	bool UpdateDriverMenu();
	HMENU CreateTunerSelectMenu();
	bool ProcessTunerSelectMenu(int Command);
	bool ShowHelpContent(int ID);
	bool StartRecord(LPCTSTR pszFileName=NULL,
					 const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					 const CRecordManager::TimeSpecInfo *pStopTime=NULL);
	bool ModifyRecord(LPCTSTR pszFileName=NULL,
					  const CRecordManager::TimeSpecInfo *pStartTime=NULL,
					  const CRecordManager::TimeSpecInfo *pStopTime=NULL);
	bool StartReservedRecord();
	bool CancelReservedRecord();
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
};


CAppMain &GetAppClass();


#endif
