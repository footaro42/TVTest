#ifndef EPG_DATA_LOADER_H
#define EPG_DATA_LOADER_H


#include "EpgDataCapDllUtil2.h"


class CEpgDataLoader {
public:
	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual void OnStart() {}
		virtual void OnEnd(bool fSuccess) {}
	};
	CEpgDataLoader(CEpgDataCapDllUtil2 *pEpgDataCap);
	~CEpgDataLoader();
	bool Load(LPCTSTR pszFolder);
	bool LoadAsync(LPCTSTR pszFolder,CEventHandler *pEventHandler=NULL);
private:
	CEpgDataCapDllUtil2 *m_pEpgDataCap;
	HANDLE m_hThread;
	LPTSTR m_pszFolder;
	CEventHandler *m_pEventHandler;
	bool LoadFromFile(LPCTSTR pszFileName);
	static DWORD WINAPI LoadThread(LPVOID lpParameter);
};


#endif	/* EPG_DATA_LOADER_H */
