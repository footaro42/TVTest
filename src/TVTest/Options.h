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
	virtual bool Read(CSettings *pSettings) { return true; }
	virtual bool Write(CSettings *pSettings) const { return true; }
	virtual bool Load(LPCTSTR pszFileName) { return false; }
	virtual bool Save(LPCTSTR pszFileName) const { return false; }
	enum {
		UPDATE_DRIVER			= 0x00000001UL,
		UPDATE_CHANNELLIST		= 0x00000002UL,
		UPDATE_NETWORKREMOCON	= 0x00000004UL,
		UPDATE_PREVIEW			= 0x00000008UL
	};
};


#endif
