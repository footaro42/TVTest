#include "stdafx.h"
#include "TVTest.h"
#include "EpgProgramList.h"
#include "HelperClass/NFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static bool CompareText(LPCTSTR pszText1,LPCTSTR pszText2)
{
	return (pszText1==NULL && pszText2==NULL)
			|| (pszText1!=NULL && pszText2!=NULL
				&& ::lstrcmp(pszText1,pszText2)==0);
}




CServiceInfoData::CServiceInfoData()
	: m_OriginalNID(0)
	, m_TSID(0)
	, m_ServiceID(0)
{
}


CServiceInfoData::CServiceInfoData(WORD OriginalNID,WORD TSID,WORD ServiceID)
	: m_OriginalNID(OriginalNID)
	, m_TSID(TSID)
	, m_ServiceID(ServiceID)
{
}


bool CServiceInfoData::operator==(const CServiceInfoData &Info) const
{
	return m_OriginalNID==Info.m_OriginalNID
		&& m_TSID==Info.m_TSID
		&& m_ServiceID==Info.m_ServiceID;
}




CEventInfoData::CEventInfoData()
	: m_pszEventName(NULL)
	, m_pszEventText(NULL)
	, m_pszEventExtText(NULL)
	, m_pszComponentTypeText(NULL)
	, m_fValidStartTime(false)
	, m_fCommonEvent(false)
	, m_UpdateTime(0)
{
}


CEventInfoData::CEventInfoData(const CEventInfoData &Info)
	: m_pszEventName(NULL)
	, m_pszEventText(NULL)
	, m_pszEventExtText(NULL)
	, m_pszComponentTypeText(NULL)
{
	*this=Info;
}


#ifdef MOVE_CONSTRUCTOR_SUPPORTED
CEventInfoData::CEventInfoData(CEventInfoData &&Info)
	: m_pszEventName(NULL)
	, m_pszEventText(NULL)
	, m_pszEventExtText(NULL)
	, m_pszComponentTypeText(NULL)
{
	*this=std::move<Info>;
}
#endif


CEventInfoData::CEventInfoData(const CEventManager::CEventInfo &Info)
	: m_pszEventName(NULL)
	, m_pszEventText(NULL)
	, m_pszEventExtText(NULL)
	, m_pszComponentTypeText(NULL)
{
	*this=Info;
}


CEventInfoData::~CEventInfoData()
{
	delete [] m_pszEventName;
	delete [] m_pszEventText;
	delete [] m_pszEventExtText;
	delete [] m_pszComponentTypeText;
}


CEventInfoData &CEventInfoData::operator=(const CEventInfoData &Info)
{
	if (&Info!=this) {
		m_OriginalNID=Info.m_OriginalNID;
		m_TSID=Info.m_TSID;
		m_ServiceID=Info.m_ServiceID;
		m_EventID=Info.m_EventID;
		SetEventName(Info.m_pszEventName);
		SetEventText(Info.m_pszEventText);
		SetEventExtText(Info.m_pszEventExtText);
		m_fValidStartTime=Info.m_fValidStartTime;
		if (m_fValidStartTime)
			m_stStartTime=Info.m_stStartTime;
		m_DurationSec=Info.m_DurationSec;
		m_ComponentType=Info.m_ComponentType;
		SetComponentTypeText(Info.m_pszComponentTypeText);
		m_AudioList=Info.m_AudioList;
		m_NibbleList=Info.m_NibbleList;
		m_fCommonEvent=Info.m_fCommonEvent;
		m_CommonEventInfo=Info.m_CommonEventInfo;
		m_UpdateTime=Info.m_UpdateTime;
	}
	return *this;
}


#ifdef MOVE_ASSIGNMENT_SUPPORTED
CEventInfoData &CEventInfoData::operator=(CEventInfoData &&Info);
{
	if (&Info!=this) {
		m_OriginalNID=Info.m_OriginalNID;
		m_TSID=Info.m_TSID;
		m_ServiceID=Info.m_ServiceID;
		m_EventID=Info.m_EventID;
		m_pszEventName=Info.m_pszEventName;
		m_pszEventTextInfo.m_pszEventText;
		m_pszEventExtText=Info.m_pszEventExtText;
		m_fValidStartTime=Info.m_fValidStartTime;
		if (m_fValidStartTime)
			m_stStartTime=Info.m_stStartTime;
		m_DurationSec=Info.m_DurationSec;
		m_ComponentType=Info.m_ComponentType;
		m_pszComponentTypeText=Info.m_pszComponentTypeText;
		m_AudioList=Info.m_AudioList;
		m_NibbleList=Info.m_NibbleList;
		m_fCommonEvent=Info.m_fCommonEvent;
		m_CommonEventInfo=Info.m_CommonEventInfo;
		m_UpdateTime=Info.m_UpdateTime;
		Info.m_pszEventName=NULL;
		Info.m_pszEventText=NULL;
		Info.m_pszEventExtText=NULL;
		Info.m_pszComponentTypeText=NULL;
	}
	return *this;
}
#endif


CEventInfoData &CEventInfoData::operator=(const CEventManager::CEventInfo &Info)
{
	m_EventID=Info.GetEventID();
	SetEventName(Info.GetEventName());
	SetEventText(Info.GetEventText());
	SetEventExtText(Info.GetEventExtendedText());
	m_fValidStartTime=true;
	m_stStartTime=Info.GetStartTime();
	m_stStartTime.wDayOfWeek=CalcDayOfWeek(m_stStartTime.wYear,
										   m_stStartTime.wMonth,
										   m_stStartTime.wDay);
	m_DurationSec=Info.GetDuration();
	m_ComponentType=Info.GetVideoInfo().ComponentType;
	SetComponentTypeText(Info.GetVideoInfo().szText[0]!='\0'?Info.GetVideoInfo().szText:NULL);
	const CEventManager::CEventInfo::AudioList &AudioList=Info.GetAudioList();
	m_AudioList.resize(AudioList.size());
	for (size_t i=0;i<AudioList.size();i++) {
		const CEventManager::CEventInfo::AudioInfo &Audio=AudioList[i];
		m_AudioList[i].ComponentType=Audio.ComponentType;
		m_AudioList[i].fESMultiLingualFlag=Audio.bESMultiLingualFlag;
		m_AudioList[i].fMainComponentFlag=Audio.bMainComponentFlag;
		m_AudioList[i].SamplingRate=Audio.SamplingRate;
		m_AudioList[i].LanguageCode=Audio.LanguageCode;
		m_AudioList[i].LanguageCode2=Audio.LanguageCode2;
		::lstrcpy(m_AudioList[i].szText,Audio.szText);
	}
	const CEventManager::CEventInfo::ContentNibble &ContentNibble=Info.GetContentNibble();
	m_NibbleList.resize(ContentNibble.NibbleCount);
	for (int i=0;i<ContentNibble.NibbleCount;i++) {
		CEventInfoData::NibbleData &Nibble=m_NibbleList[i];

		Nibble.m_ContentNibbleLv1=ContentNibble.NibbleList[i].ContentNibbleLevel1;
		Nibble.m_ContentNibbleLv2=ContentNibble.NibbleList[i].ContentNibbleLevel2;
		Nibble.m_UserNibbleLv1=ContentNibble.NibbleList[i].UserNibble1;
		Nibble.m_UserNibbleLv2=ContentNibble.NibbleList[i].UserNibble2;
	}
	m_fCommonEvent=Info.IsCommonEvent();
	if (m_fCommonEvent) {
		const CEventManager::CEventInfo::CommonEventInfo &CommonInfo=Info.GetCommonEvent();
		m_CommonEventInfo.ServiceID=CommonInfo.ServiceID;
		m_CommonEventInfo.EventID=CommonInfo.EventID;
	}
	m_UpdateTime=Info.GetUpdateTime();

	return *this;
}


