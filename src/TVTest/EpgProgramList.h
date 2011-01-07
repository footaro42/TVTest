#ifndef EPG_PROGRAM_LIST
#define EPG_PROGRAM_LIST


#include <vector>
#include <map>
#include "BonTsEngine/EventManager.h"


class CServiceInfoData
{
public:
	WORD m_NetworkID;
	WORD m_TSID;
	WORD m_ServiceID;

	CServiceInfoData();
	CServiceInfoData(WORD NetworkID,WORD TSID,WORD ServiceID);
	bool operator==(const CServiceInfoData &Info) const;
	bool operator!=(const CServiceInfoData &Info) const { return !(*this==Info); }
};

class CEventInfoData
{
public:
	typedef CEventManager::CEventInfo::VideoInfo VideoInfo;
	typedef CEventManager::CEventInfo::AudioInfo AudioInfo;
	typedef CEventManager::CEventInfo::AudioList AudioList;
	typedef CContentDesc::Nibble NibbleData;
	typedef CEventManager::CEventInfo::ContentNibble ContentNibble;
	typedef CEventManager::CEventInfo::EventGroupInfo EventGroupInfo;
	typedef CEventManager::CEventInfo::EventGroupList EventGroupList;
	typedef CEventManager::CEventInfo::CommonEventInfo CommonEventInfo;

	enum {
		AUDIOCOMPONENT_MONO=1,
		AUDIOCOMPONENT_DUALMONO,
		AUDIOCOMPONENT_STEREO,
		AUDIOCOMPONENT_3_1,
		AUDIOCOMPONENT_3_2,
		AUDIOCOMPONENT_3_2_1
	};

	enum {
		CONTENT_NEWS,
		CONTENT_SPORTS,
		CONTENT_INFORMATION,
		CONTENT_DRAMA,
		CONTENT_MUSIC,
		CONTENT_VARIETY,
		CONTENT_MOVIE,
		CONTENT_ANIME,
		CONTENT_DOCUMENTARY,
		CONTENT_THEATER,
		CONTENT_EDUCATION,
		CONTENT_WELFARE,
		CONTENT_LAST=CONTENT_WELFARE
	};

	enum {
		FREE_CA_MODE_UNKNOWN,
		FREE_CA_MODE_UNSCRAMBLED,
		FREE_CA_MODE_SCRAMBLED
	};

	WORD m_NetworkID;
	WORD m_TSID;
	WORD m_ServiceID;
	WORD m_EventID;
	bool m_fValidStartTime;
	SYSTEMTIME m_stStartTime;
	DWORD m_DurationSec;
	BYTE m_RunningStatus;
	//bool m_fFreeCaMode;
	BYTE m_FreeCaMode;
	VideoInfo m_VideoInfo;
	AudioList m_AudioList;
	ContentNibble m_ContentNibble;
	EventGroupList m_EventGroupList;
	bool m_fCommonEvent;
	CommonEventInfo m_CommonEventInfo;
	ULONGLONG m_UpdateTime;

	CEventInfoData();
	CEventInfoData(const CEventInfoData &Info);
#ifdef MOVE_SEMANTICS_SUPPORTED
	CEventInfoData(CEventInfoData &&Info);
#endif
	CEventInfoData(const CEventManager::CEventInfo &Info);
	~CEventInfoData();
	CEventInfoData &operator=(const CEventInfoData &Info);
#ifdef MOVE_SEMANTICS_SUPPORTED
	CEventInfoData &operator=(CEventInfoData &&Info);
#endif
	CEventInfoData &operator=(const CEventManager::CEventInfo &Info);
	bool operator==(const CEventInfoData &Info) const;
	bool operator!=(const CEventInfoData &Info) const { return !(*this==Info); }
	LPCWSTR GetEventName() const { return m_pszEventName; }
	bool SetEventName(LPCWSTR pszEventName);
	LPCWSTR GetEventText() const { return m_pszEventText; }
	bool SetEventText(LPCWSTR pszEventText);
	LPCWSTR GetEventExtText() const { return m_pszEventExtText; }
	bool SetEventExtText(LPCWSTR pszEventExtText);
	bool GetStartTime(SYSTEMTIME *pTime) const;
	bool GetEndTime(SYSTEMTIME *pTime) const;
	int GetMainAudioIndex() const;
	const AudioInfo *GetMainAudioInfo() const;

private:
	LPWSTR m_pszEventName;
	LPWSTR m_pszEventText;
	LPWSTR m_pszEventExtText;

	friend class CEpgProgramList;
};

class CEventInfoList
{
public:
	typedef std::map<WORD,CEventInfoData> EventMap;
	typedef std::map<WORD,CEventInfoData>::iterator EventIterator;
	EventMap EventDataMap; //ÉLÅ[ EventID

	CEventInfoList();
	CEventInfoList(const CEventInfoList &List);
	~CEventInfoList();
	CEventInfoList &operator=(const CEventInfoList &List);
	const CEventInfoData *GetEventInfo(WORD EventID);
	bool RemoveEvent(WORD EventID);
};

class CEpgServiceInfo
{
public:
	CServiceInfoData m_ServiceData;
	CEventInfoList m_EventList;

	CEpgServiceInfo();
	CEpgServiceInfo(const CEpgServiceInfo &Info);
	CEpgServiceInfo(const CServiceInfoData &ServiceData);
	~CEpgServiceInfo();
	CEpgServiceInfo &operator=(const CEpgServiceInfo &Info);
	const CEventInfoData *GetEventInfo(WORD EventID);
};

class CEpgProgramList
{
	CEventManager *m_pEventManager;
	typedef ULONGLONG ServiceMapKey;
	typedef std::map<ServiceMapKey,CEpgServiceInfo*> ServiceMap;
	ServiceMap m_ServiceMap;
	mutable CCriticalLock m_Lock;
	FILETIME m_LastWriteTime;

	static ServiceMapKey GetServiceMapKey(WORD NetworkID,WORD TSID,WORD ServiceID) {
		return ((ULONGLONG)NetworkID<<32) | ((ULONGLONG)TSID<<16) | (ULONGLONG)ServiceID;
	}
	const CEventInfoData *GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,WORD EventID);
	bool SetCommonEventInfo(CEventInfoData *pInfo);
	bool Merge(CEpgProgramList *pSrcList);

public:
	CEpgProgramList(CEventManager *pEventManager);
	~CEpgProgramList();
	bool UpdateService(const CEventManager::ServiceInfo *pService);
	bool UpdateService(CEventManager *pEventManager,
					   const CEventManager::ServiceInfo *pService);
	bool UpdateService(WORD NetworkID,WORD TSID,WORD ServiceID);
	bool UpdateServices(WORD NetworkID,WORD TSID);
	bool UpdateProgramList();
	void Clear();
	int NumServices() const;
	CEpgServiceInfo *EnumService(int ServiceIndex);
	CEpgServiceInfo *GetServiceInfo(WORD NetworkID,WORD TSID,WORD ServiceID);
	bool GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
					  WORD EventID,CEventInfoData *pInfo);
	bool GetEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
					  const SYSTEMTIME *pTime,CEventInfoData *pInfo);
	bool GetNextEventInfo(WORD NetworkID,WORD TSID,WORD ServiceID,
						  const SYSTEMTIME *pTime,CEventInfoData *pInfo);
	bool LoadFromFile(LPCTSTR pszFileName);
	bool SaveToFile(LPCTSTR pszFileName);
};


#endif
