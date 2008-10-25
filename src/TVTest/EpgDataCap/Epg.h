#ifndef EPG_H
#define EPG_H


#include "EpgDataCap.h"
#include "EpgDataCapDllUtil.h"


class CEpgDataInfo {
	WORD m_ServiceID;
	WORD m_EventID;
	LPWSTR m_pszEventName;
	LPWSTR m_pszEventText;
	LPWSTR m_pszEventExtText;
	SYSTEMTIME m_stStartTime;
	DWORD m_Duration;
public:
	CEpgDataInfo(const CEpgDataInfo &Info);
	CEpgDataInfo(const EPG_DATA_INFO *pInfo);
	~CEpgDataInfo();
	CEpgDataInfo &operator=(const CEpgDataInfo &Info);
	WORD GetServiceID() const { return m_ServiceID; }
	WORD GetEventID() const { return m_EventID; }
	LPCWSTR GetEventName() const { return m_pszEventName; }
	LPCWSTR GetEventText() const { return m_pszEventText; }
	LPCWSTR GetEventExtText() const { return m_pszEventExtText; }
	const SYSTEMTIME &GetStartTime() const { return m_stStartTime; }
	bool GetStartTime(SYSTEMTIME *pTime) const;
	DWORD GetDuration() const { return m_Duration; }
	bool GetEndTime(SYSTEMTIME *pTime) const;
};


#endif