bool CEventInfoData::operator==(const CEventInfoData &Info) const
{
	return m_OriginalNID==Info.m_OriginalNID
		&& m_TSID==Info.m_TSID
		&& m_ServiceID==Info.m_ServiceID
		&& m_EventID==Info.m_EventID
		&& CompareText(m_pszEventName,Info.m_pszEventName)
		&& CompareText(m_pszEventText,Info.m_pszEventText)
		&& CompareText(m_pszEventExtText,Info.m_pszEventExtText)
		&& m_fValidStartTime==Info.m_fValidStartTime
		&& (!m_fValidStartTime
			|| ::memcmp(&m_stStartTime,&Info.m_stStartTime,sizeof(SYSTEMTIME))==0)
		&& m_DurationSec==Info.m_DurationSec
		&& m_ComponentType==Info.m_ComponentType
		&& CompareText(m_pszComponentTypeText,Info.m_pszComponentTypeText)
		&& m_AudioList==Info.m_AudioList
		&& m_NibbleList==Info.m_NibbleList
		&& m_fCommonEvent==Info.m_fCommonEvent
		&& (!m_fCommonEvent
			|| m_CommonEventInfo==Info.m_CommonEventInfo);
}


bool CEventInfoData::SetEventName(LPCWSTR pszEventName)
{
	return ReplaceString(&m_pszEventName,pszEventName);
}


bool CEventInfoData::SetEventText(LPCWSTR pszEventText)
{
	return ReplaceString(&m_pszEventText,pszEventText);
}


bool CEventInfoData::SetEventExtText(LPCWSTR pszEventExtText)
{
	return ReplaceString(&m_pszEventExtText,pszEventExtText);
}


bool CEventInfoData::SetComponentTypeText(LPCWSTR pszText)
{
	return ReplaceString(&m_pszComponentTypeText,pszText);
}


bool CEventInfoData::GetStartTime(SYSTEMTIME *pTime) const
{
	if (!m_fValidStartTime) {
		::ZeroMemory(pTime,sizeof(SYSTEMTIME));
		return false;
	}
	*pTime=m_stStartTime;
	return true;
}


bool CEventInfoData::GetEndTime(SYSTEMTIME *pTime) const
{
	FILETIME ft;

	if (!m_fValidStartTime || !::SystemTimeToFileTime(&m_stStartTime,&ft)) {
		::ZeroMemory(pTime,sizeof(SYSTEMTIME));
		return false;
	}
	ft+=(LONGLONG)m_DurationSec*FILETIME_SECOND;
	return ::FileTimeToSystemTime(&ft,pTime)!=FALSE;
}




CEventInfoList::CEventInfoList()
{
}


CEventInfoList::CEventInfoList(const CEventInfoList &List)
	: EventDataMap(List.EventDataMap)
{
}


CEventInfoList::~CEventInfoList()
{
}


CEventInfoList &CEventInfoList::operator=(const CEventInfoList &List)
{
	EventDataMap=List.EventDataMap;
	return *this;
}


const CEventInfoData *CEventInfoList::GetEventInfo(WORD EventID)
{
	EventMap::iterator itrEvent=EventDataMap.find(EventID);

	if (itrEvent==EventDataMap.end())
		return NULL;
	return &itrEvent->second;
}


bool CEventInfoList::RemoveEvent(WORD EventID)
{
	EventMap::iterator itrEvent=EventDataMap.find(EventID);

	if (itrEvent==EventDataMap.end())
		return false;
	EventDataMap.erase(itrEvent);
	return true;
}




CEpgServiceInfo::CEpgServiceInfo()
{
}


CEpgServiceInfo::CEpgServiceInfo(const CEpgServiceInfo &Info)
	: m_ServiceData(Info.m_ServiceData)
	, m_EventList(Info.m_EventList)
{
}


CEpgServiceInfo::CEpgServiceInfo(const CServiceInfoData &ServiceData)
	: m_ServiceData(ServiceData)
{
}


CEpgServiceInfo::~CEpgServiceInfo()
{
}


CEpgServiceInfo &CEpgServiceInfo::operator=(const CEpgServiceInfo &Info)
{
	m_ServiceData=Info.m_ServiceData;
	m_EventList=Info.m_EventList;
	return *this;
}


const CEventInfoData *CEpgServiceInfo::GetEventInfo(WORD EventID)
{
	return m_EventList.GetEventInfo(EventID);
}




class CGlobalLock
{
	HANDLE m_hMutex;
	CGlobalLock(const CGlobalLock &);
	CGlobalLock &operator=(const CGlobalLock &);

public:
	CGlobalLock::CGlobalLock(LPCTSTR pszName)
	{
		SECURITY_DESCRIPTOR sd;
		SECURITY_ATTRIBUTES sa;
		::ZeroMemory(&sd,sizeof(sd));
		::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
		::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
		::ZeroMemory(&sa,sizeof(sa));
		sa.nLength=sizeof(sa);
		sa.lpSecurityDescriptor=&sd;
		m_hMutex=::CreateMutex(&sa,FALSE,pszName);
	}

	CGlobalLock::~CGlobalLock()
	{
		if (m_hMutex!=NULL)
			::CloseHandle(m_hMutex);
	}

	bool Wait(DWORD Timeout=INFINITE)
	{
		if (m_hMutex==NULL)
			return false;
		return ::WaitForSingleObject(m_hMutex,Timeout)==WAIT_OBJECT_0;
	}

	void Release()
	{
		if (m_hMutex!=NULL)
			::ReleaseMutex(m_hMutex);
	}
};




CEpgProgramList::CEpgProgramList(CEventManager *pEventManager)
	: m_pEventManager(pEventManager)
{
	::ZeroMemory(&m_LastWriteTime,sizeof(FILETIME));
}


CEpgProgramList::~CEpgProgramList()
{
	Clear();
}


bool CEpgProgramList::UpdateService(const CEventManager::ServiceInfo *pService)
{
	return UpdateService(m_pEventManager,pService);
}


