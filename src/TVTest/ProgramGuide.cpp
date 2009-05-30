#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramGuide.h"
#include "DialogUtil.h"
#include "Help/HelpID.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define PROGRAM_GUIDE_WINDOW_CLASS APP_NAME TEXT(" Program Guide")
#define TITLE_TEXT TEXT("EPG番組表")

#define MENU_DRIVER 8




class CProgramGuideItem {
	CEventInfoData m_EventInfo;
	bool m_fNullItem;
	SYSTEMTIME m_stStartTime;
	SYSTEMTIME m_stEndTime;
	DWORD m_Duration;
	int m_TitleLines;
	int m_TextLines;
	int m_ItemPos;
	int m_ItemLines;
	void DrawString(HDC hdc,LPCTSTR pszText,const RECT *pRect,int LineHeight);
	int CalcStringLines(HDC hdc,LPCTSTR pszText,int Width);
public:
	CProgramGuideItem(const SYSTEMTIME *pStartTime,DWORD Duration);
	CProgramGuideItem(const CEventInfoData &Info);
	~CProgramGuideItem();
	const CEventInfoData &GetEventInfo() const { return m_EventInfo; }
	bool GetStartTime(SYSTEMTIME *pTime) const;
	bool SetStartTime(const SYSTEMTIME *pTime);
	bool GetEndTime(SYSTEMTIME *pTime) const;
	bool SetEndTime(const SYSTEMTIME *pTime);
	int GetTitleLines() const { return m_TitleLines; }
	int GetTextLines() const { return m_TextLines; }
	int GetLines() const { return m_TitleLines+m_TextLines; }
	int CalcLines(HDC hdc,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth);
	void DrawTitle(HDC hdc,const RECT *pRect,int LineHeight);
	void DrawText(HDC hdc,const RECT *pRect,int LineHeight);
	int GetItemPos() const { return m_ItemPos; }
	bool SetItemPos(int Pos) { m_ItemPos=Pos; return true; }
	int GetItemLines() const { return m_ItemLines; }
	bool SetItemLines(int Lines) { m_ItemLines=Lines; return true; }
	bool IsNullItem() const { return m_fNullItem; }
};


CProgramGuideItem::CProgramGuideItem(const SYSTEMTIME *pStartTime,DWORD Duration)
{
	m_EventInfo.m_stStartTime=*pStartTime;
	m_EventInfo.m_DurationSec=Duration;
	m_fNullItem=true;
	m_stStartTime=*pStartTime;
	m_stStartTime.wMilliseconds=0;
	m_EventInfo.GetEndTime(&m_stEndTime);
	m_stEndTime.wMilliseconds=0;
	m_TitleLines=0;
	m_TextLines=0;
	m_ItemPos=-1;
	m_ItemLines=0;
}


CProgramGuideItem::CProgramGuideItem(const CEventInfoData &Info)
	: m_EventInfo(Info)
{
	m_fNullItem=false;
	m_EventInfo.GetStartTime(&m_stStartTime);
	m_stStartTime.wMilliseconds=0;
	m_EventInfo.GetEndTime(&m_stEndTime);
	m_stEndTime.wMilliseconds=0;
	m_TitleLines=0;
	m_TextLines=0;
	m_ItemPos=-1;
	m_ItemLines=0;
}


CProgramGuideItem::~CProgramGuideItem()
{
}


bool CProgramGuideItem::GetStartTime(SYSTEMTIME *pTime) const
{
	*pTime=m_stStartTime;
	return true;
}


bool CProgramGuideItem::SetStartTime(const SYSTEMTIME *pTime)
{
	if (CompareSystemTime(pTime,&m_stEndTime)>=0)
		return false;
	m_stStartTime=*pTime;
	return true;
}


bool CProgramGuideItem::GetEndTime(SYSTEMTIME *pTime) const
{
	*pTime=m_stEndTime;
	return true;
}


bool CProgramGuideItem::SetEndTime(const SYSTEMTIME *pTime)
{
	if (CompareSystemTime(&m_stStartTime,pTime)>=0)
		return false;
	m_stEndTime=*pTime;
	return true;
}


void CProgramGuideItem::DrawString(HDC hdc,LPCTSTR pszText,const RECT *pRect,
																int LineHeight)
{
	LPCTSTR p;
	int y;
	int Length;
	int Fit;
	SIZE sz;

	p=pszText;
	y=pRect->top;
	while (*p!='\0' && y<pRect->bottom) {
		if (*p=='\r' || *p=='\n') {
			p++;
			if (*p=='\n')
				p++;
			y+=LineHeight;
			continue;
		}
		for (Length=0;p[Length]!='\0' && p[Length]!='\r' && p[Length]!='\n';Length++);
		::GetTextExtentExPoint(hdc,p,Length,pRect->right-pRect->left,&Fit,NULL,&sz);
		if (Fit<1)
			Fit=1;
		::TextOut(hdc,pRect->left,y,p,Fit);
		p+=Fit;
		y+=LineHeight;
	}
}


int CProgramGuideItem::CalcStringLines(HDC hdc,LPCTSTR pszText,int Width)
{
	LPCTSTR p;
	int Length;
	int Fit;
	SIZE sz;
	int Lines;

	Lines=0;
	p=pszText;
	while (*p!='\0') {
		if (*p=='\r' || *p=='\n') {
			p++;
			if (*p=='\n')
				p++;
			if (*p=='\0')
				break;
			Lines++;
			continue;
		}
		for (Length=0;p[Length]!='\0' && p[Length]!='\r' && p[Length]!='\n';Length++);
		::GetTextExtentExPoint(hdc,p,Length,Width,&Fit,NULL,&sz);
		if (Fit<1)
			Fit=1;
		p+=Fit;
		Lines++;
	}
	return Lines;
}


int CProgramGuideItem::CalcLines(HDC hdc,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth)
{
	TCHAR szText[2048];

#ifndef _DEBUG
	::wsprintf(szText,TEXT("%d:%02d"),m_EventInfo.m_stStartTime.wHour,
										m_EventInfo.m_stStartTime.wMinute);
#else
	::wsprintf(szText,TEXT("%d:%02d.%02d-%d:%02d.%02d"),
			m_stStartTime.wHour,m_stStartTime.wMinute,m_stStartTime.wSecond,
			m_stEndTime.wHour,m_stEndTime.wMinute,m_stEndTime.wSecond);
#endif
	if (m_EventInfo.GetEventName()) {
		::wsprintf(szText+lstrlen(szText),TEXT(" %s"),m_EventInfo.GetEventName());
	}
	SelectFont(hdc,hfontTitle);
	m_TitleLines=CalcStringLines(hdc,szText,TitleWidth);
	if (m_EventInfo.GetEventText()) {
		SelectFont(hdc,hfontText);
		m_TextLines=CalcStringLines(hdc,m_EventInfo.GetEventText(),TextWidth);
	} else
		m_TextLines=0;
	return m_TitleLines+m_TextLines;
}


void CProgramGuideItem::DrawTitle(HDC hdc,const RECT *pRect,int LineHeight)
{
	TCHAR szText[2048];

#ifndef _DEBUG
	::wsprintf(szText,TEXT("%d:%02d"),m_EventInfo.m_stStartTime.wHour,
											m_EventInfo.m_stStartTime.wMinute);
#else
	::wsprintf(szText,TEXT("%d:%02d.%02d-%d:%02d.%02d"),
			m_stStartTime.wHour,m_stStartTime.wMinute,m_stStartTime.wSecond,
			m_stEndTime.wHour,m_stEndTime.wMinute,m_stEndTime.wSecond);
#endif
	if (m_EventInfo.GetEventName()) {
		::wsprintf(szText+lstrlen(szText),TEXT(" %s"),m_EventInfo.GetEventName());
	}
	DrawString(hdc,szText,pRect,LineHeight);
}


void CProgramGuideItem::DrawText(HDC hdc,const RECT *pRect,int LineHeight)
{
	if (m_EventInfo.GetEventText()) {
		DrawString(hdc,m_EventInfo.GetEventText(),pRect,LineHeight);
	}
}




class CProgramGuideServiceInfo {
	CServiceInfoData m_ServiceData;
	CProgramGuideItem **m_ppProgramList;
	int m_NumPrograms;
	int m_ProgramListLength;
	int m_FirstItem;
	int m_LastItem;
	void SortSub(CProgramGuideItem **ppFirst,CProgramGuideItem **ppLast);
	bool InsertProgram(int Index,CProgramGuideItem *pItem);
	void InsertNullItems(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime);
public:
	CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo);
	CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo,const CEpgServiceInfo &Info);
	~CProgramGuideServiceInfo();
	const CServiceInfoData *GetServiceInfoData() const { return &m_ServiceData; }
	WORD GetOriginalNID() const { return m_ServiceData.m_OriginalNID; }
	WORD GetTSID() const { return m_ServiceData.m_TSID; }
	WORD GetServiceID() const { return m_ServiceData.m_ServiceID; }
	LPCTSTR GetServiceName() const { return m_ServiceData.GetServiceName(); }
	int NumPrograms() const { return m_NumPrograms; }
	CProgramGuideItem *GetProgram(int Index);
	const CProgramGuideItem *GetProgram(int Index) const;
	bool AddProgram(CProgramGuideItem *pItem);
	void ClearPrograms();
	void SortPrograms();
	void CalcLayout(HDC hdc,const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime,int LinesPerHour,
		HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth);
	int GetFirstItem() const { return m_FirstItem; }
	int GetLastItem() const { return m_LastItem; }
	bool SaveiEpgFile(int Program,LPCTSTR pszFileName,bool fVersion2) const;
};


CProgramGuideServiceInfo::CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo)
	: m_ServiceData(pChannelInfo->GetNetworkID(),
					pChannelInfo->GetTransportStreamID(),
					pChannelInfo->GetServiceID(),0,
					pChannelInfo->GetName())
{
	m_ppProgramList=NULL;
	m_NumPrograms=0;
	m_ProgramListLength=0;
	m_FirstItem=-1;
	m_LastItem=-1;
}


CProgramGuideServiceInfo::CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo,const CEpgServiceInfo &Info)
	: m_ServiceData(Info.m_ServiceData)
{
	m_ServiceData.SetServiceName(pChannelInfo->GetName());
	m_ppProgramList=NULL;
	m_NumPrograms=0;
	m_ProgramListLength=0;
	m_FirstItem=-1;
	m_LastItem=-1;
}


CProgramGuideServiceInfo::~CProgramGuideServiceInfo()
{
	ClearPrograms();
}


