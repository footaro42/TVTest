#include "stdafx.h"
#include "TVTest.h"
#include "EpgProgramList.h"
#include "NFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CServiceInfoData::CServiceInfoData()
{
	m_OriginalNID=0;
	m_TSID=0;
	m_ServiceID=0;
	m_ServiceType=0;
	m_pszServiceName=NULL;
}


CServiceInfoData::CServiceInfoData(const CServiceInfoData &Info)
{
	m_OriginalNID=Info.m_OriginalNID;
	m_TSID=Info.m_TSID;
	m_ServiceID=Info.m_ServiceID;
	m_ServiceType=Info.m_ServiceType;
	m_pszServiceName=DuplicateString(Info.m_pszServiceName);
}


CServiceInfoData::CServiceInfoData(WORD OriginalNID,WORD TSID,WORD ServiceID,WORD ServiceType,LPCWSTR pszServiceName)
{
	m_OriginalNID=OriginalNID;
	m_TSID=TSID;
	m_ServiceID=ServiceID;
	m_ServiceType=ServiceType;
	m_pszServiceName=DuplicateString(pszServiceName);
}


CServiceInfoData::~CServiceInfoData()
{
	delete [] m_pszServiceName;
}


CServiceInfoData &CServiceInfoData::operator=(const CServiceInfoData &Info)
{
	if (&Info!=this) {
		m_OriginalNID=Info.m_OriginalNID;
		m_TSID=Info.m_TSID;
		m_ServiceID=Info.m_ServiceID;
		m_ServiceType=Info.m_ServiceType;
		ReplaceString(&m_pszServiceName,Info.m_pszServiceName);
	}
	return *this;
}


bool CServiceInfoData::operator==(const CServiceInfoData &Info) const
{
	return m_OriginalNID==Info.m_OriginalNID
		&& m_TSID==Info.m_TSID
		&& m_ServiceID==Info.m_ServiceID
		&& m_ServiceType==Info.m_ServiceType
		&& ((m_pszServiceName==NULL && Info.m_pszServiceName==NULL)
			|| (m_pszServiceName!=NULL && Info.m_pszServiceName!=NULL
				&& ::lstrcmp(m_pszServiceName,Info.m_pszServiceName)==0));
}


bool CServiceInfoData::SetServiceName(LPCWSTR pszName)
{
	return ReplaceString(&m_pszServiceName,pszName);
}




CEventInfoData::CEventInfoData()
{
	m_pszEventName=NULL;
	m_pszEventText=NULL;
	m_pszEventExtText=NULL;
	m_pszComponentTypeText=NULL;
	m_pszAudioComponentTypeText=NULL;
}


CEventInfoData::CEventInfoData(const CEventInfoData &Info)
{
	m_OriginalNID=Info.m_OriginalNID;
	m_TSID=Info.m_TSID;
	m_ServiceID=Info.m_ServiceID;
	m_EventID=Info.m_EventID;
	m_pszEventName=DuplicateString(Info.m_pszEventName);
	m_pszEventText=DuplicateString(Info.m_pszEventText);
	m_pszEventExtText=DuplicateString(Info.m_pszEventExtText);
	m_stStartTime=Info.m_stStartTime;
	m_DurationSec=Info.m_DurationSec;
	m_ComponentType=Info.m_ComponentType;
	m_pszComponentTypeText=DuplicateString(Info.m_pszComponentTypeText);
	m_AudioComponentType=Info.m_AudioComponentType;
	m_ESMultiLangFlag=Info.m_ESMultiLangFlag;
	m_MainComponentFlag=Info.m_MainComponentFlag;
	m_SamplingRate=Info.m_SamplingRate;
	m_pszAudioComponentTypeText=DuplicateString(Info.m_pszAudioComponentTypeText);
	m_NibbleList=Info.m_NibbleList;
}


CEventInfoData::~CEventInfoData()
{
	delete [] m_pszEventName;
	delete [] m_pszEventText;
	delete [] m_pszEventExtText;
	delete [] m_pszComponentTypeText;
	delete [] m_pszAudioComponentTypeText;
}


