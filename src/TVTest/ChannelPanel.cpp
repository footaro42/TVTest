#include "stdafx.h"
#include "TVTest.h"
#include "ChannelPanel.h"
#include "StdUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define CHANNEL_LIST_WINDOW_CLASS APP_NAME TEXT(" Channel List")

#define EVENT_LEFT_MARGIN 8




HINSTANCE CChannelPanel::m_hinst=NULL;


bool CChannelPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=CHANNEL_LIST_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CChannelPanel::CChannelPanel()
{
	LOGFONT lf;

	m_pProgramList=NULL;
	::GetObject(::GetStockObject(DEFAULT_GUI_FONT),sizeof(LOGFONT),&lf);
	m_hfont=::CreateFontIndirect(&lf);
	lf.lfWeight=FW_BOLD;
	m_hfontChannel=::CreateFontIndirect(&lf);
	m_FontHeight=abs(lf.lfHeight);
	m_ChannelNameMargin=2;
	m_EventNameLines=2;
	m_ItemHeight=m_FontHeight*(m_EventNameLines*2)+(m_FontHeight+m_ChannelNameMargin*2);
	m_ChannelBackColor=RGB(128,128,128);
	m_ChannelTextColor=RGB(255,255,255);
	m_EventBackColor=RGB(0,0,0);
	m_EventTextColor=RGB(255,255,255);
	m_ScrollPos=0;
	m_pEventHandler=NULL;
	m_hwndToolTip=NULL;
}


CChannelPanel::~CChannelPanel()
{
	::DeleteObject(m_hfont);
	::DeleteObject(m_hfontChannel);
	m_ChannelList.DeleteAll();
}


bool CChannelPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 CHANNEL_LIST_WINDOW_CLASS,TEXT("ƒ`ƒƒƒ“ƒlƒ‹"),m_hinst);
}


bool CChannelPanel::SetEpgProgramList(CEpgProgramList *pList)
{
	m_pProgramList=pList;
	return true;
}


bool CChannelPanel::SetChannelList(const CChannelList *pChannelList)
{
	m_ChannelList.DeleteAll();
	if (pChannelList!=NULL) {
		SYSTEMTIME stCurrent;

		m_ScrollPos=0;
		::GetLocalTime(&stCurrent);
		for (int i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);

			if (!pChInfo->IsEnabled())
				continue;

			CChannelEventInfo *pEventInfo=new CChannelEventInfo(pChInfo);

			if (m_pProgramList!=NULL) {
				const WORD TransportStreamID=pChInfo->GetTransportStreamID();
				const WORD ServiceID=pChInfo->GetServiceID();
				CEventInfoData EventInfo;
				bool fOK=false;

				if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&stCurrent,&EventInfo)) {
					fOK=true;
				} else {
					if (m_pProgramList->UpdateProgramList(TransportStreamID,ServiceID)
							&& m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&stCurrent,&EventInfo))
						fOK=true;
				}
				if (fOK) {
					pEventInfo->SetEventInfo(0,&EventInfo);
					SYSTEMTIME st;
					EventInfo.GetEndTime(&st);
					if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
						pEventInfo->SetEventInfo(1,&EventInfo);
					}
				}
			}
			m_ChannelList.Add(pEventInfo);
			if (m_hwnd!=NULL) {
				Invalidate();
				Update();
			}
		}
	}
	if (m_hwnd!=NULL) {
		SetScrollBar();
		SetToolTips();
	}
	return true;
}


