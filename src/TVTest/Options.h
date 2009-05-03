#ifndef OPTIONS_H
#define OPTIONS_H


#include "Settings.h"


class COptions {
protected:
	HWND m_hDlg;
	DWORD m_UpdateFlags;
	static COptions *OnInitDialog(HWND hDlg,LPARAM lParam);
	static COptions *GetOptions(HWND hDlg);
	void OnDestroy();
public:
	COptions();
	virtual ~COptions();
	DWORD GetUpdateFlags() const { return m_UpdateFlags; }
	DWORD SetUpdateFlag(DWORD Flag);
	virtual bool Apply(DWORD Flags) { return true; }
	virtual bool Read(CSettings *pSettings) { return true; }
	virtual bool Write(CSettings *pSettings) const { return true; }
	virtual bool Load(LPCTSTR pszFileName) { return false; }
	virtual bool Save(LPCTSTR pszFileName) const { return false; }
};


#endif
