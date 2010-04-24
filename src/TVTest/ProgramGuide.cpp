#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "ProgramGuide.h"
#include "DrawUtil.h"
#include "DialogUtil.h"
#include "Help/HelpID.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif


#define TITLE_TEXT TEXT("EPG番組表")

// 現在時刻を表す線の幅
#define CUR_TIME_LINE_WIDTH 2

// 現在時刻を更新するタイマのID
#define TIMER_ID_UPDATECURTIME	1

// ドライバメニューの位置
#define MENU_DRIVER 8

static const LPCTSTR DayText[] = {
	TEXT("今日"), TEXT("明日"), TEXT("明後日"), TEXT("明々後日"),
	TEXT("4日後"), TEXT("5日後"), TEXT("6日後")
};




class CProgramGuideItem
{
	CEventInfoData m_EventInfo;
	bool m_fNullItem;
	SYSTEMTIME m_stStartTime;
	SYSTEMTIME m_stEndTime;
	DWORD m_Duration;
	int m_TitleLines;
	int m_TextLines;
	int m_ItemPos;
	int m_ItemLines;
	LPCTSTR GetEventText() const;

public:
	CProgramGuideItem(const SYSTEMTIME *pStartTime,DWORD Duration);
	CProgramGuideItem(const CEventInfoData &Info);
	~CProgramGuideItem();
	const CEventInfoData &GetEventInfo() const { return m_EventInfo; }
	bool GetStartTime(SYSTEMTIME *pTime) const;
	bool SetStartTime(const SYSTEMTIME *pTime);
	bool GetEndTime(SYSTEMTIME *pTime) const;
	bool SetEndTime(const SYSTEMTIME *pTime);
	bool SetCommonEventItem(const CProgramGuideItem *pItem);
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
	m_EventInfo.m_fValidStartTime=true;
	m_EventInfo.m_stStartTime=*pStartTime;
	m_EventInfo.m_DurationSec=Duration;
	m_fNullItem=true;
	m_stStartTime=*pStartTime;
	m_stStartTime.wMilliseconds=0;
	m_EventInfo.GetEndTime(&m_stEndTime);
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


bool CProgramGuideItem::SetCommonEventItem(const CProgramGuideItem *pItem)
{
	if (!m_EventInfo.m_fCommonEvent)
		return false;
	if (m_EventInfo.GetEventName()==NULL) {
		m_EventInfo.SetEventName(pItem->m_EventInfo.GetEventName());
		m_TitleLines=pItem->m_TitleLines;	// とりあえず
	}
	if (m_EventInfo.m_NibbleList.size()==0)
		m_EventInfo.m_NibbleList=pItem->m_EventInfo.m_NibbleList;
	return true;
}


LPCTSTR CProgramGuideItem::GetEventText() const
{
	LPCTSTR pszEventText,p;

	pszEventText=m_EventInfo.GetEventText();
	if (pszEventText!=NULL) {
		p=pszEventText;
		while (*p!='\0') {
			if (*p<=0x20) {
				p++;
				continue;
			}
			return p;
		}
	}
	pszEventText=m_EventInfo.GetEventExtText();
	if (pszEventText!=NULL) {
		p=pszEventText;
		if (memcmp(p,TEXT("番組内容"),4*(3-sizeof(TCHAR)))==0)
			p+=4*(3-sizeof(TCHAR));
		while (*p!='\0') {
			if (*p<=0x20) {
				p++;
				continue;
			}
			return p;
		}
	}
	return pszEventText;
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
	m_TitleLines=DrawUtil::CalcWrapTextLines(hdc,szText,TitleWidth);
	LPCTSTR pszEventText=GetEventText();
	if (pszEventText!=NULL) {
		SelectFont(hdc,hfontText);
		m_TextLines=DrawUtil::CalcWrapTextLines(hdc,pszEventText,TextWidth);
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
	DrawUtil::DrawWrapText(hdc,szText,pRect,LineHeight);
}


void CProgramGuideItem::DrawText(HDC hdc,const RECT *pRect,int LineHeight)
{
	LPCTSTR pszEventText=GetEventText();
	if (pszEventText!=NULL) {
		DrawUtil::DrawWrapText(hdc,pszEventText,pRect,LineHeight);
	}
}




class CProgramGuideServiceInfo
{
	CServiceInfoData m_ServiceData;
	LPTSTR m_pszServiceName;
	CProgramGuideItem **m_ppProgramList;
	int m_NumPrograms;
	int m_ProgramListLength;
	typedef std::map<WORD,CProgramGuideItem*> EventIDMap;
	EventIDMap m_EventIDMap;
	int m_FirstItem;
	int m_LastItem;
	HBITMAP m_hbmLogo;

	void SortSub(CProgramGuideItem **ppFirst,CProgramGuideItem **ppLast);
	bool InsertProgram(int Index,CProgramGuideItem *pItem);
	void InsertNullItems(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime);
public:
	CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo);
	CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo,const CEpgServiceInfo &Info);
	~CProgramGuideServiceInfo();
	const CServiceInfoData *GetServiceInfoData() const { return &m_ServiceData; }
	WORD GetNetworkID() const { return m_ServiceData.m_OriginalNID; }
	WORD GetTSID() const { return m_ServiceData.m_TSID; }
	WORD GetServiceID() const { return m_ServiceData.m_ServiceID; }
	LPCTSTR GetServiceName() const { return m_pszServiceName; }
	int NumPrograms() const { return m_NumPrograms; }
	CProgramGuideItem *GetProgram(int Index);
	const CProgramGuideItem *GetProgram(int Index) const;
	CProgramGuideItem *GetProgramByEventID(WORD EventID);
	bool AddProgram(CProgramGuideItem *pItem);
	void ClearPrograms();
	void SortPrograms();
	void CalcLayout(HDC hdc,const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime,int LinesPerHour,
		HFONT hfontTitle,int TitleWidth,HFONT hfontText,int TextWidth);
	int GetFirstItem() const { return m_FirstItem; }
	int GetLastItem() const { return m_LastItem; }
	void SetLogo(HBITMAP hbm) { m_hbmLogo=hbm; }
	HBITMAP GetLogo() const { return m_hbmLogo; }
	bool SaveiEpgFile(int Program,LPCTSTR pszFileName,bool fVersion2) const;
};


CProgramGuideServiceInfo::CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo)
	: m_ServiceData(pChannelInfo->GetNetworkID(),
					pChannelInfo->GetTransportStreamID(),
					pChannelInfo->GetServiceID())
	, m_pszServiceName(DuplicateString(pChannelInfo->GetName()))
	, m_ppProgramList(NULL)
	, m_NumPrograms(0)
	, m_ProgramListLength(0)
	, m_FirstItem(-1)
	, m_LastItem(-1)
	, m_hbmLogo(NULL)
{
}


CProgramGuideServiceInfo::CProgramGuideServiceInfo(const CChannelInfo *pChannelInfo,const CEpgServiceInfo &Info)
	: m_ServiceData(Info.m_ServiceData)
	, m_pszServiceName(DuplicateString(pChannelInfo->GetName()))
	, m_ppProgramList(NULL)
	, m_NumPrograms(0)
	, m_ProgramListLength(0)
	, m_FirstItem(-1)
	, m_LastItem(-1)
	, m_hbmLogo(NULL)
{
}