CEventInfoData &CEventInfoData::operator=(const CEventInfoData &Info)
{
	if (&Info==this)
		return *this;
	m_OriginalNID=Info.m_OriginalNID;
	m_TSID=Info.m_TSID;
	m_ServiceID=Info.m_ServiceID;
	m_EventID=Info.m_EventID;
	SetEventName(Info.m_pszEventName);
	SetEventText(Info.m_pszEventText);
	SetEventExtText(Info.m_pszEventExtText);
	m_stStartTime=Info.m_stStartTime;
	m_DurationSec=Info.m_DurationSec;
	m_ComponentType=Info.m_ComponentType;
	SetComponentTypeText(Info.m_pszComponentTypeText);
	m_AudioComponentType=Info.m_AudioComponentType;
	m_ESMultiLangFlag=Info.m_ESMultiLangFlag;
	m_MainComponentFlag=Info.m_MainComponentFlag;
	m_SamplingRate=Info.m_SamplingRate;
	SetAudioComponentTypeText(Info.m_pszAudioComponentTypeText);
	m_NibbleList=Info.m_NibbleList;
	return *this;
}


bool CEventInfoData::operator==(const CEventInfoData &Info) const
{
	return m_OriginalNID==Info.m_OriginalNID
		&& m_TSID==Info.m_TSID
		&& m_ServiceID==Info.m_ServiceID
		&& m_EventID==Info.m_EventID
		&& ((m_pszEventName==NULL && Info.m_pszEventName==NULL)
			|| (m_pszEventName!=NULL && Info.m_pszEventName!=NULL
				&& ::lstrcmp(m_pszEventName,Info.m_pszEventName)==0))
		&& ((m_pszEventText==NULL && Info.m_pszEventText==NULL)
			|| (m_pszEventText!=NULL && Info.m_pszEventText!=NULL
				&& ::lstrcmp(m_pszEventText,Info.m_pszEventText)==0))
		&& ((m_pszEventExtText==NULL && Info.m_pszEventExtText==NULL)
			|| (m_pszEventExtText!=NULL && Info.m_pszEventExtText!=NULL
				&& ::lstrcmp(m_pszEventExtText,Info.m_pszEventExtText)==0))
		&& ::memcmp(&m_stStartTime,&Info.m_stStartTime,sizeof(SYSTEMTIME))==0
		&& m_DurationSec==Info.m_DurationSec
		&& m_ComponentType==Info.m_ComponentType
		&& ((m_pszComponentTypeText==NULL && Info.m_pszComponentTypeText==NULL)
			|| (m_pszComponentTypeText!=NULL && Info.m_pszComponentTypeText!=NULL
				&& ::lstrcmp(m_pszComponentTypeText,Info.m_pszComponentTypeText)==0))
		&& m_AudioComponentType==Info.m_AudioComponentType
		&& m_ESMultiLangFlag==Info.m_ESMultiLangFlag
		&& m_MainComponentFlag==Info.m_MainComponentFlag
		&& m_SamplingRate==Info.m_SamplingRate
		&& ((m_pszAudioComponentTypeText==NULL && Info.m_pszAudioComponentTypeText==NULL)
			|| (m_pszAudioComponentTypeText!=NULL && Info.m_pszAudioComponentTypeText!=NULL
				&& ::lstrcmp(m_pszAudioComponentTypeText,Info.m_pszAudioComponentTypeText)==0))
		&& m_NibbleList==Info.m_NibbleList;
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


bool CEventInfoData::SetAudioComponentTypeText(LPCWSTR pszText)
{
	return ReplaceString(&m_pszAudioComponentTypeText,pszText);
}


bool CEventInfoData::GetStartTime(SYSTEMTIME *pTime) const
{
	*pTime=m_stStartTime;
	return true;
}


bool CEventInfoData::GetEndTime(SYSTEMTIME *pTime) const
{
	FILETIME ft;

	if (!::SystemTimeToFileTime(&m_stStartTime,&ft))
		return false;
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
	EventIterator itrEvent;

	itrEvent=EventDataMap.find(EventID);
	if (itrEvent==EventDataMap.end())
		return NULL;
	return &itrEvent->second;
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




CEpgProgramList::CEpgProgramList()
{
	m_pEpgDll=NULL;
}


CEpgProgramList::~CEpgProgramList()
{
}


inline bool IsMemoryZero(const BYTE *pData,int Size)
{
	for (int i=0;i<Size;i++) {
		if (pData[i]!=0)
			return false;
	}
	return true;
}

bool CEpgProgramList::UpdateService(const SERVICE_INFO *pService)
{
	CServiceInfoData ServiceData((WORD)pService->dwOriginalNID,
								 (WORD)pService->dwTSID,
								 (WORD)pService->dwServiceID,
								 (WORD)pService->dwServiceType,
								 pService->lpwszServiceName);
	CEpgServiceInfo ServiceInfo(ServiceData);

	EPG_DATA_INFO2* pEpgData=NULL;
	DWORD dwEpgDataCount=0;
	if (!m_pEpgDll->GetEpgDataListDB(ServiceData.m_OriginalNID,
									ServiceData.m_TSID,ServiceData.m_ServiceID,
									&pEpgData,&dwEpgDataCount)
			|| dwEpgDataCount==0)
		return false;

	SYSTEMTIME stOldestTime,stNewestTime;
	FILETIME ftNewestTime;
	::FillMemory(&stOldestTime,sizeof(SYSTEMTIME),0xFF);
	::FillMemory(&ftNewestTime,sizeof(FILETIME),0);

	FILETIME ft;
	ULARGE_INTEGER li;
	LONGLONG BaseTime;
	GetLocalTimeAsFileTime(&ft);
	li.LowPart=ft.dwLowDateTime;
	li.HighPart=ft.dwHighDateTime;
	BaseTime=(li.QuadPart-FILETIME_HOUR)/FILETIME_MINUTE;
	const int TimeTableLength=14*24*60;	// 1分単位で2週間分
	BYTE TimeTable[TimeTableLength];
	::ZeroMemory(TimeTable,TimeTableLength);

	for (DWORD j=0;j<dwEpgDataCount;j++) {
		CEventInfoData EventData;

		EventData.m_OriginalNID=(WORD)pEpgData[j].dwOriginalNID;
		EventData.m_TSID=(WORD)pEpgData[j].dwTSID;
		EventData.m_ServiceID=(WORD)pEpgData[j].dwServiceID;
		EventData.m_EventID=(WORD)pEpgData[j].dwEventID;
		EventData.SetEventName(pEpgData[j].lpwszEventName);
		EventData.SetEventText(pEpgData[j].lpwszEventText);
		EventData.SetEventExtText(pEpgData[j].lpwszEventExtText);
		EventData.m_stStartTime=pEpgData[j].stStartTime;
		EventData.m_stStartTime.wDayOfWeek=CalcDayOfWeek(
									EventData.m_stStartTime.wYear,
									EventData.m_stStartTime.wMonth,
									EventData.m_stStartTime.wDay);
		EventData.m_DurationSec=pEpgData[j].dwDurationSec;
		EventData.m_ComponentType=pEpgData[j].ucComponentType;
		EventData.SetComponentTypeText(pEpgData[j].lpwszComponentTypeText);
		EventData.m_AudioComponentType=pEpgData[j].ucAudioComponentType;
		EventData.m_ESMultiLangFlag=pEpgData[j].ucESMultiLangFlag;
		EventData.m_MainComponentFlag=pEpgData[j].ucMainComponentFlag;
		EventData.m_SamplingRate=pEpgData[j].ucSamplingRate;
		EventData.SetAudioComponentTypeText(pEpgData[j].lpwszAudioComponentTypeText);
		for (DWORD k=0;k<pEpgData[j].dwContentNibbleListCount;k++) {
			CEventInfoData::NibbleData Nibble;

			Nibble.m_ContentNibbleLv1=(BYTE)((pEpgData[j].dwContentNibbleList[k]&0xFF000000)>>24);
			Nibble.m_ContentNibbleLv2=(BYTE)((pEpgData[j].dwContentNibbleList[k]&0x00FF0000)>>16);
			Nibble.m_UserNibbleLv1=(BYTE)((pEpgData[j].dwContentNibbleList[k]&0x0000FF00)>>8);
			Nibble.m_UserNibbleLv2=(BYTE)(pEpgData[j].dwContentNibbleList[k]&0x000000FF);
			EventData.m_NibbleList.push_back(Nibble);
		}
		ServiceInfo.m_EventList.EventDataMap.insert(
				std::pair<WORD,CEventInfoData>(EventData.m_EventID,EventData));

		if (CompareSystemTime(&EventData.m_stStartTime,&stOldestTime)<0)
			stOldestTime=EventData.m_stStartTime;
		::SystemTimeToFileTime(&EventData.m_stStartTime,&ft);
		ft+=(LONGLONG)EventData.m_DurationSec*FILETIME_SECOND;
		//if (::CompareFileTime(&ft,&ftNewestTime)>0)
		if (ft.dwHighDateTime>ftNewestTime.dwHighDateTime
				|| (ft.dwHighDateTime==ftNewestTime.dwHighDateTime
					&& ft.dwLowDateTime>ftNewestTime.dwLowDateTime))
			ftNewestTime=ft;

		int Start,End;
		li.LowPart=ft.dwLowDateTime;
		li.HighPart=ft.dwHighDateTime;
		Start=(int)((LONGLONG)(li.QuadPart/FILETIME_MINUTE)-BaseTime);
		End=(int)((LONGLONG)((li.QuadPart+(ULONGLONG)EventData.m_DurationSec*FILETIME_SECOND+(FILETIME_MINUTE-1))/FILETIME_MINUTE)-BaseTime);
		if (Start<0)
			Start=0;
		if (End>TimeTableLength)
			End=TimeTableLength;
		if (Start<TimeTableLength && End>0 && Start<End)
			::FillMemory(&TimeTable[Start],End-Start,1);
	}
	::FileTimeToSystemTime(&ftNewestTime,&stNewestTime);

	TRACE(TEXT("CEpgProgramList::UpdateService() (%ld) %d/%d %d:%02d - %d/%d %d:%02d %ld Events\n"),
		  pService->dwServiceID,
		  stOldestTime.wMonth,stOldestTime.wDay,stOldestTime.wHour,stOldestTime.wMinute,
		  stNewestTime.wMonth,stNewestTime.wDay,stNewestTime.wHour,stNewestTime.wMinute,
		  dwEpgDataCount);

	// 既存のイベントで新しいリストに無いものを追加する
	ServiceMapKey Key=GenerateServiceMapKey(ServiceData.m_OriginalNID,
								ServiceData.m_TSID,ServiceData.m_ServiceID);
	ServiceIterator itrService=ServiceMap.find(Key);
	if (itrService!=ServiceMap.end()) {
		CEventInfoList::EventIterator itrEvent;
#ifdef _DEBUG
		int NewerCount=0,OlderCount=0,PaddingCount=0;
#endif

		for (itrEvent=itrService->second.m_EventList.EventDataMap.begin();
				itrEvent!=itrService->second.m_EventList.EventDataMap.end();
				itrEvent++) {
			bool fInsert=false;

			if (CompareSystemTime(&itrEvent->second.m_stStartTime,&stNewestTime)>=0) {
				fInsert=true;
#ifdef _DEBUG
				NewerCount++;
#endif
			} else {
				SYSTEMTIME stEnd;
				::SystemTimeToFileTime(&itrEvent->second.m_stStartTime,&ft);
				ft+=(LONGLONG)itrEvent->second.m_DurationSec*FILETIME_SECOND;
				::FileTimeToSystemTime(&ft,&stEnd);
				if (CompareSystemTime(&stEnd,&stOldestTime)<=0) {
					fInsert=true;
#ifdef _DEBUG
					OlderCount++;
#endif
				} else {
					int Start,End;
					li.LowPart=ft.dwLowDateTime;
					li.HighPart=ft.dwHighDateTime;
					Start=(int)((LONGLONG)(li.QuadPart/FILETIME_MINUTE)-BaseTime);
					End=(int)((LONGLONG)((li.QuadPart+(ULONGLONG)itrEvent->second.m_DurationSec*FILETIME_SECOND+(FILETIME_MINUTE-1))/FILETIME_MINUTE)-BaseTime);
					if (Start>=0 && Start<TimeTableLength
							&& End>0 && End<=TimeTableLength && Start<End
							&& IsMemoryZero(&TimeTable[Start],End-Start)) {
						fInsert=true;
#ifdef _DEBUG
						PaddingCount++;
#endif
					}
				}
			}
			if (fInsert)
				ServiceInfo.m_EventList.EventDataMap.insert(
					std::pair<WORD,CEventInfoData>(itrEvent->second.m_EventID,itrEvent->second));
		}
#ifdef _DEBUG
		TRACE(TEXT("古いイベントの生き残り Newer %d / Older %d / Padding %d (Total %d)\n"),
			  NewerCount,OlderCount,PaddingCount,
			  ServiceInfo.m_EventList.EventDataMap.size());
#endif
	}

	if (!ServiceMap.insert(std::pair<ServiceMapKey,CEpgServiceInfo>(Key,ServiceInfo)).second)
		ServiceMap[Key]=ServiceInfo;
	return true;
}


bool CEpgProgramList::UpdateProgramList()
{
	CBlockLock Lock(&m_Lock);

	if (m_pEpgDll==NULL)
		return false;

	SERVICE_INFO *pService=NULL;
	DWORD dwServiceCount=0;

	if (!m_pEpgDll->GetServiceListDB(&pService,&dwServiceCount))
		return false;
	for (DWORD i=0;i<dwServiceCount;i++) {
		if (pService[i].dwServiceType!=0x01 && pService[i].dwServiceType!=0xA5)
			continue;
		UpdateService(&pService[i]);
	}
	return true;
}


bool CEpgProgramList::UpdateProgramList(WORD TSID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);

	if (m_pEpgDll==NULL)
		return false;

	SERVICE_INFO *pService=NULL;
	DWORD dwServiceCount=0;

	if (!m_pEpgDll->GetServiceListDB(&pService,&dwServiceCount))
		return false;
	for (DWORD i=0;i<dwServiceCount;i++) {
		if ((pService[i].dwServiceType==0x01 || pService[i].dwServiceType==0xA5)
				&& (TSID==0 || (WORD)pService[i].dwTSID==TSID)
				&& (WORD)pService[i].dwServiceID==ServiceID) {
			return UpdateService(&pService[i]);
		}
	}
	return false;
}


void CEpgProgramList::Clear()
{
	CBlockLock Lock(&m_Lock);

	ServiceMap.clear();
}


int CEpgProgramList::NumServices() const
{
	return ServiceMap.size();
}


CEpgServiceInfo *CEpgProgramList::EnumService(int ServiceIndex)
{
	CBlockLock Lock(&m_Lock);
	ServiceIterator itrService;
	int i;

	if (ServiceIndex<0)
		return NULL;
	itrService=ServiceMap.begin();
	for (i=0;i<ServiceIndex && itrService!=ServiceMap.end();i++)
		itrService++;
	if (itrService==ServiceMap.end())
		return NULL;
	return &itrService->second;
}


CEpgServiceInfo *CEpgProgramList::GetServiceInfo(WORD OriginalNID,WORD TSID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);
	ServiceIterator itrService;

	itrService=ServiceMap.find(GenerateServiceMapKey(OriginalNID,TSID,ServiceID));
	if (itrService==ServiceMap.end())
		return NULL;
	return &itrService->second;
}


