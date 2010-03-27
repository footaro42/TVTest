#include "stdafx.h"
#include "TVTest.h"
#include "ChannelHistory.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CDriverChannelInfo::CDriverChannelInfo(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo)
	: CChannelInfo(*pChannelInfo)
	, m_pszDriverName(DuplicateString(pszDriverName))
{
}


CDriverChannelInfo::~CDriverChannelInfo()
{
	delete [] m_pszDriverName;
}




CChannelHistory::CChannelHistory()
	: m_MaxChannelHistory(20)
	, m_MaxChannelHistoryMenu(20)
{
}


CChannelHistory::~CChannelHistory()
{
	Clear();
}


int CChannelHistory::NumChannels() const
{
	return (int)m_ChannelList.size();
}


void CChannelHistory::Clear()
{
	for (size_t i=0;i<m_ChannelList.size();i++)
		delete m_ChannelList[i];
	m_ChannelList.clear();
}


const CDriverChannelInfo *CChannelHistory::GetChannelInfo(int Index) const
{
	if (Index<0 || Index>=NumChannels())
		return NULL;
	return m_ChannelList[Index];
}


bool CChannelHistory::Add(LPCTSTR pszDriverName,const CChannelInfo *pChannelInfo)
{
	if (pszDriverName==NULL || pChannelInfo==NULL)
		return false;

	std::deque<CDriverChannelInfo*>::iterator itr;
	for (itr=m_ChannelList.begin();itr!=m_ChannelList.end();itr++) {
		if (::lstrcmpi((*itr)->GetDriverFileName(),pszDriverName)==0
				&& (*itr)->GetSpace()==pChannelInfo->GetSpace()
				&& (*itr)->GetChannelIndex()==pChannelInfo->GetChannelIndex()
				&& (*itr)->GetServiceID()==pChannelInfo->GetServiceID()) {
			if (itr==m_ChannelList.begin())
				return true;
			delete *itr;
			m_ChannelList.erase(itr);
			break;
		}
	}
	m_ChannelList.push_front(new CDriverChannelInfo(pszDriverName,pChannelInfo));
	if ((int)m_ChannelList.size()>m_MaxChannelHistory) {
		delete m_ChannelList[m_ChannelList.size()-1];
		m_ChannelList.pop_back();
	}
	return true;
}


bool CChannelHistory::SetMenu(HMENU hmenu,bool fClear) const
{
	ClearMenu(hmenu);
	for (int i=0;i<m_MaxChannelHistoryMenu;i++) {
		const CDriverChannelInfo *pChannelInfo=GetChannelInfo(i);
		TCHAR szText[64];
		int Length;

		if (pChannelInfo==NULL)
			break;
		Length=::wsprintf(szText,TEXT("&%c: "),i<10?i+'0':(i-10)+'A');
		CopyToMenuText(pChannelInfo->GetName(),
					   szText+Length,lengthof(szText)-Length);
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_CHANNELHISTORY_FIRST+i,szText);
	}
	if (fClear && NumChannels()>0) {
		::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
		::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,CM_CHANNELHISTORY_CLEAR,TEXT("—š—ð‚ðƒNƒŠƒA"));
	}
	return true;
}


bool CChannelHistory::Load(LPCTSTR pszFileName)
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("RecentChannel"),CSettings::OPEN_READ)) {
		int Count;

		if (Settings.Read(TEXT("Count"),&Count) && Count>0) {
			for (int i=0;i<Count;i++) {
				TCHAR szName[32],szDriverName[MAX_PATH],szChannelName[MAX_CHANNEL_NAME];
				int Space,Channel,Service,ServiceID;

				::wsprintf(szName,TEXT("History%d_Driver"),i);
				if (!Settings.Read(szName,szDriverName,lengthof(szDriverName))
						|| szDriverName[0]=='\0')
					break;
				::wsprintf(szName,TEXT("History%d_Name"),i);
				if (!Settings.Read(szName,szChannelName,lengthof(szChannelName))
						|| szChannelName[0]=='\0')
					break;
				::wsprintf(szName,TEXT("History%d_Space"),i);
				if (!Settings.Read(szName,&Space))
					break;
				::wsprintf(szName,TEXT("History%d_Channel"),i);
				if (!Settings.Read(szName,&Channel))
					break;
				::wsprintf(szName,TEXT("History%d_Service"),i);
				if (!Settings.Read(szName,&Service))
					break;
				::wsprintf(szName,TEXT("History%d_ServiceID"),i);
				if (!Settings.Read(szName,&ServiceID))
					break;
				CChannelInfo ChannelInfo(Space,0,Channel,0,Service,szChannelName);
				ChannelInfo.SetServiceID(ServiceID);
				m_ChannelList.push_front(new CDriverChannelInfo(szDriverName,&ChannelInfo));
			}
		}
		Settings.Close();
	}
	return true;
}


bool CChannelHistory::Save(LPCTSTR pszFileName) const
{
	CSettings Settings;

	if (Settings.Open(pszFileName,TEXT("RecentChannel"),CSettings::OPEN_WRITE)) {
		Settings.Write(TEXT("Count"),NumChannels());
		for (size_t i=0;i<m_ChannelList.size();i++) {
			const CDriverChannelInfo *pChannelInfo=m_ChannelList[i];
			TCHAR szName[64];

			::wsprintf(szName,TEXT("History%d_Driver"),i);
			Settings.Write(szName,pChannelInfo->GetDriverFileName());
			::wsprintf(szName,TEXT("History%d_Name"),i);
			Settings.Write(szName,pChannelInfo->GetName());
			::wsprintf(szName,TEXT("History%d_Space"),i);
			Settings.Write(szName,pChannelInfo->GetSpace());
			::wsprintf(szName,TEXT("History%d_Channel"),i);
			Settings.Write(szName,pChannelInfo->GetChannelIndex());
			::wsprintf(szName,TEXT("History%d_Service"),i);
			Settings.Write(szName,pChannelInfo->GetService());
			::wsprintf(szName,TEXT("History%d_ServiceID"),i);
			Settings.Write(szName,pChannelInfo->GetServiceID());
		}
		Settings.Close();
	}
	return true;
}