CProgramGuideItem *CProgramGuideServiceInfo::GetProgram(int Index)
{
	if (Index<0 || Index>=m_NumPrograms) {
		TRACE(TEXT("CProgramGuideServiceInfo::GetProgram() Out of range %d\n"),Index);
		return NULL;
	}
	return m_ppProgramList[Index];
}


const CProgramGuideItem *CProgramGuideServiceInfo::GetProgram(int Index) const
{
	if (Index<0 || Index>=m_NumPrograms) {
		TRACE(TEXT("CProgramGuideServiceInfo::GetProgram() const : Out of range %d\n"),Index);
		return NULL;
	}
	return m_ppProgramList[Index];
}


bool CProgramGuideServiceInfo::AddProgram(CProgramGuideItem *pItem)
{
	if (m_NumPrograms==m_ProgramListLength) {
		if (m_ProgramListLength==0)
			m_ProgramListLength=32;
		else
			m_ProgramListLength*=2;
		m_ppProgramList=static_cast<CProgramGuideItem**>(realloc(m_ppProgramList,m_ProgramListLength*sizeof(CProgramGuideItem*)));
	}
	m_ppProgramList[m_NumPrograms++]=pItem;
	return true;
}


void CProgramGuideServiceInfo::ClearPrograms()
{
	if (m_ppProgramList!=NULL) {
		for (int i=m_NumPrograms-1;i>=0;i--)
			delete m_ppProgramList[i];
		free(m_ppProgramList);
		m_ppProgramList=NULL;
		m_NumPrograms=0;
		m_ProgramListLength=0;
	}
}


void CProgramGuideServiceInfo::SortSub(CProgramGuideItem **ppFirst,CProgramGuideItem **ppLast)
{
	SYSTEMTIME stKey=ppFirst[(ppLast-ppFirst)/2]->GetEventInfo().m_stStartTime;
	CProgramGuideItem **p,**q;

	p=ppFirst;
	q=ppLast;
	while (p<=q) {
		while (CompareSystemTime(&(*p)->GetEventInfo().m_stStartTime,&stKey)<0)
			p++;
		while (CompareSystemTime(&(*q)->GetEventInfo().m_stStartTime,&stKey)>0)
			q--;
		if (p<=q) {
			CProgramGuideItem *pTemp;

			pTemp=*p;
			*p=*q;
			*q=pTemp;
			p++;
			q--;
		}
	}
	if (q>ppFirst)
		SortSub(ppFirst,q);
	if (p<ppLast)
		SortSub(p,ppLast);
}


void CProgramGuideServiceInfo::SortPrograms()
{
	if (m_NumPrograms>1)
		SortSub(&m_ppProgramList[0],&m_ppProgramList[m_NumPrograms-1]);
}


bool CProgramGuideServiceInfo::InsertProgram(int Index,CProgramGuideItem *pItem)
{
	if (Index<0 || Index>m_NumPrograms)
		return false;
	if (m_NumPrograms==m_ProgramListLength) {
		if (m_ProgramListLength==0)
			m_ProgramListLength=32;
		else
			m_ProgramListLength*=2;
		m_ppProgramList=static_cast<CProgramGuideItem**>(realloc(m_ppProgramList,m_ProgramListLength*sizeof(CProgramGuideItem*)));
	}
	if (Index<m_NumPrograms)
		::MoveMemory(&m_ppProgramList[Index+1],&m_ppProgramList[Index],
					(m_NumPrograms-Index)*sizeof(CProgramGuideItem*));
	m_ppProgramList[Index]=pItem;
	m_NumPrograms++;
	return true;
}


void CProgramGuideServiceInfo::InsertNullItems(
					const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime)
{
	int FirstItem,LastItem;
	int i;
	CProgramGuideItem *pItem,*pPrevItem;
	SYSTEMTIME stPrev,stStart,stEnd;
	int EmptyCount;

	FirstItem=-1;
	LastItem=-1;
	EmptyCount=0;
	stPrev=*pFirstTime;
	for (i=0;i<m_NumPrograms;i++) {
		pItem=m_ppProgramList[i];
		pItem->GetStartTime(&stStart);
		pItem->GetEndTime(&stEnd);
		if (CompareSystemTime(&stStart,pLastTime)<0
				&& CompareSystemTime(&stEnd,pFirstTime)>0) {
			if (FirstItem<0) {
				FirstItem=i;
				LastItem=i+1;
			} else if (LastItem<i+1) {
				LastItem=i+1;
			}
			if (CompareSystemTime(&stPrev,&stStart)<0)
				EmptyCount++;
		}
		if (CompareSystemTime(&stEnd,pLastTime)>=0)
			break;
		stPrev=stEnd;
	}
	if (EmptyCount>0) {
		int j;
		int Cmp;

		pPrevItem=NULL;
		stPrev=*pFirstTime;
		for (i=FirstItem,j=i;i<LastItem;i++,j++) {
			pItem=m_ppProgramList[j];
			pItem->GetStartTime(&stStart);
			Cmp=CompareSystemTime(&stPrev,&stStart);
			if (Cmp>0) {
				if (pPrevItem)
					pPrevItem->SetEndTime(&stStart);
			} else if (Cmp<0) {
				FILETIME ftStart,ftEnd;

				::SystemTimeToFileTime(&stPrev,&ftStart);
				::SystemTimeToFileTime(&stStart,&ftEnd);
				if (ftEnd-ftStart<=FILETIME_SECOND*60) {
					if (pPrevItem)
						pPrevItem->SetEndTime(&stStart);
				} else {
					InsertProgram(j,new CProgramGuideItem(&stPrev,
									(DWORD)((ftEnd-ftStart)/FILETIME_SECOND)));
					j++;
				}
			}
			pItem->GetEndTime(&stPrev);
			pPrevItem=pItem;
		}
	}
}


void CProgramGuideServiceInfo::CalcLayout(HDC hdc,
	const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime,
	int LinesPerHour,HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth)
{
	int i,j;
	CProgramGuideItem *pItem;
	SYSTEMTIME stStart,stEnd;

	InsertNullItems(pFirstTime,pLastTime);
	m_FirstItem=-1;
	m_LastItem=-1;
	for (i=0;i<m_NumPrograms;i++) {
		pItem=m_ppProgramList[i];
		pItem->GetStartTime(&stStart);
		pItem->GetEndTime(&stEnd);
		if (CompareSystemTime(&stStart,pLastTime)<0
				&& CompareSystemTime(&stEnd,pFirstTime)>0) {
			if (m_FirstItem<0) {
				m_FirstItem=i;
				m_LastItem=i+1;
			} else if (m_LastItem<i+1) {
				m_LastItem=i+1;
			}
			pItem->CalcLines(hdc,hfontTitle,TitleWidth,hfontText,TextWidth);
			pItem->SetItemPos(-1);
			pItem->SetItemLines(0);
		}
		if (CompareSystemTime(&stEnd,pLastTime)>=0)
			break;
	}
	if (m_FirstItem<0)
		return;

	SYSTEMTIME stFirst,stLast;
	int ItemPos=0;

	stFirst=*pFirstTime;
	for (i=m_FirstItem;i<m_LastItem;) {
		FILETIME ft;
		::SystemTimeToFileTime(&stFirst,&ft);
		if (CompareSystemTime(&stFirst,pLastTime)>=0)
			break;
		ft+=(LONGLONG)60*60*FILETIME_SECOND;
		::FileTimeToSystemTime(&ft,&stLast);
		do {
			m_ppProgramList[i]->GetEndTime(&stEnd);
			if (CompareSystemTime(&stEnd,&stFirst)>0)
				break;
			i++;
		} while (i<m_LastItem);
		if (i==m_LastItem)
			break;
		int ProgramsPerHour=0;
		do {
			m_ppProgramList[i+ProgramsPerHour]->GetStartTime(&stStart);
			if (CompareSystemTime(&stStart,&stLast)>=0)
				break;
			ProgramsPerHour++;
		} while (i+ProgramsPerHour<m_LastItem);
		if (ProgramsPerHour==0) {
			ItemPos+=LinesPerHour;
		} else if (ProgramsPerHour==1) {
			pItem=m_ppProgramList[i];
			pItem->SetItemLines(pItem->GetItemLines()+LinesPerHour);
			if (pItem->GetItemPos()<0)
				pItem->SetItemPos(ItemPos);
			ItemPos+=LinesPerHour;
		} else {
			int *pItemLines;

			pItemLines=new int[ProgramsPerHour];
			for (j=0;j<ProgramsPerHour;j++)
				pItemLines[j]=j<LinesPerHour?1:0;
			if (LinesPerHour>ProgramsPerHour) {
				int TotalLines=ProgramsPerHour;

				do {
					DWORD Time,MaxTime;
					int MaxItem;

					MaxTime=0;
					for (j=0;j<ProgramsPerHour;j++) {
						FILETIME ftStart,ftEnd;

						pItem=m_ppProgramList[i+j];
						pItem->GetStartTime(&stStart);
						if (CompareSystemTime(&stStart,&stFirst)<0)
							stStart=stFirst;
						pItem->GetEndTime(&stEnd);
						if (CompareSystemTime(&stEnd,&stLast)>0)
							stEnd=stLast;
						::SystemTimeToFileTime(&stStart,&ftStart);
						::SystemTimeToFileTime(&stEnd,&ftEnd);
						Time=(DWORD)((ftEnd-ftStart)/FILETIME_SECOND)/pItemLines[j];
						if (Time>MaxTime) {
							MaxTime=Time;
							MaxItem=j;
						}
					}
					if (MaxTime==0)
						break;
					pItemLines[MaxItem]++;
					TotalLines++;
				} while (TotalLines<LinesPerHour);
			}
			for (j=0;j<min(ProgramsPerHour,LinesPerHour);j++) {
				pItem=m_ppProgramList[i+j];
				if (pItem->GetItemPos()<0)
					pItem->SetItemPos(ItemPos);
				pItem->SetItemLines(pItem->GetItemLines()+pItemLines[j]);
				ItemPos+=pItemLines[j];
			}
			delete [] pItemLines;
		}
		stFirst=stLast;
	}
}