bool CEpgProgramList::UpdateService(CEventManager *pEventManager,
									const CEventManager::ServiceInfo *pService)
{
	CBlockLock Lock(&m_Lock);

	if (!pEventManager->IsServiceUpdated(pService->OriginalNetworkID,
										 pService->TransportStreamID,
										 pService->ServiceID))
		return true;

	CEventManager::EventList EventList;
	if (!pEventManager->GetEventList(pService->OriginalNetworkID,
									 pService->TransportStreamID,
									 pService->ServiceID,
									 &EventList)
			|| EventList.size()==0)
		return false;

	CServiceInfoData ServiceData(pService->OriginalNetworkID,
								 pService->TransportStreamID,
								 pService->ServiceID);
	CEpgServiceInfo *pServiceInfo=new CEpgServiceInfo(ServiceData);

	struct EventTime {
		ULONGLONG StartTime;
		DWORD Duration;
		WORD EventID;
		ULONGLONG UpdateTime;
		EventTime(const CEventInfoData &Info)
			: Duration(Info.m_DurationSec)
			, EventID(Info.m_EventID)
			, UpdateTime(Info.m_UpdateTime)
		{
			FILETIME ft;
			::SystemTimeToFileTime(&Info.m_stStartTime, &ft);
			StartTime = (((ULONGLONG)ft.dwHighDateTime << 32)
				| (ULONGLONG)ft.dwLowDateTime) / 10000000ULL;
		}
		bool operator<(const EventTime &Obj) const {
			return StartTime < Obj.StartTime;
		}
	};
	std::set<EventTime> EventTimeTable;

#ifdef _DEBUG
	SYSTEMTIME stOldestTime,stNewestTime;
	::FillMemory(&stOldestTime,sizeof(SYSTEMTIME),0xFF);
	::FillMemory(&stNewestTime,sizeof(SYSTEMTIME),0x00);
#endif

	for (size_t j=0;j<EventList.size();j++) {
		const CEventManager::CEventInfo &Event = EventList[j];
		CEventInfoData &EventData=
			pServiceInfo->m_EventList.EventDataMap.insert(
				std::pair<WORD,CEventInfoData>(Event.GetEventID(),CEventInfoData())).first->second;

		EventData=Event;
		EventData.m_OriginalNID=ServiceData.m_OriginalNID;
		EventData.m_TSID=ServiceData.m_TSID;
		EventData.m_ServiceID=ServiceData.m_ServiceID;

#ifdef _DEBUG
		if (CompareSystemTime(&EventData.m_stStartTime,&stOldestTime)<0)
			stOldestTime=EventData.m_stStartTime;
		SYSTEMTIME stEnd;
		EventData.GetEndTime(&stEnd);
		if (CompareSystemTime(&stEnd,&stNewestTime)>0)
			stNewestTime=EventData.m_stStartTime;
#endif

		EventTimeTable.insert(EventTime(EventData));
	}

#ifdef _DEBUG
	TRACE(TEXT("CEpgProgramList::UpdateService() (%d) %d/%d %d:%02d - %d/%d %d:%02d %d Events\n"),
		  pService->ServiceID,
		  stOldestTime.wMonth,stOldestTime.wDay,stOldestTime.wHour,stOldestTime.wMinute,
		  stNewestTime.wMonth,stNewestTime.wDay,stNewestTime.wHour,stNewestTime.wMinute,
		  (int)EventList.size());
#endif

	// 既存のイベントで新しいリストに無いものを追加する
	ServiceMapKey Key=GetServiceMapKey(ServiceData.m_OriginalNID,
								ServiceData.m_TSID,ServiceData.m_ServiceID);
	ServiceMap::iterator itrService=m_ServiceMap.find(Key);
	if (itrService!=m_ServiceMap.end()) {
		CEventInfoList::EventMap::iterator itrEvent;
#ifdef _DEBUG
		int Count=0;
#endif

		for (itrEvent=itrService->second->m_EventList.EventDataMap.begin();
				itrEvent!=itrService->second->m_EventList.EventDataMap.end();
				itrEvent++) {
			if (pServiceInfo->m_EventList.EventDataMap.find(itrEvent->second.m_EventID)!=
					pServiceInfo->m_EventList.EventDataMap.end())
				continue;

			std::pair<std::set<EventTime>::iterator,bool> Result=
				EventTimeTable.insert(EventTime(itrEvent->second));
			if (Result.second) {
				const EventTime &Time=*Result.first;
				bool fSkip=false;
				std::set<EventTime>::iterator itr;
				itr=Result.first;
				for (itr++;itr!=EventTimeTable.end();) {
					if (itr->StartTime>=Time.StartTime+Time.Duration)
						break;
					if (itr->UpdateTime>Time.UpdateTime) {
						fSkip=true;
						break;
					}
					pServiceInfo->m_EventList.RemoveEvent(itr->EventID);
					EventTimeTable.erase(itr++);
				}
				if (!fSkip && Result.first!=EventTimeTable.begin()) {
					itr=Result.first;
					itr--;
					while (true) {
						if (itr->StartTime+itr->Duration<=Time.StartTime)
							break;
						if (itr->UpdateTime>Time.UpdateTime) {
							fSkip=true;
							break;
						}
						pServiceInfo->m_EventList.RemoveEvent(itr->EventID);
						if (itr==EventTimeTable.begin()) {
							EventTimeTable.erase(itr);
							break;
						}
						EventTimeTable.erase(itr--);
					}
				}
				if (!fSkip) {
					pServiceInfo->m_EventList.EventDataMap.insert(
						std::pair<WORD,CEventInfoData>(itrEvent->second.m_EventID,itrEvent->second));
#ifdef _DEBUG
					Count++;
#endif
				} else {
					EventTimeTable.erase(Result.first);
				}
			}
		}
#ifdef _DEBUG
		TRACE(TEXT("古いイベントの生き残り %d (Total %d)\n"),
			  Count,(int)pServiceInfo->m_EventList.EventDataMap.size());
#endif
	}

	std::pair<ServiceMap::iterator, bool> Result =
		m_ServiceMap.insert(std::pair<ServiceMapKey,CEpgServiceInfo*>(Key,pServiceInfo));
	if (!Result.second) {
		delete Result.first->second;
		m_ServiceMap[Key]=pServiceInfo;
	}
	return true;
}


bool CEpgProgramList::UpdateProgramList()
{
	CBlockLock Lock(&m_Lock);

	CEventManager::ServiceList ServiceList;
	if (!m_pEventManager->GetServiceList(&ServiceList))
		return false;
	for (size_t i=0;i<ServiceList.size();i++) {
		UpdateService(&ServiceList[i]);
	}
	return true;
}


bool CEpgProgramList::UpdateService(WORD TSID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);

	CEventManager::ServiceList ServiceList;
	if (!m_pEventManager->GetServiceList(&ServiceList))
		return false;
	for (size_t i=0;i<ServiceList.size();i++) {
		if ((TSID==0 || ServiceList[i].TransportStreamID==TSID)
				&& ServiceList[i].ServiceID==ServiceID)
			return UpdateService(&ServiceList[i]);
	}
	return false;
}


