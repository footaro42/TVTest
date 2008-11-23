#include "stdafx.h"
#include "TVTest.h"
#include "Epg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CEpgDataInfo::CEpgDataInfo(const CEpgDataInfo &Info)
{
	m_ServiceID=Info.m_ServiceID;
	m_EventID=Info.m_EventID;
	m_pszEventName=DuplicateString(Info.m_pszEventName);
	m_pszEventText=DuplicateString(Info.m_pszEventText);
	m_pszEventExtText=DuplicateString(Info.m_pszEventExtText);
	m_stStartTime=Info.m_stStartTime;
	m_Duration=Info.m_Duration;
}


CEpgDataInfo::CEpgDataInfo(const EPG_DATA_INFO *pInfo)
{
	m_ServiceID=(WORD)pInfo->dwServiceID;
	m_EventID=(WORD)pInfo->dwEventID;
	m_pszEventName=DuplicateString(pInfo->lpwszEventName);
	m_pszEventText=DuplicateString(pInfo->lpwszEventText);
	m_pszEventExtText=DuplicateString(pInfo->lpwszEventExtText);
	m_stStartTime=pInfo->stStartTime;
	// DayOfWeek‚ªí‚É0‚Ý‚½‚¢‚È‚Ì‚ÅŒvŽZ‚·‚é
	/*
	FILETIME ft;
	::SystemTimeToFileTime(&pInfo->stStartTime,&ft);
	::FileTimeToSystemTime(&ft,&m_stStartTime);
	*/
	m_stStartTime.wDayOfWeek=CalcDayOfWeek(m_stStartTime.wYear,m_stStartTime.wMonth,m_stStartTime.wDay);
	m_Duration=pInfo->dwDurationSec;
}


CEpgDataInfo::~CEpgDataInfo()
{
	delete [] m_pszEventName;
	delete [] m_pszEventText;
	delete [] m_pszEventExtText;
}


CEpgDataInfo &CEpgDataInfo::operator=(const CEpgDataInfo &Info)
{
	if (&Info==this)
		return *this;
	m_ServiceID=Info.m_ServiceID;
	m_EventID=Info.m_EventID;
	ReplaceString(&m_pszEventName,Info.m_pszEventName);
	ReplaceString(&m_pszEventText,Info.m_pszEventText);
	ReplaceString(&m_pszEventExtText,Info.m_pszEventExtText);
	m_stStartTime=Info.m_stStartTime;
	m_Duration=Info.m_Duration;
	return *this;
}


bool CEpgDataInfo::GetStartTime(SYSTEMTIME *pTime) const
{
	*pTime=m_stStartTime;
	return true;
}


bool CEpgDataInfo::GetEndTime(SYSTEMTIME *pTime) const
{
	FILETIME ft;

	if (!::SystemTimeToFileTime(&m_stStartTime,&ft))
		return false;
	ft+=(LONGLONG)m_Duration*FILETIME_SECOND;
	return ::FileTimeToSystemTime(&ft,pTime)!=FALSE;
}
