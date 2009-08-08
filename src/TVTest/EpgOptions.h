#ifndef EPG_OPTIONS_H
#define EPG_OPTIONS_H


#include "CoreEngine.h"
#include "EpgProgramList.h"
#include "Options.h"
#include "EpgDataCap/EpgDataLoader.h"


class CEpgOptions : public COptions {
public:
	class CEpgLoadEventHandler {
	public:
		virtual ~CEpgLoadEventHandler() {}
		virtual void OnStart() {}
		virtual void OnEnd(bool fSuccess) {}
	};

private:
	TCHAR m_szEpgDataCapDllPath[MAX_PATH];
	bool m_fSaveEpgFile;
	TCHAR m_szEpgFileName[MAX_PATH];
	bool m_fUpdateWhenStandby;
	bool m_fUseEpgData;
	TCHAR m_szEpgDataFolder[MAX_PATH];
	CCoreEngine *m_pCoreEngine;
	HANDLE m_hLoadThread;
	CEpgDataLoader *m_pEpgDataLoader;

	class CEpgDataLoaderEventHandler : public CEpgDataLoader::CEventHandler {
		CTsPacketParser *m_pPacketParser;
		CEpgLoadEventHandler *m_pEventHandler;
		bool m_fLoading;
	public:
		CEpgDataLoaderEventHandler() : m_pPacketParser(NULL), m_pEventHandler(NULL), m_fLoading(false) {}
		void SetPacketParser(CTsPacketParser *pPacketParser) {
			m_pPacketParser=pPacketParser;
		}
		void SetEventHandler(CEpgLoadEventHandler *pHandler) {
			m_pEventHandler=pHandler;
		}
		void OnStart() {
			m_pPacketParser->LockEpgDataCap();
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnStart();
			m_fLoading=true;
		}
		void OnEnd(bool fSuccess) {
			m_pPacketParser->UnlockEpgDataCap();
			m_fLoading=false;
			if (m_pEventHandler!=NULL)
				m_pEventHandler->OnEnd(fSuccess);
		}
		bool IsLoading() const { return m_fLoading; }
	};
	CEpgDataLoaderEventHandler m_EpgDataLoaderEventHandler;

	bool GetEpgFileFullPath(LPTSTR pszFileName);
	static DWORD WINAPI LoadThread(LPVOID lpParameter);
	static CEpgOptions *GetThis(HWND hDlg);

public:
	CEpgOptions(CCoreEngine *pCoreEngine);
	~CEpgOptions();
	void Finalize();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool InitializeEpgDataCap();
	bool LoadEpgFile(CEpgProgramList *pEpgList);
	bool AsyncLoadEpgFile(CEpgProgramList *pEpgList);
	bool SaveEpgFile(CEpgProgramList *pEpgList);
	LPCTSTR GetEpgFileName() const { return m_szEpgFileName; }
	bool GetUpdateWhenStandby() const { return m_fUpdateWhenStandby; }
	bool LoadEpgData();
	bool AsyncLoadEpgData(CEpgLoadEventHandler *pEventHandler=NULL);
	bool IsEpgDataLoading() const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
