#ifndef EPG_OPTIONS_H
#define EPG_OPTIONS_H


#include "CoreEngine.h"
#include "EpgProgramList.h"
#include "Options.h"


class CEpgOptions : public COptions {
	TCHAR m_szEpgDataCapDllPath[MAX_PATH];
	bool m_fSaveEpgFile;
	TCHAR m_szEpgFileName[MAX_PATH];
	bool m_fUpdateWhenStandby;
	CCoreEngine *m_pCoreEngine;
	HANDLE m_hLoadThread;
	bool GetEpgFileFullPath(LPTSTR pszFileName);
	static DWORD WINAPI LoadThread(LPVOID lpParameter);
	static CEpgOptions *GetThis(HWND hDlg);
public:
	CEpgOptions(CCoreEngine *pCoreEngine);
	~CEpgOptions();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool InitializeEpgDataCap();
	bool LoadEpgFile(CEpgProgramList *pEpgList);
	bool AsyncLoadEpgFile(CEpgProgramList *pEpgList);
	bool SaveEpgFile(CEpgProgramList *pEpgList);
	LPCTSTR GetEpgFileName() const { return m_szEpgFileName; }
	bool GetUpdateWhenStandby() const { return m_fUpdateWhenStandby; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