CEpgServiceInfo *CEpgProgramList::GetServiceInfo(WORD TSID,WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);
	ServiceIterator itrService;

	for (itrService=ServiceMap.begin();itrService!=ServiceMap.end();itrService++) {
		if ((TSID==0 || itrService->second.m_ServiceData.m_TSID==TSID)
				&& itrService->second.m_ServiceData.m_ServiceID==ServiceID)
			return &itrService->second;
	}
	return NULL;
}


bool CEpgProgramList::GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID,CEventInfoData *pInfo)
{
	const CEventInfoData *pEventInfo=GetEventInfo(TSID,ServiceID,EventID);

	if (pEventInfo==NULL)
		return false;
	*pInfo=*pEventInfo;
	return true;
}


const CEventInfoData *CEpgProgramList::GetEventInfo(WORD TSID,WORD ServiceID,WORD EventID)
{
	if (m_pEpgDll==NULL)
		return NULL;

	CEpgServiceInfo *pServiceInfo=GetServiceInfo(TSID,ServiceID);

	if (pServiceInfo==NULL)
		return NULL;
	return pServiceInfo->GetEventInfo(EventID);
}


bool CEpgProgramList::GetEventInfo(WORD TSID,WORD ServiceID,const SYSTEMTIME *pTime,CEventInfoData *pInfo)
{
	CBlockLock Lock(&m_Lock);

	if (m_pEpgDll==NULL)
		return NULL;

	CEpgServiceInfo *pServiceInfo;
	CEventInfoList::EventIterator itrEvent;

	pServiceInfo=GetServiceInfo(TSID,ServiceID);
	if (pServiceInfo==NULL)
		return NULL;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();itrEvent++) {
		SYSTEMTIME stStart,stEnd;

		itrEvent->second.GetStartTime(&stStart);
		itrEvent->second.GetEndTime(&stEnd);
		if (CompareSystemTime(&stStart,pTime)<=0
				&& CompareSystemTime(&stEnd,pTime)>0) {
			*pInfo=itrEvent->second;
			return true;
		}
	}
	return false;
}


