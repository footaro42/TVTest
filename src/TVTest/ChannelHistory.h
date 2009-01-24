#ifndef CHANNEL_HISTORY_H
#define CHANNEL_HISTORY_H


#include <deque>
#include "Options.h"
#include "ChannelList.h"


class CDriverChannelInfo : public CChannelInfo {
	LPTSTR m_pszDriverName;
public:
	CDriverChannelInfo(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
	~CDriverChannelInfo();
	LPCTSTR GetDriverFileName() const { return m_pszDriverName; }
};

class CChannelHistory : public COptions {
	std::deque<CDriverChannelInfo*> m_ChannelList;
	int m_MaxChannelHistory;
	int m_MaxChannelHistoryMenu;
public:
	CChannelHistory();
	~CChannelHistory();
	int NumChannels() const;
	void Clear();
	const CDriverChannelInfo *GetChannelInfo(int Index) const;
	bool Add(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
	bool SetMenu(HMENU hmenu) const;
	// COptions
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
};


#endif