CProgramGuideServiceInfo::~CProgramGuideServiceInfo()
{
	delete [] m_pszServiceName;
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


CProgramGuideItem *CProgramGuideServiceInfo::GetProgramByEventID(WORD EventID)
{
	EventIDMap::const_iterator itr=m_EventIDMap.find(EventID);
	if (itr==m_EventIDMap.end())
		return NULL;
	return itr->second;
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
	int Index=m_NumPrograms++;
	m_ppProgramList[Index]=pItem;
	m_EventIDMap[pItem->GetEventInfo().m_EventID]=pItem;
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
	m_EventIDMap.clear();
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
	if (m_NumPrograms>1) {
		SortSub(&m_ppProgramList[0],&m_ppProgramList[m_NumPrograms-1]);

		for (int i=0;i<m_NumPrograms;i++)
			m_EventIDMap[m_ppProgramList[i]->GetEventInfo().m_EventID]=m_ppProgramList[i];
	}
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
	m_EventIDMap[pItem->GetEventInfo().m_EventID]=pItem;
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
		pPrevItem=NULL;
		stPrev=*pFirstTime;
		for (i=FirstItem;i<LastItem;i++) {
			pItem=m_ppProgramList[i];
			pItem->GetStartTime(&stStart);
			int Cmp=CompareSystemTime(&stPrev,&stStart);
			if (Cmp>0) {
				if (pPrevItem)
					pPrevItem->SetEndTime(&stStart);
			} else if (Cmp<0) {
				LONGLONG Diff=DiffSystemTime(&stPrev,&stStart);

				if (Diff<60*1000) {
					if (pPrevItem)
						pPrevItem->SetEndTime(&stStart);
				} else {
					InsertProgram(i,new CProgramGuideItem(&stPrev,(DWORD)(Diff/1000)));
					i++;
					LastItem++;
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
		if (CompareSystemTime(&stFirst,pLastTime)>=0)
			break;
		stLast=stFirst;
		OffsetSystemTime(&stLast,60*60*1000);
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
		if (ProgramsPerHour>0) {
			int Lines=LinesPerHour,Offset=0;

			m_ppProgramList[i]->GetStartTime(&stStart);
			if (CompareSystemTime(&stStart,&stFirst)>0) {
				Offset=(int)(DiffSystemTime(&stFirst,&stStart)*LinesPerHour/(60*60*1000));
				Lines-=Offset;
			}
			if (Lines>ProgramsPerHour) {
				m_ppProgramList[i+ProgramsPerHour-1]->GetEndTime(&stEnd);
				if (CompareSystemTime(&stEnd,&stLast)<0) {
					Lines-=(int)(DiffSystemTime(&stEnd,&stLast)*LinesPerHour/(60*60*1000));
					if (Lines<ProgramsPerHour)
						Lines=ProgramsPerHour;
				}
			}
			if (ProgramsPerHour==1) {
				pItem=m_ppProgramList[i];
				pItem->SetItemLines(pItem->GetItemLines()+Lines);
				if (pItem->GetItemPos()<0)
					pItem->SetItemPos(ItemPos+Offset);
			} else {
				int *pItemLines=new int[ProgramsPerHour];

				for (j=0;j<ProgramsPerHour;j++)
					pItemLines[j]=j<Lines?1:0;
				if (Lines>ProgramsPerHour) {
					int LineCount=ProgramsPerHour;

					do {
						DWORD Time,MaxTime;
						int MaxItem;

						MaxTime=0;
						for (j=0;j<ProgramsPerHour;j++) {
							pItem=m_ppProgramList[i+j];
							pItem->GetStartTime(&stStart);
							if (CompareSystemTime(&stStart,&stFirst)<0)
								stStart=stFirst;
							pItem->GetEndTime(&stEnd);
							if (CompareSystemTime(&stEnd,&stLast)>0)
								stEnd=stLast;
							Time=(DWORD)(DiffSystemTime(&stStart,&stEnd)/pItemLines[j]);
							if (Time>MaxTime) {
								MaxTime=Time;
								MaxItem=j;
							}
						}
						if (MaxTime==0)
							break;
						pItemLines[MaxItem]++;
						LineCount++;
					} while (LineCount<Lines);
				}
				int Pos=ItemPos+Offset;
				for (j=0;j<min(ProgramsPerHour,Lines);j++) {
					pItem=m_ppProgramList[i+j];
					if (pItem->GetItemPos()<0)
						pItem->SetItemPos(Pos);
					pItem->SetItemLines(pItem->GetItemLines()+pItemLines[j]);
					Pos+=pItemLines[j];
				}
				delete [] pItemLines;
				i+=ProgramsPerHour-1;
			}
		}
		ItemPos+=LinesPerHour;
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
	if (m_pszServiceName!=NULL)
		::WideCharToMultiByte(CP_ACP,0,m_pszServiceName,-1,
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
	: m_ppServiceList(NULL)
	, m_NumServices(0)
	, m_ServiceListLength(0)
{
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


CProgramGuideServiceInfo *CProgramGuideServiceList::FindItem(WORD TransportStreamID,WORD ServiceID)
{
	for (int i=0;i<m_NumServices;i++) {
		if (m_ppServiceList[i]->GetTSID()==TransportStreamID
				&& m_ppServiceList[i]->GetServiceID()==ServiceID)
			return m_ppServiceList[i];
	}
	return NULL;
}


CProgramGuideItem *CProgramGuideServiceList::FindEvent(WORD TransportStreamID,WORD ServiceID,WORD EventID)
{
	CProgramGuideServiceInfo *pService=FindItem(TransportStreamID,ServiceID);
	if (pService==NULL)
		return NULL;
	return pService->GetProgramByEventID(EventID);
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




CProgramGuide::CEventHandler::CEventHandler()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CEventHandler::~CEventHandler()
{
}




const LPCTSTR CProgramGuide::m_pszWindowClass=APP_NAME TEXT(" Program Guide");
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
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuide::CProgramGuide()
	: m_pProgramList(NULL)
	, m_LinesPerHour(12)
	, m_hfont(NULL)
	, m_hfontTitle(NULL)
	, m_hfontTime(NULL)
	, m_LineMargin(1)
	, m_ItemWidth(140)
	, m_ItemMargin(4)
	, m_TextLeftMargin(8)
	, m_fDragScroll(false)
	, m_hDragCursor1(NULL)
	, m_hDragCursor2(NULL)
	//, m_hwndToolTip(NULL)
	, m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_EventInfoPopupHandler(this)
	, m_fShowToolTip(true)
	, m_CurrentTuningSpace(-2)
	, m_pDriverManager(NULL)
	, m_fUpdating(false)
	, m_pEventHandler(NULL)
	, m_pFrame(NULL)
	, m_pLogoManager(NULL)
	, m_WheelScrollLines(0)
	, m_ProgramSearchEventHandler(this)
{
	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FONT_DEFAULT,&lf);
	SetFont(&lf);
	m_WindowPosition.Left=0;
	m_WindowPosition.Top=0;
	m_WindowPosition.Width=640;
	m_WindowPosition.Height=480;
	m_ScrollPos.x=0;
	m_ScrollPos.y=0;
	m_szDriverFileName[0]='\0';
	m_ColorList[COLOR_BACK]=::GetSysColor(COLOR_WINDOW);
	m_ColorList[COLOR_TEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_CHANNELNAMETEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_TIMETEXT]=::GetSysColor(COLOR_WINDOWTEXT);
	m_ColorList[COLOR_TIMELINE]=m_ColorList[COLOR_TIMETEXT];
	m_ColorList[COLOR_CURTIMELINE]=RGB(255,64,0);
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
	m_ChannelNameBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_ChannelNameBackGradient.Direction=Theme::DIRECTION_VERT;
	m_ChannelNameBackGradient.Color1=::GetSysColor(COLOR_3DFACE);
	m_ChannelNameBackGradient.Color2=m_ChannelNameBackGradient.Color1;
	m_CurChannelNameBackGradient=m_ChannelNameBackGradient;
	m_TimeBarMarginGradient.Type=Theme::GRADIENT_NORMAL;
	m_TimeBarMarginGradient.Direction=Theme::DIRECTION_HORZ;
	m_TimeBarMarginGradient.Color1=::GetSysColor(COLOR_3DFACE);
	m_TimeBarMarginGradient.Color2=m_TimeBarMarginGradient.Color1;
	for (int i=0;i<TIME_BAR_BACK_COLORS;i++) {
		m_TimeBarBackGradient[i].Type=Theme::GRADIENT_NORMAL;
		m_TimeBarBackGradient[i].Direction=Theme::DIRECTION_HORZ;
		m_TimeBarBackGradient[i].Color1=m_TimeBarMarginGradient.Color1;
		m_TimeBarBackGradient[i].Color2=m_TimeBarMarginGradient.Color2;
	}
	m_EventInfoPopup.SetEventHandler(&m_EventInfoPopupHandler);
}


CProgramGuide::~CProgramGuide()
{
	if (m_hfont)
		::DeleteObject(m_hfont);
	if (m_hfontTitle)
		::DeleteObject(m_hfontTitle);
	if (m_hfontTime)
		::DeleteObject(m_hfontTime);
}


bool CProgramGuide::SetEpgProgramList(CEpgProgramList *pList)
{
	m_pProgramList=pList;
	return true;
}


void CProgramGuide::Clear()
{
	m_ServiceList.Clear();
	m_ScrollPos.x=0;
	m_ScrollPos.y=0;
	m_ChannelList.Clear();
	m_TuningSpaceList.Clear();
	m_CurrentTuningSpace=-2;
	//m_CurrentChannel.Clear();
	m_szDriverFileName[0]='\0';
	if (m_hwnd!=NULL) {
		SetCaption();
		Invalidate();
	}
}


bool CProgramGuide::UpdateProgramGuide(bool fUpdateList)
{
	if (m_hwnd!=NULL) {
		if (m_pFrame!=NULL)
			m_pFrame->SetCaption(TITLE_TEXT TEXT(" - 番組表を作成しています..."));
		if (UpdateList(fUpdateList)) {
			CalcLayout();
			SetScrollBar();
			//SetToolTip();
			::GetLocalTime(&m_stCurTime);
			Invalidate();
		}
		SetCaption();
		if (m_pFrame!=NULL) {
			m_pFrame->OnDateChanged();
			m_pFrame->OnSpaceChanged();
		}
	}
	return true;
}


bool CProgramGuide::UpdateList(bool fUpdateList)
{
	if (m_pProgramList==NULL)
		return false;

	m_ServiceList.Clear();
	for (int i=0;i<m_ChannelList.NumChannels();i++) {
		const CChannelInfo *pChannelInfo=m_ChannelList.GetChannelInfo(i);
		if (fUpdateList)
			m_pProgramList->UpdateService(
				pChannelInfo->GetTransportStreamID(),pChannelInfo->GetServiceID());
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
			if (m_pLogoManager!=NULL) {
				HBITMAP hbmLogo=m_pLogoManager->GetAssociatedLogoBitmap(
					pService->GetNetworkID(),pService->GetServiceID(),CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo!=NULL)
					pService->SetLogo(hbmLogo);
			}
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
	const int LineHeight=m_FontHeight+m_LineMargin;
	const int CurTimePos=pRect->top+GetCurTimeLinePos();
	RECT rcItem,rc;
	COLORREF crOldTextColor;
	HFONT hfontOld;

	hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
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

			const CEventInfoData *pEventInfo=&pItem->GetEventInfo();
			if (pEventInfo->GetEventName()==NULL
					&& pEventInfo->m_fCommonEvent) {
				CProgramGuideItem *pCommonItem=m_ServiceList.FindEvent(
					pServiceInfo->GetTSID(),
					pEventInfo->m_CommonEventInfo.ServiceID,
					pEventInfo->m_CommonEventInfo.EventID);
				if (pCommonItem!=NULL) {
					pItem->SetCommonEventItem(pCommonItem);
				}
			}

			int ColorType;
			if (pEventInfo->m_NibbleList.size()>0
					&& pEventInfo->m_NibbleList[0].m_ContentNibbleLv1<=CEventInfoData::CONTENT_LAST)
				ColorType=COLOR_CONTENT_FIRST+
					pEventInfo->m_NibbleList[0].m_ContentNibbleLv1;
			else
				ColorType=COLOR_CONTENT_OTHER;
			COLORREF BackColor=m_ColorList[ColorType];
			if (pEventInfo->m_fCommonEvent)
				BackColor=MixColor(BackColor,m_ColorList[COLOR_BACK],192);
			DrawUtil::Fill(hdc,&rcItem,BackColor);
			HPEN hpen=::CreatePen(PS_SOLID,1,MixColor(BackColor,RGB(0,0,0),224));
			HPEN hpenOld=static_cast<HPEN>(::SelectObject(hdc,hpen));
			::MoveToEx(hdc,rcItem.left,rcItem.top,NULL);
			::LineTo(hdc,rcItem.right,rcItem.top);
			::SelectObject(hdc,hpenOld);
			::DeleteObject(hpen);
			if (m_Day==DAY_TODAY
					&& CurTimePos>=rcItem.top && CurTimePos<rcItem.bottom) {
				LOGBRUSH lb;

				lb.lbStyle=BS_SOLID;
				lb.lbColor=MixColor(m_ColorList[COLOR_CURTIMELINE],BackColor,64);
				hpen=::ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT,
									CUR_TIME_LINE_WIDTH,&lb,0,NULL);
				::SelectObject(hdc,hpen);
				::MoveToEx(hdc,rcItem.left,CurTimePos,NULL);
				::LineTo(hdc,rcItem.right,CurTimePos);
				::SelectObject(hdc,hpenOld);
				::DeleteObject(hpen);
			}
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
	const CProgramGuideServiceInfo *pServiceInfo=m_ServiceList.GetItem(Service);
	bool fCur=pServiceInfo->GetNetworkID()==m_CurrentChannel.NetworkID
		//&& pServiceInfo->GetTransportStreamID()==m_CurrentChannel.TransportStreamID
		&& pServiceInfo->GetServiceID()==m_CurrentChannel.ServiceID;
	const Theme::GradientInfo *pGradient=
		fCur?&m_CurChannelNameBackGradient:&m_ChannelNameBackGradient;
	RECT rc;

	rc=*pRect;
	rc.left++;
	rc.right--;
	Theme::FillGradient(hdc,&rc,pGradient);

	Theme::GradientInfo Border;
	Border.Type=pGradient->Type;
	Border.Direction=Theme::DIRECTION_VERT;
	Border.Color1=MixColor(pGradient->Color1,RGB(255,255,255),192);
	Border.Color2=MixColor(pGradient->Color2,RGB(255,255,255),192);
	rc=*pRect;
	rc.right=rc.left+1;
	Theme::FillGradient(hdc,&rc,&Border);
	Border.Type=pGradient->Type;
	Border.Direction=Theme::DIRECTION_VERT;
	Border.Color1=MixColor(pGradient->Color1,RGB(0,0,0),192);
	Border.Color2=MixColor(pGradient->Color2,RGB(0,0,0),192);
	rc=*pRect;
	rc.left=rc.right-1;
	Theme::FillGradient(hdc,&rc,&Border);

	HFONT hfontOld=SelectFont(hdc,m_hfontTitle);
	COLORREF crOldTextColor=::SetTextColor(hdc,
		m_ColorList[fCur?COLOR_CURCHANNELNAMETEXT:COLOR_CHANNELNAMETEXT]);
	rc=*pRect;
	rc.left+=4;
	rc.right-=4;

	HBITMAP hbmLogo=pServiceInfo->GetLogo();
	if (hbmLogo!=NULL) {
		int LogoWidth,LogoHeight;
		LogoHeight=min(rc.bottom-rc.top-4,14);
		LogoWidth=LogoHeight*16/9;
		DrawUtil::DrawBitmap(hdc,rc.left,rc.top+(rc.bottom-rc.top-LogoHeight)/2,
							 LogoWidth,LogoHeight,hbmLogo,NULL,192);
		rc.left+=LogoWidth+4;
	}

	::DrawText(hdc,pServiceInfo->GetServiceName(),-1,&rc,
			   (hbmLogo!=NULL?DT_LEFT:DT_CENTER) |
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	::SetTextColor(hdc,crOldTextColor);
	::SelectObject(hdc,hfontOld);
}


void CProgramGuide::DrawTimeBar(HDC hdc,const RECT *pRect,bool fRight)
{
	const int PixelsPerHour=(m_FontHeight+m_LineMargin)*m_LinesPerHour;
	const int CurTimePos=pRect->top+GetCurTimeLinePos();
	HFONT hfontOld;
	COLORREF crOldTextColor;
	HPEN hpen,hpenOld;
	RECT rc;

	hfontOld=SelectFont(hdc,m_hfontTime);
	crOldTextColor=::SetTextColor(hdc,m_ColorList[COLOR_TIMETEXT]);
	hpen=::CreatePen(PS_SOLID,0,m_ColorList[COLOR_TIMETEXT]);
	hpenOld=SelectPen(hdc,hpen);
	rc.left=pRect->left;
	rc.top=pRect->top;
	rc.right=pRect->right;
	for (int i=0;i<m_Hours;i++) {
		TCHAR szText[32];
		int Hour=(m_stFirstTime.wHour+i)%24;

		rc.bottom=rc.top+PixelsPerHour;
		Theme::FillGradient(hdc,&rc,&m_TimeBarBackGradient[Hour/3]);
		::MoveToEx(hdc,rc.left,rc.top,NULL);
		::LineTo(hdc,rc.right,rc.top);
		if (m_Day==DAY_TODAY && CurTimePos>=rc.top && CurTimePos<rc.bottom) {
			const int TriangleHeight=m_FontHeight*2/3;
			const int TriangleWidth=TriangleHeight*8/10;
			POINT ptTriangle[3];
			HBRUSH hbr,hbrOld;

			hbr=::CreateSolidBrush(m_ColorList[COLOR_CURTIMELINE]);
			hbrOld=SelectBrush(hdc,hbr);
			SelectObject(hdc,::CreatePen(PS_SOLID,1,m_ColorList[COLOR_CURTIMELINE]));
			if (fRight) {
				ptTriangle[0].x=rc.left;
				ptTriangle[0].y=CurTimePos;
				ptTriangle[1].x=ptTriangle[0].x+TriangleWidth;
				ptTriangle[1].y=ptTriangle[0].y-TriangleHeight/2;
				ptTriangle[2].x=ptTriangle[0].x+TriangleWidth;
				ptTriangle[2].y=ptTriangle[1].y+TriangleHeight;
			} else {
				ptTriangle[0].x=rc.right-1;
				ptTriangle[0].y=CurTimePos;
				ptTriangle[1].x=ptTriangle[0].x-TriangleWidth;
				ptTriangle[1].y=ptTriangle[0].y-TriangleHeight/2;
				ptTriangle[2].x=ptTriangle[0].x-TriangleWidth;
				ptTriangle[2].y=ptTriangle[1].y+TriangleHeight;
			}
			::Polygon(hdc,ptTriangle,3);
			::SelectObject(hdc,hbrOld);
			::DeleteObject(hbr);
			::DeleteObject(::SelectObject(hdc,hpen));
		}
		if (i==0 || Hour==0) {
			SYSTEMTIME st=m_stFirstTime;
			OffsetSystemTime(&st,(LONGLONG)(m_Day+(i+23)/24)*(1000LL*60*60*24));
			::wsprintf(szText,TEXT("%d/%d(%s) %d時"),
					   st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek),Hour);
		} else {
			::wsprintf(szText,TEXT("%d"),Hour);
		}
		::TextOut(hdc,rc.right-4,rc.top+4,szText,lstrlen(szText));
		rc.top=rc.bottom;
	}

	if (m_Day<DAY_LAST) {
		// ▼
		RECT rcClient;

		GetClientRect(&rcClient);
		if (rc.top-m_TimeBarWidth<rcClient.bottom) {
			const int TriangleWidth=m_FontHeight*2/3;
			const int TriangleHeight=TriangleWidth*8/10;
			POINT ptTriangle[3];
			HBRUSH hbr,hbrOld;

			hbr=::CreateSolidBrush(m_ColorList[COLOR_TIMETEXT]);
			hbrOld=SelectBrush(hdc,hbr);
			ptTriangle[0].x=m_TimeBarWidth/2;
			ptTriangle[0].y=rc.top-(m_TimeBarWidth-TriangleHeight)/2;
			ptTriangle[1].x=ptTriangle[0].x-TriangleWidth/2;
			ptTriangle[1].y=ptTriangle[0].y-TriangleHeight;
			ptTriangle[2].x=ptTriangle[0].x+TriangleWidth/2;
			ptTriangle[2].y=ptTriangle[1].y;
			::Polygon(hdc,ptTriangle,3);
			for (int i=0;i<3;i++)
				ptTriangle[i].x+=rcClient.right-m_TimeBarWidth;
			::Polygon(hdc,ptTriangle,3);
			::SelectObject(hdc,hbrOld);
			::DeleteObject(hbr);
		}
	}

	::SelectObject(hdc,hpenOld);
	::DeleteObject(hpen);
	::SetTextColor(hdc,crOldTextColor);
	SelectFont(hdc,hfontOld);
}


void CProgramGuide::Draw(HDC hdc,const RECT *pPaintRect)
{
	RECT rcClient,rcGuide,rc;
	HRGN hrgn;

	::GetClientRect(m_hwnd,&rcClient);
	GetProgramGuideRect(&rcGuide);
	if (::IntersectRect(&rc,&rcGuide,pPaintRect)) {
		HBRUSH hbr=::CreateSolidBrush(m_ColorList[COLOR_BACK]);
		::FillRect(hdc,&rc,hbr);
		::DeleteObject(hbr);
	}
	int OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	if (m_ServiceList.NumServices()>0) {
		int i;

		if (pPaintRect->top<m_ServiceNameHeight) {
			rc.left=rcClient.left+m_TimeBarWidth;
			rc.top=0;
			rc.right=rcClient.right-m_TimeBarWidth;
			rc.bottom=m_ServiceNameHeight;
			hrgn=::CreateRectRgnIndirect(&rc);
			::SelectClipRgn(hdc,hrgn);
			rc.left=m_TimeBarWidth-m_ScrollPos.x;
			for (i=0;i<m_ServiceList.NumServices();i++) {
				rc.right=rc.left+(m_ItemWidth+m_ItemMargin*2);
				if (rc.left<pPaintRect->right && rc.right>pPaintRect->left)
					DrawServiceName(i,hdc,&rc);
				rc.left=rc.right;
			}
			if (rc.left<pPaintRect->right) {
				rc.right=pPaintRect->right;
				Theme::FillGradient(hdc,&rc,&m_ChannelNameBackGradient);
			}
			::SelectClipRgn(hdc,NULL);
			::DeleteObject(hrgn);
		}
		hrgn=::CreateRectRgnIndirect(&rcGuide);
		::SelectClipRgn(hdc,hrgn);
		rc.top=m_ServiceNameHeight-m_ScrollPos.y*(m_FontHeight+m_LineMargin);
		rc.left=m_TimeBarWidth+m_ItemMargin-m_ScrollPos.x;
		HPEN hpen,hpenOld;
		hpen=::CreatePen(PS_SOLID,0,m_ColorList[COLOR_TIMELINE]);
		hpenOld=SelectPen(hdc,hpen);
		int PixelsPerHour=(m_FontHeight+m_LineMargin)*m_LinesPerHour;
		int CurTimePos=rc.top+GetCurTimeLinePos();
		for (i=0;i<m_ServiceList.NumServices();i++) {
			rc.right=rc.left+m_ItemWidth;
			if (rc.top<pPaintRect->bottom) {
				for (int j=0;j<m_Hours;j++) {
					int y=rc.top+j*PixelsPerHour;
					if (y>=pPaintRect->top && y<pPaintRect->bottom) {
						/*
						::MoveToEx(hdc,rc.left-m_ItemMargin,y,NULL);
						::LineTo(hdc,rc.left,y);
						::MoveToEx(hdc,rc.right,y,NULL);
						::LineTo(hdc,rc.right+m_ItemMargin,y);
						*/
						::MoveToEx(hdc,rc.left-m_ItemMargin,y,NULL);
						::LineTo(hdc,rc.right+m_ItemMargin,y);
					}
					if (m_Day==DAY_TODAY
							&& CurTimePos>=y && CurTimePos<y+PixelsPerHour) {
						HPEN hpenCurTime;
						LOGBRUSH lb;

						lb.lbStyle=BS_SOLID;
						lb.lbColor=m_ColorList[COLOR_CURTIMELINE];
						hpenCurTime=::ExtCreatePen(
							PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT,
							CUR_TIME_LINE_WIDTH,&lb,0,NULL);
						::SelectObject(hdc,hpenCurTime);
						::MoveToEx(hdc,rc.left-m_ItemMargin,CurTimePos,NULL);
						::LineTo(hdc,rc.right+m_ItemMargin,CurTimePos);
						::SelectObject(hdc,hpen);
						::DeleteObject(hpenCurTime);
					}
				}
				if (rc.left<pPaintRect->right && rc.right>pPaintRect->left)
					DrawProgramList(i,hdc,&rc,pPaintRect);
			}
			rc.left=rc.right+m_ItemMargin*2;
		}
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hpen);
		::SelectClipRgn(hdc,NULL);
		::DeleteObject(hrgn);
	} else {
		if (pPaintRect->top<m_ServiceNameHeight) {
			rc.left=max(pPaintRect->left,m_TimeBarWidth);
			rc.right=min(pPaintRect->right,rcClient.right-m_TimeBarWidth);
			if (rc.left<rc.right) {
				rc.top=0;
				rc.bottom=m_ServiceNameHeight;
				Theme::FillGradient(hdc,&rc,&m_ChannelNameBackGradient);
			}
		}
	}

	rc.left=0;
	rc.top=m_ServiceNameHeight;
	rc.right=rcClient.right;
	rc.bottom=rcClient.bottom;
	hrgn=::CreateRectRgnIndirect(&rc);
	::SelectClipRgn(hdc,hrgn);
	rc.top=m_ServiceNameHeight-m_ScrollPos.y*(m_FontHeight+m_LineMargin);
		rc.bottom=rc.top+(m_FontHeight+m_LineMargin)*m_LinesPerHour*m_Hours;
	if (pPaintRect->left<m_TimeBarWidth) {
		rc.left=0;
		rc.right=m_TimeBarWidth;
		DrawTimeBar(hdc,&rc,false);
	}
	rc.left=rcClient.right-m_TimeBarWidth;
	if (rc.left<pPaintRect->right) {
		rc.right=rcClient.right;
		DrawTimeBar(hdc,&rc,true);
	}
	::SelectClipRgn(hdc,NULL);
	::DeleteObject(hrgn);

	if (rc.bottom<pPaintRect->bottom) {
		::SetRect(&rc,0,rc.bottom,m_TimeBarWidth,rcClient.bottom);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
		::OffsetRect(&rc,rcGuide.right,0);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
	}
	if (pPaintRect->top<m_ServiceNameHeight) {
		::SetRect(&rc,0,0,m_TimeBarWidth,m_ServiceNameHeight);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
		::OffsetRect(&rc,rcGuide.right,0);
		Theme::FillGradient(hdc,&rc,&m_TimeBarMarginGradient);
	}

	if (m_Day!=DAY_TODAY
			&& pPaintRect->top<m_ServiceNameHeight) {
		// ▲
		const int TriangleWidth=m_FontHeight*2/3;
		const int TriangleHeight=TriangleWidth*8/10;
		POINT ptTriangle[3];
		HPEN hpen,hpenOld;
		HBRUSH hbr,hbrOld;

		hbr=::CreateSolidBrush(m_ColorList[COLOR_TIMETEXT]);
		hpen=::CreatePen(PS_SOLID,0,m_ColorList[COLOR_TIMETEXT]);
		hbrOld=SelectBrush(hdc,hbr);
		hpenOld=SelectPen(hdc,hpen);
		ptTriangle[0].x=m_TimeBarWidth/2;
		ptTriangle[0].y=(m_ServiceNameHeight-TriangleHeight)/2;
		ptTriangle[1].x=ptTriangle[0].x-TriangleWidth/2;
		ptTriangle[1].y=ptTriangle[0].y+TriangleHeight;
		ptTriangle[2].x=ptTriangle[0].x+TriangleWidth/2;
		ptTriangle[2].y=ptTriangle[1].y;
		::Polygon(hdc,ptTriangle,3);
		for (int i=0;i<3;i++)
			ptTriangle[i].x+=rcClient.right-m_TimeBarWidth;
		::Polygon(hdc,ptTriangle,3);
		::SelectObject(hdc,hbrOld);
		::SelectObject(hdc,hpenOld);
		::DeleteObject(hbr);
		::DeleteObject(hpen);
	}

	::SetBkMode(hdc,OldBkMode);
}


int CProgramGuide::GetCurTimeLinePos() const
{
	return (int)(DiffSystemTime(&m_stFirstTime,&m_stCurTime)*
				 ((m_FontHeight+m_LineMargin)*m_LinesPerHour)/(60*60*1000));
}


void CProgramGuide::GetProgramGuideRect(RECT *pRect) const
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
	if (abs(YScrollSize)<rc.bottom-rc.top && abs(XScrollSize)<rc.right-rc.left) {
		::ScrollWindowEx(m_hwnd,XScrollSize,YScrollSize,&rc,&rc,NULL,NULL,SW_INVALIDATE);
		if (XScrollSize!=0) {
			RECT rcChannel;

			::SetRect(&rcChannel,rc.left,0,rc.right,rc.top);
			::ScrollWindowEx(m_hwnd,XScrollSize,0,&rcChannel,&rcChannel,NULL,NULL,SW_INVALIDATE);
		}
		if (YScrollSize!=0) {
			RECT rcTime;

			::SetRect(&rcTime,0,rc.top,rc.left,rc.bottom);
			::ScrollWindowEx(m_hwnd,0,YScrollSize,&rcTime,&rcTime,NULL,NULL,SW_INVALIDATE);
			rcTime.left=rc.right;
			rcTime.right=rcTime.left+m_TimeBarWidth;
			::ScrollWindowEx(m_hwnd,0,YScrollSize,&rcTime,&rcTime,NULL,NULL,SW_INVALIDATE);
		}
	} else {
		Invalidate();
	}
	m_ScrollPos=Pos;
	/*
	if (XScroll!=0)
		SetToolTip();
	*/
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


void CProgramGuide::SetCaption()
{
	if (m_hwnd!=NULL && m_pFrame!=NULL) {
		if (m_pProgramList!=NULL) {
			if (m_fUpdating) {
				m_pFrame->SetCaption(TITLE_TEXT TEXT(" - 番組表の取得中..."));
			} else {
				TCHAR szText[256];
				SYSTEMTIME stFirst=m_stFirstTime,stLast=m_stLastTime;

				OffsetSystemTime(&stLast,-60*60*1000);
				if (m_Day!=DAY_TODAY) {
					LONGLONG Offset=(LONGLONG)m_Day*(24*60*60*1000);

					OffsetSystemTime(&stFirst,Offset);
					OffsetSystemTime(&stLast,Offset);
				}
				::wsprintf(szText,TITLE_TEXT TEXT(" - %s %d/%d(%s) %d時 ～ %d/%d(%s) %d時"),
						   DayText[m_Day],stFirst.wMonth,stFirst.wDay,
						   GetDayOfWeekText(stFirst.wDayOfWeek),stFirst.wHour,
						   stLast.wMonth,stLast.wDay,
						   GetDayOfWeekText(stLast.wDayOfWeek),stLast.wHour);
				m_pFrame->SetCaption(szText);
			}
		} else {
			m_pFrame->SetCaption(TITLE_TEXT);
		}
	}
}


bool CProgramGuide::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,NULL,m_hinst);
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


void CProgramGuide::SetCurrentService(WORD NetworkID,WORD TSID,WORD ServiceID)
{
	m_CurrentChannel.NetworkID=NetworkID;
	m_CurrentChannel.TransportStreamID=TSID;
	m_CurrentChannel.ServiceID=ServiceID;
	if (m_hwnd!=NULL) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=m_ServiceNameHeight;
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


bool CProgramGuide::GetTuningSpaceName(int Space,LPTSTR pszName,int MaxName) const
{
	if (MaxName<1)
		return false;
	if (Space==-1) {
		::lstrcpyn(pszName,TEXT("すべてのチャンネル"),MaxName);
	} else {
		if (Space<0 || Space>=m_TuningSpaceList.NumSpaces()) {
			pszName[0]='\0';
			return false;
		}
		LPCTSTR p=m_TuningSpaceList.GetTuningSpaceName(Space);
		if (p!=NULL) {
			::lstrcpyn(pszName,p,MaxName);
		} else {
			::wnsprintf(pszName,MaxName-1,TEXT("チューニング空間 %d"),Space+1);
			pszName[MaxName-1]='\0';
		}
	}
	return true;
}


bool CProgramGuide::EnumDriver(int *pIndex,LPTSTR pszName,int MaxName) const
{
	if (m_pDriverManager==NULL)
		return false;
	(*pIndex)++;
	if (*pIndex<0 || *pIndex>=m_pDriverManager->NumDrivers())
		return false;
	const CDriverInfo *pDriverInfo=m_pDriverManager->GetDriverInfo(*pIndex);
	if (pDriverInfo==NULL)
		return false;
	if (CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName()))
		return EnumDriver(pIndex,pszName,MaxName);
	::lstrcpyn(pszName,pDriverInfo->GetFileName(),MaxName);
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


bool CProgramGuide::GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	if (pFirstTime!=NULL)
		*pFirstTime=m_stFirstTime;
	if (pLastTime!=NULL)
		*pLastTime=m_stLastTime;
	return true;
}


bool CProgramGuide::GetCurrentTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const
{
	SYSTEMTIME stFirst=m_stFirstTime,stLast=m_stLastTime;

	if (m_Day!=DAY_TODAY) {
		LONGLONG Offset=(LONGLONG)m_Day*(24*60*60*1000);

		OffsetSystemTime(&stFirst,Offset);
		OffsetSystemTime(&stLast,Offset);
	}
	if (pFirstTime!=NULL)
		*pFirstTime=stFirst;
	if (pLastTime!=NULL)
		*pLastTime=stLast;
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
			//SetToolTip();
			Invalidate();
		}
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


void CProgramGuide::SetBackColors(const Theme::GradientInfo *pChannelBackGradient,
								  const Theme::GradientInfo *pCurChannelBackGradient,
								  const Theme::GradientInfo *pTimeBarMarginGradient,
								  const Theme::GradientInfo *pTimeBarBackGradient)
{
	m_ChannelNameBackGradient=*pChannelBackGradient;
	m_CurChannelNameBackGradient=*pCurChannelBackGradient;
	m_TimeBarMarginGradient=*pTimeBarMarginGradient;
	for (int i=0;i<TIME_BAR_BACK_COLORS;i++)
		m_TimeBarBackGradient[i]=pTimeBarBackGradient[i];
	if (m_hwnd!=NULL)
		Invalidate();
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
		::SelectObject(hdc,hfontOld);
		::DeleteDC(hdc);
		//m_FontHeight=tm.tmHeight-tm.tmInternalLeading;
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
		//SetToolTip();
		Invalidate();
	}
	return true;
}


bool CProgramGuide::SetEventInfoFont(const LOGFONT *pFont)
{
	return m_EventInfoPopup.SetFont(pFont);
}


bool CProgramGuide::SetShowToolTip(bool fShow)
{
	if (m_fShowToolTip!=fShow) {
		m_fShowToolTip=fShow;
		m_EventInfoPopupManager.SetEnable(fShow);
		/*
		if (m_hwndToolTip!=NULL)
			::SendMessage(m_hwndToolTip,TTM_ACTIVATE,fShow,0);
		*/
	}
	return true;
}


bool CProgramGuide::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler)
		m_pEventHandler->m_pProgramGuide=NULL;
	if (pEventHandler)
		pEventHandler->m_pProgramGuide=this;
	m_pEventHandler=pEventHandler;
	return true;
}


bool CProgramGuide::SetFrame(CFrame *pFrame)
{
	if (m_pFrame)
		m_pFrame->m_pProgramGuide=NULL;
	if (pFrame)
		pFrame->m_pProgramGuide=this;
	m_pFrame=pFrame;
	return true;
}


bool CProgramGuide::SetLogoManager(CLogoManager *pLogoManager)
{
	m_pLogoManager=pLogoManager;
	return true;
}


bool CProgramGuide::SetDragScroll(bool fDragScroll)
{
	if (m_fDragScroll!=fDragScroll) {
		m_fDragScroll=fDragScroll;
		if (m_hwnd!=NULL) {
			POINT pt;

			::GetCursorPos(&pt);
			SendMessage(WM_SETCURSOR,(WPARAM)m_hwnd,
						MAKELPARAM(SendMessage(WM_NCHITTEST,0,MAKELPARAM(pt.x,pt.y)),WM_MOUSEMOVE));
		}
	}
	return true;
}


bool CProgramGuide::OnClose()
{
	if (m_pEventHandler!=NULL && !m_pEventHandler->OnClose())
		return false;
	if (m_fUpdating) {
		if (m_pEventHandler)
			m_pEventHandler->OnEndUpdate();
		m_fUpdating=false;
	}
	return true;
}


/*
void CProgramGuide::SetToolTip()
{
	if (m_hwndToolTip!=NULL) {
		int NumTools=::SendMessage(m_hwndToolTip,TTM_GETTOOLCOUNT,0,0);
		int Columns=m_ServiceList.NumServices();
		TOOLINFO ti;

		ti.cbSize=TTTOOLINFOA_V2_SIZE;
		ti.hwnd=m_hwnd;
		if (NumTools<Columns) {
			ti.uFlags=TTF_SUBCLASS;
			ti.hinst=NULL;
			ti.lpszText=LPSTR_TEXTCALLBACK;
			::SetRect(&ti.rect,0,0,0,0);
			for (int i=NumTools;i<Columns;i++) {
				ti.uId=i;
				ti.lParam=i;
				::SendMessage(m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
			}
		} else if (NumTools>Columns) {
			for (int i=Columns;i<NumTools;i++) {
				ti.uId=i;
				::SendMessage(m_hwndToolTip,TTM_DELTOOL,0,(LPARAM)&ti);
			}
		}
		GetProgramGuideRect(&ti.rect);
		ti.rect.left=-m_ScrollPos.x+m_ItemMargin;
		ti.uId=0;
		for (int i=0;i<Columns;i++) {
			ti.rect.right=ti.rect.left+m_ItemWidth;
			::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,(LPARAM)&ti);
			ti.uId++;
			ti.rect.left=ti.rect.right+m_ItemMargin;
		}
	}
}
*/


bool CProgramGuide::HitTest(int x,int y,int *pServiceIndex,int *pProgramIndex)
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetProgramGuideRect(&rc);
	if (::PtInRect(&rc,pt)) {
		const int XPos=x-rc.left+m_ScrollPos.x;
		const int ServiceWidth=m_ItemWidth+m_ItemMargin*2;
		int Service;

		if (XPos%ServiceWidth<m_ItemMargin
				|| XPos%ServiceWidth>=m_ItemMargin+m_ItemWidth)
			return false;
		Service=XPos/ServiceWidth;
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
			CProgramGuide *pThis=static_cast<CProgramGuide*>(OnCreate(hwnd,lParam));

			if (pThis->m_hDragCursor1==NULL)
				pThis->m_hDragCursor1=::LoadCursor(m_hinst,MAKEINTRESOURCE(IDC_GRAB1));
			if (pThis->m_hDragCursor2==NULL)
				pThis->m_hDragCursor2=::LoadCursor(m_hinst,MAKEINTRESOURCE(IDC_GRAB2));

			/*
			pThis->m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,TOOLTIPS_CLASS,NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,0,0,0,0,
				hwnd,NULL,m_hinst,NULL);
			::SendMessage(pThis->m_hwndToolTip,TTM_SETMAXTIPWIDTH,0,320);
			::SendMessage(pThis->m_hwndToolTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,30000);
			::SendMessage(pThis->m_hwndToolTip,TTM_ACTIVATE,pThis->m_fShowToolTip,0);
			*/
			pThis->m_EventInfoPopupManager.Initialize(hwnd,&pThis->m_EventInfoPopupHandler);
			::GetLocalTime(&pThis->m_stCurTime);
			::SetTimer(hwnd,TIMER_ID_UPDATECURTIME,1000,NULL);
		}
		return 0;

	case WM_PAINT:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			pThis->Draw(ps.hdc,&ps.rcPaint);
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
			pThis->SetScrollBar();
			if (Pos.x!=pThis->m_ScrollPos.x || Pos.y!=pThis->m_ScrollPos.y)
				pThis->Scroll(Pos.x-pThis->m_ScrollPos.x,Pos.y-pThis->m_ScrollPos.y);
		}
		return 0;

	case WM_VSCROLL:
	case WM_MOUSEWHEEL:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			int Pos;
			RECT rc;
			int Page;
			int TotalLines=pThis->m_Hours*pThis->m_LinesPerHour;

			Pos=pThis->m_ScrollPos.y;
			pThis->GetProgramGuideRect(&rc);
			Page=(rc.bottom-rc.top)/(pThis->m_FontHeight+pThis->m_LineMargin);
			if (uMsg==WM_VSCROLL) {
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
			} else {
				int Lines;

				if (pThis->m_WheelScrollLines==0) {
					UINT SysLines;

					if (::SystemParametersInfo(SPI_GETWHEELSCROLLLINES,0,&SysLines,0))
						Lines=SysLines;
					else
						Lines=2;
				} else {
					Lines=pThis->m_WheelScrollLines;
				}
				Pos-=GET_WHEEL_DELTA_WPARAM(wParam)*Lines/WHEEL_DELTA;
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
	case WM_MOUSEHWHEEL:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			int Pos;
			RECT rc;
			int Page;
			int TotalWidth=pThis->m_ServiceList.NumServices()*(pThis->m_ItemWidth+pThis->m_ItemMargin*2);

			Pos=pThis->m_ScrollPos.x;
			pThis->GetProgramGuideRect(&rc);
			Page=rc.right-rc.left;
			if (uMsg==WM_HSCROLL) {
				switch (LOWORD(wParam)) {
				case SB_LINELEFT:	Pos-=pThis->m_FontHeight;	break;
				case SB_LINERIGHT:	Pos+=pThis->m_FontHeight;	break;
				case SB_PAGELEFT:	Pos-=Page;					break;
				case SB_PAGERIGHT:	Pos+=Page;					break;
				case SB_THUMBTRACK:	Pos=HIWORD(wParam);			break;
				case SB_LEFT:		Pos=0;						break;
				case SB_RIGHT:		Pos=max(TotalWidth-Page,0);	break;
				default:	return 0;
				}
			} else {
				Pos+=GET_WHEEL_DELTA_WPARAM(wParam)*pThis->m_FontHeight/WHEEL_DELTA;
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>max(TotalWidth-Page,0))
				Pos=max(TotalWidth-Page,0);
			if (Pos!=pThis->m_ScrollPos.x)
				pThis->Scroll(Pos-pThis->m_ScrollPos.x,0);
		}
		return uMsg==WM_MOUSEHWHEEL;

	case WM_LBUTTONDOWN:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			POINT pt;
			RECT rc;

			::SetFocus(hwnd);
			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::GetClientRect(hwnd,&rc);
			if (pt.y<pThis->m_ServiceNameHeight
					&& pt.x>=pThis->m_TimeBarWidth
					&& pt.x<rc.right-pThis->m_TimeBarWidth
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
			} else if (pt.x<pThis->m_TimeBarWidth
					|| pt.x>=rc.right-pThis->m_TimeBarWidth) {
				if (pThis->m_Day>DAY_TODAY
						&& pt.y<pThis->m_ServiceNameHeight) {
					::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_TODAY+(pThis->m_Day-1),0);
				} else if (pThis->m_Day<DAY_LAST) {
					int y=(pThis->m_Hours*pThis->m_LinesPerHour-pThis->m_ScrollPos.y)*
						(pThis->m_FontHeight+pThis->m_LineMargin);
					if (pt.y-pThis->m_ServiceNameHeight>=y-pThis->m_TimeBarWidth
							&& pt.y-pThis->m_ServiceNameHeight<y) {
						::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_TODAY+(pThis->m_Day+1),0);
					}
				}
			} else if (pThis->m_fDragScroll) {
				pThis->m_DragInfo.StartCursorPos=pt;
				pThis->m_DragInfo.StartScrollPos=pThis->m_ScrollPos;
				::SetCursor(pThis->m_hDragCursor2);
				::SetCapture(hwnd);
			} else {
				CProgramGuide *pThis=GetThis(hwnd);

				pThis->m_EventInfoPopupManager.Popup(pt.x,pt.y);
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (::GetCapture()==hwnd) {
			CProgramGuide *pThis=GetThis(hwnd);

			::ReleaseCapture();
			::SetCursor(pThis->m_hDragCursor1);
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CProgramGuide *pThis=GetThis(hwnd);
			POINT pt;
			HMENU hmenu,hmenuPopup;
			TCHAR szText[256],szMenu[64];

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			pThis->m_CurItem.fValid=
				pThis->HitTest(pt.x,pt.y,&pThis->m_CurItem.Service,&pThis->m_CurItem.Program);
			::SetFocus(hwnd);
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
				::wsprintf(szText+::lstrlen(szText),TEXT(" %d/%d(%s) %d時～"),
						   st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek),st.wHour);
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
				pThis->GetTuningSpaceName(i,szText,lengthof(szText));
				CopyToMenuText(szText,szMenu,lengthof(szMenu));
				::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
							 CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+i,szMenu);
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
					int i=-1;
					while (pThis->EnumDriver(&i,szText,lengthof(szText))) {
						CopyToMenuText(szText,szMenu,lengthof(szMenu));
						::AppendMenu(hmenuDriver,MFT_STRING | MFS_ENABLED,
									 CM_PROGRAMGUIDE_DRIVER_FIRST+i,szMenu);
						if (::lstrcmpi(pThis->m_szDriverFileName,szText)==0)
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
			::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_ALWAYSONTOP,
				MF_BYCOMMAND | (pThis->m_pFrame->GetAlwaysOnTop()?MFS_CHECKED:MFS_UNCHECKED));
			::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_DRAGSCROLL,
				MF_BYCOMMAND | (pThis->m_fDragScroll?MFS_CHECKED:MFS_UNCHECKED));
			::CheckMenuItem(hmenu,CM_PROGRAMGUIDE_POPUPEVENTINFO,
				MF_BYCOMMAND | (pThis->m_fShowToolTip?MFS_CHECKED:MFS_UNCHECKED));
			::EnableMenuItem(hmenu,CM_PROGRAMGUIDE_IEPGASSOCIATE,
				MF_BYCOMMAND | (pThis->m_CurItem.fValid?MFS_ENABLED:MFS_GRAYED));
			if (pThis->m_ToolList.NumTools()>0) {
				::AppendMenu(hmenuPopup,MFT_SEPARATOR | MFS_ENABLED,0,NULL);
				for (int i=0;i<pThis->m_ToolList.NumTools();i++) {
					const CProgramGuideTool *pTool=pThis->m_ToolList.GetTool(i);

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

	case WM_MOUSEMOVE:
		if (::GetCapture()==hwnd) {
			CProgramGuide *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int XScroll,YScroll;

			XScroll=(pThis->m_DragInfo.StartScrollPos.x+(pThis->m_DragInfo.StartCursorPos.x-x))-pThis->m_ScrollPos.x;
			YScroll=(pThis->m_DragInfo.StartScrollPos.y+(pThis->m_DragInfo.StartCursorPos.y-y)/(pThis->m_FontHeight+pThis->m_LineMargin))-pThis->m_ScrollPos.y;
			if (XScroll!=0 || YScroll!=0)
				pThis->Scroll(XScroll,YScroll);
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
			} else if (pt.x<pThis->m_TimeBarWidth
					|| pt.x>=rc.right-pThis->m_TimeBarWidth) {
				if (pThis->m_Day>DAY_TODAY
						&& pt.y<pThis->m_ServiceNameHeight) {
					::SetCursor(::LoadCursor(NULL,IDC_HAND));
					return TRUE;
				}
				int y=(pThis->m_Hours*pThis->m_LinesPerHour-pThis->m_ScrollPos.y)*
					(pThis->m_FontHeight+pThis->m_LineMargin);
				if (pThis->m_Day<DAY_LAST
						&& pt.y-pThis->m_ServiceNameHeight>=y-pThis->m_TimeBarWidth
						&& pt.y-pThis->m_ServiceNameHeight<y) {
					::SetCursor(::LoadCursor(NULL,IDC_HAND));
					return TRUE;
				}
			} else if (pThis->m_fDragScroll) {
				::SetCursor(pThis->m_hDragCursor1);
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
					&& pThis->m_pEventHandler->OnKeyDown((UINT)wParam,(UINT)lParam))
				return 0;
		}
		break;

	case WM_TIMER:
		if (wParam==TIMER_ID_UPDATECURTIME) {
			CProgramGuide *pThis=GetThis(hwnd);

			if (pThis->m_Day==DAY_TODAY) {
				SYSTEMTIME st;

				::GetLocalTime(&st);
				if (pThis->m_stCurTime.wMinute!=st.wMinute
						|| pThis->m_stCurTime.wHour!=st.wHour
						|| pThis->m_stCurTime.wDay!=st.wDay
						|| pThis->m_stCurTime.wMonth!=st.wMonth
						|| pThis->m_stCurTime.wYear!=st.wYear) {
					int OldTimeLinePos=pThis->GetCurTimeLinePos(),NewTimeLinePos;

					pThis->m_stCurTime=st;
					NewTimeLinePos=pThis->GetCurTimeLinePos();
					if (NewTimeLinePos!=OldTimeLinePos) {
						RECT rc;
						int Offset;

						::GetClientRect(hwnd,&rc);
						Offset=pThis->m_ServiceNameHeight-
							pThis->m_ScrollPos.y*(pThis->m_FontHeight+pThis->m_LineMargin);
						rc.top=Offset+OldTimeLinePos-pThis->m_FontHeight/2;
						rc.bottom=Offset+NewTimeLinePos+pThis->m_FontHeight/2;
						::InvalidateRect(hwnd,&rc,FALSE);
					}
				}
			}
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case CM_PROGRAMGUIDE_UPDATE:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (!pThis->m_fUpdating) {
					if (pThis->m_pEventHandler!=NULL
							&& pThis->m_pEventHandler->OnBeginUpdate()) {
						pThis->m_fUpdating=true;
						pThis->SetCaption();
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
					pThis->SetCaption();
				}
			}
			return 0;

		case CM_PROGRAMGUIDE_REFRESH:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_pEventHandler==NULL
						|| pThis->m_pEventHandler->OnRefresh()) {
					HCURSOR hcurOld;

					hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
					pThis->UpdateProgramGuide(true);
					::SetCursor(hcurOld);
				}
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
						HCURSOR hcurOld=::SetCursor(::LoadCursor(NULL,IDC_WAIT));
						pThis->CalcLayout();
						pThis->m_ScrollPos.y=0;
						pThis->SetScrollBar();
						pThis->SetCaption();
						//pThis->SetToolTip();
						::GetLocalTime(&pThis->m_stCurTime);
						pThis->Invalidate();
						::SetCursor(hcurOld);
						if (pThis->m_pFrame!=NULL)
							pThis->m_pFrame->OnDateChanged();
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

		case CM_PROGRAMGUIDE_SEARCH:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (!pThis->m_ProgramSearch.IsCreated()) {
					RECT rc;

					pThis->m_ProgramSearch.SetEventHandler(&pThis->m_ProgramSearchEventHandler);
					pThis->m_ProgramSearch.GetPosition(&rc);
					if (rc.left==rc.right || rc.top==rc.bottom) {
						POINT pt={0,0};
						::ClientToScreen(hwnd,&pt);
						pThis->m_ProgramSearch.SetPosition(pt.x,pt.y,0,0);
					}
					pThis->m_ProgramSearch.Create(hwnd);
				} else {
					pThis->m_ProgramSearch.Destroy();
				}
			}
			return 0;

		case CM_PROGRAMGUIDE_ALWAYSONTOP:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_pFrame!=NULL)
					pThis->m_pFrame->SetAlwaysOnTop(!pThis->m_pFrame->GetAlwaysOnTop());
			}
			return 0;

		case CM_PROGRAMGUIDE_DRAGSCROLL:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				pThis->SetDragScroll(!pThis->m_fDragScroll);
			}
			return 0;

		case CM_PROGRAMGUIDE_POPUPEVENTINFO:
			{
				CProgramGuide *pThis=GetThis(hwnd);

				pThis->SetShowToolTip(!pThis->m_fShowToolTip);
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
				return 0;
			}
			if (LOWORD(wParam)>=CM_PROGRAMGUIDE_DRIVER_FIRST
					&& LOWORD(wParam)<=CM_PROGRAMGUIDE_DRIVER_LAST) {
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_fUpdating)
					::SendMessage(hwnd,WM_COMMAND,CM_PROGRAMGUIDE_ENDUPDATE,0);
				if (pThis->m_pDriverManager!=NULL) {
					const CDriverInfo *pDriverInfo=pThis->m_pDriverManager->GetDriverInfo(LOWORD(wParam)-CM_PROGRAMGUIDE_DRIVER_FIRST);

					if (pDriverInfo!=NULL) {
						CDriverInfo DriverInfo(pDriverInfo->GetFileName());

						if (DriverInfo.LoadTuningSpaceList()) {
							pThis->SetTuningSpaceList(DriverInfo.GetFileName(),
												  DriverInfo.GetTuningSpaceList(),-1);
							pThis->UpdateProgramGuide();
						}
					}
				}
				return 0;
			}
			if (LOWORD(wParam)>=CM_PROGRAMGUIDETOOL_FIRST
					&& LOWORD(wParam)<=CM_PROGRAMGUIDETOOL_LAST) {
				CProgramGuide *pThis=GetThis(hwnd);

				if (pThis->m_CurItem.fValid) {
					CProgramGuideServiceInfo *pServiceInfo=
						pThis->m_ServiceList.GetItem(pThis->m_CurItem.Service);

					if (pServiceInfo!=NULL) {
						CProgramGuideTool *pTool=pThis->m_ToolList.GetTool(LOWORD(wParam)-CM_PROGRAMGUIDETOOL_FIRST);

						pTool->Execute(pServiceInfo,pThis->m_CurItem.Program,
									   ::GetAncestor(hwnd,GA_ROOT));
					}
				}
				return 0;
			}
		}
		return 0;

	case WM_DESTROY:
		{
			CProgramGuide *pThis=GetThis(hwnd);

			pThis->m_ProgramSearch.Destroy();
			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnDestroy();
			//pThis->m_hwndToolTip=NULL;
			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


CProgramGuide::CFrame::CFrame()
	: m_pProgramGuide(NULL)
{
}


CProgramGuide::CFrame::~CFrame()
{
}


CProgramGuide::CEventInfoPopupHandler::CEventInfoPopupHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CEventInfoPopupHandler::HitTest(int x,int y,LPARAM *pParam)
{
	/*if (m_pProgramGuide->m_fShowToolTip)*/ {
		int Service,Program;

		if (m_pProgramGuide->HitTest(x,y,&Service,&Program)) {
			*pParam=MAKELONG(Service,Program);
			return true;
		}
	}
	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	int Service=LOWORD(Param),Program=HIWORD(Param);
	const CProgramGuideServiceInfo *pServiceInfo=m_pProgramGuide->m_ServiceList.GetItem(Service);
	if (pServiceInfo!=NULL) {
		const CProgramGuideItem *pItem=pServiceInfo->GetProgram(Program);
		if (pItem!=NULL) {
			const CEventInfoData *pEventInfo=&pItem->GetEventInfo();
			/*
			if (pEventInfo->GetEventName()==NULL && pEventInfo->m_fCommonEvent) {
				const CProgramGuideItem *pCommonItem=m_pProgramGuide->m_ServiceList.FindEvent(
					pServiceInfo->GetTSID(),
					pEventInfo->m_CommonEventInfo.ServiceID,
					pEventInfo->m_CommonEventInfo.EventID);
				if (pCommonItem!=NULL)
					pEventInfo=&pCommonItem->GetEventInfo();
			}
			*/
			*ppInfo=pEventInfo;
			return true;
		}
	}
	return false;
}


bool CProgramGuide::CEventInfoPopupHandler::OnShow(const CEventInfoData *pInfo)
{
	COLORREF Color;
	int Red,Green,Blue;
	Theme::GradientInfo BackGradient;

	if (pInfo->m_NibbleList.size()>0
			&& pInfo->m_NibbleList[0].m_ContentNibbleLv1<=CEventInfoData::CONTENT_LAST)
		Color=m_pProgramGuide->m_ColorList[COLOR_CONTENT_FIRST+pInfo->m_NibbleList[0].m_ContentNibbleLv1];
	else
		Color=m_pProgramGuide->m_ColorList[COLOR_CONTENT_OTHER];
	BackGradient.Type=Theme::GRADIENT_NORMAL;
	BackGradient.Direction=Theme::DIRECTION_VERT;
	Red=GetRValue(Color);
	Green=GetGValue(Color);
	Blue=GetBValue(Color);
	BackGradient.Color1=RGB(Red+(255-Red)/2,Green+(255-Green)/2,Blue+(255-Blue)/2);
	BackGradient.Color2=Color;
	CEventInfoPopupManager::CEventHandler::m_pPopup->SetTitleColor(&BackGradient,m_pProgramGuide->m_ColorList[COLOR_TEXT]);
	return true;
}


bool CProgramGuide::CEventInfoPopupHandler::OnMenuPopup(HMENU hmenu)
{
	::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
	::AppendMenu(hmenu,MFT_STRING | (CEventInfoPopup::CEventHandler::m_pPopup->IsSelected()?MFS_ENABLED:MFS_GRAYED),
				 COMMAND_FIRST,TEXT("選択文字列を検索(&S)"));
	return true;
}


void CProgramGuide::CEventInfoPopupHandler::OnMenuSelected(int Command)
{
	LPTSTR pszText=CEventInfoPopup::CEventHandler::m_pPopup->GetSelectedText();
	if (pszText!=NULL) {
		if (!m_pProgramGuide->m_ProgramSearch.IsCreated())
			m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_SEARCH,0);
		m_pProgramGuide->m_ProgramSearch.Search(pszText);
		delete [] pszText;
	}
}




