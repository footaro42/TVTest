#include "stdafx.h"
#include "TVTest.h"
#include "EpgProgramList.h"

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
	m_OriginalNID=Info.m_OriginalNID;
	m_TSID=Info.m_TSID;
	m_ServiceID=Info.m_ServiceID;
	m_ServiceType=Info.m_ServiceType;
	ReplaceString(&m_pszServiceName,Info.m_pszServiceName);
	return *this;
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
									&pEpgData,&dwEpgDataCount))
		return false;
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
	}

	ServiceMapKey Key=GenerateServiceMapKey(ServiceData.m_OriginalNID,
								ServiceData.m_TSID,ServiceData.m_ServiceID);
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


bool CEpgProgramList::UpdateProgramList(WORD ServiceID)
{
	CTryBlockLock Lock(&m_Lock);

	if (m_pEpgDll==NULL)
		return false;

	SERVICE_INFO *pService=NULL;
	DWORD dwServiceCount=0;

	if (!m_pEpgDll->GetServiceListDB(&pService,&dwServiceCount))
		return false;
	for (DWORD i=0;i<dwServiceCount;i++) {
		if ((pService[i].dwServiceType==0x01 || pService[i].dwServiceType==0xA5)
				&& (WORD)pService[i].dwServiceID==ServiceID) {
			return UpdateService(&pService[i]);
		}
	}
	return false;
}


void CEpgProgramList::Clear()
{
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


CEpgServiceInfo *CEpgProgramList::GetServiceInfo(WORD ServiceID)
{
	CBlockLock Lock(&m_Lock);
	ServiceIterator itrService;

	for (itrService=ServiceMap.begin();itrService!=ServiceMap.end();itrService++) {
		if (itrService->second.m_ServiceData.m_ServiceID==ServiceID)
			return &itrService->second;
	}
	return NULL;
}


bool CEpgProgramList::GetEventInfo(WORD ServiceID,WORD EventID,CEventInfoData *pInfo)
{
	const CEventInfoData *pEventInfo=GetEventInfo(ServiceID,EventID);

	if (pEventInfo==NULL)
		return false;
	*pInfo=*pEventInfo;
	return true;
}


const CEventInfoData *CEpgProgramList::GetEventInfo(WORD ServiceID,WORD EventID)
{
	if (m_pEpgDll==NULL)
		return NULL;

	CEpgServiceInfo *pServiceInfo=GetServiceInfo(ServiceID);

	if (pServiceInfo==NULL)
		return NULL;
	return pServiceInfo->GetEventInfo(EventID);
}


bool CEpgProgramList::GetEventInfo(WORD ServiceID,const SYSTEMTIME *pTime,CEventInfoData *pInfo)
{
	if (m_pEpgDll==NULL)
		return NULL;

	CEpgServiceInfo *pServiceInfo;
	CEventInfoList::EventIterator itrEvent;

	pServiceInfo=GetServiceInfo(ServiceID);
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


static bool ReadString(HANDLE hFile,LPWSTR *ppszString)
{
	DWORD Length,Read;

	*ppszString=NULL;
	if (!::ReadFile(hFile,&Length,sizeof(DWORD),&Read,NULL) || Read!=sizeof(DWORD))
		return false;
	if (Length>0) {
		*ppszString=new WCHAR[Length+1];
		if (!::ReadFile(hFile,*ppszString,Length*sizeof(WCHAR),&Read,NULL)
				|| Read!=Length*sizeof(WCHAR)) {
			delete [] *ppszString;
			*ppszString=NULL;
			return false;
		}
		(*ppszString)[Length]='\0';
	}
	return true;
}


static bool WriteString(HANDLE hFile,LPCWSTR pszString)
{
	DWORD Length,Write;

	if (pszString!=NULL)
		Length=::lstrlenW(pszString);
	else
		Length=0;
	if (!::WriteFile(hFile,&Length,sizeof(DWORD),&Write,NULL) || Write!=sizeof(DWORD))
		return false;
	if (Length>0) {
		if (!::WriteFile(hFile,pszString,Length*sizeof(WCHAR),&Write,NULL)
				|| Write!=Length*sizeof(WCHAR))
			return false;
	}
	return true;
}


bool CEpgProgramList::LoadFromFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);
	HANDLE hFile;
	DWORD Read;

	hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
									OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;

	EpgListFileHeader FileHeader;

	::ReadFile(hFile,&FileHeader,sizeof(EpgListFileHeader),&Read,NULL);
	if (memcmp(FileHeader.Type,EPGLISTFILEHEADER_TYPE,sizeof(FileHeader.Type))!=0
			|| FileHeader.Version!=EPGLISTFILEHEADER_VERSION) {
		::CloseHandle(hFile);
		return false;
	}

	Clear();
	for (DWORD i=0;i<FileHeader.NumServices;i++) {
		ServiceInfoHeader ServiceHeader;
		LPWSTR pszText;

		::ReadFile(hFile,&ServiceHeader,sizeof(ServiceInfoHeader),&Read,NULL);
		ReadString(hFile,&pszText);
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

			::ReadFile(hFile,&EventHeader,sizeof(EventInfoHeader),&Read,NULL);
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
					::ReadFile(hFile,&Nibble,sizeof(Nibble),&Read,NULL);
					EventData.m_NibbleList.push_back(Nibble);
				}
			}
			if (ReadString(hFile,&pszText)) {
				EventData.SetEventName(pszText);
				delete [] pszText;
			}
			if (ReadString(hFile,&pszText)) {
				EventData.SetEventText(pszText);
				delete [] pszText;
			}
			if (ReadString(hFile,&pszText)) {
				EventData.SetEventExtText(pszText);
				delete [] pszText;
			}
			if (ReadString(hFile,&pszText)) {
				EventData.SetComponentTypeText(pszText);
				delete [] pszText;
			}
			if (ReadString(hFile,&pszText)) {
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
	::CloseHandle(hFile);
	return true;
}


