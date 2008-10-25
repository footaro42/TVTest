#ifndef CHANNEL_LIST_H
#define CHANNEL_LIST_H


#define FIRST_UHF_CHANNEL 13
#define MAX_CHANNEL_NAME 64


class CChannelInfo {
	int m_Space;			// チューニング空間
	int m_Channel;			// 物理チャンネル番号
	int m_ChannelIndex;		// チャンネルインデックス(BonDriverでの番号)
	int m_ChannelNo;		// リモコンチャンネル番号
	int m_Service;			// サービス
	TCHAR m_szName[MAX_CHANNEL_NAME];
	WORD m_NetworkID;		// ネットワークID(Unused)
	WORD m_ServiceID;		// サービスID
public:
	CChannelInfo(int Space,int Channel,int Index,int No,int Service,LPCTSTR pszName);
	CChannelInfo(const CChannelInfo &Info);
	CChannelInfo &operator=(const CChannelInfo &Info);
	int GetSpace() const { return m_Space; }
	int GetChannel() const { return m_Channel; }
	int GetChannelIndex() const { return m_ChannelIndex; }
	int GetChannelNo() const { return m_ChannelNo; }
	int GetService() const { return m_Service; }
	LPCTSTR GetName() const { return m_szName; }
	bool SetName(LPCTSTR pszName);
	bool SetNetworkID(WORD NetworkID);
	WORD GetNetworkID() const { return m_NetworkID; }
	bool SetServiceID(WORD ServiceID);
	WORD GetServiceID() const { return m_ServiceID; }
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
	bool AddChannel(int Space,int Channel,int Index,int No,int Service,LPCTSTR pszName);
	bool AddChannel(const CChannelInfo &Info);
	CChannelInfo *GetChannelInfo(int Index);
	const CChannelInfo *GetChannelInfo(int Index) const;
	int GetSpace(int Index) const;
	int GetChannel(int Index) const;
	int GetChannelIndex(int Index) const;
	int GetChannelNo(int Index) const;
	int GetService(int Index) const;
	LPCTSTR GetName(int Index) const;
	bool DeleteChannel(int Index);
	void Clear();
	int Find(const CChannelInfo *pInfo) const;
	int Find(int Space,int ChannelIndex,int Service) const;
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
	bool SetServiceID(int Space,int ChannelIndex,int Service,WORD ServiceID);
private:
	void SortSub(SortType Type,bool fDescending,int First,int Last,CChannelInfo **ppTemp);
};

class CTuningSpaceList {
	CChannelList **m_ppChannelList;
	int m_NumSpaces;
	CChannelList m_AllChannelList;
	bool MakeTuningSpaceList(const CChannelList *pList,int NumSpaces=0);
public:
	CTuningSpaceList();
	CTuningSpaceList(const CTuningSpaceList &List);
	~CTuningSpaceList();
	CTuningSpaceList &operator=(const CTuningSpaceList &List);
	int NumSpaces() const { return m_NumSpaces; }
	CChannelList *GetChannelList(int Space);
	const CChannelList *GetChannelList(int Space) const;
	CChannelList *GetAllChannelList() { return &m_AllChannelList; }
	const CChannelList *GetAllChannelList() const { return &m_AllChannelList; }
	bool Create(const CChannelList *pList,int NumSpaces=0);
	bool Reserve(int NumSpaces);
	bool MakeAllChannelList();
	void Clear();
	bool SaveToFile(LPCTSTR pszFileName) const;
	bool LoadFromFile(LPCTSTR pszFileName);
	bool SetServiceID(int Space,int ChannelIndex,int Service,WORD ServiceID);
};


#endif