#include <pshpack1.h>


struct EpgListFileHeader {
	char Type[8];
	DWORD Version;
	DWORD NumServices;
};

#define EPGLISTFILEHEADER_TYPE		"EPG-LIST"
#define EPGLISTFILEHEADER_VERSION	0

struct ServiceInfoHeader {
	DWORD NumEvents;
	WORD OriginalNID;
	WORD TSID;
	WORD ServiceID;
	WORD ServiceType;
	ServiceInfoHeader() {}
	ServiceInfoHeader(const CServiceInfoData &Data) {
		NumEvents=0;
		OriginalNID=Data.m_OriginalNID;
		TSID=Data.m_TSID;
		ServiceID=Data.m_ServiceID;
		ServiceType=Data.m_ServiceType;
	};
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
	EventInfoHeader() {}
	EventInfoHeader(const CEventInfoData &Data) {
		ServiceID=Data.m_ServiceID;
		EventID=Data.m_EventID;
		StartTime=Data.m_stStartTime;
		DurationSec=Data.m_DurationSec;
		ComponentType=Data.m_ComponentType;
		AudioComponentType=Data.m_AudioComponentType;
		ESMultiLangFlag=Data.m_ESMultiLangFlag;
		MainComponentFlag=Data.m_MainComponentFlag;
		SamplingRate=Data.m_SamplingRate;
		Reserved[0]=0;
		Reserved[1]=0;
		Reserved[2]=0;
		ContentNibbleListCount=Data.m_NibbleList.size();
	}
};