bool CProgramGuideServiceInfo::SaveiEpgFile(int Program,LPCTSTR pszFileName,bool fVersion2) const
{
	if (Program<0 || Program>=m_NumPrograms)
		return false;

	const CEventInfoData &EventData=m_ppProgramList[Program]->GetEventInfo();
	HANDLE hFile;
	char szText[2048],szServiceName[64],szEventName[256];
	SYSTEMTIME stStart,stEnd;
	DWORD Length,Write;

	hFile=::CreateFile(pszFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
					   CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (m_ServiceData.GetServiceName()!=NULL)
		::WideCharToMultiByte(CP_ACP,0,m_ServiceData.GetServiceName(),-1,
							  szServiceName,sizeof(szServiceName),NULL,NULL);
	else
		szServiceName[0]='\0';
	EventData.GetStartTime(&stStart);
	EventData.GetEndTime(&stEnd);
	if (EventData.GetEventName()!=NULL)
		::WideCharToMultiByte(CP_ACP,0,EventData.GetEventName(),-1,
							  szEventName,sizeof(szEventName),NULL,NULL);
	else
		szEventName[0]='\0';
	if (fVersion2) {
		Length=::wnsprintfA(szText,sizeof(szText),
			"Content-type: application/x-tv-program-digital-info; charset=shift_jis\r\n"
			"version: 2\r\n"
			"station: DFS%05x\r\n"
			"station-name: %s\r\n"
			"year: %d\r\n"
			"month: %02d\r\n"
			"date: %02d\r\n"
			"start: %02d:%02d\r\n"
			"end: %02d:%02d\r\n"
			"program-title: %s\r\n"
			"program-id: %d\r\n",
			m_ServiceData.m_ServiceID,szServiceName,
			stStart.wYear,stStart.wMonth,stStart.wDay,
			stStart.wHour,stStart.wMinute,
			stEnd.wHour,stEnd.wMinute,
			szEventName,EventData.m_EventID);
	} else {
		Length=::wnsprintfA(szText,sizeof(szText),
			"Content-type: application/x-tv-program-digital-info; charset=shift_jis\r\n"
			"version: 1\r\n"
			"station: %s\r\n"
			"year: %d\r\n"
			"month: %02d\r\n"
			"date: %02d\r\n"
			"start: %02d:%02d\r\n"
			"end: %02d:%02d\r\n"
			"program-title: %s\r\n",
			szServiceName,
			stStart.wYear,stStart.wMonth,stStart.wDay,
			stStart.wHour,stStart.wMinute,
			stEnd.wHour,stEnd.wMinute,
			szEventName);
	}
	bool fOK=::WriteFile(hFile,szText,Length,&Write,NULL) && Write==Length;
	::FlushFileBuffers(hFile);
	::CloseHandle(hFile);
	return fOK;
}




CProgramGuideServiceList::CProgramGuideServiceList()
{
	m_ppServiceList=NULL;
	m_NumServices=0;
	m_ServiceListLength=0;
}


CProgramGuideServiceList::~CProgramGuideServiceList()
{
	Clear();
}


CProgramGuideServiceInfo *CProgramGuideServiceList::GetItem(int Index)
{
	if (Index<0 || Index>=m_NumServices) {
		TRACE(TEXT("CProgramGuideServiceList::GetItem() Out of range %d\n"),Index);
		return NULL;
	}
	return m_ppServiceList[Index];
}


bool CProgramGuideServiceList::Add(CProgramGuideServiceInfo *pInfo)
{
	if (m_NumServices==m_ServiceListLength) {
		if (m_ServiceListLength==0)
			m_ServiceListLength=16;
		else
			m_ServiceListLength*=2;
		m_ppServiceList=static_cast<CProgramGuideServiceInfo**>(realloc(m_ppServiceList,m_ServiceListLength*sizeof(CProgramGuideServiceInfo*)));
	}
	m_ppServiceList[m_NumServices++]=pInfo;
	return true;
}


void CProgramGuideServiceList::Clear()
{
	if (m_ppServiceList!=NULL) {
		for (int i=m_NumServices-1;i>=0;i--)
			delete m_ppServiceList[i];
		free(m_ppServiceList);
		m_ppServiceList=NULL;
		m_NumServices=0;
		m_ServiceListLength=0;
	}
}




#if 0

CProgramGuideServiceIDList::CProgramGuideServiceIDList()
{
	m_pServiceList=NULL;
	m_NumServices=0;
	m_ServiceListLength=0;
}


CProgramGuideServiceIDList::~CProgramGuideServiceIDList()
{
	Clear();
}


CProgramGuideServiceIDList &CProgramGuideServiceIDList::operator=(const CProgramGuideServiceIDList &List)
{
	Clear();
	if (List.m_NumServices>0) {
		m_pServiceList=(ServiceInfo*)malloc(List.m_NumServices*sizeof(ServiceInfo));
		m_NumServices=List.m_NumServices;
		m_ServiceListLength=List.m_NumServices;
		::CopyMemory(m_pServiceList,List.m_pServiceList,List.m_NumServices*sizeof(ServiceInfo));
	}
	return *this;
}


WORD CProgramGuideServiceIDList::GetTransportStreamID(int Index) const
{
	if (Index<0 || Index>=m_NumServices)
		return 0;
	return m_pServiceList[Index].TransportStreamID;
}


WORD CProgramGuideServiceIDList::GetServiceID(int Index) const
{
	if (Index<0 || Index>=m_NumServices)
		return 0;
	return m_pServiceList[Index].ServiceID;
}


bool CProgramGuideServiceIDList::Add(WORD TransportStreamID,WORD ServiceID)
{
	if (m_ServiceListLength==m_NumServices) {
		if (m_ServiceListLength==0)
			m_ServiceListLength=16;
		else
			m_ServiceListLength*=2;
		m_pServiceList=(ServiceInfo*)realloc(m_pServiceList,m_ServiceListLength*sizeof(ServiceInfo));
	}
	m_pServiceList[m_NumServices++].TransportStreamID=TransportStreamID;
	m_pServiceList[m_NumServices++].ServiceID=ServiceID;
	return true;
}


void CProgramGuideServiceIDList::Clear()
{
	if (m_pServiceList!=NULL) {
		free(m_pServiceList);
		m_pServiceList=NULL;
		m_NumServices=0;
		m_ServiceListLength=0;
	}
}


int CProgramGuideServiceIDList::FindServiceID(WORD ServiceID) const
{
	int i;

	for (i=m_NumServices-1;i>=0;i--) {
		if (m_pServiceList[i].ServiceID==ServiceID)
			break;
	}
	return i;
}

#endif




CProgramGuideEventHandler::CProgramGuideEventHandler()
{
	m_pProgramGuide=NULL;
}


CProgramGuideEventHandler::~CProgramGuideEventHandler()
{
}




HINSTANCE CProgramGuide::m_hinst=NULL;


bool CProgramGuide::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=PROGRAM_GUIDE_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuide::CProgramGuide()
	: m_hfont(NULL)
	, m_hfontTitle(NULL)
	, m_hfontTime(NULL)
{
	LOGFONT lf;
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	SetFont(&lf);
	m_WindowPosition.Left=0;
	m_WindowPosition.Top=0;
	m_WindowPosition.Width=640;
	m_WindowPosition.Height=480;
	m_pProgramList=NULL;
	m_LinesPerHour=12;
	m_LineMargin=1;
	m_ItemWidth=140;
	m_ItemMargin=4;
	m_TextLeftMargin=8;
	m_ScrollPos.x=0;
	m_ScrollPos.y=0;
	m_CurrentTuningSpace=-1;
	m_szDriverFileName[0]='\0';
	m_pDriverManager=NULL;
	m_ColorList[COLOR_BACK]=::GetSysColor(COLOR_WINDOW);
	m_ColorList[COLOR_TEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_CHANNELNAMEBACK]=::GetSysColor(COLOR_3DFACE);
	m_ColorList[COLOR_CHANNELNAMETEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_TIMEBACK]=::GetSysColor(COLOR_3DFACE);
	m_ColorList[COLOR_TIMETEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	for (int i=COLOR_CONTENT_FIRST;i<=COLOR_CONTENT_LAST;i++)
		m_ColorList[i]=RGB(240,240,240);
	m_ColorList[COLOR_CONTENT_NEWS]=RGB(255,255,224);
	m_ColorList[COLOR_CONTENT_SPORTS]=RGB(255,255,224);
	//m_ColorList[COLOR_CONTENT_INFORMATION]=RGB(255,255,224);
	m_ColorList[COLOR_CONTENT_DRAMA]=RGB(255,224,224);
	m_ColorList[COLOR_CONTENT_MUSIC]=RGB(224,255,224);
	m_ColorList[COLOR_CONTENT_VARIETY]=RGB(224,224,255);
	m_ColorList[COLOR_CONTENT_MOVIE]=RGB(224,255,255);
	m_ColorList[COLOR_CONTENT_ANIME]=RGB(255,224,255);
	m_ColorList[COLOR_CONTENT_DOCUMENTARY]=RGB(255,255,224);
	m_ColorList[COLOR_CONTENT_THEATER]=RGB(224,255,255);
	m_fUpdating=false;
	m_pEventHandler=NULL;
}


CProgramGuide::~CProgramGuide()
{
	::DeleteObject(m_hfont);
	::DeleteObject(m_hfontTitle);
	::DeleteObject(m_hfontTime);
}


bool CProgramGuide::SetEpgProgramList(CEpgProgramList *pList)
{
	m_pProgramList=pList;
	return true;
}


bool CProgramGuide::UpdateProgramGuide()
{
	if (m_hwnd!=NULL) {
		::SetWindowText(m_hwnd,TITLE_TEXT TEXT(" - 番組表を作成しています..."));
		if (UpdateList()) {
			CalcLayout();
			SetScrollBar();
			Invalidate();
		}
		SetTitleBar();
	}
	return true;
}


bool CProgramGuide::UpdateList()
{
	if (m_pProgramList==NULL)
		return false;

	m_ServiceList.Clear();
	for (int i=0;i<m_ChannelList.NumChannels();i++) {
		const CChannelInfo *pChannelInfo=m_ChannelList.GetChannelInfo(i);
		CEpgServiceInfo *pServiceInfo=m_pProgramList->GetServiceInfo(
			pChannelInfo->GetTransportStreamID(),pChannelInfo->GetServiceID());
		CProgramGuideServiceInfo *pService;

		if (pServiceInfo!=NULL) {
			pService=new CProgramGuideServiceInfo(pChannelInfo,*pServiceInfo);
			CEventInfoList::EventIterator itrEvent;
			for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
					itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();
					itrEvent++) {
				pService->AddProgram(new CProgramGuideItem(itrEvent->second));
			}
			pService->SortPrograms();
#if 1
			m_ServiceList.Add(pService);
		}
#else
		} else {
			pService=new CProgramGuideServiceInfo(pChannelInfo);
		}
		m_ServiceList.Add(pService);
#endif
	}
	return true;
}