bool CEpgProgramList::SaveToFile(LPCTSTR pszFileName)
{
	CBlockLock Lock(&m_Lock);
	HANDLE hFile;
	DWORD Write;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
												FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;

	EpgListFileHeader FileHeader;

	::CopyMemory(FileHeader.Type,EPGLISTFILEHEADER_TYPE,sizeof(FileHeader.Type));
	FileHeader.Version=EPGLISTFILEHEADER_VERSION;
	FileHeader.NumServices=ServiceMap.size();

	::WriteFile(hFile,&FileHeader,sizeof(EpgListFileHeader),&Write,NULL);

	ServiceIterator itrService;

	for (itrService=ServiceMap.begin();itrService!=ServiceMap.end();itrService++) {
		ServiceInfoHeader ServiceHeader(itrService->second.m_ServiceData);

		ServiceHeader.NumEvents=itrService->second.m_EventList.EventDataMap.size();
		::WriteFile(hFile,&ServiceHeader,sizeof(ServiceInfoHeader),&Write,NULL);
		WriteString(hFile,itrService->second.m_ServiceData.GetServiceName());
		if (ServiceHeader.NumEvents>0) {
			CEventInfoList::EventIterator itrEvent;

			for (itrEvent=itrService->second.m_EventList.EventDataMap.begin();
					itrEvent!=itrService->second.m_EventList.EventDataMap.end();
					itrEvent++) {
				EventInfoHeader EventHeader(itrEvent->second);

				::WriteFile(hFile,&EventHeader,sizeof(EventInfoHeader),&Write,NULL);
				if (EventHeader.ContentNibbleListCount>0) {
					for (DWORD i=0;i<EventHeader.ContentNibbleListCount;i++) {
						CEventInfoData::NibbleData Nibble=itrEvent->second.m_NibbleList[i];
						::WriteFile(hFile,&Nibble,sizeof(Nibble),&Write,NULL);
					}
				}
				WriteString(hFile,itrEvent->second.GetEventName());
				WriteString(hFile,itrEvent->second.GetEventText());
				WriteString(hFile,itrEvent->second.GetEventExtText());
				WriteString(hFile,itrEvent->second.GetComponentTypeText());
				WriteString(hFile,itrEvent->second.GetAudioComponentTypeText());
			}
		}
	}
	::CloseHandle(hFile);
	return true;
}
