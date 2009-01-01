#ifndef LOGGER_H
#define LOGGER_H


#include "Options.h"
#include "TsUtilClass.h"


class CLogItem {
	FILETIME m_Time;
	LPTSTR m_pszText;
public:
	CLogItem(LPCTSTR pszText);
	~CLogItem();
	LPCTSTR GetText() const { return m_pszText; }
	void GetTime(SYSTEMTIME *pTime) const;
	int Format(char *pszText,int MaxLength) const;
};

class CLogger : public COptions, public CTracer {
	int m_NumLogItems;
	CLogItem **m_ppList;
	int m_ListLength;
	bool m_fOutputToFile;
	static CLogger *GetThis(HWND hDlg);
public:
	CLogger();
	~CLogger();
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	bool AddLog(LPCTSTR pszText, ...);
	bool AddLogV(LPCTSTR pszText,va_list Args);
	void Clear();
	bool SetOutputToFile(bool fOutput);
	bool GetOutputToFile() const { return m_fOutputToFile; }
	bool SaveToFile(LPCTSTR pszFileName,bool fAppend) const;
	void GetDefaultLogFileName(LPTSTR pszFileName) const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
protected:
	// CTracer
	void OnTrace(LPCTSTR pszOutput);
};


#endif