void CProgramGuide::CalcLayout()
{
	SYSTEMTIME stFirst=m_stFirstTime,stLast=m_stLastTime;
	HDC hdc;
	HFONT hfontOld;

	if (m_Day!=DAY_TODAY) {
		LONGLONG Offset=(LONGLONG)m_Day*(24*60*60*1000);

		OffsetSystemTime(&stFirst,Offset);
		OffsetSystemTime(&stLast,Offset);
	}
	hdc=::GetDC(m_hwnd);
	hfontOld=static_cast<HFONT>(GetCurrentObject(hdc,OBJ_FONT));
	for (int i=0;i<m_ServiceList.NumServices();i++) {
		m_ServiceList.GetItem(i)->CalcLayout(hdc,&stFirst,&stLast,
			m_LinesPerHour,m_hfontTitle,m_ItemWidth,m_hfont,m_ItemWidth-m_TextLeftMargin);
	}
	SelectFont(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


void CProgramGuide::DrawProgramList(int Service,HDC hdc,const RECT *pRect,const RECT *pPaintRect)
{
	CProgramGuideServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);
	RECT rcItem,rc;
	int LineHeight=m_FontHeight+m_LineMargin;
	COLORREF crOldTextColor;
	HFONT hfontOld;
	int ColorType;

	hfontOld=static_cast<HFONT>(GetCurrentObject(hdc,OBJ_FONT));
	crOldTextColor=::SetTextColor(hdc,m_ColorList[COLOR_TEXT]);
	rcItem.left=pRect->left;
	rcItem.right=pRect->right;
	for (int i=pServiceInfo->GetFirstItem();i<pServiceInfo->GetLastItem();i++) {
		CProgramGuideItem *pItem=pServiceInfo->GetProgram(i);

		if (!pItem->IsNullItem() && pItem->GetItemLines()>0) {
			rcItem.top=pRect->top+pItem->GetItemPos()*LineHeight;
			rcItem.bottom=rcItem.top+pItem->GetItemLines()*LineHeight;
			if (rcItem.top>=pPaintRect->bottom || rcItem.bottom<=pPaintRect->top)
				continue;
			if (pItem->GetEventInfo().m_NibbleList.size()>0
					&& pItem->GetEventInfo().m_NibbleList[0].m_ContentNibbleLv1<=CEventInfoData::CONTENT_LAST)
				ColorType=COLOR_CONTENT_FIRST+
					pItem->GetEventInfo().m_NibbleList[0].m_ContentNibbleLv1;
			else
				ColorType=COLOR_CONTENT_OTHER;
			HBRUSH hbr=::CreateSolidBrush(m_ColorList[ColorType]);
			::FillRect(hdc,&rcItem,hbr);
			::DeleteObject(hbr);
			rc=rcItem;
			rc.bottom=min(rc.bottom,rc.top+pItem->GetTitleLines()*LineHeight);
			SelectFont(hdc,m_hfontTitle);
			pItem->DrawTitle(hdc,&rc,LineHeight);
			if (rc.bottom<rcItem.bottom) {
				rc.left+=m_TextLeftMargin;
				rc.top=rc.bottom;
				rc.bottom=rcItem.bottom;
				SelectFont(hdc,m_hfont);
				pItem->DrawText(hdc,&rc,LineHeight);
			}
		}
	}
	::SetTextColor(hdc,crOldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawServiceName(int Service,HDC hdc,const RECT *pRect)
{
	CProgramGuideServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);
	HBRUSH hbr;
	COLORREF crOldTextColor;
	HFONT hfontOld;
	RECT rc;

	hbr=::CreateSolidBrush(m_ColorList[COLOR_CHANNELNAMEBACK]);
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	hfontOld=SelectFont(hdc,m_hfontTitle);
	crOldTextColor=::SetTextColor(hdc,m_ColorList[COLOR_CHANNELNAMETEXT]);
	rc=*pRect;
	::DrawText(hdc,pServiceInfo->GetServiceName(),-1,&rc,
			   DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
	::SetTextColor(hdc,crOldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawTimeBar(HDC hdc,const RECT *pRect)
{
	HBRUSH hbr;
	HFONT hfontOld;
	COLORREF crOldTextColor;
	HPEN hpen,hpenOld;
	RECT rc;

	hbr=::CreateSolidBrush(m_ColorList[COLOR_TIMEBACK]);
	::FillRect(hdc,pRect,hbr);
	::DeleteObject(hbr);
	hfontOld=SelectFont(hdc,m_hfontTime);
	crOldTextColor=::SetTextColor(hdc,m_ColorList[COLOR_TIMETEXT]);
	hpen=::CreatePen(PS_SOLID,0,m_ColorList[COLOR_TIMETEXT]);
	hpenOld=SelectPen(hdc,hpen);
	rc.left=pRect->left;
	rc.top=pRect->top;
	rc.right=pRect->right;
	for (int i=0;i<m_Hours;i++) {
		TCHAR szText[4];

		::MoveToEx(hdc,rc.left,rc.top,NULL);
		::LineTo(hdc,rc.right,rc.top);
		::wsprintf(szText,TEXT("%d"),(m_stFirstTime.wHour+i)%24);
		rc.bottom=rc.top+(m_FontHeight+m_LineMargin)*m_LinesPerHour;
		::TextOut(hdc,rc.right-4,rc.top+4,szText,lstrlen(szText));
		rc.top=rc.bottom;
	}
	::SelectObject(hdc,hpenOld);
	::DeleteObject(hpen);
	::SetTextColor(hdc,crOldTextColor);
	SelectFont(hdc,hfontOld);
}


void CProgramGuide::GetProgramGuideRect(RECT *pRect)
{
	GetClientRect(pRect);
	pRect->left+=m_TimeBarWidth;
	pRect->right-=m_TimeBarWidth;
	pRect->top+=m_ServiceNameHeight;
}


void CProgramGuide::Scroll(int XScroll,int YScroll)
{
	POINT Pos=m_ScrollPos;
	RECT rc;
	SCROLLINFO si;
	int XScrollSize=0,YScrollSize=0;

	GetProgramGuideRect(&rc);
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_POS;
	if (XScroll!=0) {
		int TotalWidth=m_ServiceList.NumServices()*(m_ItemWidth+m_ItemMargin*2);
		int XPage=max(rc.right-rc.left,0);

		Pos.x=m_ScrollPos.x+XScroll;
		if (Pos.x<0)
			Pos.x=0;
		else if (Pos.x>max(TotalWidth-XPage,0))
			Pos.x=max(TotalWidth-XPage,0);
		si.nPos=Pos.x;
		::SetScrollInfo(m_hwnd,SB_HORZ,&si,TRUE);
		XScrollSize=m_ScrollPos.x-Pos.x;
	}
	if (YScroll!=0) {
		int TotalLines=m_Hours*m_LinesPerHour;
		int YPage=(rc.bottom-rc.top)/(m_FontHeight+m_LineMargin);

		Pos.y=m_ScrollPos.y+YScroll;
		if (Pos.y<0)
			Pos.y=0;
		else if (Pos.y>max(TotalLines-YPage,0))
			Pos.y=max(TotalLines-YPage,0);
		si.nPos=Pos.y;
		::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
		YScrollSize=(m_ScrollPos.y-Pos.y)*(m_FontHeight+m_LineMargin);
	}
	if (abs(YScrollSize)<rc.bottom-rc.top && abs(XScrollSize)<rc.right-rc.left
			&& (XScrollSize==0 || YScrollSize==0)) {
		RECT rcClip;

		GetClientRect(&rcClip);
		if (XScrollSize!=0) {
			rcClip.left+=m_TimeBarWidth;
			rcClip.right-=m_TimeBarWidth;
		} else {
			rcClip.top=rc.top;
		}
		::ScrollWindowEx(m_hwnd,XScrollSize,YScrollSize,NULL,&rcClip,
										NULL,NULL,SW_ERASE | SW_INVALIDATE);
	} else {
		Invalidate();
	}
	m_ScrollPos=Pos;
}


void CProgramGuide::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	GetProgramGuideRect(&rc);
	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=m_Hours*m_LinesPerHour-1;
	si.nPage=(rc.bottom-rc.top)/(m_FontHeight+m_LineMargin);
	si.nPos=m_ScrollPos.y;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
	si.nMax=m_ServiceList.NumServices()*(m_ItemWidth+m_ItemMargin*2)-1;
	si.nPage=rc.right-rc.left;
	si.nPos=m_ScrollPos.x;
	::SetScrollInfo(m_hwnd,SB_HORZ,&si,TRUE);
}


void CProgramGuide::SetTitleBar()
{
	if (m_hwnd!=NULL) {
		if (m_pProgramList!=NULL) {
			if (m_fUpdating) {
				::SetWindowText(m_hwnd,TITLE_TEXT TEXT(" - 番組表の取得中..."));
			} else {
				static const LPCTSTR pszDay[] = {
					TEXT("今日"), TEXT("明日"), TEXT("明後日"), TEXT("明々後日"),
					TEXT("4日後"), TEXT("5日後"), TEXT("6日後")
				};
				TCHAR szText[256];
				SYSTEMTIME stFirst=m_stFirstTime,stLast=m_stLastTime;

				if (m_Day!=DAY_TODAY) {
					LONGLONG Offset=(LONGLONG)m_Day*(24*60*60*1000);

					OffsetSystemTime(&stFirst,Offset);
					OffsetSystemTime(&stLast,Offset);
				}
				::wsprintf(szText,TITLE_TEXT TEXT(" - %s %d/%d(%s) %d時 ～ %d/%d(%s) %d時"),
						   pszDay[m_Day],stFirst.wMonth,stFirst.wDay,
						   GetDayOfWeekText(stFirst.wDayOfWeek),stFirst.wHour,
						   stLast.wMonth,stLast.wDay,
						   GetDayOfWeekText(stLast.wDayOfWeek),stLast.wHour);
				::SetWindowText(m_hwnd,szText);
			}
		} else {
			::SetWindowText(m_hwnd,TITLE_TEXT);
		}
	}
}


bool CProgramGuide::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 PROGRAM_GUIDE_WINDOW_CLASS,TITLE_TEXT,m_hinst);
}


/*
bool CProgramGuide::SetServiceIDList(const CProgramGuideServiceIDList *pList)
{
	m_ServiceIDList=*pList;
	return true;
}
*/


bool CProgramGuide::SetTuningSpaceList(LPCTSTR pszDriverFileName,const CTuningSpaceList *pList,int Space)
{
	m_TuningSpaceList=*pList;
	SetTuningSpace(Space);
	::lstrcpy(m_szDriverFileName,pszDriverFileName);
	return true;
}


bool CProgramGuide::SetTuningSpace(int Space)
{
	const CChannelList *pList;

	if (Space<0)
		pList=m_TuningSpaceList.GetAllChannelList();
	else
		pList=m_TuningSpaceList.GetChannelList(Space);

	int OldTSID,OldNumChannels=m_ChannelList.NumChannels();
	if (OldNumChannels>0)
		OldTSID=m_ChannelList.GetChannelInfo(0)->GetTransportStreamID();
	else
		OldTSID=0;
	m_ChannelList.Clear();
	if (pList!=NULL && pList->NumChannels()>0) {
		for (int i=0;i<pList->NumChannels();i++) {
			const CChannelInfo *pChannelInfo=pList->GetChannelInfo(i);

			if (pChannelInfo->IsEnabled())
				m_ChannelList.AddChannel(*pChannelInfo);
		}
		if (OldTSID!=pList->GetChannelInfo(0)->GetTransportStreamID()
				|| OldNumChannels!=pList->NumChannels()) {
			m_ScrollPos.x=0;
			m_ScrollPos.y=0;
		}
	} else {
		m_ScrollPos.x=0;
		m_ScrollPos.y=0;
	}
	m_CurrentTuningSpace=Space;
	return true;
}


bool CProgramGuide::UpdateChannelList()
{
	return SetTuningSpace(m_CurrentTuningSpace);
}


bool CProgramGuide::SetDriverList(const CDriverManager *pDriverManager)
{
	m_pDriverManager=pDriverManager;
	return true;
}


bool CProgramGuide::SetTimeRange(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime)
{
	FILETIME ftFirst,ftLast;

	if (CompareSystemTime(pFirstTime,pLastTime)>=0)
		return false;
	m_stFirstTime=*pFirstTime;
	m_stFirstTime.wMinute=0;
	m_stFirstTime.wSecond=0;
	m_stFirstTime.wMilliseconds=0;
	m_stLastTime=*pLastTime;
	m_stLastTime.wMinute=0;
	m_stLastTime.wSecond=0;
	m_stLastTime.wMilliseconds=0;
	::SystemTimeToFileTime(&m_stFirstTime,&ftFirst);
	::SystemTimeToFileTime(&m_stLastTime,&ftLast);
	m_Hours=(int)((ftLast-ftFirst)/(FILETIME_SECOND*60*60));
	return true;
}


bool CProgramGuide::GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime)
{
	if (pFirstTime!=NULL)
		*pFirstTime=m_stFirstTime;
	if (pLastTime!=NULL)
		*pLastTime=m_stLastTime;
	return true;
}


bool CProgramGuide::SetViewDay(int Day)
{
	if (Day<DAY_TODAY || Day>DAY_LAST)
		return false;
	m_Day=Day;
	return true;
}


bool CProgramGuide::SetUIOptions(int LinesPerHour,int ItemWidth)
{
	if (LinesPerHour<MIN_LINES_PER_HOUR || LinesPerHour>MAX_LINES_PER_HOUR
			|| ItemWidth<MIN_ITEM_WIDTH || ItemWidth>MAX_ITEM_WIDTH)
		return false;
	if (m_LinesPerHour!=LinesPerHour
			|| m_ItemWidth!=ItemWidth) {
		m_LinesPerHour=LinesPerHour;
		m_ItemWidth=ItemWidth;
		if (m_hwnd!=NULL) {
			m_ScrollPos.x=0;
			m_ScrollPos.y=0;
			CalcLayout();
			SetScrollBar();
			Invalidate();
		}
	}
	return true;
}


bool CProgramGuide::SetFont(const LOGFONT *pFont)
{
	LOGFONT lf;

	if (m_hfont!=NULL)
		::DeleteObject(m_hfont);
	if (m_hfontTitle!=NULL)
		::DeleteObject(m_hfontTitle);
	if (m_hfontTime!=NULL)
		::DeleteObject(m_hfontTime);
	m_hfont=::CreateFontIndirect(pFont);
	lf=*pFont;
	lf.lfWeight=FW_BOLD;
	m_hfontTitle=::CreateFontIndirect(&lf);
	HDC hdc=::CreateDC(TEXT("DISPLAY"),NULL,NULL,NULL);
	if (hdc!=NULL) {
		HFONT hfontOld=static_cast<HFONT>(::SelectObject(hdc,m_hfont));
		TEXTMETRIC tm;
		::GetTextMetrics(hdc,&tm);
		::DeleteDC(hdc);
		//m_FontHeight=tm.tmHeight+tm.tmInternalLeading;
		m_FontHeight=tm.tmHeight;
	} else {
		m_FontHeight=abs(lf.lfHeight);
	}
	m_ServiceNameHeight=m_FontHeight+8;
	m_TimeBarWidth=m_FontHeight+8;
	lf.lfWeight=FW_NORMAL;
	lf.lfEscapement=lf.lfOrientation=2700;
	m_hfontTime=::CreateFontIndirect(&lf);
	if (m_hwnd!=NULL) {
		m_ScrollPos.x=0;
		m_ScrollPos.y=0;
		CalcLayout();
		SetScrollBar();
		Invalidate();
	}
	return true;
}


bool CProgramGuide::SetColor(int Type,COLORREF Color)
{
	if (Type<0 || Type>COLOR_LAST)
		return false;
	m_ColorList[Type]=Color;
	return true;
}


bool CProgramGuide::SetEventHandler(CProgramGuideEventHandler *pEventHandler)
{
	if (m_pEventHandler)
		m_pEventHandler->m_pProgramGuide=NULL;
	if (pEventHandler)
		pEventHandler->m_pProgramGuide=this;
	m_pEventHandler=pEventHandler;
	return true;
}


bool CProgramGuide::HitTest(int x,int y,int *pServiceIndex,int *pProgramIndex)
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetProgramGuideRect(&rc);
	if (::PtInRect(&rc,pt)) {
		int Service;

		Service=(x-rc.left+m_ScrollPos.x)/(m_ItemWidth+m_ItemMargin*2);
		y-=rc.top;
		if (Service<m_ServiceList.NumServices()) {
			CProgramGuideServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);
			int LineHeight=m_FontHeight+m_LineMargin;
			int Program=-1;

			y+=m_ScrollPos.y*LineHeight;
			for (int i=pServiceInfo->GetFirstItem();i<pServiceInfo->GetLastItem();i++) {
				CProgramGuideItem *pItem=pServiceInfo->GetProgram(i);

				if (!pItem->IsNullItem() && pItem->GetItemLines()>0) {
					rc.top=pItem->GetItemPos()*LineHeight;
					rc.bottom=rc.top+pItem->GetItemLines()*LineHeight;
					if (y>=rc.top && y<rc.bottom) {
						Program=i;
						break;
					}
				}
			}
			if (Program>=0) {
				if (pServiceIndex)
					*pServiceIndex=Service;
				if (pProgramIndex)
					*pProgramIndex=Program;
				return true;
			}
		}
	}
	return false;
}