#include <poppack.h>


static bool ReadString(CNFile *pFile,LPWSTR *ppszString)
{
	DWORD Length;

	*ppszString=NULL;
	if (pFile->Read(&Length,sizeof(DWORD))!=sizeof(DWORD))
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


static bool WriteString(CNFile *pFile,LPCWSTR pszString)
{
	DWORD Length;

	if (pszString!=NULL)
		Length=::lstrlenW(pszString);
	else
		Length=0;
	if (!pFile->Write(&Length,sizeof(DWORD)))
		return false;
	if (Length>0) {
		if (!pFile->Write(pszString,Length*sizeof(WCHAR)))
			return false;
	}
	return true;
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
			|| FileHeader.Version!=EPGLISTFILEHEADER_VERSION)
		return false;

	Clear();
	for (DWORD i=0;i<FileHeader.NumServices;i++) {
		ServiceInfoHeader ServiceHeader;
		LPWSTR pszText;

		if (File.Read(&ServiceHeader,sizeof(ServiceInfoHeader))!=sizeof(ServiceInfoHeader))
			goto OnError;
		ReadString(&File,&pszText);
		CServiceInfoData ServiceData(ServiceHeader.OriginalNID,
									 ServiceHeader.TSID,
									 ServiceHeader.ServiceID,
									 ServiceHeader.ServiceType,
									 pszText);
		delete [] pszText;
		CEpgServiceInfo ServiceInfo(ServiceData);
		for (DWORD j=0;j<ServiceHeader.NumEvents;j++) {
			EventInfoHeader EventHeader;
			CEventInfoData EventData;

			if (File.Read(&EventHeader,sizeof(EventInfoHeader))!=sizeof(EventInfoHeader))
				goto OnError;
			EventData.m_OriginalNID=ServiceHeader.OriginalNID;
			EventData.m_TSID=ServiceHeader.TSID;
			EventData.m_ServiceID=EventHeader.ServiceID;
			EventData.m_EventID=EventHeader.EventID;
			EventData.m_stStartTime=EventHeader.StartTime;
			EventData.m_DurationSec=EventHeader.DurationSec;
			EventData.m_ComponentType=EventHeader.ComponentType;
			EventData.m_AudioComponentType=EventHeader.AudioComponentType;
			EventData.m_ESMultiLangFlag=EventHeader.ESMultiLangFlag;
			EventData.m_MainComponentFlag=EventHeader.MainComponentFlag;
			EventData.m_SamplingRate=EventHeader.SamplingRate;
			if (EventHeader.ContentNibbleListCount>0) {
				for (DWORD k=0;k<EventHeader.ContentNibbleListCount;k++) {
					CEventInfoData::NibbleData Nibble;
					if (File.Read(&Nibble,sizeof(Nibble))!=sizeof(Nibble))
						goto OnError;
					EventData.m_NibbleList.push_back(Nibble);
				}
			}
			if (!ReadString(&File,&pszText))
				goto OnError;
			if (pszText!=NULL) {
				EventData.SetEventName(pszText);
				delete [] pszText;
			}
			if (!ReadString(&File,&pszText))
				goto OnError;
			if (pszText!=NULL) {
				EventData.SetEventText(pszText);
				delete [] pszText;
			}
			if (!ReadString(&File,&pszText))
				goto OnError;
			if (pszText!=NULL) {
				EventData.SetEventExtText(pszText);
				delete [] pszText;
			}
			if (!ReadString(&File,&pszText))
				goto OnError;
			if (pszText!=NULL) {
				EventData.SetComponentTypeText(pszText);
				delete [] pszText;
			}
			if (!ReadString(&File,&pszText))
				goto OnError;
			if (pszText!=NULL) {
				EventData.SetAudioComponentTypeText(pszText);
				delete [] pszText;
			}
			ServiceInfo.m_EventList.EventDataMap.insert(
				std::pair<WORD,CEventInfoData>(EventData.m_EventID,EventData));
		}
		ServiceMapKey Key=GenerateServiceMapKey(ServiceData.m_OriginalNID,
								ServiceData.m_TSID,ServiceData.m_ServiceID);
		ServiceMap.insert(std::pair<ServiceMapKey,CEpgServiceInfo>(Key,ServiceInfo));
	}
	return true;

OnError:
	Clear();
	return false;
}