void CEpgProgramList::Clear()
{
	CBlockLock Lock(&m_Lock);

	for (ServiceMap::iterator itr=m_ServiceMap.begin();itr!=m_ServiceMap.end();itr++)
		delete itr->second;
	m_ServiceMap.clear();
}


int CEpgProgramList::NumServices() const
{
	CBlockLock Lock(&m_Lock);

	return (int)m_ServiceMap.size();
}


CEpgServiceInfo *CEpgProgramList::EnumService(int ServiceIndex)
{
	CBlockLock Lock(&m_Lock);

	if (ServiceIndex<0)
		return NULL;
	ServiceMap::iterator itrService=m_ServiceMap.begin();
	for (int i=0;i<ServiceIndex && itrService!=m_ServiceMap.end();i++)
		itrService++;
	if (itrService==m_ServiceMap.end())
		return NULL;
	return itrService->second;
}


CEpgServiceInfo *CEpgProgramList::GetServiceInfo(WORD OriginalNID,WORD TSID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);
	ServiceMap::iterator itrService;

	itrService=m_ServiceMap.find(GetServiceMapKey(OriginalNID,TSID,ServiceID));
	if (itrService==m_ServiceMap.end())
		return NULL;
	return itrService->second;
}


CEpgServiceInfo *CEpgProgramList::GetServiceInfo(WORD TSID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);
	ServiceMap::iterator itrService;

	for (itrService=m_ServiceMap.begin();itrService!=m_ServiceMap.end();itrService++) {
		if ((TSID==0 || itrService->second->m_ServiceData.m_TSID==TSID)
				&& itrService->second->m_ServiceData.m_ServiceID==ServiceID)
			return itrService->second;
	}
	return NULL;
}


bool CEpgProgramList::GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID,CEventInfoData *pInfo)
{
	CBlockLock Lock(&m_Lock);

	CEpgServiceInfo *pServiceInfo=GetServiceInfo(TSID,ServiceID);
	if (pServiceInfo==NULL)
		return false;

	CEventManager::CEventInfo EventInfo;
	if (m_pEventManager->GetEventInfo(pServiceInfo->m_ServiceData.m_OriginalNID,
									  pServiceInfo->m_ServiceData.m_TSID,
									  pServiceInfo->m_ServiceData.m_ServiceID,
									  EventID, &EventInfo)) {
		*pInfo=EventInfo;
		pInfo->m_OriginalNID=pServiceInfo->m_ServiceData.m_OriginalNID;
		pInfo->m_TSID=pServiceInfo->m_ServiceData.m_TSID;
		pInfo->m_ServiceID=pServiceInfo->m_ServiceData.m_ServiceID;
	} else {
		const CEventInfoData *pEventInfo=pServiceInfo->GetEventInfo(EventID);
		if (pEventInfo==NULL)
			return false;
		*pInfo=*pEventInfo;
	}
	SetCommonEventInfo(pInfo);

	return false;
}


bool CEpgProgramList::GetEventInfo(WORD TSID,WORD ServiceID,const SYSTEMTIME *pTime,CEventInfoData *pInfo)
{
	CBlockLock Lock(&m_Lock);

	CEpgServiceInfo *pServiceInfo=GetServiceInfo(TSID,ServiceID);
	if (pServiceInfo==NULL)
		return NULL;

	CEventManager::CEventInfo EventInfo;
	if (m_pEventManager->GetEventInfo(pServiceInfo->m_ServiceData.m_OriginalNID,
									  pServiceInfo->m_ServiceData.m_TSID,
									  pServiceInfo->m_ServiceData.m_ServiceID,
									  pTime, &EventInfo)) {
		*pInfo=EventInfo;
		pInfo->m_OriginalNID=pServiceInfo->m_ServiceData.m_OriginalNID;
		pInfo->m_TSID=pServiceInfo->m_ServiceData.m_TSID;
		pInfo->m_ServiceID=pServiceInfo->m_ServiceData.m_ServiceID;
		SetCommonEventInfo(pInfo);
		return true;
	}

	CEventInfoList::EventMap::iterator itrEvent;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();itrEvent++) {
		SYSTEMTIME stStart,stEnd;

		itrEvent->second.GetStartTime(&stStart);
		itrEvent->second.GetEndTime(&stEnd);
		if (CompareSystemTime(&stStart,pTime)<=0
				&& CompareSystemTime(&stEnd,pTime)>0) {
			*pInfo=itrEvent->second;
			SetCommonEventInfo(pInfo);
			return true;
		}
	}
	return false;
}


bool CEpgProgramList::GetNextEventInfo(WORD TSID,WORD ServiceID,const SYSTEMTIME *pTime,CEventInfoData *pInfo)
{
	CBlockLock Lock(&m_Lock);

	CEpgServiceInfo *pServiceInfo=GetServiceInfo(TSID,ServiceID);
	if (pServiceInfo==NULL)
		return NULL;

	CEventInfoList::EventMap::iterator itrEvent;
	const CEventInfoData *pNextEvent=NULL;
	SYSTEMTIME stNearest;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();itrEvent++) {
		SYSTEMTIME stStart;

		itrEvent->second.GetStartTime(&stStart);
		if (CompareSystemTime(&stStart,pTime)>0
				&& (pNextEvent==NULL || CompareSystemTime(&stStart,&stNearest)<0)) {
			pNextEvent=&itrEvent->second;
			stNearest=stStart;
		}
	}
	if (pNextEvent!=NULL) {
		*pInfo=*pNextEvent;
		SetCommonEventInfo(pInfo);
		return true;
	}
	return false;
}


const CEventInfoData *CEpgProgramList::GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID)
{
	CEpgServiceInfo *pServiceInfo=GetServiceInfo(TSID,ServiceID);

	if (pServiceInfo==NULL)
		return NULL;
	return pServiceInfo->GetEventInfo(EventID);
}