CProgramGuide *CProgramGuide::GetThis(HWND hwnd)
{
	return static_cast<CProgramGuide*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CProgramGuide::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			//CProgramGuide *pThis=dynamic_cast<CProgramGuide*>(OnCreate(hwnd,lParam));
			OnCreate(hwnd,lParam);
			::SendMessage(hwnd,WM_SETICON,ICON_BIG,(LPARAM)::LoadIcon(m_hinst,MAKEINTRESOURCE(IDI_PROGRAMGUIDE)));
			::SendMessage(hwnd,WM_SETICON,ICON_SMALL,
				(LPARAM)::LoadImage(m_hinst,MAKEINTRESOURCE(IDI_PROGRAMGUIDE),
									IMAGE_ICON,
									::GetSystemMetrics(SM_CXSMICON),
									::GetSystemMetrics(SM_CYSMICON),
									LR_DEFAULTCOLOR));
		}
		return 0;

	case WM_PAINT:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			int OldBkMode;
			RECT rcClient,rc;
			HRGN hrgn;

			::BeginPaint(hwnd,&ps);
			HBRUSH hbr=::CreateSolidBrush(pThis->m_ColorList[COLOR_BACK]);
			::FillRect(ps.hdc,&ps.rcPaint,hbr);
			::DeleteObject(hbr);
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			::GetClientRect(hwnd,&rcClient);
			if (pThis->m_ServiceList.NumServices()>0) {
				int i;

				if (ps.rcPaint.top<pThis->m_ServiceNameHeight) {
					rc.left=rcClient.left+pThis->m_TimeBarWidth;
					rc.top=0;
					rc.right=rcClient.right-pThis->m_TimeBarWidth;
					rc.bottom=pThis->m_ServiceNameHeight;
					hrgn=::CreateRectRgnIndirect(&rc);
					::SelectClipRgn(ps.hdc,hrgn);
					rc.left=pThis->m_TimeBarWidth-pThis->m_ScrollPos.x;
					for (i=0;i<pThis->m_ServiceList.NumServices();i++) {
						rc.right=rc.left+(pThis->m_ItemWidth+pThis->m_ItemMargin*2);
						if (rc.left<ps.rcPaint.right && rc.right>ps.rcPaint.left)
							pThis->DrawServiceName(i,ps.hdc,&rc);
						rc.left=rc.right;
					}
					::SelectClipRgn(ps.hdc,NULL);
					::DeleteObject(hrgn);
				}
				rc.left=pThis->m_TimeBarWidth;
				rc.top=pThis->m_ServiceNameHeight;
				rc.right=rcClient.right-pThis->m_TimeBarWidth;
				rc.bottom=rcClient.bottom;
				hrgn=::CreateRectRgnIndirect(&rc);
				::SelectClipRgn(ps.hdc,hrgn);
				rc.top=pThis->m_ServiceNameHeight-pThis->m_ScrollPos.y*(pThis->m_FontHeight+pThis->m_LineMargin);
				rc.left=pThis->m_TimeBarWidth+pThis->m_ItemMargin-pThis->m_ScrollPos.x;
				for (i=0;i<pThis->m_ServiceList.NumServices();i++) {
					rc.right=rc.left+pThis->m_ItemWidth;
					if (rc.left<ps.rcPaint.right && rc.right>ps.rcPaint.left
							&& rc.top<ps.rcPaint.bottom)
						pThis->DrawProgramList(i,ps.hdc,&rc,&ps.rcPaint);
					rc.left=rc.right+pThis->m_ItemMargin*2;
				}
				::SelectClipRgn(ps.hdc,NULL);
				::DeleteObject(hrgn);
			}
			rc.left=0;
			rc.top=pThis->m_ServiceNameHeight;
			rc.right=rcClient.right;
			rc.bottom=rcClient.bottom;
			hrgn=::CreateRectRgnIndirect(&rc);
			::SelectClipRgn(ps.hdc,hrgn);
			rc.top=pThis->m_ServiceNameHeight-pThis->m_ScrollPos.y*(pThis->m_FontHeight+pThis->m_LineMargin);
				rc.bottom=rc.top+(pThis->m_FontHeight+pThis->m_LineMargin)*pThis->m_LinesPerHour*pThis->m_Hours;
			if (ps.rcPaint.left<pThis->m_TimeBarWidth) {
				rc.left=0;
				rc.right=pThis->m_TimeBarWidth;
				pThis->DrawTimeBar(ps.hdc,&rc);
			}
			rc.left=rcClient.right-pThis->m_TimeBarWidth;
			if (rc.left<ps.rcPaint.right) {
				rc.right=rcClient.right;
				pThis->DrawTimeBar(ps.hdc,&rc);
			}
			::SelectClipRgn(ps.hdc,NULL);
			::DeleteObject(hrgn);
			::SetBkMode(ps.hdc,OldBkMode);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			RECT rc;
			POINT Pos;
			int TotalWidth=pThis->m_ServiceList.NumServices()*(pThis->m_ItemWidth+pThis->m_ItemMargin*2);
			int TotalLines=pThis->m_Hours*pThis->m_LinesPerHour;
			int XPage,YPage;

			pThis->GetProgramGuideRect(&rc);
			Pos=pThis->m_ScrollPos;
			XPage=max(rc.right-rc.left,0);
			if (Pos.x>max(TotalWidth-XPage,0))
				Pos.x=max(TotalWidth-XPage,0);
			YPage=(rc.bottom-rc.top)/(pThis->m_FontHeight+pThis->m_LineMargin);
			if (Pos.y>max(TotalLines-YPage,0))
				Pos.y=max(TotalLines-YPage,0);
			if (Pos.x!=pThis->m_ScrollPos.x || Pos.y!=pThis->m_ScrollPos.y)
				pThis->Scroll(Pos.x-pThis->m_ScrollPos.x,Pos.y-pThis->m_ScrollPos.y);
			else
				pThis->SetScrollBar();
		}
		return 0;

	case WM_MOUSEWHEEL:
		if (GET_WHEEL_DELTA_WPARAM(wParam)<0)
			wParam=SB_LINEDOWN;
		else
			wParam=SB_LINEUP;
	case WM_VSCROLL:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			int Pos;
			RECT rc;
			int Page;
			int TotalLines=pThis->m_Hours*pThis->m_LinesPerHour;

			Pos=pThis->m_ScrollPos.y;
			pThis->GetProgramGuideRect(&rc);
			Page=(rc.bottom-rc.top)/(pThis->m_FontHeight+pThis->m_LineMargin);
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos--;				break;
			case SB_LINEDOWN:	Pos++;				break;
			case SB_PAGEUP:		Pos-=Page;			break;
			case SB_PAGEDOWN:	Pos+=Page;			break;
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);	break;
			case SB_TOP:		Pos=0;				break;
			case SB_BOTTOM:		Pos=max(TotalLines-Page,0);	break;
			default:	return 0;
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>max(TotalLines-Page,0))
				Pos=max(TotalLines-Page,0);
			if (Pos!=pThis->m_ScrollPos.y)
				pThis->Scroll(0,Pos-pThis->m_ScrollPos.y);
		}
		return 0;

	case WM_HSCROLL:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			int Pos;
			RECT rc;
			int Page;
			int TotalWidth=pThis->m_ServiceList.NumServices()*(pThis->m_ItemWidth+pThis->m_ItemMargin*2);

			Pos=pThis->m_ScrollPos.x;
			pThis->GetProgramGuideRect(&rc);
			Page=rc.right-rc.left;
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos-=pThis->m_FontHeight;	break;
			case SB_LINEDOWN:	Pos+=pThis->m_FontHeight;	break;
			case SB_PAGEUP:		Pos-=Page;					break;
			case SB_PAGEDOWN:	Pos+=Page;					break;
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);			break;
			case SB_TOP:		Pos=0;						break;
			case SB_BOTTOM:		Pos=max(TotalWidth-Page,0);	break;
			default:	return 0;
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>max(TotalWidth-Page,0))
				Pos=max(TotalWidth-Page,0);
			if (Pos!=pThis->m_ScrollPos.x)
				pThis->Scroll(Pos-pThis->m_ScrollPos.x,0);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			POINT pt;

			//::SetFocus(hwnd);
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			if (pt.y<pThis->m_ServiceNameHeight
					&& pt.x>=pThis->m_TimeBarWidth
					&& pt.x-pThis->m_TimeBarWidth<
						pThis->m_ServiceList.NumServices()*(
							pThis->m_ItemWidth+pThis->m_ItemMargin*2)-pThis->m_ScrollPos.x) {
				if (pThis->m_pEventHandler) {
					int Service=(pt.x+pThis->m_ScrollPos.x-pThis->m_TimeBarWidth)/(pThis->m_ItemWidth+pThis->m_ItemMargin*2);
					CProgramGuideServiceInfo *pServiceInfo=
										pThis->m_ServiceList.GetItem(Service);

					if (pServiceInfo)
						pThis->m_pEventHandler->OnServiceTitleLButtonDown(
										pThis->m_szDriverFileName,
										pServiceInfo->GetServiceInfoData());
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			POINT pt;
			HMENU hmenu,hmenuPopup;
			TCHAR szText[64];

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			pThis->m_CurItem.fValid=
				pThis->HitTest(pt.x,pt.y,&pThis->m_CurItem.Service,&pThis->m_CurItem.Program);
			//::SetFocus(hwnd);
			hmenu=::LoadMenu(m_hinst,MAKEINTRESOURCE(IDM_PROGRAMGUIDE));
			hmenuPopup=::GetSubMenu(hmenu,0);
			::CheckMenuRadioItem(hmenu,CM_PROGRAMGUIDE_TODAY,
								 CM_PROGRAMGUIDE_TODAY+DAY_LAST,
								 CM_PROGRAMGUIDE_TODAY+pThis->m_Day,MF_BYCOMMAND);
			SYSTEMTIME st=pThis->m_stFirstTime;
			MENUITEMINFO mii;
			mii.cbSize=sizeof(mii);
			mii.fMask=MIIM_STRING;
			mii.dwTypeData=szText;
			for (int i=CM_PROGRAMGUIDE_TODAY;i<=CM_PROGRAMGUIDE_TODAY+DAY_LAST;i++) {
				mii.cch=lengthof(szText);
				::GetMenuItemInfo(hmenu,i,FALSE,&mii);
				::wsprintf(szText+::lstrlen(szText),TEXT(" %d/%d (%s)"),
						   st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek));
				::SetMenuItemInfo(hmenu,i,FALSE,&mii);
				OffsetSystemTime(&st,24*60*60*1000);
			}

			HMENU hmenuDriver=::GetSubMenu(hmenuPopup,MENU_DRIVER);
			ClearMenu(hmenuDriver);
			if (pThis->m_TuningSpaceList.GetAllChannelList()->NumChannels()>0) {
				::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
							 CM_PROGRAMGUIDE_TUNINGSPACE_ALL,TEXT("すべて"));
			}
			for (int i=0;i<pThis->m_TuningSpaceList.NumSpaces();i++) {
				LPCTSTR pszName=pThis->m_TuningSpaceList.GetTuningSpaceName(i);

				if (pszName==NULL) {
					::wsprintf(szText,TEXT("チューニング空間 %d"),i+1);
					pszName=szText;
				}
				::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
							 CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+i,pszName);
			}
			if (::GetMenuItemCount(hmenuDriver)>0) {
				::CheckMenuRadioItem(hmenuDriver,CM_PROGRAMGUIDE_TUNINGSPACE_ALL,
									 CM_PROGRAMGUIDE_TUNINGSPACE_ALL+pThis->m_TuningSpaceList.NumSpaces(),
									 CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+pThis->m_CurrentTuningSpace,
									 MF_BYCOMMAND);
			}
			if (pThis->m_pDriverManager!=NULL) {
				int NumDrivers=pThis->m_pDriverManager->NumDrivers();

				if (NumDrivers>0) {
					::AppendMenu(hmenuDriver,MFT_SEPARATOR,0,NULL);
					int CurDriver=-1;
					for (int i=0;i<NumDrivers;i++) {
						const CDriverInfo *pDriverInfo=pThis->m_pDriverManager->GetDriverInfo(i);

						if (CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName()))
							continue;
						::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
									 CM_PROGRAMGUIDE_DRIVER_FIRST+i,
									 pDriverInfo->GetFileName());
						if (::lstrcmpi(pThis->m_szDriverFileName,pDriverInfo->GetFileName())==0)
							CurDriver=i;
					}
					if (CurDriver>=0)
						::CheckMenuRadioItem(hmenuDriver,CM_PROGRAMGUIDE_DRIVER_FIRST,
											 CM_PROGRAMGUIDE_DRIVER_FIRST+NumDrivers-1,
											 CM_PROGRAMGUIDE_DRIVER_FIRST+CurDriver,
											 MF_BYCOMMAND);
				}
			}
			::EnableMenuItem(hmenuPopup,MENU_DRIVER,
							 MF_BYPOSITION | (::GetMenuItemCount(hmenuDriver)>0?MFS_ENABLED:MFS_GRAYED));

			::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_UPDATE,
				MF_BYCOMMAND | (pThis->m_fUpdating?MFS_GRAYED:MFS_ENABLED));
			::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_ENDUPDATE,
				MF_BYCOMMAND | (pThis->m_fUpdating?MFS_ENABLED:MFS_GRAYED));
			::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_IEPGASSOCIATE,
				MF_BYCOMMAND | (pThis->m_CurItem.fValid?MFS_ENABLED:MFS_GRAYED));
			if (pThis->m_ToolList.NumTools()>0) {
				::AppendMenu(hmenuPopup,MFT_SEPARATOR | MFS_ENABLED,0,NULL);
				for (int i=0;i<pThis->m_ToolList.NumTools();i++) {
					const CProgramGuideTool *pTool=pThis->m_ToolList.GetTool(i);
					TCHAR szText[CProgramGuideTool::MAX_NAME*2];

					CopyToMenuText(pTool->GetName(),szText,lengthof(szText));
					::AppendMenu(hmenuPopup,
						MFT_STRING | (pThis->m_CurItem.fValid?MFS_ENABLED:MFS_GRAYED),
						CM_PROGRAMGUIDETOOL_FIRST+i,szText);
				}
			}
			::ClientToScreen(hwnd,&pt);
			::TrackPopupMenu(::GetSubMenu(hmenu,0),TPM_RIGHTBUTTON,pt.x,pt.y,
							 0,hwnd,NULL);
			::DestroyMenu(hmenu);
		}
		return 0;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			CProgramGuide *pThis=GetThis(hwnd);
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			::GetClientRect(hwnd,&rc);
			if (pt.y<pThis->m_ServiceNameHeight
					&& pt.x>=pThis->m_TimeBarWidth
					&& pt.x<rc.right-pThis->m_TimeBarWidth
					&& pt.x-pThis->m_TimeBarWidth<pThis->m_ServiceList.NumServices()*
						(pThis->m_ItemWidth+pThis->m_ItemMargin*2)-pThis->m_ScrollPos.x) {
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
				return TRUE;
			}
		}
		break;

	case WM_KEYDOWN:
		{
			static const struct {
				WORD KeyCode;
				WORD Message;
				WORD Request;
			} KeyMap[] = {
				{VK_PRIOR,	WM_VSCROLL,	SB_PAGEUP},
				{VK_NEXT,	WM_VSCROLL,	SB_PAGEDOWN},
				{VK_UP,		WM_VSCROLL,	SB_LINEUP},
				{VK_DOWN,	WM_VSCROLL,	SB_LINEDOWN},
				{VK_LEFT,	WM_HSCROLL,	SB_LINEUP},
				{VK_RIGHT,	WM_HSCROLL,	SB_LINEDOWN},
				{VK_HOME,	WM_VSCROLL,	SB_TOP},
				{VK_END,	WM_VSCROLL,	SB_BOTTOM},
			};

			for (int i=0;i<lengthof(KeyMap);i++) {
				if (wParam==(WPARAM)KeyMap[i].KeyCode) {
					::SendMessage(hwnd,KeyMap[i].Message,KeyMap[i].Request,0);
					return 0;
				}
			}

			CProgramGuide *pThis=GetThis(hwnd);
			if (pThis->m_pEventHandler!=NULL
					&& pThis->m_pEventHandler->OnKeyDown(wParam,lParam))
				return 0;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_PROGRAMGUIDE_UPDATE:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (!pThis->m_fUpdating) {
					if (pThis->m_pEventHandler!=NULL
							&& pThis->m_pEventHandler->OnBeginUpdate()) {
						pThis->m_fUpdating=true;
						pThis->SetTitleBar();
					}
				}
			}
			return 0;

		case CM_PROGRAMGUIDE_ENDUPDATE:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_fUpdating) {
					if (pThis->m_pEventHandler)
						pThis->m_pEventHandler->OnEndUpdate();
					pThis->m_fUpdating=false;
					pThis->SetTitleBar();
				}
			}
			return 0;

		case CM_PROGRAMGUIDE_REFRESH:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_pEventHandler==NULL
						|| pThis->m_pEventHandler->OnRefresh())
					pThis->UpdateProgramGuide();
			}
			return 0;

		case CM_PROGRAMGUIDE_TODAY:
		case CM_PROGRAMGUIDE_TOMORROW:
		case CM_PROGRAMGUIDE_DAYAFTERTOMORROW:
		case CM_PROGRAMGUIDE_2DAYSAFTERTOMORROW:
		case CM_PROGRAMGUIDE_3DAYSAFTERTOMORROW:
		case CM_PROGRAMGUIDE_4DAYSAFTERTOMORROW:
		case CM_PROGRAMGUIDE_5DAYSAFTERTOMORROW:
			{
				CProgramGuide *pThis=GetThis(hwnd);
				int Day=LOWORD(wParam)-CM_PROGRAMGUIDE_TODAY;

				if (pThis->m_Day!=Day) {
					pThis->m_Day=Day;
					if (pThis->m_pProgramList!=NULL) {
						pThis->CalcLayout();
						pThis->m_ScrollPos.y=0;
						pThis->SetScrollBar();
						pThis->SetTitleBar();
						pThis->Invalidate();
					}
				}
			}
			return 0;

		case CM_PROGRAMGUIDE_IEPGASSOCIATE:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_CurItem.fValid) {
					CProgramGuideServiceInfo *pServiceInfo=
						pThis->m_ServiceList.GetItem(pThis->m_CurItem.Service);

					if (pServiceInfo!=NULL) {
						TCHAR szFileName[MAX_PATH];

						GetAppClass().GetAppDirectory(szFileName);
						::PathAppend(szFileName,TEXT("iepg.tvpid"));
						if (!pServiceInfo->SaveiEpgFile(pThis->m_CurItem.Program,
															szFileName,true))
							return 0;
						::ShellExecute(NULL,NULL,szFileName,NULL,NULL,SW_SHOWNORMAL);
					}
				}
			}
			return 0;

		default:
			if (LOWORD(wParam)>=CM_PROGRAMGUIDE_TUNINGSPACE_ALL
					&& LOWORD(wParam)<=CM_PROGRAMGUIDE_TUNINGSPACE_LAST) {
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_fUpdating)
					::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
				pThis->SetTuningSpace((int)LOWORD(wParam)-CM_PROGRAMGUIDE_TUNINGSPACE_FIRST);
				pThis->UpdateProgramGuide();
			} else if (LOWORD(wParam)>=CM_PROGRAMGUIDE_DRIVER_FIRST
					&& LOWORD(wParam)<=CM_PROGRAMGUIDE_DRIVER_LAST) {
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_fUpdating)
					::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
				if (pThis->m_pDriverManager!=NULL) {
					const CDriverInfo *pDriverInfo=pThis->m_pDriverManager->GetDriverInfo(LOWORD(wParam)-CM_PROGRAMGUIDE_DRIVER_FIRST);

					if (pDriverInfo!=NULL) {
						CDriverInfo DriverInfo(pDriverInfo->GetFileName());

						if (DriverInfo.LoadTuningSpaceList(false)) {
							pThis->SetTuningSpaceList(DriverInfo.GetFileName(),
												  DriverInfo.GetTuningSpaceList(),-1);
							pThis->UpdateProgramGuide();
						}
					}
				}
			} else if (LOWORD(wParam)>=CM_PROGRAMGUIDETOOL_FIRST
					&& LOWORD(wParam)<=CM_PROGRAMGUIDETOOL_LAST) {
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_CurItem.fValid) {
					CProgramGuideServiceInfo *pServiceInfo=
						pThis->m_ServiceList.GetItem(pThis->m_CurItem.Service);

					if (pServiceInfo!=NULL) {
						CProgramGuideTool *pTool=pThis->m_ToolList.GetTool(LOWORD(wParam)-CM_PROGRAMGUIDETOOL_FIRST);

						pTool->Execute(pServiceInfo,pThis->m_CurItem.Program);
					}
				}
			}
		}
		return 0;

	case WM_CLOSE:
		{
			CProgramGuide *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler && !pThis->m_pEventHandler->OnClose())
				return 0;
			if (pThis->m_fUpdating) {
				if (pThis->m_pEventHandler)
					pThis->m_pEventHandler->OnEndUpdate();
				pThis->m_fUpdating=false;
			}
		}
		break;

	case WM_DESTROY:
		{
			CProgramGuide *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CProgramGuideTool::CProgramGuideTool()
{
	m_szName[0]='\0';
	m_szCommand[0]='\0';
}


CProgramGuideTool::CProgramGuideTool(const CProgramGuideTool &Tool)
{
	::lstrcpy(m_szName,Tool.m_szName);
	::lstrcpy(m_szCommand,Tool.m_szCommand);
}


CProgramGuideTool::CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand)
{
	::lstrcpy(m_szName,pszName);
	::lstrcpy(m_szCommand,pszCommand);
}


