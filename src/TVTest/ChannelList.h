#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H


#include "PointerArray.h"


#define FIRST_UHF_CHANNEL 13
#define MAX_CHANNEL_NAME 64


class CChannelInfo {
	int m_Space;				// チューニング空間
	int m_Channel;				// 物理チャンネル番号
	int m_ChannelIndex;			// チャンネルインデックス(BonDriverでの番号)
	int m_ChannelNo;			// リモコンチャンネル番号
	int m_Service;				// サービス
	TCHAR m_szName[MAX_CHANNEL_NAME];
	WORD m_NetworkID;			// ネットワークID
	WORD m_TransportStreamID;	// トランスポートストリームID
	WORD m_ServiceID;			// サービスID
	bool m_fEnabled;			// 有効
public:
	CChannelInfo(int Space,int Channel,int Index,int No,int Service,LPCTSTR pszName);
	CChannelInfo(const CChannelInfo &Info);
	virtual ~CChannelInfo() {}
	CChannelInfo &operator=(const CChannelInfo &Info);
	int GetSpace() const { return m_Space; }
	bool SetSpace(int Space);
	int GetChannel() const { return m_Channel; }
	bool SetChannel(int Channel);
	int GetChannelIndex() const { return m_ChannelIndex; }
	bool SetChannelNo(int ChannelNo);
	int GetChannelNo() const { return m_ChannelNo; }
	int GetService() const { return m_Service; }
	LPCTSTR GetName() const { return m_szName; }
	bool SetName(LPCTSTR pszName);
	bool SetNetworkID(WORD NetworkID);
	WORD GetNetworkID() const { return m_NetworkID; }
	bool SetTransportStreamID(WORD TransportStreamID);
	WORD GetTransportStreamID() const { return m_TransportStreamID; }
	bool SetServiceID(WORD ServiceID);
	WORD GetServiceID() const { return m_ServiceID; }
	void Enable(bool fEnable) { m_fEnabled=fEnable; }
	bool IsEnabled() const { return m_fEnabled; }
};

class CChannelList {
	int m_NumChannels;
	class CChannelInfo **m_ppList;
	int m_ListLength;
public:
	CChannelList();
	CChannelList(const CChannelList &List);
	~CChannelList();
	CChannelList &operator=(const CChannelList &List);
	int NumChannels() const { return m_NumChannels; }
	int NumEnableChannels() const;
	bool AddChannel(int Space,int Channel,int Index,int No,int Service,LPCTSTR pszName);
	bool AddChannel(const CChannelInfo &Info);
	bool AddChannel(CChannelInfo *pInfo);
	CChannelInfo *GetChannelInfo(int Index);
	const CChannelInfo *GetChannelInfo(int Index) const;
	int GetSpace(int Index) const;
	int GetChannel(int Index) const;
	int GetChannelIndex(int Index) const;
	int GetChannelNo(int Index) const;
	int GetService(int Index) const;
	LPCTSTR GetName(int Index) const;
	bool IsEnabled(int Index) const;
	bool DeleteChannel(int Index);
	void Clear();
	int Find(const CChannelInfo *pInfo) const;
	int Find(int Space,int ChannelIndex,int ServiceID=-1) const;
	int FindChannel(int Channel) const;
	int FindChannelNo(int No) const;
	int FindServiceID(WORD ServiceID) const;
	int GetNextChannelNo(int ChannelNo,bool fWrap=false) const;
	int GetPrevChannelNo(int ChannelNo,bool fWrap=false) const;
	int GetMaxChannelNo() const;
	enum SortType {
		SORT_SPACE,
		SORT_CHANNEL,
		SORT_CHANNELINDEX,
		SORT_CHANNELNO,
		SORT_SERVICE,
		SORT_NAME,
		SORT_NETWORKID,
		SORT_SERVICEID
	};
	void Sort(SortType Type,bool fDescending=false);
	bool UpdateStreamInfo(int Space,int ChannelIndex,int Service,
						WORD NetworkID,WORD TransportStreamID,WORD ServiceID);
	bool HasRemoteControlKeyID() const;
private:
	void SortSub(SortType Type,bool fDescending,int First,int Last,CChannelInfo **ppTemp);
};

class CTuningSpaceInfo {
public:
	enum TuningSpaceType {
		SPACE_ERROR=-1,
		SPACE_UNKNOWN=0,
		SPACE_TERRESTRIAL,
		SPACE_BS,
		SPACE_110CS
	};
private:
	CChannelList *m_pChannelList;
	LPTSTR m_pszName;
	TuningSpaceType m_Space;
public:
	CTuningSpaceInfo();
	CTuningSpaceInfo(const CTuningSpaceInfo &Info);
	~CTuningSpaceInfo();
	CTuningSpaceInfo &operator=(const CTuningSpaceInfo &Info);
	bool Create(const CChannelList *pList=NULL,LPCTSTR pszName=NULL);
	CChannelList *GetChannelList() { return m_pChannelList; }
	const CChannelList *GetChannelList() const { return m_pChannelList; }
	LPCTSTR GetName() const { return m_pszName; }
	bool SetName(LPCTSTR pszName);
	TuningSpaceType GetType() const { return m_Space; }
	int NumChannels() const;
};

class CTuningSpaceList {
	CPointerVector<CTuningSpaceInfo> m_TuningSpaceList;
	CChannelList m_AllChannelList;
	bool MakeTuningSpaceList(const CChannelList *pList,int Spaces=0);
public:
	CTuningSpaceList();
	CTuningSpaceList(const CTuningSpaceList &List);
	~CTuningSpaceList();
	CTuningSpaceList &operator=(const CTuningSpaceList &List);
	int NumSpaces() const { return m_TuningSpaceList.Length(); }
	CTuningSpaceInfo *GetTuningSpaceInfo(int Space);
	const CTuningSpaceInfo *GetTuningSpaceInfo(int Space) const;
	CChannelList *GetChannelList(int Space);
	const CChannelList *GetChannelList(int Space) const;
	CChannelList *GetAllChannelList() { return &m_AllChannelList; }
	const CChannelList *GetAllChannelList() const { return &m_AllChannelList; }
	LPCTSTR GetTuningSpaceName(int Space) const;
	CTuningSpaceInfo::TuningSpaceType GetTuningSpaceType(int Space) const;
	bool Create(const CChannelList *pList,int Spaces=0);
	bool Reserve(int Spaces);
	bool MakeAllChannelList();
	void Clear();
	bool SaveToFile(LPCTSTR pszFileName) const;
	bool LoadFromFile(LPCTSTR pszFileName);
	bool UpdateStreamInfo(int Space,int ChannelIndex,int Service,
						WORD NetworkID,WORD TransportStreamID,WORD ServiceID);
};


#endif