CProgramGuide::CProgramSearchEventHandler::CProgramSearchEventHandler(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
{
}


bool CProgramGuide::CProgramSearchEventHandler::Search(LPCTSTR pszKeyword)
{
	for (int i=0;i<m_pProgramGuide->m_ServiceList.NumServices();i++) {
		const CProgramGuideServiceInfo *pServiceInfo=m_pProgramGuide->m_ServiceList.GetItem(i);

		for (int j=0;j<pServiceInfo->NumPrograms();j++) {
			const CEventInfoData &EventInfo=pServiceInfo->GetProgram(j)->GetEventInfo();

			if (MatchKeyword(&EventInfo,pszKeyword))
				AddSearchResult(&EventInfo,pServiceInfo->GetServiceName());
		}
	}
	return true;
}




#include "Menu.h"

enum {
	STATUS_ITEM_TUNER,
	STATUS_ITEM_DATE,
	STATUS_ITEM_DATEPREV,
	STATUS_ITEM_DATENEXT
};


class CProgramGuideTunerStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

public:
	CProgramGuideTunerStatusItem::CProgramGuideTunerStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_TUNER,160)
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetName() const { return TEXT("チューナー"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		TCHAR szText[256];

		m_pProgramGuide->GetTuningSpaceName(m_pProgramGuide->GetCurrentTuningSpace(),szText,lengthof(szText));
		DrawText(hdc,pRect,szText);
	}

	void OnFocus(bool fFocus) {
		if (fFocus) {
			int i;
			TCHAR szText[256];

			m_Menu.Clear();
			for (i=-1;m_pProgramGuide->GetTuningSpaceName(i,szText,lengthof(szText));i++)
				m_Menu.AppendItem(CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+i,szText);
			m_Menu.AppendSeparator();
			i=-1;
			while (m_pProgramGuide->EnumDriver(&i,szText,lengthof(szText)))
				m_Menu.AppendItem(CM_PROGRAMGUIDE_DRIVER_FIRST+i,szText);
			RECT rc;
			POINT pt;
			GetRect(&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_pStatus->GetHandle(),&pt);
			m_Menu.Show(GetParent(m_pStatus->GetHandle()),m_pProgramGuide->GetHandle(),&pt,
						CM_PROGRAMGUIDE_TUNINGSPACE_FIRST+m_pProgramGuide->GetCurrentTuningSpace());
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc,pt))
				m_Menu.Hide();
		}
	}
};

class CDateStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;
	CDropDownMenu m_Menu;

public:
	CDateStatusItem::CDateStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATE,160)
		, m_pProgramGuide(pProgramGuide)
	{
		for (int i=CProgramGuide::DAY_TODAY;i<=CProgramGuide::DAY_LAST;i++)
			m_Menu.AppendItem(CM_PROGRAMGUIDE_TODAY+i,NULL);
	}

	LPCTSTR GetName() const { return TEXT("日付"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		SYSTEMTIME stFirst;
		TCHAR szText[256];

		m_pProgramGuide->GetCurrentTimeRange(&stFirst,NULL);
		::wsprintf(szText,TEXT("%s %d/%d(%s) %d時～"),
				   DayText[m_pProgramGuide->GetViewDay()],
				   stFirst.wMonth,stFirst.wDay,
				   GetDayOfWeekText(stFirst.wDayOfWeek),stFirst.wHour);
		DrawText(hdc,pRect,szText);
	}

	void OnFocus(bool fFocus) {
		if (fFocus) {
			SYSTEMTIME st;
			TCHAR szText[256];

			m_pProgramGuide->GetTimeRange(&st,NULL);
			for (int i=CProgramGuide::DAY_TODAY;i<=CProgramGuide::DAY_LAST;i++) {
				::wsprintf(szText,TEXT("%s %d/%d(%s) %d時～"),
						   DayText[i],
						   st.wMonth,st.wDay,GetDayOfWeekText(st.wDayOfWeek),st.wHour);
				m_Menu.SetItemText(CM_PROGRAMGUIDE_TODAY+i,szText);
				OffsetSystemTime(&st,24*60*60*1000);
			}
			RECT rc;
			POINT pt;
			GetRect(&rc);
			pt.x=rc.left;
			pt.y=rc.bottom;
			::ClientToScreen(m_pStatus->GetHandle(),&pt);
			m_Menu.Show(GetParent(m_pStatus->GetHandle()),m_pProgramGuide->GetHandle(),&pt,
						CM_PROGRAMGUIDE_TODAY+m_pProgramGuide->GetViewDay());
		} else {
			POINT pt;
			RECT rc;

			::GetCursorPos(&pt);
			if (!m_Menu.GetPosition(&rc) || !::PtInRect(&rc,pt))
				m_Menu.Hide();
		}
	}
};

class CDatePrevStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;

public:
	CDatePrevStatusItem::CDatePrevStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATEPREV,16)
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetName() const { return TEXT("前の日へ"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		bool fEnabled=m_pProgramGuide->GetViewDay()>CProgramGuide::DAY_TODAY;
		COLORREF OldTextColor;

		if (!fEnabled)
			OldTextColor=::SetTextColor(hdc,MixColor(::GetTextColor(hdc),GetBkColor(hdc)));
		DrawText(hdc,pRect,TEXT("▲"),DRAWTEXT_HCENTER);
		if (!fEnabled)
			::SetTextColor(hdc,OldTextColor);
	}

	void OnLButtonDown(int x,int y)
	{
		int Day=m_pProgramGuide->GetViewDay();
		if (Day>CProgramGuide::DAY_TODAY)
			m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_TODAY+Day-1,0);
	}
};

class CDateNextStatusItem : public CStatusItem
{
	CProgramGuide *m_pProgramGuide;

public:
	CDateNextStatusItem::CDateNextStatusItem(CProgramGuide *pProgramGuide)
		: CStatusItem(STATUS_ITEM_DATENEXT,16)
		, m_pProgramGuide(pProgramGuide)
	{
	}

	LPCTSTR GetName() const { return TEXT("次の日へ"); }