CProgramGuideTool::~CProgramGuideTool()
{
}


CProgramGuideTool &CProgramGuideTool::operator=(const CProgramGuideTool &Tool)
{
	if (&Tool!=this) {
		::lstrcpy(m_szName,Tool.m_szName);
		::lstrcpy(m_szCommand,Tool.m_szCommand);
	}
	return *this;
}


bool CProgramGuideTool::Execute(const CProgramGuideServiceInfo *pServiceInfo,
								int Program)
{
	const CProgramGuideItem *pProgram=pServiceInfo->GetProgram(Program);
	SYSTEMTIME stStart,stEnd;
	TCHAR szFileName[MAX_PATH],szCommand[2048];
	LPCTSTR p;
	LPTSTR q;

	pProgram->GetStartTime(&stStart);
	pProgram->GetEndTime(&stEnd);
	p=m_szCommand;
	GetCommandFileName(&p,szFileName);
	while (*p==' ')
		p++;
	q=szCommand;
	while (*p!='\0') {
		if (*p=='%') {
			p++;
			if (*p=='%') {
				*q++='%';
				p++;
			} else {
				TCHAR szKeyword[32];
				int i;

				for (i=0;*p!='%' && *p!='\0';i++) {
					if (i<lengthof(szKeyword)-1)
						szKeyword[i]=*p;
					p++;
				}
				if (*p=='%')
					p++;
				szKeyword[i]='\0';
				if (::lstrcmpi(szKeyword,TEXT("tvpid"))==0) {
					TCHAR sziEpgFileName[MAX_PATH+2];

					GetAppClass().GetAppDirectory(sziEpgFileName);
					::PathAppend(sziEpgFileName,TEXT("iepg.tvpid"));
					pServiceInfo->SaveiEpgFile(Program,sziEpgFileName,true);
					::lstrcpy(q,sziEpgFileName);
					q+=::lstrlen(q);
				} else if (::lstrcmpi(szKeyword,TEXT("eid"))==0) {
					q+=::wsprintf(q,TEXT("%d"),pProgram->GetEventInfo().m_EventID);
				} else if (::lstrcmpi(szKeyword,TEXT("nid"))==0) {
					q+=::wsprintf(q,TEXT("%d"),pServiceInfo->GetOriginalNID());
				} else if (::lstrcmpi(szKeyword,TEXT("tsid"))==0) {
					q+=::wsprintf(q,TEXT("%d"),pServiceInfo->GetTSID());
				} else if (::lstrcmpi(szKeyword,TEXT("sid"))==0) {
					q+=::wsprintf(q,TEXT("%d"),pServiceInfo->GetServiceID());
				} else if (::lstrcmpi(szKeyword,TEXT("start-year"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stStart.wYear);
				} else if (::lstrcmpi(szKeyword,TEXT("start-month"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stStart.wMonth);
				} else if (::lstrcmpi(szKeyword,TEXT("start-day"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stStart.wDay);
				} else if (::lstrcmpi(szKeyword,TEXT("start-hour"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stStart.wHour);
				} else if (::lstrcmpi(szKeyword,TEXT("start-minute"))==0) {
					q+=::wsprintf(q,TEXT("%02d"),stStart.wMinute);
				} else if (::lstrcmpi(szKeyword,TEXT("end-year"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stEnd.wYear);
				} else if (::lstrcmpi(szKeyword,TEXT("end-month"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stEnd.wMonth);
				} else if (::lstrcmpi(szKeyword,TEXT("end-day"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stEnd.wDay);
				} else if (::lstrcmpi(szKeyword,TEXT("end-hour"))==0) {
					q+=::wsprintf(q,TEXT("%d"),stEnd.wHour);
				} else if (::lstrcmpi(szKeyword,TEXT("end-minute"))==0) {
					q+=::wsprintf(q,TEXT("%02d"),stEnd.wMinute);
				} else if (::lstrcmpi(szKeyword,TEXT("duration-sec"))==0) {
					q+=::wsprintf(q,TEXT("%d"),pProgram->GetEventInfo().m_DurationSec);
				} else if (::lstrcmpi(szKeyword,TEXT("duration-min"))==0) {
					q+=::wsprintf(q,TEXT("%d"),(pProgram->GetEventInfo().m_DurationSec+59)/60);
				} else if (::lstrcmpi(szKeyword,TEXT("event-name"))==0) {
					q+=::wsprintf(q,TEXT("%s"),pProgram->GetEventInfo().GetEventName());
				} else if (::lstrcmpi(szKeyword,TEXT("service-name"))==0) {
					q+=::wsprintf(q,TEXT("%s"),pServiceInfo->GetServiceName());
				}
			}
		} else {
#ifndef UNICODE
			if (::IsDBCSLeadByteEx(CP_ACP,*p))
				*q++=*p++;
#endif
			*q++=*p++;
		}
	}
	*q='\0';
#ifdef DEBUG
	if (::MessageBox(NULL,szCommand,szFileName,MB_OKCANCEL)!=IDOK)
		return false;
#endif
	return ::ShellExecute(NULL,NULL,szFileName,szCommand,NULL,SW_SHOWNORMAL)>=
																(HINSTANCE)32;
}


bool CProgramGuideTool::ShowDialog(HWND hwndOwner)
{
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_PROGRAMGUIDETOOL),
							hwndOwner,DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}


CProgramGuideTool *CProgramGuideTool::GetThis(HWND hDlg)
{
	return static_cast<CProgramGuideTool*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CProgramGuideTool::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CProgramGuideTool *pThis=reinterpret_cast<CProgramGuideTool*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
								 EM_LIMITTEXT,MAX_NAME-1,0);
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,pThis->m_szName);
			::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,
								 EM_LIMITTEXT,MAX_COMMAND-1,0);
			::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,pThis->m_szCommand);
			if (pThis->m_szName[0]!='\0' && pThis->m_szCommand[0]!='\0')
				EnableDlgItem(hDlg,IDOK,true);
			AdjustDialogPos(::GetParent(hDlg),hDlg);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_PROGRAMGUIDETOOL_NAME:
		case IDC_PROGRAMGUIDETOOL_COMMAND:
			if (HIWORD(wParam)==EN_CHANGE)
				EnableDlgItem(hDlg,IDOK,
					GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)>0
					&& GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND)>0);
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szCommand[MAX_COMMAND];
				TCHAR szFileName[MAX_PATH],szDirectory[MAX_PATH];

				GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,szCommand,MAX_COMMAND);
				if (szCommand[0]!='\0') {
					LPCTSTR p;

					p=szCommand;
					GetCommandFileName(&p,szFileName);
				} else
					szFileName[0]='\0';
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("実行ファイル(*.exe;*.bat)\0*.exe;*.bat\0")
								TEXT("すべてのファイル\0*.*\0");
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=MAX_PATH;
				if (szFileName[0]!='\0') {
					CFilePath Path(szFileName);
					Path.GetDirectory(szDirectory);
					ofn.lpstrInitialDir=szDirectory;
					::lstrcpy(szFileName,Path.GetFileName());
				}
				ofn.Flags=OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::PathQuoteSpaces(szFileName);
					//::lstrcat(szFileName,TEXT(" \"%tvpid%\""));
					::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,szFileName);
					if (GetDlgItemTextLength(hDlg,IDC_PROGRAMGUIDETOOL_NAME)==0) {
						*::PathFindExtension(szFileName)='\0';
						::SetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
										 ::PathFindFileName(szFileName));
					}
				}
				return TRUE;

			case IDC_PROGRAMGUIDETOOL_PARAMETER:
				{
					static const struct {
						LPCTSTR pszParameter;
						LPCTSTR pszText;
					} ParameterList[] = {
						{TEXT("%tvpid%"),			TEXT("iEPGファイル")},
						{TEXT("%eid%"),				TEXT("イベントID")},
						{TEXT("%nid%"),				TEXT("ネットワークID")},
						{TEXT("%tsid%"),			TEXT("ストリームID")},
						{TEXT("%sid%"),				TEXT("サービスID")},
						{TEXT("%start-year%"),		TEXT("開始年")},
						{TEXT("%start-month%"),		TEXT("開始月")},
						{TEXT("%start-day%"),		TEXT("開始日")},
						{TEXT("%start-hour%"),		TEXT("開始時間")},
						{TEXT("%start-minute%"),	TEXT("開始分")},
						{TEXT("%end-year%"),		TEXT("終了年")},
						{TEXT("%end-month%"),		TEXT("終了月")},
						{TEXT("%end-day%"),			TEXT("終了日")},
						{TEXT("%end-hour%"),		TEXT("終了時間")},
						{TEXT("%end-minute%"),		TEXT("終了分")},
						{TEXT("%duration-sec%"),	TEXT("秒単位の長さ")},
						{TEXT("%duration-min%"),	TEXT("分単位の長さ")},
						{TEXT("%event-name%"),		TEXT("イベント名")},
						{TEXT("%service-name%"),	TEXT("サービス名")},
					};
					HMENU hmenu=::CreatePopupMenu();
					RECT rc;
					int Command;

					for (int i=0;i<lengthof(ParameterList);i++) {
						TCHAR szText[128];

						::wsprintf(szText,TEXT("%s (%s)"),
								   ParameterList[i].pszParameter,
								   ParameterList[i].pszText);
						::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,i+1,
									 szText);
					}
					::GetWindowRect(::GetDlgItem(hDlg,IDC_PROGRAMGUIDETOOL_PARAMETER),&rc);
					Command=::TrackPopupMenu(hmenu,TPM_RETURNCMD,
											 rc.left,rc.bottom,0,hDlg,NULL);
					::DestroyMenu(hmenu);
					if (Command>0) {
						DWORD Start,End;

						::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,EM_GETSEL,
							reinterpret_cast<LPARAM>(&Start),
							reinterpret_cast<LPARAM>(&End));
						::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,EM_REPLACESEL,
							TRUE,reinterpret_cast<LPARAM>(ParameterList[Command-1].pszParameter));
						if (End<Start)
							Start=End;
						::SendDlgItemMessage(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,EM_SETSEL,
							Start,Start+::lstrlen(ParameterList[Command-1].pszParameter));
					}
				}
				return TRUE;
			}
			return TRUE;

		case IDC_PROGRAMGUIDETOOL_HELP:
			GetAppClass().ShowHelpContent(HELP_ID_PROGRAMGUIDETOOL);
			return TRUE;

		case IDOK:
			{
				CProgramGuideTool *pThis=GetThis(hDlg);

				::GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_NAME,
								 pThis->m_szName,MAX_NAME);
				::GetDlgItemText(hDlg,IDC_PROGRAMGUIDETOOL_COMMAND,
								 pThis->m_szCommand,MAX_COMMAND);
			}
		case IDCANCEL:
			EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


