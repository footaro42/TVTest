#ifndef EPG_PROGRAM_LIST
#define EPG_PROGRAM_LIST


#include <vector>
#include <map>
#include "BonTsEngine/EventManager.h"


class CServiceInfoData
{
public:
	WORD m_OriginalNID;
	WORD m_TSID;
	WORD m_ServiceID;
	CServiceInfoData();
	CServiceInfoData(WORD OriginalNID,WORD TSID,WORD ServiceID);
	bool operator==(const CServiceInfoData &Info) const;
	bool operator!=(const CServiceInfoData &Info) const { return !(*this==Info); }
};

class CEventInfoData
{
	LPWSTR m_pszEventName;
	LPWSTR m_pszEventText;
	LPWSTR m_pszEventExtText;
	LPWSTR m_pszComponentTypeText;
	friend class CEpgProgramList;

public:
	struct AudioInfo {
		enum { MAX_TEXT=CEventManager::CEventInfo::AudioInfo::MAX_TEXT };
		BYTE ComponentType;
		bool fESMultiLingualFlag;
		bool fMainComponentFlag;
		BYTE SamplingRate;
		DWORD LanguageCode;
		DWORD LanguageCode2;
		TCHAR szText[MAX_TEXT];
		bool operator==(const AudioInfo &Op) const {
			return ComponentType==Op.ComponentType
				&& fESMultiLingualFlag==Op.fESMultiLingualFlag
				&& fMainComponentFlag==Op.fMainComponentFlag
				&& SamplingRate==Op.SamplingRate
				&& LanguageCode==Op.LanguageCode
				&& LanguageCode2==Op.LanguageCode2
				&& ::lstrcmp(szText,Op.szText)==0;
		}
		bool operator!=(const AudioInfo &Op) const { return !(*this==Op); }
	};
	struct NibbleData {
		BYTE m_ContentNibbleLv1;	//content_nibble_level_1
		BYTE m_ContentNibbleLv2;	//content_nibble_level_2
		BYTE m_UserNibbleLv1;		//user_nibble
		BYTE m_UserNibbleLv2;		//user_nibble
		bool operator==(const NibbleData &Data) const {
			return m_ContentNibbleLv1==Data.m_ContentNibbleLv1
				&& m_ContentNibbleLv2==Data.m_ContentNibbleLv2
				&& m_UserNibbleLv1==Data.m_UserNibbleLv1
				&& m_UserNibbleLv2==Data.m_UserNibbleLv2;
		}
		bool operator!=(const NibbleData &Data) const { return !(*this==Data); }
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
		AUDIOCOMPONENT_MONO=1,
		AUDIOCOMPONENT_DUALMONO,
		AUDIOCOMPONENT_STEREO,
		AUDIOCOMPONENT_3_1,
		AUDIOCOMPONENT_3_2,
		AUDIOCOMPONENT_3_2_1
	};
	struct CommonEventInfo {
		WORD ServiceID;
		WORD EventID;
		bool operator==(const CommonEventInfo &Op) const {
			return ServiceID==Op.ServiceID
				&& EventID==Op.EventID;
		}
		bool operator!=(const CommonEventInfo &Op) const {
			return !(*this==Op);
		}
	};

	WORD m_OriginalNID;
	WORD m_TSID;
	WORD m_ServiceID;
	WORD m_EventID;
	bool m_fValidStartTime;
	SYSTEMTIME m_stStartTime;
	DWORD m_DurationSec;
	BYTE m_ComponentType;
	std::vector<AudioInfo> m_AudioList;
	std::vector<NibbleData> m_NibbleList;
	bool m_fCommonEvent;
	CommonEventInfo m_CommonEventInfo;
	ULONGLONG m_UpdateTime;
	CEventInfoData();
	CEventInfoData(const CEventInfoData &Info);
#ifdef MOVE_CONSTRUCTOR_SUPPORTED
	CEventInfoData(CEventInfoData &&Info);
#endif
	CEventInfoData(const CEventManager::CEventInfo &Info);
	~CEventInfoData();
	CEventInfoData &operator=(const CEventInfoData &Info);
#ifdef MOVE_ASSIGNMENT_SUPPORTED
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
	LPCWSTR GetComponentTypeText() const { return m_pszComponentTypeText; }
	bool SetComponentTypeText(LPCWSTR pszText);
	bool GetStartTime(SYSTEMTIME *pTime) const;
	bool GetEndTime(SYSTEMTIME *pTime) const;
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
	static ServiceMapKey GetServiceMapKey(WORD OriginalNID,WORD TSID,WORD ServiceID) {
		return ((ULONGLONG)OriginalNID<<32) | ((ULONGLONG)TSID<<16) | (ULONGLONG)ServiceID;
	}
	const CEventInfoData *GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID);
	bool SetCommonEventInfo(CEventInfoData *pInfo);
	bool Merge(CEpgProgramList *pSrcList);

public:
	CEpgProgramList(CEventManager *pEventManager);
	~CEpgProgramList();
	bool UpdateService(const CEventManager::ServiceInfo *pService);
	bool UpdateService(CEventManager *pEventManager,
					   const CEventManager::ServiceInfo *pService);
	bool UpdateService(WORD TSID,WORD ServiceID);
	bool UpdateProgramList();
	void Clear();
	int NumServices() const;
	CEpgServiceInfo *EnumService(int ServiceIndex);
	CEpgServiceInfo *GetServiceInfo(WORD OriginalNID,WORD TSID,WORD ServiceID);
	CEpgServiceInfo *GetServiceInfo(WORD TSID,WORD ServiceID);
	bool GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID,CEventInfoData *pInfo);
	bool GetEventInfo(WORD TSID,WORD ServiceID,const SYSTEMTIME *pTime,CEventInfoData *pInfo);
	bool GetNextEventInfo(WORD TSID,WORD ServiceID,const SYSTEMTIME *pTime,CEventInfoData *pInfo);
	bool LoadFromFile(LPCTSTR pszFileName);
	bool SaveToFile(LPCTSTR pszFileName);
};


#endif
