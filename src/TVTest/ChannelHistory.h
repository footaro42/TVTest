#ifndef CHANNEL_HISTORY_H
#define CHANNEL_HISTORY_H


#include <deque>
#include "Options.h"
#include "ChannelList.h"


class CChannelHistory
{
public:
	class CChannel : public CChannelInfo
	{
		CDynamicString m_DriverName;

	public:
		CChannel(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
		LPCTSTR GetDriverFileName() const { return m_DriverName.Get(); }
	};

	CChannelHistory();
	~CChannelHistory();
	void Clear();
	bool SetCurrentChannel(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
	const CChannel *Forward();
	const CChannel *Backward();

private:
	std::deque<CChannel*> m_ChannelList;
	int m_MaxChannelHistory;
	int m_CurrentChannel;
};

class CRecentChannelList : public COptions
{
public:
	class CChannel : public ::CChannelInfo
	{
		CDynamicString m_DriverName;

	public:
		CChannel(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
		LPCTSTR GetDriverFileName() const { return m_DriverName.Get(); }
	};

	CRecentChannelList();
	~CRecentChannelList();
	int NumChannels() const;
	void Clear();
	const CChannel *GetChannelInfo(int Index) const;
	bool Add(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo);
	bool SetMenu(HMENU hmenu,bool fClear=true) const;
// COptions
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;

private:
	std::deque<CChannel*> m_ChannelList;
	int m_MaxChannelHistory;
	int m_MaxChannelHistoryMenu;
};


#endif
