#ifndef LOGGER_H
#define LOGGER_H


#include "Options.h"


class CLogItem {
	FILETIME m_Time;
	LPTSTR m_pszText;
public:
	CLogItem(LPCTSTR pszText);
	~CLogItem();
	LPCTSTR GetText() const { return m_pszText; }
	void GetTime(SYSTEMTIME *pTime) const;
};

class CLogger : public COptions {
	int m_NumLogItems;
	CLogItem **m_ppList;
	int m_ListLength;
	static CLogger *GetThis(HWND hDlg);
public:
	CLogger();
	~CLogger();
	bool AddLog(LPCTSTR pszText, ...);
	void Clear();
	bool SaveToFile(LPCTSTR pszFileName) const;
	void GetDefaultLogFileName(LPTSTR pszFileName) const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