LPTSTR CProgramGuideTool::GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName)
{
	LPCTSTR p;
	LPTSTR q;
	TCHAR cDelimiter;

	p=*ppszCommand;
	q=pszFileName;
	if (*p=='"') {
		cDelimiter='"';
		p++;
	} else
		cDelimiter=' ';
	while (*p!='\0' && *p!=cDelimiter) {
#ifndef UNICODE
		if (IsDBCSLeadByteEx(CP_ACP,*p))
			*q++=*p++;
#endif
		*q++=*p++;
	}
	*q='\0';
	if (*p=='"')
		p++;
	*ppszCommand=p;
	return pszFileName;
}




CProgramGuideToolList::CProgramGuideToolList()
{
	m_ppToolList=NULL;
	m_NumTools=0;
	m_ToolListLength=0;
}


CProgramGuideToolList::CProgramGuideToolList(const CProgramGuideToolList &List)
{
	m_NumTools=List.m_NumTools;
	if (m_NumTools>0) {
		m_ToolListLength=m_NumTools;
		m_ppToolList=static_cast<CProgramGuideTool**>(malloc(m_ToolListLength*sizeof(CProgramGuideTool*)));
		for (int i=0;i<m_NumTools;i++)
			m_ppToolList[i]=new CProgramGuideTool(*List.m_ppToolList[i]);
	} else {
		m_ToolListLength=0;
		m_ppToolList=NULL;
	}
}


