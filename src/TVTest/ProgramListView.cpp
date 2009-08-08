#include "stdafx.h"
#include "TVTest.h"
#include "ProgramListView.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define PROGRAM_LIST_WINDOW_CLASS APP_NAME TEXT(" Program List")

#define TEXT_LEFT_MARGIN 8




class CProgramItemInfo {
	CEventInfoData m_EventInfo;
	WORD m_EventID;
	int m_NameLines;
	int m_TextLines;
	int CalcStringLines(HDC hdc,LPCTSTR pszText,int Width);
	void DrawString(HDC hdc,LPCTSTR pszText,const RECT *pRect,int LineHeight);
public:
	CProgramItemInfo(const CEventInfoData &EventInfo);
	//~CProgramItemInfo();
	const CEventInfoData &GetEventInfo() const { return m_EventInfo; }
	WORD GetEventID() const { return m_EventID; }
	int GetTitleLines() const { return m_NameLines; }
	int GetTextLines() const { return m_TextLines; }
	int GetLines() const { return m_NameLines+m_TextLines; }
	int CalcLines(HDC hdc,int TitleWidth,int TextWidth);
	void DrawTitle(HDC hdc,const RECT *pRect,int LineHeight);
	void DrawText(HDC hdc,const RECT *pRect,int LineHeight);
	bool IsChanged(const CProgramItemInfo *pItem) const;
};


CProgramItemInfo::CProgramItemInfo(const CEventInfoData &EventInfo)
{
	m_EventInfo=EventInfo;
	m_EventID=EventInfo.m_EventID;
	m_NameLines=0;
	m_TextLines=0;
}


int CProgramItemInfo::CalcStringLines(HDC hdc,LPCTSTR pszText,int Width)
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


int CProgramItemInfo::CalcLines(HDC hdc,int TitleWidth,int TextWidth)
{
	TCHAR szText[2048];

	::wsprintf(szText,TEXT("%02d:%02d"),m_EventInfo.m_stStartTime.wHour,
										m_EventInfo.m_stStartTime.wMinute);
	if (m_EventInfo.GetEventName()) {
		::wsprintf(szText+lstrlen(szText),TEXT(" %s"),m_EventInfo.GetEventName());
	}
	m_NameLines=CalcStringLines(hdc,szText,TitleWidth);
	if (m_EventInfo.GetEventText())
		m_TextLines=CalcStringLines(hdc,m_EventInfo.GetEventText(),TextWidth);
	else
		m_TextLines=0;
	return m_NameLines+m_TextLines;
}