bool CEpgProgramList::SetCommonEventInfo(CEventInfoData *pInfo)
{
	if (pInfo->m_fCommonEvent) {
		CEventManager::CEventInfo EventInfo;
		if (m_pEventManager->GetEventInfo(pInfo->m_OriginalNID,
										  pInfo->m_TSID,
										  pInfo->m_ServiceID,
										  pInfo->m_CommonEventInfo.EventID,
										  &EventInfo)) {
			pInfo->SetEventName(EventInfo.GetEventName());
			pInfo->SetEventText(EventInfo.GetEventText());
			pInfo->SetEventExtText(EventInfo.GetEventExtendedText());
			pInfo->m_ComponentType=EventInfo.GetVideoInfo().ComponentType;
			const CEventManager::CEventInfo::AudioList &AudioList=EventInfo.GetAudioList();
			pInfo->m_AudioList.resize(AudioList.size());
			for (size_t i=0;i<AudioList.size();i++) {
				const CEventManager::CEventInfo::AudioInfo &Audio=AudioList[0];
				pInfo->m_AudioList[i].ComponentType=Audio.ComponentType;
				pInfo->m_AudioList[i].fESMultiLingualFlag=Audio.bESMultiLingualFlag;
				pInfo->m_AudioList[i].fMainComponentFlag=Audio.bMainComponentFlag;
				pInfo->m_AudioList[i].SamplingRate=Audio.SamplingRate;
				pInfo->m_AudioList[i].LanguageCode=Audio.LanguageCode;
				pInfo->m_AudioList[i].LanguageCode2=Audio.LanguageCode2;
				::lstrcpy(pInfo->m_AudioList[i].szText,Audio.szText);
			}
			const CEventManager::CEventInfo::ContentNibble &ContentNibble=EventInfo.GetContentNibble();
			pInfo->m_NibbleList.resize(ContentNibble.NibbleCount);
			for (int k=0;k<ContentNibble.NibbleCount;k++) {
				CEventInfoData::NibbleData &Nibble=pInfo->m_NibbleList[k];
				Nibble.m_ContentNibbleLv1=ContentNibble.NibbleList[k].ContentNibbleLevel1;
				Nibble.m_ContentNibbleLv2=ContentNibble.NibbleList[k].ContentNibbleLevel2;
				Nibble.m_UserNibbleLv1=ContentNibble.NibbleList[k].UserNibble1;
				Nibble.m_UserNibbleLv2=ContentNibble.NibbleList[k].UserNibble2;
			}
		} else {
			const CEventInfoData *pCommonEvent=
				GetEventInfo(pInfo->m_TSID,
							 pInfo->m_CommonEventInfo.ServiceID,
							 pInfo->m_CommonEventInfo.EventID);
			if (pCommonEvent==NULL)
				return false;
			pInfo->SetEventName(pCommonEvent->GetEventName());
			pInfo->SetEventText(pCommonEvent->GetEventText());
			pInfo->SetEventExtText(pCommonEvent->GetEventExtText());
			pInfo->m_ComponentType=pCommonEvent->m_ComponentType;
			pInfo->m_AudioList=pCommonEvent->m_AudioList;
			pInfo->m_NibbleList=pCommonEvent->m_NibbleList;
		}
	}
	return true;
}


#include <pshpack1.h>

/*
	EPGファイルのフォーマット
	┌─────────────────────┐
	│EpgListFileHeader                         │
	├─────────────────────┤
	│┌───────────────────┐│
	││ServiceInfoHeader2                    ││
	│├───────────────────┤│
	││┌─────────────────┐││
	│││EventInfoHeader2                  │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││EventAudioHeader              ││││
	│││├───────────────┤│││
	││││音声コンポーネントテキスト    ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││├─────────────────┤││
	│││┌───────────────┐│││
	││││NibbleData                    ││││
	│││└───────────────┘│││
	│││ ...                              │││
	││├─────────────────┤││
	│││(イベント名)                      │││
	││├─────────────────┤││
	│││(イベントテキスト)                │││
	││├─────────────────┤││
	│││(イベント拡張テキスト)            │││
	││├─────────────────┤││
	│││CRC                               │││
	││└─────────────────┘││
	││ ...                                  ││
	│└───────────────────┘│
	│ ...                                      │
	└─────────────────────┘
*/

struct EpgListFileHeader {
	char Type[8];
	DWORD Version;
	DWORD NumServices;
};

#define EPGLISTFILEHEADER_TYPE		"EPG-LIST"
#define EPGLISTFILEHEADER_VERSION	1

struct ServiceInfoHeader {
	DWORD NumEvents;
	WORD OriginalNID;
	WORD TSID;
	WORD ServiceID;
	WORD ServiceType;
	ServiceInfoHeader() {}
	ServiceInfoHeader(const CServiceInfoData &Data)
		: NumEvents(0)
		, OriginalNID(Data.m_OriginalNID)
		, TSID(Data.m_TSID)
		, ServiceID(Data.m_ServiceID)
		, ServiceType(0)
	{
	}
};

struct EventInfoHeader {
	WORD ServiceID;
	WORD EventID;
	SYSTEMTIME StartTime;
	DWORD DurationSec;
	BYTE ComponentType;
	BYTE AudioComponentType;
	BYTE ESMultiLangFlag;
	BYTE MainComponentFlag;
	BYTE SamplingRate;
	BYTE Reserved[3];
	DWORD ContentNibbleListCount;
};

struct ServiceInfoHeader2 {
	WORD OriginalNetworkID;
	WORD TransportStreamID;
	WORD ServiceID;
	WORD NumEvents;
	DWORD CRC;
	ServiceInfoHeader2() {}
	ServiceInfoHeader2(const CServiceInfoData &Data)
		: OriginalNetworkID(Data.m_OriginalNID)
		, TransportStreamID(Data.m_TSID)
		, ServiceID(Data.m_ServiceID)
		, NumEvents(0)
	{
	}
};

#define TEXT_FLAG_EVENTNAME		0x80
#define TEXT_FLAG_EVENTTEXT		0x40
#define TEXT_FLAG_EVENTEXTTEXT	0x20

struct EventInfoHeader2 {
	WORD EventID;
	WORD CommonServiceID;
	WORD CommonEventID;
	WORD Reserved1;
	ULONGLONG UpdateTime;
	SYSTEMTIME StartTime;
	DWORD Duration;
	BYTE ComponentType;
	BYTE AudioListCount;
	BYTE ContentNibbleListCount;
	BYTE TextFlags;
	EventInfoHeader2() {}
	EventInfoHeader2(const CEventInfoData &Data)
		: EventID(Data.m_EventID)
		, CommonServiceID(Data.m_fCommonEvent?Data.m_CommonEventInfo.ServiceID:0)
		, CommonEventID(Data.m_fCommonEvent?Data.m_CommonEventInfo.EventID:0)
		, Reserved1(0)
		, UpdateTime(Data.m_UpdateTime)
		, StartTime(Data.m_stStartTime)
		, Duration(Data.m_DurationSec)
		, ComponentType(Data.m_ComponentType)
		, AudioListCount((BYTE)Data.m_AudioList.size())
		, ContentNibbleListCount((BYTE)Data.m_NibbleList.size())
		, TextFlags((Data.GetEventName()!=NULL?TEXT_FLAG_EVENTNAME:0) |
					(Data.GetEventText()!=NULL?TEXT_FLAG_EVENTTEXT:0) |
					(Data.GetEventExtText()!=NULL?TEXT_FLAG_EVENTEXTTEXT:0))
	{
	}
};

struct EventAudioHeader {
	BYTE Flags;
	BYTE ComponentType;
	BYTE SamplingRate;
	BYTE Reserved;
	DWORD LanguageCode;
	DWORD LanguageCode2;
};

#define AUDIO_FLAG_MULTILINGUAL		0x01
#define AUDIO_FLAG_MAINCOMPONENT	0x02

#define MAX_EPG_TEXT_LENGTH 1024
#define MAX_CONTENT_NIBBLE_COUNT 7

#include <poppack.h>


static bool ReadData(CNFile *pFile,void *pData,DWORD DataSize,CCrc32 *pCrc)
{
	if (pFile->Read(pData,DataSize)!=DataSize)
		return false;
	pCrc->Calc(pData,DataSize);
	return true;
}