	void Draw(HDC hdc,const RECT *pRect)
	{
		bool fEnabled=m_pProgramGuide->GetViewDay()<CProgramGuide::DAY_LAST;
		COLORREF OldTextColor;

		if (!fEnabled)
			OldTextColor=::SetTextColor(hdc,MixColor(::GetTextColor(hdc),GetBkColor(hdc)));
		DrawText(hdc,pRect,TEXT("▼"),DRAWTEXT_HCENTER);
		if (!fEnabled)
			::SetTextColor(hdc,OldTextColor);
	}

	void OnLButtonDown(int x,int y)
	{
		int Day=m_pProgramGuide->GetViewDay();
		if (Day<CProgramGuide::DAY_LAST)
			m_pProgramGuide->SendMessage(WM_COMMAND,CM_PROGRAMGUIDE_TODAY+Day+1,0);
	}
};




const LPCTSTR CProgramGuideFrame::m_pszWindowClass=APP_NAME TEXT(" Program Guide Frame");
HINSTANCE CProgramGuideFrame::m_hinst=NULL;


bool CProgramGuideFrame::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=::LoadIcon(hinst,MAKEINTRESOURCE(IDI_PROGRAMGUIDE));
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=::CreateSolidBrush(0xFF000000);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuideFrame::CProgramGuideFrame(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
	, m_fAlwaysOnTop(false)
{
	m_StatusMargin.cx=6;
	m_StatusMargin.cy=5;
	m_StatusView[0].AddItem(new CProgramGuideTunerStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CDateStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CDatePrevStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CDateNextStatusItem(m_pProgramGuide));
}


CProgramGuideFrame::~CProgramGuideFrame()
{
}


bool CProgramGuideFrame::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	if (m_WindowPosition.fMaximized)
		Style|=WS_MAXIMIZE;
	if (m_fAlwaysOnTop)
		ExStyle|=WS_EX_TOPMOST;
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,TITLE_TEXT,m_hinst);
}