void CProgramItemInfo::DrawString(HDC hdc,LPCTSTR pszText,const RECT *pRect,
																int LineHeight)
{
	LPCTSTR p;
	int y;
	int Length;
	int Fit;
	SIZE sz;

	p=pszText;
	y=pRect->top;
	while (*p!='\0') {
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


void CProgramItemInfo::DrawTitle(HDC hdc,const RECT *pRect,int LineHeight)
{
	TCHAR szText[2048];

	::wsprintf(szText,TEXT("%02d:%02d"),m_EventInfo.m_stStartTime.wHour,
											m_EventInfo.m_stStartTime.wMinute);
	if (m_EventInfo.GetEventName()) {
		::wsprintf(szText+lstrlen(szText),TEXT(" %s"),m_EventInfo.GetEventName());
	}
	DrawString(hdc,szText,pRect,LineHeight);
}


void CProgramItemInfo::DrawText(HDC hdc,const RECT *pRect,int LineHeight)
{
	if (m_EventInfo.GetEventText()) {
		DrawString(hdc,m_EventInfo.GetEventText(),pRect,LineHeight);
	}
}


bool CProgramItemInfo::IsChanged(const CProgramItemInfo *pItem) const
{
	return m_EventID!=pItem->m_EventID
		|| memcmp(&m_EventInfo.m_stStartTime,&pItem->m_EventInfo.m_stStartTime,sizeof(SYSTEMTIME))!=0
		|| m_EventInfo.m_DurationSec!=pItem->m_EventInfo.m_DurationSec;
}




CProgramItemList::CProgramItemList()
{
	m_NumItems=0;
	m_ppItemList=NULL;
	m_ItemListLength=0;
}


CProgramItemList::~CProgramItemList()
{
	Clear();
}


CProgramItemInfo *CProgramItemList::GetItem(int Index)
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ppItemList[Index];
}


const CProgramItemInfo *CProgramItemList::GetItem(int Index) const
{
	if (Index<0 || Index>=m_NumItems)
		return NULL;
	return m_ppItemList[Index];
}


bool CProgramItemList::Add(CProgramItemInfo *pItem)
{
	if (m_NumItems==m_ItemListLength)
		return false;
	m_ppItemList[m_NumItems++]=pItem;
	return true;
}


void CProgramItemList::Clear()
{
	if (m_ppItemList!=NULL) {
		int i;

		for (i=0;i<m_NumItems;i++)
			delete m_ppItemList[i];
		delete [] m_ppItemList;
		m_ppItemList=NULL;
		m_NumItems=0;
		m_ItemListLength=0;
	}
}


void CProgramItemList::SortSub(CProgramItemInfo **ppFirst,CProgramItemInfo **ppLast)
{
	SYSTEMTIME stKey=ppFirst[(ppLast-ppFirst)/2]->GetEventInfo().m_stStartTime;
	CProgramItemInfo **p,**q;

	p=ppFirst;
	q=ppLast;
	while (p<=q) {
		while (CompareSystemTime(&(*p)->GetEventInfo().m_stStartTime,&stKey)<0)
			p++;
		while (CompareSystemTime(&(*q)->GetEventInfo().m_stStartTime,&stKey)>0)
			q--;
		if (p<=q) {
			CProgramItemInfo *pTemp;

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


void CProgramItemList::Sort()
{
	if (m_NumItems>1)
		SortSub(&m_ppItemList[0],&m_ppItemList[m_NumItems-1]);
}


void CProgramItemList::Reserve(int NumItems)
{
	Clear();
	m_ppItemList=new CProgramItemInfo*[NumItems];
	m_ItemListLength=NumItems;
}


void CProgramItemList::Attach(CProgramItemList *pList)
{
	Clear();
	m_NumItems=pList->m_NumItems;
	m_ppItemList=pList->m_ppItemList;
	m_ItemListLength=pList->m_ItemListLength;
	pList->m_NumItems=0;
	pList->m_ppItemList=NULL;
	pList->m_ItemListLength=0;
}




HINSTANCE CProgramListView::m_hinst=NULL;


bool CProgramListView::Initialize(HINSTANCE hinst)
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
		wc.lpszClassName=PROGRAM_LIST_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CProgramListView::CProgramListView()
{
	LOGFONT lf;

	m_pProgramList=NULL;
	m_hwnd=NULL;
	GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	m_hfont=CreateFontIndirect(&lf);
	//m_FontHeight=abs(lf.lfHeight);
	m_FontHeight=0;
	m_LineMargin=1;
	m_EventBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_EventBackGradient.Direction=Theme::DIRECTION_VERT;
	m_EventBackGradient.Color1=RGB(0,0,0);
	m_EventBackGradient.Color2=RGB(0,0,0);
	m_EventTextColor=RGB(255,255,255);
	m_CurEventBackGradient=m_EventBackGradient;
	m_CurEventTextColor=m_EventTextColor;
	m_TitleBackGradient.Type=Theme::GRADIENT_NORMAL;
	m_TitleBackGradient.Direction=Theme::DIRECTION_VERT;
	m_TitleBackGradient.Color1=RGB(128,128,128);
	m_TitleBackGradient.Color2=RGB(128,128,128);
	m_TitleTextColor=RGB(255,255,255);
	m_CurTitleBackGradient=m_TitleBackGradient;
	m_CurTitleTextColor=m_TitleTextColor;
	m_MarginColor=RGB(0,0,0);
	m_CurEventID=-1;
	m_ScrollPos=0;
}


CProgramListView::~CProgramListView()
{
	::DeleteObject(m_hfont);
}


bool CProgramListView::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 PROGRAM_LIST_WINDOW_CLASS,TEXT("”Ô‘g•\"),m_hinst);
}


bool CProgramListView::UpdateProgramList(WORD TransportStreamID,WORD ServiceID)
{
	if (m_pProgramList==NULL)
		return false;
	m_pProgramList->UpdateProgramList(TransportStreamID,ServiceID);
	if (m_hwnd!=NULL) {
		if (UpdateListInfo(TransportStreamID,ServiceID)) {
			CalcDimentions();
			SetScrollBar();
			Invalidate();
		}
	}
	return true;
}


bool CProgramListView::OnProgramListChanged()
{
	/*
	if (m_hwnd!=NULL) {
		if (UpdateListInfo(ServiceID)) {
			CalcDimentions();
			SetScrollBar();
			Invalidate();
		}
	}
	*/
	return true;
}


bool CProgramListView::UpdateListInfo(WORD TransportStreamID,WORD ServiceID)
{
	if (m_pProgramList==NULL)
		return false;

	CEpgServiceInfo *pServiceInfo;
	CEventInfoList::EventIterator itrEvent;
	int NumEvents;
	int i,j;
	CProgramItemList NewItemList;
	SYSTEMTIME stFirst,stLast;
	bool fChanged;

	pServiceInfo=m_pProgramList->GetServiceInfo(TransportStreamID,ServiceID);
	if (pServiceInfo==NULL)
		return false;
	NumEvents=pServiceInfo->m_EventList.EventDataMap.size();
	if (NumEvents==0) {
		if (m_ItemList.NumItems()>0) {
			m_ItemList.Clear();
			return true;
		}
		return false;
	}
	NewItemList.Reserve(NumEvents);
	::GetLocalTime(&stFirst);
	stFirst.wSecond=0;
	stFirst.wMilliseconds=0;
	stLast=stFirst;
	OffsetSystemTime(&stLast,24*60*60*1000);
	fChanged=false;
	i=0;
	for (itrEvent=pServiceInfo->m_EventList.EventDataMap.begin();
			itrEvent!=pServiceInfo->m_EventList.EventDataMap.end();itrEvent++) {
		SYSTEMTIME stEnd;

		itrEvent->second.GetEndTime(&stEnd);
		if (CompareSystemTime(&stFirst,&stEnd)<0
				&& CompareSystemTime(&stLast,&itrEvent->second.m_stStartTime)>0) {
			NewItemList.Add(new CProgramItemInfo(itrEvent->second));
			i++;
		}
	}
	if (i>1)
		NewItemList.Sort();
	if (i>m_ItemList.NumItems()) {
		fChanged=true;
	} else {
		for (j=0;j<i;j++) {
			if (m_ItemList.GetItem(j)->IsChanged(NewItemList.GetItem(j))) {
				fChanged=true;
				break;
			}
		}
	}
	if (i==m_ItemList.NumItems() && !fChanged)
		return false;
	m_ItemList.Attach(&NewItemList);
	return true;
}


void CProgramListView::ClearProgramList()
{
	if (m_ItemList.NumItems()>0) {
		m_ItemList.Clear();
		m_CurEventID=-1;
		m_ScrollPos=0;
		m_TotalLines=0;
		if (m_hwnd!=NULL) {
			SetScrollBar();
			Invalidate();
		}
	}
	//if (m_pProgramList!=NULL)
	//	m_pProgramList->Clear();
}


void CProgramListView::SetCurrentEventID(int EventID)
{
	m_CurEventID=EventID;
	if (m_hwnd!=NULL)
		Invalidate();
}


void CProgramListView::CalcDimentions()
{
	HDC hdc;
	RECT rc;
	HFONT hfontOld;
	int i;

	hdc=GetDC(m_hwnd);
	GetClientRect(&rc);
	hfontOld=SelectFont(hdc,m_hfont);
	m_TotalLines=0;
	for (i=0;i<m_ItemList.NumItems();i++) {
		m_TotalLines+=m_ItemList.GetItem(i)->CalcLines(hdc,rc.right,rc.right-TEXT_LEFT_MARGIN);
	}
	SelectFont(hdc,hfontOld);
	ReleaseDC(m_hwnd,hdc);
}


void CProgramListView::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=m_TotalLines<1?0:m_TotalLines-1;
	GetClientRect(&rc);
	si.nPage=rc.bottom/(m_FontHeight+m_LineMargin);
	si.nPos=m_ScrollPos;
	SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
}


void CProgramListView:: SetColors(
	const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
	const Theme::GradientInfo *pCurEventBackGradient,COLORREF CurEventTextColor,
	const Theme::GradientInfo *pTitleBackGradient,COLORREF TitleTextColor,
	const Theme::GradientInfo *pCurTitleBackGradient,COLORREF CurTitleTextColor,
	COLORREF MarginColor)
{
	m_EventBackGradient=*pEventBackGradient;
	m_EventTextColor=EventTextColor;
	m_CurEventBackGradient=*pCurEventBackGradient;
	m_CurEventTextColor=CurEventTextColor;
	m_TitleBackGradient=*pTitleBackGradient;
	m_TitleTextColor=TitleTextColor;
	m_CurTitleBackGradient=*pCurTitleBackGradient;
	m_CurTitleTextColor=CurTitleTextColor;
	m_MarginColor=MarginColor;
	if (m_hwnd!=NULL)
		Invalidate();
}


bool CProgramListView::SetFont(const LOGFONT *pFont)
{
	HFONT hfont=::CreateFontIndirect(pFont);

	if (hfont==NULL)
		return false;
	::DeleteObject(m_hfont);
	m_hfont=hfont;
	m_ScrollPos=0;
	if (m_hwnd!=NULL) {
		CalcFontHeight();
		CalcDimentions();
		SetScrollBar();
		Invalidate();
	}
	return true;
}


void CProgramListView::CalcFontHeight()
{
	HDC hdc;
	HFONT hfontOld;
	TEXTMETRIC tm;

	hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	hfontOld=static_cast<HFONT>(::SelectObject(hdc,m_hfont));
	::GetTextMetrics(hdc,&tm);
	//m_FontHeight=tm.tmHeight+tm.tmInternalLeading;
	m_FontHeight=tm.tmHeight;
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


CProgramListView *CProgramListView::GetThis(HWND hwnd)
{
	return reinterpret_cast<CProgramListView*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CProgramListView::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CProgramListView *pThis=static_cast<CProgramListView*>(OnCreate(hwnd,lParam));

			pThis->CalcFontHeight();
		}
		return 0;

	case WM_PAINT:
		{
			CProgramListView *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			pThis->DrawProgramList(ps.hdc,&ps.rcPaint);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		{
			CProgramListView *pThis=GetThis(hwnd);

			pThis->CalcDimentions();
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
			CProgramListView *pThis=GetThis(hwnd);
			int Pos,Max;
			RECT rc;
			int Page;

			Pos=pThis->m_ScrollPos;
			pThis->GetClientRect(&rc);
			Page=rc.bottom/(pThis->m_FontHeight+pThis->m_LineMargin);
			Max=max(pThis->m_TotalLines-Page,0);
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos--;				break;
			case SB_LINEDOWN:	Pos++;				break;
			case SB_PAGEUP:		Pos-=Page;			break;
			case SB_PAGEDOWN:	Pos+=Page;			break;
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);	break;
			case SB_TOP:		Pos=0;				break;
			case SB_BOTTOM:		Pos=Max;			break;
			default:	return 0;
			}
			if (Pos<0)
				Pos=0;
			else if (Pos>Max)
				Pos=Max;
			if (Pos!=pThis->m_ScrollPos) {
				int Offset=Pos-pThis->m_ScrollPos;
				SCROLLINFO si;

				pThis->m_ScrollPos=Pos;
				si.cbSize=sizeof(SCROLLINFO);
				si.fMask=SIF_POS;
				si.nPos=Pos;
				SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
				if (abs(Offset*(pThis->m_FontHeight+pThis->m_LineMargin))<rc.bottom) {
					ScrollWindowEx(hwnd,0,-Offset*(pThis->m_FontHeight+pThis->m_LineMargin),
								NULL,NULL,NULL,NULL,SW_ERASE | SW_INVALIDATE);
				} else {
					InvalidateRect(hwnd,NULL,TRUE);
				}
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		SetFocus(hwnd);
		return 0;

	case WM_DESTROY:
		{
			CProgramListView *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CProgramListView::DrawProgramList(HDC hdc,const RECT *prcPaint)
{
	HBRUSH hbr;
	HFONT hfontOld;
	COLORREF crOldTextColor;
	int OldBkMode;
	RECT rc,rcMargin;
	int i;

	hbr=CreateSolidBrush(m_MarginColor);
	hfontOld=SelectFont(hdc,m_hfont);
	crOldTextColor=GetTextColor(hdc);
	OldBkMode=SetBkMode(hdc,TRANSPARENT);
	GetClientRect(&rc);
	rc.top=-(m_ScrollPos*(m_FontHeight+m_LineMargin));
	for (i=0;i<m_ItemList.NumItems();i++) {
		CProgramItemInfo *pItem=m_ItemList.GetItem(i);
		bool fCur=pItem->GetEventID()==m_CurEventID;

		rc.bottom=rc.top+pItem->GetTitleLines()*(m_FontHeight+m_LineMargin);
		if (rc.bottom>prcPaint->top) {
			SetTextColor(hdc,fCur?m_CurTitleTextColor:m_TitleTextColor);
			rc.left=0;
			Theme::FillGradient(hdc,&rc,fCur?&m_CurTitleBackGradient:&m_TitleBackGradient);
			pItem->DrawTitle(hdc,&rc,m_FontHeight+m_LineMargin);
		}
		rc.top=rc.bottom;
		rc.bottom=rc.top+pItem->GetTextLines()*(m_FontHeight+m_LineMargin);
		if (rc.bottom>prcPaint->top) {
			/*
			rc.left=TEXT_LEFT_MARGIN;
			if (prcPaint->left<rc.left) {
				rcMargin.left=prcPaint->left;
				rcMargin.top=rc.top;
				rcMargin.right=min(rc.left,prcPaint->right);
				rcMargin.bottom=rc.bottom;
				FillRect(hdc,&rcMargin,hbr);
			}
			*/
			SetTextColor(hdc,fCur?m_CurEventTextColor:m_EventTextColor);
			rc.left=0;
			Theme::FillGradient(hdc,&rc,fCur?&m_CurEventBackGradient:&m_EventBackGradient);
			rc.left=TEXT_LEFT_MARGIN;
			pItem->DrawText(hdc,&rc,m_FontHeight+m_LineMargin);
		}
		rc.top=rc.bottom;
		if (rc.top>=prcPaint->bottom)
			break;
	}
	if (rc.top<prcPaint->bottom) {
		rcMargin.left=prcPaint->left;
		rcMargin.top=max(rc.top,prcPaint->top);
		rcMargin.right=prcPaint->right;
		rcMargin.bottom=prcPaint->bottom;
		FillRect(hdc,&rcMargin,hbr);
	}
	SetTextColor(hdc,crOldTextColor);
	SetBkMode(hdc,OldBkMode);
	SelectFont(hdc,hfontOld);
	DeleteObject(hbr);
}