bool CChannelPanel::UpdateChannelList()
{
	if (m_pProgramList!=NULL && m_ChannelList.Length()>0) {
		SYSTEMTIME stCurrent;

		::GetLocalTime(&stCurrent);
		for (int i=0;i<m_ChannelList.Length();i++) {
			CChannelEventInfo *pEventInfo=m_ChannelList[i];
			const WORD TransportStreamID=pEventInfo->GetTransportStreamID();
			const WORD ServiceID=pEventInfo->GetServiceID();
			CEventInfoData EventInfo;
			bool fOK=false;

			if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&stCurrent,&EventInfo)) {
				fOK=true;
			} else {
				if (m_pProgramList->UpdateProgramList(TransportStreamID,ServiceID)
						&& m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&stCurrent,&EventInfo))
					fOK=true;
			}
			if (fOK) {
				pEventInfo->SetEventInfo(0,&EventInfo);
				SYSTEMTIME st;
				EventInfo.GetEndTime(&st);
				if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
					pEventInfo->SetEventInfo(1,&EventInfo);
				}
			}
		}
		if (m_hwnd!=NULL) {
			Invalidate();
			Update();
		}
	}
	return true;
}


void CChannelPanel::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


bool CChannelPanel::SetColors(COLORREF ChannelBackColor,COLORREF ChannelTextColor,

							  COLORREF EventBackColor,COLORREF EventTextColor)
{
	m_ChannelBackColor=ChannelBackColor;
	m_ChannelTextColor=ChannelTextColor;
	m_EventBackColor=EventBackColor;
	m_EventTextColor=EventTextColor;
	if (m_hwnd!=NULL) {
		Invalidate();
		Update();
	}
	return true;
}


