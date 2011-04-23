#ifndef DRIVER_OPTIONS_H
#define DRIVER_OPTIONS_H


#include "Options.h"
#include "DriverManager.h"
#include "PointerArray.h"
#include "ChannelManager.h"


class CDriverSettings;

class CDriverSettingList
{
	CPointerVector<CDriverSettings> m_SettingList;
public:
	CDriverSettingList();
	~CDriverSettingList();
	CDriverSettingList &operator=(const CDriverSettingList &List);
	void Clear();
	int NumDrivers() const { return m_SettingList.Length(); }
	bool Add(CDriverSettings *pSettings);
	CDriverSettings *GetDriverSettings(int Index);
	const CDriverSettings *GetDriverSettings(int Index) const;
	int Find(LPCTSTR pszFileName) const;
};

class CDriverOptions : public COptions
{
	CDriverManager *m_pDriverManager;
	CDriverSettingList m_SettingList;
	CDriverSettingList m_CurSettingList;

// CBasicDialog
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void InitDlgItem(int Driver);
	void SetChannelList(int Driver);
	void AddChannelList(const CChannelList *pChannelList);
	CDriverSettings *GetCurSelDriverSettings() const;

public:
	CDriverOptions();
	~CDriverOptions();
// COptions
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
// CBasicDialog
	bool Create(HWND hwndOwner);
// CDriverOptions
	bool Initialize(CDriverManager *pDriverManager);
	struct ChannelInfo {
		int Space;
		int Channel;
		int ServiceID;
		bool fAllChannels;
	};
	bool GetInitialChannel(LPCTSTR pszFileName,ChannelInfo *pChannelInfo) const;
	bool SetLastChannel(LPCTSTR pszFileName,const ChannelInfo *pChannelInfo);
	bool IsDescrambleDriver(LPCTSTR pszFileName) const;
	bool IsNoSignalLevel(LPCTSTR pszFileName) const;
	bool IsIgnoreInitialStream(LPCTSTR pszFileName) const;
	bool IsPurgeStreamOnChannelChange(LPCTSTR pszFileName) const;
	bool IsResetChannelChangeErrorCount(LPCTSTR pszFileName) const;
};


#endif
