#ifndef EPG_PROGRAM_LIST
#define EPG_PROGRAM_LIST


#include <vector>
#include <map>
#include "EpgDataCap/Epg.h"
#include "BonTsEngine/TsUtilClass.h"


class CServiceInfoData {
	LPWSTR m_pszServiceName;
public:
	WORD m_OriginalNID;
	WORD m_TSID;
	WORD m_ServiceID;
	WORD m_ServiceType;
	CServiceInfoData();
	CServiceInfoData(const CServiceInfoData &Info);
	CServiceInfoData(WORD OriginalNID,WORD TSID,WORD ServiceID,WORD ServiceType,LPCWSTR pszServiceName);
	~CServiceInfoData();
	CServiceInfoData &operator=(const CServiceInfoData &Info);
	bool operator==(const CServiceInfoData &Info) const;
	bool operator!=(const CServiceInfoData &Info) const { return !(*this==Info); }
	LPCWSTR GetServiceName() const { return m_pszServiceName; }
	bool SetServiceName(LPCWSTR pszName);
};

class CEventInfoData {
	LPWSTR m_pszEventName;
	LPWSTR m_pszEventText;
	LPWSTR m_pszEventExtText;
	LPWSTR m_pszComponentTypeText;
	LPWSTR m_pszAudioComponentTypeText;
public:
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
	WORD m_OriginalNID;
	WORD m_TSID;
	WORD m_ServiceID;
	WORD m_EventID;
	bool m_fValidStartTime;
	SYSTEMTIME m_stStartTime;
	DWORD m_DurationSec;
	BYTE m_ComponentType;
	BYTE m_AudioComponentType;
	bool m_fESMultiLangFlag;
	//bool m_fMainComponentFlag;
	BYTE m_SamplingRate;
	std::vector<NibbleData> m_NibbleList;
	CEventInfoData();
	CEventInfoData(const CEventInfoData &Info);
	~CEventInfoData();
	CEventInfoData &operator=(const CEventInfoData &Info);
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
	LPCWSTR GetAudioComponentTypeText() const { return m_pszAudioComponentTypeText; }
	bool SetAudioComponentTypeText(LPCWSTR pszText);
	bool GetStartTime(SYSTEMTIME *pTime) const;
	bool GetEndTime(SYSTEMTIME *pTime) const;
};

class CEventInfoList {
public:
	typedef std::map<WORD,CEventInfoData>::iterator EventIterator;
	std::map<WORD,CEventInfoData> EventDataMap; //ÉLÅ[ EventID
	CEventInfoList();
	CEventInfoList(const CEventInfoList &List);
	~CEventInfoList();
	CEventInfoList &operator=(const CEventInfoList &List);
	const CEventInfoData *GetEventInfo(WORD EventID);
};

class CEpgServiceInfo {
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

class CEpgProgramList {
	CEpgDataCapDllUtil2 *m_pEpgDll;
	typedef ULONGLONG ServiceMapKey;
	typedef std::map<ServiceMapKey,CEpgServiceInfo>::iterator ServiceIterator;
	std::map<ServiceMapKey,CEpgServiceInfo> ServiceMap;
	CCriticalLock m_Lock;
	ServiceMapKey GenerateServiceMapKey(WORD OriginalNID,WORD TSID,WORD ServiceID) const {
		return ((ULONGLONG)OriginalNID<<32) | ((ULONGLONG)TSID<<16) | (ULONGLONG)ServiceID;
	}
	bool UpdateService(const SERVICE_INFO *pService);
public:
	CEpgProgramList();
	~CEpgProgramList();
	void SetEpgDataCapDllUtil(CEpgDataCapDllUtil2 *pEpgDll) { m_pEpgDll=pEpgDll; }
	bool UpdateProgramList();
	bool UpdateProgramList(WORD TSID,WORD ServiceID);
	void Clear();
	int NumServices() const;
	CEpgServiceInfo *EnumService(int ServiceIndex);
	CEpgServiceInfo *GetServiceInfo(WORD OriginalNID,WORD TSID,WORD ServiceID);
	CEpgServiceInfo *GetServiceInfo(WORD TSID,WORD ServiceID);
	bool GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID,CEventInfoData *pInfo);
	const CEventInfoData *GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID);
	bool GetEventInfo(WORD TSID,WORD ServiceID,const SYSTEMTIME *pTime,CEventInfoData *pInfo);
	bool LoadFromFile(LPCTSTR pszFileName);
	bool SaveToFile(LPCTSTR pszFileName);
};


#endif