static bool ReadString(CNFile *pFile,LPWSTR *ppszString,CCrc32 *pCrc)
{
	WORD Length;

	*ppszString=NULL;
	if (!ReadData(pFile,&Length,sizeof(WORD),pCrc)
			|| Length>MAX_EPG_TEXT_LENGTH)
		return false;
	if (Length>0) {
		*ppszString=new WCHAR[Length+1];
		if (!ReadData(pFile,*ppszString,Length*sizeof(WCHAR),pCrc)) {
			delete [] *ppszString;
			*ppszString=NULL;
			return false;
		}
		(*ppszString)[Length]='\0';
	}
	return true;
}

// 旧形式用
static bool ReadString(CNFile *pFile,LPWSTR *ppszString)
{
	DWORD Length;

	*ppszString=NULL;
	if (pFile->Read(&Length,sizeof(DWORD))!=sizeof(DWORD)
			|| Length>MAX_EPG_TEXT_LENGTH)
		return false;
	if (Length>0) {
		*ppszString=new WCHAR[Length+1];
		if (pFile->Read(*ppszString,Length*sizeof(WCHAR))!=Length*sizeof(WCHAR)) {
			delete [] *ppszString;
			*ppszString=NULL;
			return false;
		}
		(*ppszString)[Length]='\0';
	}
	return true;
}

static bool WriteData(CNFile *pFile,const void *pData,DWORD DataSize,CCrc32 *pCrc)
{
	pCrc->Calc(pData,DataSize);
	return pFile->Write(pData,DataSize);
}

static bool WriteString(CNFile *pFile,LPCWSTR pszString,CCrc32 *pCrc)
{
	WORD Length;

	if (pszString!=NULL) {
		Length=(WORD)::lstrlenW(pszString);
		if (Length>MAX_EPG_TEXT_LENGTH)
			Length=MAX_EPG_TEXT_LENGTH;
	} else
		Length=0;
	if (!WriteData(pFile,&Length,sizeof(WORD),pCrc))
		return false;
	if (Length>0) {
		if (!WriteData(pFile,pszString,Length*sizeof(WCHAR),pCrc))
			return false;
	}
	return true;
}

static bool WriteCRC(CNFile *pFile,const CCrc32 *pCrc)
{
	DWORD CRC32=pCrc->GetCrc();
	return pFile->Write(&CRC32,sizeof(CRC32));
}