void CProgramGuideFrame::SetStatusColor(
	const Theme::GradientInfo *pBackGradient,COLORREF crText,
	const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightText)
{
	for (int i=0;i<lengthof(m_StatusView);i++)
		m_StatusView[i].SetColor(pBackGradient,crText,pHighlightBackGradient,crHighlightText);
}


void CProgramGuideFrame::SetStatusBorderType(Theme::BorderType Type)
{
	for (int i=0;i<lengthof(m_StatusView);i++)
		m_StatusView[i].SetBorderType(Type);
}


bool CProgramGuideFrame::SetAlwaysOnTop(bool fTop)
{
	if (m_fAlwaysOnTop!=fTop) {
		m_fAlwaysOnTop=fTop;
		if (m_hwnd!=NULL)
			::SetWindowPos(m_hwnd,fTop?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
	}
	return true;
}


void CProgramGuideFrame::SetCaption(LPCTSTR pszFileName)
{
	::SetWindowText(m_hwnd,pszFileName);
}


void CProgramGuideFrame::OnDateChanged()
{
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATE);
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATEPREV);
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATENEXT);
}


void CProgramGuideFrame::OnSpaceChanged()
{
	m_StatusView[0].UpdateItem(STATUS_ITEM_TUNER);
}


CProgramGuideFrame *CProgramGuideFrame::GetThis(HWND hwnd)
{
	return static_cast<CProgramGuideFrame*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CProgramGuideFrame::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CProgramGuideFrame *pThis=static_cast<CProgramGuideFrame*>(OnCreate(hwnd,lParam));

			pThis->m_pProgramGuide->SetFrame(pThis);
			pThis->m_pProgramGuide->Create(hwnd,WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL);
			for (int i=0;i<lengthof(pThis->m_StatusView);i++) {
				pThis->m_StatusView[i].Create(hwnd,WS_CHILD | WS_VISIBLE);
				pThis->m_StatusView[i].EnableBufferedPaint(true);
			}
		}
		return 0;

	case WM_SHOWWINDOW:
		if (!wParam)
			break;
