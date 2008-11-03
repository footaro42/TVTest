#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H


#include "ChannelList.h"
#include "BonSrcDecoder.h"


class CChannelManager {
	int m_CurrentSpace;
	int m_CurrentChannel;
	int m_CurrentService;
	int m_ChangingChannel;
	CTuningSpaceList m_TuningSpaceList;
	CTuningSpaceList m_DriverTuningSpaceList;
	bool m_fUseDriverChannelList;
	bool m_fNetworkRemocon;
	CChannelList *m_pNetworkRemoconChannelList;
	int m_NetworkRemoconCurrentChannel;
public:
	enum {
		SPACE_INVALID=-2,
		SPACE_ALL=-1
	};
	CChannelManager();
	~CChannelManager();
	bool LoadChannelList(LPCTSTR pszFileName);
	bool SetTuningSpaceList(const CTuningSpaceList *pList);
	bool MakeDriverTuningSpaceList(const CBonSrcDecoder *pSrcDecoder);
	bool SetUseDriverChannelList(bool fUse);
	bool GetUseDriverChannelList() const { return m_fUseDriverChannelList; }
	int GetCurrentSpace() const { return m_CurrentSpace; }
	bool SetCurrentSpace(int Space);
	int GetCurrentChannel() const { return m_CurrentChannel; }
	bool SetCurrentChannel(int Channel);
	int GetCurrentService() const { return m_CurrentService; }
	bool SetCurrentService(int Service);
	bool SetChangingChannel(int Channel);
	const CChannelInfo *GetCurrentChannelInfo() const;
	const CChannelInfo *GetCurrentRealChannelInfo() const;
	const CChannelInfo *GetChangingChannelInfo() const;
	const CChannelInfo *GetNextChannelInfo(bool fNext) const;
	const CChannelList *GetCurrentChannelList() const;
	const CChannelList *GetCurrentRealChannelList() const;
	const CChannelList *GetChannelList(int Space) const;
	const CChannelList *GetFileChannelList(int Space) const;
	const CChannelList *GetDriverChannelList(int Space) const;
	const CChannelList *GetAllChannelList() const;
	const CChannelList *GetFileAllChannelList() const;
	const CChannelList *GetDriverAllChannelList() const;
	const CTuningSpaceList *GetTuningSpaceList() const { return &m_TuningSpaceList; }
	const CTuningSpaceList *GetDriverTuningSpaceList() const { return &m_DriverTuningSpaceList; }
	int FindChannelInfo(const CChannelInfo *pInfo) const;
	int NumSpaces() const;
	bool SetNetworkRemoconMode(bool fNetworkRemocon,CChannelList *pList=NULL);
	int GetNetworkRemoconCurrentChannel() const { return m_NetworkRemoconCurrentChannel; }
	bool SetNetworkRemoconCurrentChannel(int Channel);
	bool SetServiceID(int Space,int ChannelIndex,int Service,WORD ServiceID);
	bool LoadChannelSettings(LPCTSTR pszFileName,LPCTSTR pszDriverName);
	bool SaveChannelSettings(LPCTSTR pszFileName,LPCTSTR pszDriverName);
};


#endif