bool CEpgProgramList::LoadFromFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);
	CNFile File;

	if (!File.Open(pszFileName,CNFile::CNF_READ | CNFile::CNF_SHAREREAD))
		return false;

	EpgListFileHeader FileHeader;

	if (File.Read(&FileHeader,sizeof(EpgListFileHeader))!=sizeof(EpgListFileHeader))
		return false;
	if (memcmp(FileHeader.Type,EPGLISTFILEHEADER_TYPE,sizeof(FileHeader.Type))!=0
			|| FileHeader.Version>EPGLISTFILEHEADER_VERSION)
		return false;

	Clear();

	for (DWORD i=0;i<FileHeader.NumServices;i++) {
		LPWSTR pszText;

		ServiceInfoHeader2 ServiceHeader2;
		if (FileHeader.Version==0) {
			ServiceInfoHeader ServiceHeader;

			if (File.Read(&ServiceHeader,sizeof(ServiceInfoHeader))!=sizeof(ServiceInfoHeader)
					|| !ReadString(&File,&pszText))
				goto OnError;
			delete [] pszText;
			if (ServiceHeader.NumEvents>0xFFFF)
				goto OnError;
			ServiceHeader2.OriginalNetworkID=ServiceHeader.OriginalNID;
			ServiceHeader2.TransportStreamID=ServiceHeader.TSID;
			ServiceHeader2.ServiceID=ServiceHeader.ServiceID;
			ServiceHeader2.NumEvents=(WORD)ServiceHeader.NumEvents;
		} else {
			if (File.Read(&ServiceHeader2,sizeof(ServiceInfoHeader2))!=sizeof(ServiceInfoHeader2))
				goto OnError;
			if (ServiceHeader2.CRC!=CCrcCalculator::CalcCrc32((const BYTE*)&ServiceHeader2,sizeof(ServiceInfoHeader2)-sizeof(DWORD))) {
				TRACE(TEXT("CEpgProgramList::LoadFromFile() : Service CRC error!\n"));
				goto OnError;
			}
		}

		CServiceInfoData ServiceData(ServiceHeader2.OriginalNetworkID,
									 ServiceHeader2.TransportStreamID,
									 ServiceHeader2.ServiceID);

		CEpgServiceInfo *pServiceInfo=new CEpgServiceInfo(ServiceData);
		if (!m_ServiceMap.insert(
				std::pair<ServiceMapKey,CEpgServiceInfo*>(
					GetServiceMapKey(ServiceData.m_OriginalNID,
									 ServiceData.m_TSID,
									 ServiceData.m_ServiceID),
					pServiceInfo)).second) {
			delete pServiceInfo;
			break;
		}

		bool fCRCError=false;

		for (WORD j=0;j<ServiceHeader2.NumEvents;j++) {
			CEventInfoData *pEventData;

			if (FileHeader.Version==0) {
				EventInfoHeader EventHeader;
				if (File.Read(&EventHeader,sizeof(EventInfoHeader))!=sizeof(EventInfoHeader))
					goto OnError;

				pEventData=
					&pServiceInfo->m_EventList.EventDataMap.insert(
						std::pair<WORD,CEventInfoData>(EventHeader.EventID,CEventInfoData())).first->second;

				pEventData->m_OriginalNID=ServiceHeader2.OriginalNetworkID;
				pEventData->m_TSID=ServiceHeader2.TransportStreamID;
				pEventData->m_ServiceID=EventHeader.ServiceID;
				pEventData->m_EventID=EventHeader.EventID;
				pEventData->m_fValidStartTime=true;
				pEventData->m_stStartTime=EventHeader.StartTime;
				pEventData->m_DurationSec=EventHeader.DurationSec;
				pEventData->m_ComponentType=EventHeader.ComponentType;
				pEventData->m_AudioList.resize(1);
				pEventData->m_AudioList[0].ComponentType=EventHeader.AudioComponentType;
				pEventData->m_AudioList[0].fESMultiLingualFlag=EventHeader.ESMultiLangFlag!=0;
				//pEventData->m_AudioList[0].fMainComponentFlag=EventHeader.MainComponentFlag!=0;
				pEventData->m_AudioList[0].fMainComponentFlag=true;
				pEventData->m_AudioList[0].SamplingRate=EventHeader.SamplingRate;
				pEventData->m_AudioList[0].LanguageCode=0;
				pEventData->m_AudioList[0].LanguageCode2=0;
				pEventData->m_AudioList[0].szText[0]='\0';
				if (EventHeader.ContentNibbleListCount>0) {
					if (EventHeader.ContentNibbleListCount>MAX_CONTENT_NIBBLE_COUNT)
						goto OnError;
					pEventData->m_NibbleList.resize(EventHeader.ContentNibbleListCount);
					for (DWORD k=0;k<EventHeader.ContentNibbleListCount;k++) {
						CEventInfoData::NibbleData Nibble;
						if (File.Read(&Nibble,sizeof(Nibble))!=sizeof(Nibble))
							goto OnError;
						pEventData->m_NibbleList[k]=Nibble;
					}
				}
				pEventData->m_fCommonEvent=false;
				pEventData->m_UpdateTime=0;
				if (!ReadString(&File,&pEventData->m_pszEventName)
						|| !ReadString(&File,&pEventData->m_pszEventText)
						|| !ReadString(&File,&pEventData->m_pszEventExtText)
						|| !ReadString(&File,&pEventData->m_pszComponentTypeText))
					goto OnError;
				if (!ReadString(&File,&pszText))
					goto OnError;
				if (pszText!=NULL) {
					::lstrcpyn(pEventData->m_AudioList[0].szText,pszText,CEventInfoData::AudioInfo::MAX_TEXT);
					delete [] pszText;
				}
			} else {
				EventInfoHeader2 EventHeader2;
				CCrc32 CRC;
				if (!ReadData(&File,&EventHeader2,sizeof(EventInfoHeader2),&CRC))
					goto OnError;

				CEventInfoList::EventMap::iterator itrEvent=
					pServiceInfo->m_EventList.EventDataMap.insert(
						std::pair<WORD,CEventInfoData>(EventHeader2.EventID,CEventInfoData())).first;
				pEventData=&itrEvent->second;

				pEventData->m_OriginalNID=ServiceHeader2.OriginalNetworkID;
				pEventData->m_TSID=ServiceHeader2.TransportStreamID;
				pEventData->m_ServiceID=ServiceHeader2.ServiceID;
				pEventData->m_EventID=EventHeader2.EventID;
				pEventData->m_fValidStartTime=true;
				pEventData->m_stStartTime=EventHeader2.StartTime;
				pEventData->m_DurationSec=EventHeader2.Duration;
				pEventData->m_ComponentType=EventHeader2.ComponentType;
				pEventData->m_AudioList.resize(EventHeader2.AudioListCount);
				if (EventHeader2.AudioListCount>0) {
					for (int k=0;k<EventHeader2.AudioListCount;k++) {
						EventAudioHeader AudioHeader;
						if (!ReadData(&File,&AudioHeader,sizeof(AudioHeader),&CRC))
							goto OnError;
						CEventInfoData::AudioInfo &AudioInfo=pEventData->m_AudioList[k];
						AudioInfo.ComponentType=AudioHeader.ComponentType;
						AudioInfo.fESMultiLingualFlag=(AudioHeader.Flags&AUDIO_FLAG_MULTILINGUAL)!=0;
						AudioInfo.fMainComponentFlag=(AudioHeader.Flags&AUDIO_FLAG_MAINCOMPONENT)!=0;
						AudioInfo.SamplingRate=AudioHeader.SamplingRate;
						AudioInfo.LanguageCode=AudioHeader.LanguageCode;
						AudioInfo.LanguageCode2=AudioHeader.LanguageCode2;
						AudioInfo.szText[0]='\0';
						if (!ReadString(&File,&pszText,&CRC))
							goto OnError;
						if (pszText!=NULL) {
							::lstrcpyn(AudioInfo.szText,pszText,CEventInfoData::AudioInfo::MAX_TEXT);
							delete [] pszText;
						}
					}
				}
				if (EventHeader2.ContentNibbleListCount>0) {
					if (EventHeader2.ContentNibbleListCount>MAX_CONTENT_NIBBLE_COUNT)
						goto OnError;
					pEventData->m_NibbleList.resize(EventHeader2.ContentNibbleListCount);
					for (int k=0;k<EventHeader2.ContentNibbleListCount;k++) {
						CEventInfoData::NibbleData Nibble;
						if (!ReadData(&File,&Nibble,sizeof(Nibble),&CRC))
							goto OnError;
						pEventData->m_NibbleList[k]=Nibble;
					}
				}
				pEventData->m_fCommonEvent=EventHeader2.CommonServiceID!=0
											&& EventHeader2.CommonEventID!=0;
				if (pEventData->m_fCommonEvent) {
					pEventData->m_CommonEventInfo.ServiceID=EventHeader2.CommonServiceID;
					pEventData->m_CommonEventInfo.EventID=EventHeader2.CommonEventID;
				}
				pEventData->m_UpdateTime=EventHeader2.UpdateTime;
				for (int k=0;k<8;k++) {
					if ((EventHeader2.TextFlags&(0x80>>k))!=0) {
						if (!ReadString(&File,&pszText,&CRC))
							goto OnError;
						if (pszText!=NULL) {
							switch (k) {
							case 0: pEventData->m_pszEventName=pszText;		break;
							case 1: pEventData->m_pszEventText=pszText;		break;
							case 2: pEventData->m_pszEventExtText=pszText;	break;
							default: delete [] pszText;
							}
						}
					}
				}
				DWORD CRC32;
				if (File.Read(&CRC32,sizeof(DWORD))!=sizeof(DWORD))
					goto OnError;
				if (CRC32!=CRC.GetCrc()) {
					TRACE(TEXT("CEpgProgramList::LoadFromFile() : Event CRC error!\n"));
					// 2回続けてCRCエラーの場合は読み込み中止
					if (fCRCError)
						goto OnError;
					fCRCError=true;
					pServiceInfo->m_EventList.EventDataMap.erase(itrEvent);
				} else {
					fCRCError=false;
				}
			}
		}
	}

	File.GetTime(NULL,NULL,&m_LastWriteTime);
	return true;

OnError:
	Clear();
	return false;
}