#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED 0x031E
#endif
	case WM_DWMCOMPOSITIONCHANGED:
		{
			CProgramGuideFrame *pThis=GetThis(hwnd);
			RECT rc={0,0,0,0};

			rc.top=pThis->m_StatusView[0].GetHeight()+pThis->m_StatusMargin.cy;
			pThis->m_AeroGlass.ApplyAeroGlass(hwnd,&rc);
		}
		return 0;

	case WM_ERASEBKGND:
		{
			CProgramGuideFrame *pThis=GetThis(hwnd);

			if (!pThis->m_AeroGlass.IsEnabled()) {
				RECT rc;
				::GetClientRect(hwnd,&rc);
				::FillRect(reinterpret_cast<HDC>(wParam),&rc,
						   reinterpret_cast<HBRUSH>(COLOR_3DFACE+1));
				return 1;
			}
		}
		break;

	case WM_SIZE:
		{
			CProgramGuideFrame *pThis=GetThis(hwnd);
			int Width=LOWORD(lParam),Height=HIWORD(lParam);

			int x=0;
			for (int i=0;i<lengthof(pThis->m_StatusView);i++) {
				int w=pThis->m_StatusView[i].GetIntegralWidth();
				pThis->m_StatusView[i].SetPosition(x,0,w,pThis->m_StatusView[i].GetHeight());
				x+=w+pThis->m_StatusMargin.cx;
			}
			int y=pThis->m_StatusView[0].GetHeight()+pThis->m_StatusMargin.cy;
			pThis->m_pProgramGuide->SetPosition(0,y,Width,Height-y);
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			::SendMessage(hwnd,WM_CLOSE,0,0);
			return 0;
		}
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			CProgramGuideFrame *pThis=GetThis(hwnd);

			pThis->m_pProgramGuide->SendMessage(uMsg,wParam,lParam);
		}
		return uMsg==WM_MOUSEHWHEEL;

	case WM_CLOSE:
		{
			CProgramGuideFrame *pThis=GetThis(hwnd);

			if (!pThis->m_pProgramGuide->OnClose())
				return 0;
		}
		break;

	case WM_DESTROY:
		{
			CProgramGuideFrame *pThis=GetThis(hwnd);

			pThis->m_pProgramGuide->SetFrame(NULL);
			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




const LPCTSTR CProgramGuideDisplay::m_pszWindowClass=APP_NAME TEXT(" Program Guide Display");
HINSTANCE CProgramGuideDisplay::m_hinst=NULL;


bool CProgramGuideDisplay::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=::CreateSolidBrush(RGB(0,0,0));
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramGuideDisplay::CProgramGuideDisplay(CProgramGuide *pProgramGuide)
	: m_pProgramGuide(pProgramGuide)
	, m_pEventHandler(NULL)
{
	m_StatusMargin.cx=6;
	m_StatusMargin.cy=5;
	m_StatusView[0].AddItem(new CProgramGuideTunerStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CDateStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CDatePrevStatusItem(m_pProgramGuide));
	m_StatusView[1].AddItem(new CDateNextStatusItem(m_pProgramGuide));
}


CProgramGuideDisplay::~CProgramGuideDisplay()
{
}


bool CProgramGuideDisplay::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszWindowClass,NULL,m_hinst);
}


void CProgramGuideDisplay::SetEventHandler(CEventHandler *pHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pProgramGuideDisplay=NULL;
	if (pHandler!=NULL)
		pHandler->m_pProgramGuideDisplay=this;
	m_pEventHandler=pHandler;
}


void CProgramGuideDisplay::SetStatusColor(
	const Theme::GradientInfo *pBackGradient,COLORREF crText,
	const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightText)
{
	for (int i=0;i<lengthof(m_StatusView);i++)
		m_StatusView[i].SetColor(pBackGradient,crText,pHighlightBackGradient,crHighlightText);
}


void CProgramGuideDisplay::SetStatusBorderType(Theme::BorderType Type)
{
	for (int i=0;i<lengthof(m_StatusView);i++)
		m_StatusView[i].SetBorderType(Type);
}


bool CProgramGuideDisplay::OnVisibleChange(bool fVisible)
{
	if (!fVisible && m_pEventHandler!=NULL)
		return m_pEventHandler->OnHide();
	return true;
}


void CProgramGuideDisplay::OnDateChanged()
{
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATE);
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATEPREV);
	m_StatusView[1].UpdateItem(STATUS_ITEM_DATENEXT);
}


void CProgramGuideDisplay::OnSpaceChanged()
{
	m_StatusView[0].UpdateItem(STATUS_ITEM_TUNER);
}


bool CProgramGuideDisplay::SetAlwaysOnTop(bool fTop)
{
	if (m_pEventHandler==NULL)
		return false;
	return m_pEventHandler->SetAlwaysOnTop(fTop);
}


bool CProgramGuideDisplay::GetAlwaysOnTop() const
{
	if (m_pEventHandler==NULL)
		return false;
	return m_pEventHandler->GetAlwaysOnTop();
}


CProgramGuideDisplay *CProgramGuideDisplay::GetThis(HWND hwnd)
{
	return static_cast<CProgramGuideDisplay*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CProgramGuideDisplay::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CProgramGuideDisplay *pThis=static_cast<CProgramGuideDisplay*>(OnCreate(hwnd,lParam));

			pThis->m_pProgramGuide->SetFrame(pThis);
			pThis->m_pProgramGuide->Create(hwnd,WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL);
			for (int i=0;i<lengthof(pThis->m_StatusView);i++) {
				pThis->m_StatusView[i].Create(hwnd,WS_CHILD | WS_VISIBLE);
			}
		}
		return 0;

	case WM_SIZE:
		{
			CProgramGuideDisplay *pThis=GetThis(hwnd);
			int Width=LOWORD(lParam),Height=HIWORD(lParam);

			int x=pThis->m_StatusMargin.cx;
			int y=pThis->m_StatusMargin.cy;
			for (int i=0;i<lengthof(pThis->m_StatusView);i++) {
				int w=pThis->m_StatusView[i].GetIntegralWidth();
				pThis->m_StatusView[i].SetPosition(x,y,w,pThis->m_StatusView[i].GetHeight());
				x+=w+pThis->m_StatusMargin.cx;
			}
			y+=pThis->m_StatusView[0].GetHeight()+pThis->m_StatusMargin.cy;
			pThis->m_pProgramGuide->SetPosition(0,y,Width,Height-y);
		}
		return 0;

	case WM_PAINT:
		{
			CProgramGuideDisplay *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			::BeginPaint(hwnd,&ps);
			pThis->DrawCloseButton(ps.hdc);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CProgramGuideDisplay *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

			if (pThis->CloseButtonHitTest(x,y))
				pThis->SetVisible(false);
		}
		return 0;

	case WM_LBUTTONDBLCLK:
		{
			CProgramGuideDisplay *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnLButtonDoubleClick(
					GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CProgramGuideDisplay *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnRButtonDown(
					GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam==VK_ESCAPE) {
			CProgramGuideDisplay *pThis=GetThis(hwnd);

			pThis->SetVisible(false);
			return 0;
		}
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		{
			CProgramGuideDisplay *pThis=GetThis(hwnd);

			pThis->m_pProgramGuide->SendMessage(uMsg,wParam,lParam);
		}
		return uMsg==WM_MOUSEHWHEEL;

	case WM_DESTROY:
		{
			CProgramGuideDisplay *pThis=GetThis(hwnd);

			pThis->m_pProgramGuide->SetFrame(NULL);
			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


CProgramGuideDisplay::CEventHandler::CEventHandler()
	: m_pProgramGuideDisplay(NULL)
{
}


CProgramGuideDisplay::CEventHandler::~CEventHandler()
{
}




CProgramGuideTool::CProgramGuideTool()
	: m_hIcon(NULL)
{
	m_szName[0]='\0';
	m_szCommand[0]='\0';
}


CProgramGuideTool::CProgramGuideTool(const CProgramGuideTool &Tool)
	: m_hIcon(NULL)
{
	*this=Tool;
}


CProgramGuideTool::CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand)
	: m_hIcon(NULL)
{
	::lstrcpy(m_szName,pszName);
	::lstrcpy(m_szCommand,pszCommand);
}


CProgramGuideTool::~CProgramGuideTool()
{
	if (m_hIcon!=NULL)
		::DestroyIcon(m_hIcon);
}


CProgramGuideTool &CProgramGuideTool::operator=(const CProgramGuideTool &Tool)
{
	if (&Tool!=this) {
		::lstrcpy(m_szName,Tool.m_szName);
		::lstrcpy(m_szCommand,Tool.m_szCommand);
		if (m_hIcon!=NULL)
			::DestroyIcon(m_hIcon);
		if (Tool.m_hIcon!=NULL)
			m_hIcon=::CopyIcon(Tool.m_hIcon);
		else
			m_hIcon=NULL;
	}
	return *this;
}


bool CProgramGuideTool::GetPath(LPTSTR pszPath,int MaxLength) const
{
	LPCTSTR p=m_szCommand;

	return GetCommandFileName(&p,pszPath,MaxLength);
}


HICON CProgramGuideTool::GetIcon()
{
	if (m_hIcon==NULL && m_szCommand[0]!='\0') {
		TCHAR szFileName[MAX_PATH];
		LPCTSTR p=m_szCommand;

		if (GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
			SHFILEINFO shfi;
			if (::SHGetFileInfo(szFileName,0,&shfi,sizeof(shfi),
								SHGFI_ICON | SHGFI_SMALLICON))
				m_hIcon=shfi.hIcon;
		}
	}
	return m_hIcon;
}


bool CProgramGuideTool::Execute(const CProgramGuideServiceInfo *pServiceInfo,
								int Program,HWND hwnd)
{
	const CProgramGuideItem *pProgram=pServiceInfo->GetProgram(Program);
	SYSTEMTIME stStart,stEnd;
	TCHAR szFileName[MAX_PATH],szCommand[2048];
	LPCTSTR p;
	LPTSTR q;

	pProgram->GetStartTime(&stStart);
	pProgram->GetEndTime(&stEnd);
	p=m_szCommand;
	if (!GetCommandFileName(&p,szFileName,lengthof(szFileName))) {
		::MessageBox(hwnd,TEXT("パスが長すぎます。"),NULL,MB_OK | MB_ICONEXCLAMATION);
		return false;
	}
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
					q+=::wsprintf(q,TEXT("%d"),pServiceInfo->GetNetworkID());
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
#ifdef _DEBUG
	if (::MessageBox(hwnd,szCommand,szFileName,MB_OKCANCEL)!=IDOK)
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


INT_PTR CALLBACK CProgramGuideTool::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
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
					GetCommandFileName(&p,szFileName,lengthof(szFileName));
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
				if (pThis->m_hIcon!=NULL) {
					::DestroyIcon(pThis->m_hIcon);
					pThis->m_hIcon=NULL;
				}
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


bool CProgramGuideTool::GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName,int MaxFileName)
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
	for (int i=0;*p!='\0' && *p!=cDelimiter;i++) {
#ifndef UNICODE
		if (IsDBCSLeadByteEx(CP_ACP,*p) && *(p+1)!='\0') {
			*q++=*p++;
			i++;
		}
#endif
		if (i>=MaxFileName-1) {
			pszFileName[0]='\0';
			return false;
		}
		*q++=*p++;
	}
	*q='\0';
	if (*p=='"')
		p++;
	*ppszCommand=p;
	return true;
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