bool CEpgProgramList::SaveToFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);
	CNFile File;

	if (!File.Open(pszFileName,CNFile::CNF_WRITE | CNFile::CNF_NEW))
		return false;

	EpgListFileHeader FileHeader;

	::CopyMemory(FileHeader.Type,EPGLISTFILEHEADER_TYPE,sizeof(FileHeader.Type));
	FileHeader.Version=EPGLISTFILEHEADER_VERSION;
	FileHeader.NumServices=ServiceMap.size();

	if (!File.Write(&FileHeader,sizeof(EpgListFileHeader)))
		return false;

	SYSTEMTIME stCurrent,st;
	::GetLocalTime(&stCurrent);

	ServiceIterator itrService;

	for (itrService=ServiceMap.begin();itrService!=ServiceMap.end();itrService++) {
		ServiceInfoHeader ServiceHeader(itrService->second.m_ServiceData);
		CEventInfoList::EventIterator itrEvent;

		for (itrEvent=itrService->second.m_EventList.EventDataMap.begin();
				itrEvent!=itrService->second.m_EventList.EventDataMap.end();
				itrEvent++) {
			if (itrEvent->second.GetEndTime(&st)
					&& CompareSystemTime(&st,&stCurrent)>0)
				ServiceHeader.NumEvents++;
		}
		if (!File.Write(&ServiceHeader,sizeof(ServiceInfoHeader)))
			return false;
		if (!WriteString(&File,itrService->second.m_ServiceData.GetServiceName()))
			return false;
		if (ServiceHeader.NumEvents>0) {
			for (itrEvent=itrService->second.m_EventList.EventDataMap.begin();
					itrEvent!=itrService->second.m_EventList.EventDataMap.end();
					itrEvent++) {
				EventInfoHeader EventHeader(itrEvent->second);

				if (itrEvent->second.GetEndTime(&st)
						&& CompareSystemTime(&st,&stCurrent)>0) {
					if (!File.Write(&EventHeader,sizeof(EventInfoHeader)))
						return false;
					if (EventHeader.ContentNibbleListCount>0) {
						for (DWORD i=0;i<EventHeader.ContentNibbleListCount;i++) {
							CEventInfoData::NibbleData Nibble=itrEvent->second.m_NibbleList[i];
							if (!File.Write(&Nibble,sizeof(Nibble)))
								return false;
						}
					}
					if (!WriteString(&File,itrEvent->second.GetEventName())
							|| !WriteString(&File,itrEvent->second.GetEventText())
							|| !WriteString(&File,itrEvent->second.GetEventExtText())
							|| !WriteString(&File,itrEvent->second.GetComponentTypeText())
							|| !WriteString(&File,itrEvent->second.GetAudioComponentTypeText()))
						return false;
				}
			}
		}
	}
	return true;
}