CChannelPanel *CChannelPanel::GetThis(HWND hwnd)
{
	return static_cast<CChannelPanel*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CChannelPanel::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CChannelPanel *pThis=static_cast<CChannelPanel*>(OnCreate(hwnd,lParam));

			pThis->m_ScrollPos=0;
			pThis->m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,0,0,0,0,
				hwnd,NULL,m_hinst,NULL);
			::SendMessage(pThis->m_hwndToolTip,TTM_SETMAXTIPWIDTH,0,256);
			pThis->SetToolTips();
		}
		return 0;

	case WM_PAINT:
		{
			CChannelPanel *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;

			BeginPaint(hwnd,&ps);
			pThis->Draw(ps.hdc,&ps.rcPaint);
			EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_SIZE:
		{
			CChannelPanel *pThis=GetThis(hwnd);
			int Height=HIWORD(lParam),Max;

			Max=max(pThis->m_ItemHeight*pThis->m_ChannelList.Length()-Height,0);
			if (pThis->m_ScrollPos>Max) {
				pThis->m_ScrollPos=Max;
				pThis->Invalidate();
				pThis->SetToolTips();
			}
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
			CChannelPanel *pThis=GetThis(hwnd);
			int Pos,Max;
			RECT rc;
			int Page;

			Pos=pThis->m_ScrollPos;
			pThis->GetClientRect(&rc);
			Page=rc.bottom;
			Max=max(pThis->m_ItemHeight*pThis->m_ChannelList.Length()-Page,0);
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos-=pThis->m_FontHeight/2;	break;
			case SB_LINEDOWN:	Pos+=pThis->m_FontHeight/2;	break;
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

				pThis->m_ScrollPos=Pos;
				if (abs(Offset)<Page) {
					::ScrollWindowEx(hwnd,0,-Offset,
								NULL,NULL,NULL,NULL,SW_ERASE | SW_INVALIDATE);
				} else {
					InvalidateRect(hwnd,NULL,TRUE);
				}
				pThis->SetScrollBar();
				pThis->SetToolTips();
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CChannelPanel *pThis=GetThis(hwnd);
			int Index;

			SetFocus(hwnd);
			Index=(GET_Y_LPARAM(lParam)+pThis->m_ScrollPos)/pThis->m_ItemHeight;
			if (Index>=0 && Index<pThis->m_ChannelList.Length()) {
				if (pThis->m_pEventHandler!=NULL)
					pThis->m_pEventHandler->OnChannelClick(pThis->m_ChannelList[Index]->GetChannelInfo());
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CChannelPanel *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnRButtonDown();
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CChannelPanel *pThis=GetThis(hwnd);
			int y=GET_Y_LPARAM(lParam);

			if (y>=0 && y<pThis->m_ItemHeight*pThis->m_ChannelList.Length()-pThis->m_ScrollPos)
				::SetCursor(::LoadCursor(NULL,IDC_HAND));
			else
				::SetCursor(::LoadCursor(NULL,IDC_ARROW));
		}
		return 0;

	case WM_NOTIFY:
		switch (reinterpret_cast<LPNMHDR>(lParam)->code) {
		case TTN_NEEDTEXT:
			{
				CChannelPanel *pThis=GetThis(hwnd);
				LPNMTTDISPINFO pnmtdi=reinterpret_cast<LPNMTTDISPINFO>(lParam);
				int Channel=pnmtdi->lParam/2,Event=pnmtdi->lParam%2;

				if (Channel<pThis->m_ChannelList.Length()) {
					static TCHAR szText[512];

					pThis->m_ChannelList[Channel]->FormatEventText(szText,lengthof(szText),Event);
					pnmtdi->lpszText=szText;
				}
				pnmtdi->szText[0]='\0';
				pnmtdi->hinst=NULL;
			}
			return 0;
		}
		break;

	case WM_DESTROY:
		{
			CChannelPanel *pThis=GetThis(hwnd);

			::DestroyWindow(pThis->m_hwndToolTip);
			pThis->m_hwndToolTip=NULL;
			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CChannelPanel::Draw(HDC hdc,const RECT *prcPaint)
{
	HBRUSH hbrChannel,hbrEvent;
	HFONT hfontOld;
	COLORREF crOldTextColor;
	int OldBkMode;
	RECT rc;

	hbrChannel=::CreateSolidBrush(m_ChannelBackColor);
	hbrEvent=::CreateSolidBrush(m_EventBackColor);
	hfontOld=(HFONT)::GetCurrentObject(hdc,OBJ_FONT);
	crOldTextColor=::GetTextColor(hdc);
	OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	GetClientRect(&rc);
	rc.top=-m_ScrollPos;
	for (int i=0;i<m_ChannelList.Length();i++) {
		CChannelEventInfo *pChannelInfo=m_ChannelList[i];

		rc.bottom=rc.top+m_FontHeight+m_ChannelNameMargin*2;
		if (rc.bottom>prcPaint->top) {
			SelectFont(hdc,m_hfontChannel);
			::SetTextColor(hdc,m_ChannelTextColor);
			rc.left=0;
			::FillRect(hdc,&rc,hbrChannel);
			pChannelInfo->DrawChannelName(hdc,&rc);
		}
		for (int j=0;j<2;j++) {
			rc.top=rc.bottom;
			rc.bottom=rc.top+m_FontHeight*m_EventNameLines;
			if (rc.bottom>prcPaint->top) {
				SelectFont(hdc,m_hfont);
				::SetTextColor(hdc,m_EventTextColor);
				rc.left=0;
				::FillRect(hdc,&rc,hbrEvent);
				rc.left=EVENT_LEFT_MARGIN;
				pChannelInfo->DrawEventName(hdc,&rc,j);
			}
		}
		rc.top=rc.bottom;
		if (rc.top>=prcPaint->bottom)
			break;
	}
	if (rc.top<prcPaint->bottom) {
		rc.left=prcPaint->left;
		rc.right=prcPaint->right;
		rc.bottom=prcPaint->bottom;
		::FillRect(hdc,&rc,hbrEvent);
	}
	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	SelectFont(hdc,hfontOld);
	::DeleteObject(hbrChannel);
	::DeleteObject(hbrEvent);
}


void CChannelPanel::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=m_ItemHeight*m_ChannelList.Length();
	GetClientRect(&rc);
	si.nPage=rc.bottom;
	si.nPos=m_ScrollPos;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
}


void CChannelPanel::SetToolTips()
{
	if (m_hwndToolTip!=NULL) {
		int NumTools=::SendMessage(m_hwndToolTip,TTM_GETTOOLCOUNT,0,0);
		TOOLINFO ti;

		ti.cbSize=TTTOOLINFOA_V2_SIZE;
		ti.hwnd=m_hwnd;
		if (NumTools<m_ChannelList.Length()*2) {
			ti.uFlags=TTF_SUBCLASS;
			ti.hinst=NULL;
			ti.lpszText=LPSTR_TEXTCALLBACK;
			::SetRect(&ti.rect,0,0,0,0);
			for (int i=NumTools;i<m_ChannelList.Length()*2;i++) {
				ti.uId=i;
				ti.lParam=i;
				::SendMessage(m_hwndToolTip,TTM_ADDTOOL,0,(LPARAM)&ti);
			}
		} else if (NumTools>m_ChannelList.Length()*2) {
			for (int i=m_ChannelList.Length()*2;i<NumTools;i++) {
				ti.uId=i;
				::SendMessage(m_hwndToolTip,TTM_DELTOOL,0,(LPARAM)&ti);
			}
		}
		GetClientRect(&ti.rect);
		ti.rect.top=-m_ScrollPos;
		ti.uId=0;
		for (int i=0;i<m_ChannelList.Length();i++) {
			ti.rect.top+=m_FontHeight+m_ChannelNameMargin*2;
			ti.rect.bottom=ti.rect.top+m_EventNameLines*m_FontHeight;
			::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,(LPARAM)&ti);
			ti.uId++;
			ti.rect.top=ti.rect.bottom;
			ti.rect.bottom=ti.rect.top+m_EventNameLines*m_FontHeight;
			::SendMessage(m_hwndToolTip,TTM_NEWTOOLRECT,0,(LPARAM)&ti);
			ti.uId++;
			ti.rect.top=ti.rect.bottom;
		}
	}
}




CChannelPanel::CChannelEventInfo::CChannelEventInfo(const CChannelInfo *pInfo)
	: m_ChannelInfo(*pInfo)
{
}


CChannelPanel::CChannelEventInfo::~CChannelEventInfo()
{
}


bool CChannelPanel::CChannelEventInfo::SetEventInfo(int Index,const CEventInfoData *pInfo)
{
	if (Index<0 || Index>1)
		return false;
	m_EventInfo[Index]=*pInfo;
	return true;
}


int CChannelPanel::CChannelEventInfo::FormatEventText(LPTSTR pszText,int MaxLength,int Index) const
{
	if (Index<0 || Index>1 || m_EventInfo[Index].GetEventName()==NULL) {
		pszText[0]='\0';
		return 0;
	}

	const CEventInfoData *pInfo=&m_EventInfo[Index];
	SYSTEMTIME stEnd;
	pInfo->GetEndTime(&stEnd);
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%02d:%02d`%02d:%02d %s%s%s"),
							 pInfo->m_stStartTime.wHour,pInfo->m_stStartTime.wMinute,
							 stEnd.wHour,stEnd.wMinute,
							 pInfo->GetEventName(),
							 pInfo->GetEventText()!=NULL?TEXT("\n\n"):TEXT(""),
							 pInfo->GetEventText()!=NULL?pInfo->GetEventText():TEXT(""));
}


void CChannelPanel::CChannelEventInfo::DrawChannelName(HDC hdc,const RECT *pRect)
{
	::DrawText(hdc,m_ChannelInfo.GetName(),-1,const_cast<LPRECT>(pRect),
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}


void CChannelPanel::CChannelEventInfo::DrawEventName(HDC hdc,const RECT *pRect,int Index)
{
	CEventInfoData *pInfo=&m_EventInfo[Index];

	if (pInfo->GetEventName()!=NULL) {
		TCHAR szText[256];
		SYSTEMTIME stEnd;

		pInfo->GetEndTime(&stEnd);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%02d:%02d`%02d:%02d %s"),
						  pInfo->m_stStartTime.wHour,pInfo->m_stStartTime.wMinute,
						  stEnd.wHour,stEnd.wMinute,
						  pInfo->GetEventName());
		::DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
				   DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
}