CProgramGuideToolList::~CProgramGuideToolList()
{
	Clear();
}


CProgramGuideToolList &CProgramGuideToolList::operator=(const CProgramGuideToolList &List)
{
	Clear();
	if (List.m_NumTools>0) {
		m_NumTools=List.m_NumTools;
		m_ToolListLength=m_NumTools;
		m_ppToolList=static_cast<CProgramGuideTool**>(malloc(m_ToolListLength*sizeof(CProgramGuideTool*)));
		for (int i=0;i<m_NumTools;i++)
			m_ppToolList[i]=new CProgramGuideTool(*List.m_ppToolList[i]);
	}
	return *this;
}


void CProgramGuideToolList::Clear()
{
	if (m_ppToolList!=NULL) {
		for (int i=m_NumTools-1;i>=0;i--)
			delete m_ppToolList[i];
		free(m_ppToolList);
		m_ppToolList=NULL;
		m_NumTools=0;
		m_ToolListLength=0;
	}
}


bool CProgramGuideToolList::Add(CProgramGuideTool *pTool)
{
	if (m_NumTools==m_ToolListLength) {
		if (m_ToolListLength==0)
			m_ToolListLength=16;
		else
			m_ToolListLength*=2;
		m_ppToolList=static_cast<CProgramGuideTool**>(realloc(m_ppToolList,
								m_ToolListLength*sizeof(CProgramGuideTool*)));
	}
	m_ppToolList[m_NumTools++]=pTool;
	return true;
}


CProgramGuideTool *CProgramGuideToolList::GetTool(int Index)
{
	if (Index<0 || Index>=m_NumTools)
		return NULL;
	return m_ppToolList[Index];
}


const CProgramGuideTool *CProgramGuideToolList::GetTool(int Index) const
{
	if (Index<0 || Index>=m_NumTools)
		return NULL;
	return m_ppToolList[Index];
}
