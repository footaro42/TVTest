#ifndef APP_MAIN_H
#define APP_MAIN_H


#include "ChannelList.h"


class CAppMain {
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szDefaultChannelFileName[MAX_PATH];
	TCHAR m_szChannelSettingFileName[MAX_PATH];
	bool m_fFirstExecute;
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
	bool LoadSettings();
	bool SaveSettings();
	bool IsFirstExecute() const;
	bool SaveChannelSettings();
	bool InitializeChannel();
	bool UpdateChannelList(const CTuningSpaceList *pList);
	bool UpdateChannelMenu();
	bool SetChannel(int Space,int Channel,int Service=-1);
	bool SetServiceByIndex(int Service);
	bool SetServiceByID(WORD ServiceID,int *pServiceIndex=NULL);
};


CAppMain &GetAppClass();


#endif