bool CEpgProgramList::SaveToFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);

	TCHAR szName[MAX_PATH+8];
	::lstrcpy(szName,pszFileName);
	::CharUpperBuff(szName,::lstrlen(szName));
	for (int i=0;szName[i]!='\0';i++) {
		if (szName[i]=='\\')
			szName[i]=':';
	}
	::lstrcat(szName,TEXT(":Lock"));
	CGlobalLock GlobalLock(szName);
	if (!GlobalLock.Wait(10000)) {
		TRACE(TEXT("CEpgProgramList::SaveToFile() : Timeout\n"));
		return false;
	}

	// ファイルが読み込んだ時から更新されている場合読み込み直す
	// (複数起動して他のプロセスが更新した可能性があるため)
	WIN32_FIND_DATA fd;
	HANDLE hFind=::FindFirstFile(pszFileName,&fd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		::FindClose(hFind);
		if ((m_LastWriteTime.dwLowDateTime==0 && m_LastWriteTime.dwHighDateTime==0)
				|| ::CompareFileTime(&fd.ftLastWriteTime,&m_LastWriteTime)>0) {
			TRACE(TEXT("CEpgProgramList::SaveToFile() : Reload\n"));
			CEpgProgramList List(m_pEventManager);
			if (List.LoadFromFile(pszFileName))
				Merge(&List);
		}
	}

	CNFile File;

	if (!File.Open(pszFileName,CNFile::CNF_WRITE | CNFile::CNF_NEW)) {
		GlobalLock.Release();
		return false;
	}

	SYSTEMTIME stCurrent,st;
	::GetLocalTime(&stCurrent);

	WORD *pNumEvents=new WORD[m_ServiceMap.size()];
	DWORD NumServices=0;
	size_t ServiceIndex=0;
	for (ServiceMap::iterator itrService=m_ServiceMap.begin();itrService!=m_ServiceMap.end();itrService++) {
		const CEpgServiceInfo *pServiceInfo=itrService->second;
		CEventInfoList::EventMap::const_iterator itrEvent;
		WORD NumEvents=0;

		for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
				itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();
				itrEvent++) {
			if (itrEvent->second.GetEndTime(&st)
					&& CompareSystemTime(&st,&stCurrent)>0)
				NumEvents++;
		}
		pNumEvents[ServiceIndex++]=NumEvents;
		if (NumEvents>0)
			NumServices++;
	}

	EpgListFileHeader FileHeader;

	::CopyMemory(FileHeader.Type,EPGLISTFILEHEADER_TYPE,sizeof(FileHeader.Type));
	FileHeader.Version=EPGLISTFILEHEADER_VERSION;
	FileHeader.NumServices=NumServices;

	if (!File.Write(&FileHeader,sizeof(EpgListFileHeader)))
		goto OnError;

	ServiceIndex=0;
	for (ServiceMap::iterator itrService=m_ServiceMap.begin();
		 	itrService!=m_ServiceMap.end();itrService++,ServiceIndex++) {
		if (pNumEvents[ServiceIndex]==0)
			continue;

		const CEpgServiceInfo *pServiceInfo=itrService->second;
		ServiceInfoHeader2 ServiceHeader2(pServiceInfo->m_ServiceData);
		ServiceHeader2.NumEvents=pNumEvents[ServiceIndex];
		ServiceHeader2.CRC=CCrcCalculator::CalcCrc32((const BYTE*)&ServiceHeader2,sizeof(ServiceInfoHeader2)-sizeof(DWORD));
		if (!File.Write(&ServiceHeader2,sizeof(ServiceInfoHeader2)))
			goto OnError;

		if (ServiceHeader2.NumEvents>0) {
			CEventInfoList::EventMap::const_iterator itrEvent;

			for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
					itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();
					itrEvent++) {
				EventInfoHeader2 EventHeader2(itrEvent->second);

				if (itrEvent->second.GetEndTime(&st)
						&& CompareSystemTime(&st,&stCurrent)>0) {
					CCrc32 CRC;
					if (!WriteData(&File,&EventHeader2,sizeof(EventInfoHeader2),&CRC))
						goto OnError;
					if (EventHeader2.AudioListCount>0) {
						for (size_t i=0;i<itrEvent->second.m_AudioList.size();i++) {
							const CEventInfoData::AudioInfo &Audio=itrEvent->second.m_AudioList[i];
							EventAudioHeader AudioHeader;
							AudioHeader.Flags=0;
							if (Audio.fESMultiLingualFlag)
								AudioHeader.Flags|=AUDIO_FLAG_MULTILINGUAL;
							if (Audio.fMainComponentFlag)
								AudioHeader.Flags|=AUDIO_FLAG_MAINCOMPONENT;
							AudioHeader.ComponentType=Audio.ComponentType;
							AudioHeader.SamplingRate=Audio.SamplingRate;
							AudioHeader.LanguageCode=Audio.LanguageCode;
							AudioHeader.LanguageCode2=Audio.LanguageCode2;
							AudioHeader.Reserved=0;
							if (!WriteData(&File,&AudioHeader,sizeof(EventAudioHeader),&CRC)
									|| !WriteString(&File,Audio.szText,&CRC))
								goto OnError;
						}
					}
					if (EventHeader2.ContentNibbleListCount>0) {
						if (EventHeader2.ContentNibbleListCount>MAX_CONTENT_NIBBLE_COUNT)
							EventHeader2.ContentNibbleListCount=MAX_CONTENT_NIBBLE_COUNT;
						for (int i=0;i<EventHeader2.ContentNibbleListCount;i++) {
							CEventInfoData::NibbleData Nibble=itrEvent->second.m_NibbleList[i];
							if (!WriteData(&File,&Nibble,sizeof(Nibble),&CRC))
								goto OnError;
						}
					}
					if (((EventHeader2.TextFlags&TEXT_FLAG_EVENTNAME)!=0
								&& !WriteString(&File,itrEvent->second.GetEventName(),&CRC))
							|| ((EventHeader2.TextFlags&TEXT_FLAG_EVENTTEXT)!=0
								&& !WriteString(&File,itrEvent->second.GetEventText(),&CRC))
							|| ((EventHeader2.TextFlags&TEXT_FLAG_EVENTEXTTEXT)!=0
								&& !WriteString(&File,itrEvent->second.GetEventExtText(),&CRC)))
						goto OnError;
					if (!WriteCRC(&File,&CRC))
						goto OnError;
				}
			}
		}
	}

	delete [] pNumEvents;

	File.Close();

	HANDLE hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
	if (hFile!=INVALID_HANDLE_VALUE) {
		::GetFileTime(hFile,NULL,NULL,&m_LastWriteTime);
		::CloseHandle(hFile);
	}

	GlobalLock.Release();
	return true;

OnError:
	delete [] pNumEvents;
	File.Close();
	::DeleteFile(pszFileName);
	GlobalLock.Release();
	return false;
}


bool CEpgProgramList::Merge(CEpgProgramList *pSrcList)
{
	ServiceMap::iterator itrSrcService;

	for (itrSrcService=pSrcList->m_ServiceMap.begin();
			itrSrcService!=pSrcList->m_ServiceMap.end();) {
		CEpgServiceInfo *pSrcServiceInfo=itrSrcService->second;
		ServiceMapKey Key=GetServiceMapKey(
			pSrcServiceInfo->m_ServiceData.m_OriginalNID,
			pSrcServiceInfo->m_ServiceData.m_TSID,
			pSrcServiceInfo->m_ServiceData.m_ServiceID);
		ServiceMap::iterator itrDstService=m_ServiceMap.find(Key);
		if (itrDstService==m_ServiceMap.end()) {
			m_ServiceMap.insert(std::pair<ServiceMapKey,CEpgServiceInfo*>(Key,pSrcServiceInfo));
			pSrcList->m_ServiceMap.erase(itrSrcService++);
		} else {
			CEventInfoList::EventMap::iterator itrEvent;

			for (itrEvent=pSrcServiceInfo->m_EventList.EventDataMap.begin();
					itrEvent!=pSrcServiceInfo->m_EventList.EventDataMap.end();
					itrEvent++) {
				itrDstService->second->m_EventList.EventDataMap.insert(
					std::pair<WORD,CEventInfoData>(itrEvent->first,itrEvent->second));
			}
			itrSrcService++;
		}
	}
	return true;
}
