#include "stdafx.h"
#include "TVTest.h"
#include "ChannelPanel.h"
#include "StdUtil.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define CHANNEL_LEFT_MARGIN	2
#define EVENT_LEFT_MARGIN	8




const LPCTSTR CChannelPanel::m_pszClassName=APP_NAME TEXT(" Channel Panel");
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
		wc.lpszClassName=m_pszClassName;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CChannelPanel::CChannelPanel()
	: m_EventInfoPopupManager(&m_EventInfoPopup)
	, m_EventInfoPopupHandler(this)
	, m_pProgramList(NULL)
	, m_FontHeight(0)
	, m_ChannelNameMargin(2)
	, m_EventNameLines(2)
	, m_ItemHeight(0)
	, m_ChannelBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,
							RGB(128,128,128),RGB(128,128,128))
	, m_ChannelTextColor(RGB(255,255,255))
	, m_CurChannelBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,
							   RGB(128,128,128),RGB(128,128,128))
	, m_CurChannelTextColor(RGB(255,255,255))
	, m_EventBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,
						  RGB(0,0,0),RGB(0,0,0))
	, m_EventTextColor(RGB(255,255,255))
	, m_CurEventBackGradient(m_EventBackGradient)
	, m_CurEventTextColor(m_EventTextColor)
	, m_MarginColor(RGB(0,0,0))
	, m_ScrollPos(0)
	, m_CurChannel(-1)
	, m_pEventHandler(NULL)
	, m_hwndToolTip(NULL)
	, m_fDetailToolTip(false)
	, m_pLogoManager(NULL)
{
	LOGFONT lf;
	GetDefaultFont(&lf);
	m_hfont=::CreateFontIndirect(&lf);
	lf.lfWeight=FW_BOLD;
	m_hfontChannel=::CreateFontIndirect(&lf);

	::ZeroMemory(&m_UpdatedTime,sizeof(SYSTEMTIME));
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
							 m_pszClassName,TEXT("チャンネル"),m_hinst);
}


bool CChannelPanel::SetEpgProgramList(CEpgProgramList *pList)
{
	m_pProgramList=pList;
	return true;
}


bool CChannelPanel::SetChannelList(const CChannelList *pChannelList,bool fSetEvent)
{
	m_ChannelList.DeleteAll();
	m_ScrollPos=0;
	m_CurChannel=-1;
	if (pChannelList!=NULL) {
		SYSTEMTIME stCurrent;

		::GetLocalTime(&stCurrent);
		for (int i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);

			if (!pChInfo->IsEnabled())
				continue;

			CChannelEventInfo *pEventInfo=new CChannelEventInfo(pChInfo,i);

			if (fSetEvent && m_pProgramList!=NULL) {
				const WORD TransportStreamID=pChInfo->GetTransportStreamID();
				const WORD ServiceID=pChInfo->GetServiceID();
				CEventInfoData EventInfo;

				if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&stCurrent,&EventInfo)) {
					pEventInfo->SetEventInfo(0,&EventInfo);
					SYSTEMTIME st;
					EventInfo.GetEndTime(&st);
					if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
						pEventInfo->SetEventInfo(1,&EventInfo);
					}
				}
			}
			if (m_pLogoManager!=NULL) {
				HBITMAP hbmLogo=m_pLogoManager->GetAssociatedLogoBitmap(
					pEventInfo->GetNetworkID(),pEventInfo->GetServiceID(),
					CLogoManager::LOGOTYPE_SMALL);
				if (hbmLogo!=NULL)
					pEventInfo->SetLogo(hbmLogo);
			}
			m_ChannelList.Add(pEventInfo);
			/*
			if (m_hwnd!=NULL) {
				RECT rc;

				GetItemRect(m_ChannelList.Length()-1,&rc);
				::InvalidateRect(m_hwnd,&rc,TRUE);
				Update();
			}
			*/
		}
	}
	if (m_hwnd!=NULL) {
		SetScrollBar();
		SetToolTips();
		Invalidate();
		Update();
	}
	return true;
}


bool CChannelPanel::UpdateChannelList()
{
	if (m_pProgramList!=NULL && m_ChannelList.Length()>0) {
		SYSTEMTIME stCurrent;
		bool fChanged=false;

		::GetSystemTime(&m_UpdatedTime);
		::SystemTimeToTzSpecificLocalTime(NULL,&m_UpdatedTime,&stCurrent);
		for (int i=0;i<m_ChannelList.Length();i++) {
			CChannelEventInfo *pEventInfo=m_ChannelList[i];
			const WORD TransportStreamID=pEventInfo->GetTransportStreamID();
			const WORD ServiceID=pEventInfo->GetServiceID();
			CEventInfoData EventInfo;

			if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&stCurrent,&EventInfo)) {
				if (pEventInfo->SetEventInfo(0,&EventInfo))
					fChanged=true;
				SYSTEMTIME st;
				EventInfo.GetEndTime(&st);
				if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
					if (pEventInfo->SetEventInfo(1,&EventInfo))
						fChanged=true;
				} else {
					if (pEventInfo->SetEventInfo(1,NULL))
						fChanged=true;
				}
			} else {
				if (pEventInfo->SetEventInfo(0,NULL))
					fChanged=true;
				if (pEventInfo->SetEventInfo(1,NULL))
					fChanged=true;
			}
		}
		if (m_hwnd!=NULL && fChanged) {
			Invalidate();
			Update();
		}
	}
	return true;
}


bool CChannelPanel::UpdateChannel(int ChannelIndex)
{
	if (m_pProgramList!=NULL) {
		for (int i=0;i<m_ChannelList.Length();i++) {
			CChannelEventInfo *pEventInfo=m_ChannelList[i];

			if (pEventInfo->GetOriginalChannelIndex()==ChannelIndex) {
				SYSTEMTIME st;
				const WORD TransportStreamID=pEventInfo->GetTransportStreamID();
				const WORD ServiceID=pEventInfo->GetServiceID();
				CEventInfoData EventInfo;
				bool fChanged;

				::GetLocalTime(&st);
				if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
					fChanged=pEventInfo->SetEventInfo(0,&EventInfo);
					EventInfo.GetEndTime(&st);
					if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
						if (pEventInfo->SetEventInfo(1,&EventInfo))
							fChanged=true;
					} else {
						if (pEventInfo->SetEventInfo(1,NULL))
							fChanged=true;
					}
				} else {
					fChanged=pEventInfo->SetEventInfo(0,NULL);
					if (pEventInfo->SetEventInfo(1,NULL))
						fChanged=true;
				}
				if (pEventInfo->GetLogo()==NULL && m_pLogoManager!=NULL) {
					HBITMAP hbmLogo=m_pLogoManager->GetAssociatedLogoBitmap(
						pEventInfo->GetNetworkID(),pEventInfo->GetServiceID(),
						CLogoManager::LOGOTYPE_SMALL);
					if (hbmLogo!=NULL) {
						pEventInfo->SetLogo(hbmLogo);
						fChanged=true;
					}
				}
				if (m_hwnd!=NULL && fChanged) {
					RECT rc;

					GetItemRect(i,&rc);
					::InvalidateRect(m_hwnd,&rc,TRUE);
					Update();
				}
				break;
			}
		}
	}
	return true;
}


bool CChannelPanel::IsChannelListEmpty() const
{
	return m_ChannelList.Length()==0;
}


bool CChannelPanel::SetCurrentChannel(int CurChannel)
{
	if (CurChannel<-1)
		return false;
	m_CurChannel=CurChannel;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


void CChannelPanel::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


bool CChannelPanel::SetColors(const Theme::GradientInfo *pChannelBackGradient,COLORREF ChannelTextColor,
	const Theme::GradientInfo *pCurChannelBackGradient,COLORREF CurChannelTextColor,
	const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
	const Theme::GradientInfo *pCurEventBackGradient,COLORREF CurEventTextColor,
	COLORREF MarginColor)
{
	m_ChannelBackGradient=*pChannelBackGradient;
	m_ChannelTextColor=ChannelTextColor;
	m_CurChannelBackGradient=*pCurChannelBackGradient;
	m_CurChannelTextColor=CurChannelTextColor;
	m_EventBackGradient=*pEventBackGradient;
	m_EventTextColor=EventTextColor;
	m_CurEventBackGradient=*pCurEventBackGradient;
	m_CurEventTextColor=CurEventTextColor;
	m_MarginColor=MarginColor;
	if (m_hwnd!=NULL) {
		Invalidate();
		Update();
	}
	return true;
}


bool CChannelPanel::SetFont(const LOGFONT *pFont)
{
	HFONT hfont=::CreateFontIndirect(pFont);

	if (hfont==NULL)
		return false;
	::DeleteObject(m_hfont);
	m_hfont=hfont;
	LOGFONT lf=*pFont;
	lf.lfWeight=FW_BOLD;
	hfont=::CreateFontIndirect(&lf);
	if (hfont!=NULL) {
		::DeleteObject(m_hfontChannel);
		m_hfontChannel=hfont;
	}
	if (m_hwnd!=NULL) {
		CalcItemHeight();
		m_ScrollPos=0;
		SetScrollBar();
		Invalidate();
	}
	return true;
}


bool CChannelPanel::SetEventInfoFont(const LOGFONT *pFont)
{
	return m_EventInfoPopup.SetFont(pFont);
}


void CChannelPanel::SetDetailToolTip(bool fDetail)
{
	if (m_fDetailToolTip!=fDetail) {
		m_fDetailToolTip=fDetail;
		if (m_hwnd!=NULL) {
			if (fDetail)
				m_EventInfoPopupManager.Initialize(m_hwnd,&m_EventInfoPopupHandler);
			else
				m_EventInfoPopupManager.Finalize();
			::SendMessage(m_hwndToolTip,TTM_ACTIVATE,!fDetail,0);
		}
	}
}


void CChannelPanel::SetLogoManager(CLogoManager *pLogoManager)
{
	m_pLogoManager=pLogoManager;
}


bool CChannelPanel::QueryUpdate() const
{
	SYSTEMTIME st;

	::GetSystemTime(&st);
	return m_UpdatedTime.wMinute!=st.wMinute
		|| m_UpdatedTime.wHour!=st.wHour
		|| m_UpdatedTime.wDay!=st.wDay
		|| m_UpdatedTime.wMonth!=st.wMonth
		|| m_UpdatedTime.wYear!=st.wYear;
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

			pThis->CalcItemHeight();
			pThis->m_ScrollPos=0;
			pThis->m_hwndToolTip=::CreateWindowEx(WS_EX_TOPMOST,TOOLTIPS_CLASS,NULL,
				WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,0,0,0,0,
				hwnd,NULL,m_hinst,NULL);
			::SendMessage(pThis->m_hwndToolTip,TTM_SETMAXTIPWIDTH,0,256);
			::SendMessage(pThis->m_hwndToolTip,TTM_SETDELAYTIME,TTDT_AUTOPOP,30000);
			::SendMessage(pThis->m_hwndToolTip,TTM_ACTIVATE,!pThis->m_fDetailToolTip,0);
			pThis->SetToolTips();
			if (pThis->m_fDetailToolTip)
				pThis->m_EventInfoPopupManager.Initialize(hwnd,&pThis->m_EventInfoPopupHandler);
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
		{
			CChannelPanel *pThis=GetThis(hwnd);

			pThis->SetScrollPos(pThis->m_ScrollPos-
								GET_WHEEL_DELTA_WPARAM(wParam)*pThis->m_FontHeight/WHEEL_DELTA);
		}
		return 0;

	case WM_VSCROLL:
		{
			CChannelPanel *pThis=GetThis(hwnd);
			int Pos,Page,Max;
			RECT rc;

			Pos=pThis->m_ScrollPos;
			pThis->GetClientRect(&rc);
			Page=rc.bottom;
			Max=max(pThis->m_ItemHeight*pThis->m_ChannelList.Length()-Page,0);
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos-=pThis->m_FontHeight;	break;
			case SB_LINEDOWN:	Pos+=pThis->m_FontHeight;	break;
			case SB_PAGEUP:		Pos-=Page;					break;
			case SB_PAGEDOWN:	Pos+=Page;					break;
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);			break;
			case SB_TOP:		Pos=0;						break;
			case SB_BOTTOM:		Pos=Max;					break;
			default:	return 0;
			}
			pThis->SetScrollPos(Pos);
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
				int Channel=(int)(pnmtdi->lParam/2),Event=(int)(pnmtdi->lParam%2);

				if (Channel>=0 && Channel<pThis->m_ChannelList.Length()) {
					static TCHAR szText[1024];

					pThis->m_ChannelList[Channel]->FormatEventText(szText,lengthof(szText),Event);
					pnmtdi->lpszText=szText;
				} else {
					pnmtdi->lpszText=TEXT("");
				}
				pnmtdi->szText[0]='\0';
				pnmtdi->hinst=NULL;
			}
			return 0;

		case TTN_SHOW:
			{
				// ツールチップの位置がカーソルと重なっていると
				// 出たり消えたりを繰り返しておかしくなるのでずらす
				LPNMHDR pnmh=reinterpret_cast<LPNMHDR>(lParam);
				RECT rcTip;
				POINT pt;

				::GetWindowRect(pnmh->hwndFrom,&rcTip);
				::GetCursorPos(&pt);
				if (::PtInRect(&rcTip,pt)) {
					HMONITOR hMonitor=::MonitorFromRect(&rcTip,MONITOR_DEFAULTTONEAREST);
					if (hMonitor!=NULL) {
						MONITORINFO mi;

						mi.cbSize=sizeof(mi);
						if (::GetMonitorInfo(hMonitor,&mi)) {
							if (rcTip.left<=mi.rcMonitor.left+16)
								rcTip.left=pt.x+16;
							else if (rcTip.right>=mi.rcMonitor.right-16)
								rcTip.left=pt.x-(rcTip.right-rcTip.left)-8;
							else
								break;
							::SetWindowPos(pnmh->hwndFrom,HWND_TOPMOST,
										   rcTip.left,rcTip.top,0,0,
										   SWP_NOSIZE | SWP_NOACTIVATE);
							return TRUE;
						}
					}
				}
			}
			break;
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
	HFONT hfontOld;
	COLORREF crOldTextColor;
	int OldBkMode;
	RECT rc;

	hfontOld=(HFONT)::GetCurrentObject(hdc,OBJ_FONT);
	crOldTextColor=::GetTextColor(hdc);
	OldBkMode=::SetBkMode(hdc,TRANSPARENT);
	GetClientRect(&rc);
	rc.top=-m_ScrollPos;
	for (int i=0;i<m_ChannelList.Length();i++) {
		CChannelEventInfo *pChannelInfo=m_ChannelList[i];
		const bool fCurrent=pChannelInfo->GetOriginalChannelIndex()==m_CurChannel;

		rc.bottom=rc.top+m_FontHeight+m_ChannelNameMargin*2;
		if (rc.bottom>prcPaint->top) {
			SelectFont(hdc,m_hfontChannel);
			::SetTextColor(hdc,fCurrent?m_CurChannelTextColor:m_ChannelTextColor);
			rc.left=0;
			Theme::FillGradient(hdc,&rc,
				fCurrent?&m_CurChannelBackGradient:&m_ChannelBackGradient);
			rc.left=CHANNEL_LEFT_MARGIN;
			pChannelInfo->DrawChannelName(hdc,&rc);
		}
		for (int j=0;j<2;j++) {
			rc.top=rc.bottom;
			rc.bottom=rc.top+m_FontHeight*m_EventNameLines;
			if (rc.bottom>prcPaint->top) {
				SelectFont(hdc,m_hfont);
				::SetTextColor(hdc,fCurrent?m_CurEventTextColor:m_EventTextColor);
				rc.left=0;
				Theme::FillGradient(hdc,&rc,fCurrent?&m_CurEventBackGradient:&m_EventBackGradient);
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
		if (rc.top<prcPaint->top)
			rc.top=prcPaint->top;
		rc.right=prcPaint->right;
		rc.bottom=prcPaint->bottom;
		DrawUtil::Fill(hdc,&rc,m_MarginColor);
	}
	::SetTextColor(hdc,crOldTextColor);
	::SetBkMode(hdc,OldBkMode);
	SelectFont(hdc,hfontOld);
}


void CChannelPanel::SetScrollPos(int Pos)
{
	RECT rc;

	GetClientRect(&rc);
	if (Pos<0) {
		Pos=0;
	} else {
		int Max=max(m_ItemHeight*m_ChannelList.Length()-rc.bottom,0);
		if (Pos>Max)
			Pos=Max;
	}
	if (Pos!=m_ScrollPos) {
		int Offset=Pos-m_ScrollPos;

		m_ScrollPos=Pos;
		if (abs(Offset)<rc.bottom) {
			::ScrollWindowEx(m_hwnd,0,-Offset,
							 NULL,NULL,NULL,NULL,SW_ERASE | SW_INVALIDATE);
		} else {
			Invalidate();
		}
		SetScrollBar();
		SetToolTips();
	}
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


void CChannelPanel::CalcItemHeight()
{
	HDC hdc;
	HFONT hfontOld;
	TEXTMETRIC tm;

	hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	hfontOld=static_cast<HFONT>(::SelectObject(hdc,m_hfont));
	::GetTextMetrics(hdc,&tm);
	//m_FontHeight=tm.tmHeight-tm.tmInternalLeading;
	m_FontHeight=tm.tmHeight;
	m_ItemHeight=m_FontHeight*(m_EventNameLines*2)+(m_FontHeight+m_ChannelNameMargin*2);
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


void CChannelPanel::GetItemRect(int Index,RECT *pRect)
{
	GetClientRect(pRect);
	pRect->top=Index*m_ItemHeight-m_ScrollPos;
	pRect->bottom=pRect->top+m_ItemHeight;
}


int CChannelPanel::HitTest(int x,int y,HitType *pType) const
{
	POINT pt;
	RECT rc;

	pt.x=x;
	pt.y=y;
	GetClientRect(&rc);
	rc.top=-m_ScrollPos;
	for (int i=0;i<m_ChannelList.Length();i++) {
		rc.bottom=rc.top+m_FontHeight+m_ChannelNameMargin*2;
		if (::PtInRect(&rc,pt)) {
			if (pType!=NULL)
				*pType=HIT_CHANNELNAME;
			return i;
		}
		rc.top=rc.bottom;
		rc.bottom=rc.top+m_EventNameLines*m_FontHeight;
		if (::PtInRect(&rc,pt)) {
			if (pType!=NULL)
				*pType=HIT_EVENT1;
			return i;
		}
		rc.top=rc.bottom;
		rc.bottom=rc.top+m_EventNameLines*m_FontHeight;
		if (::PtInRect(&rc,pt)) {
			if (pType!=NULL)
				*pType=HIT_EVENT2;
			return i;
		}
		rc.top=rc.bottom;
	}
	return -1;
}


void CChannelPanel::SetToolTips()
{
	if (m_hwndToolTip!=NULL) {
		int NumTools=(int)::SendMessage(m_hwndToolTip,TTM_GETTOOLCOUNT,0,0);
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


bool CChannelPanel::EventInfoPopupHitTest(int x,int y,LPARAM *pParam)
{
	if (m_fDetailToolTip) {
		HitType Type;
		int Channel=HitTest(x,y,&Type);

		if (Channel>=0 && (Type==HIT_EVENT1 || Type==HIT_EVENT2)) {
			int Event=Type==HIT_EVENT1?0:1;
			if (m_ChannelList[Channel]->IsEventEnabled(Event)) {
				*pParam=Channel*2+Event;
				return true;
			}
		}
	}
	return false;
}


bool CChannelPanel::GetEventInfoPopupEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	int Channel=(int)(Param/2),Event=(int)(Param%2);

	if (Channel<0 || Channel>=m_ChannelList.Length() || Event<0 && Event>1)
		return false;
	*ppInfo=&m_ChannelList[Channel]->GetEventInfo(Event);
	return true;
}


CChannelPanel::CEventInfoPopupHandler::CEventInfoPopupHandler(CChannelPanel *pChannelPanel)
	: m_pChannelPanel(pChannelPanel)
{
}


bool CChannelPanel::CEventInfoPopupHandler::HitTest(int x,int y,LPARAM *pParam)
{
	return m_pChannelPanel->EventInfoPopupHitTest(x,y,pParam);
}


bool CChannelPanel::CEventInfoPopupHandler::GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	return m_pChannelPanel->GetEventInfoPopupEventInfo(Param,ppInfo);
}




CChannelPanel::CChannelEventInfo::CChannelEventInfo(const CChannelInfo *pInfo,int OriginalIndex)
	: m_ChannelInfo(*pInfo)
	, m_OriginalChannelIndex(OriginalIndex)
	, m_hbmLogo(NULL)
{
}


CChannelPanel::CChannelEventInfo::~CChannelEventInfo()
{
}


bool CChannelPanel::CChannelEventInfo::SetEventInfo(int Index,const CEventInfoData *pInfo)
{
	if (Index<0 || Index>1)
		return false;
	bool fChanged=false;
	if (pInfo!=NULL) {
		if (m_EventInfo[Index]!=*pInfo) {
			m_EventInfo[Index]=*pInfo;
			fChanged=true;
		}
	} else if (m_EventInfo[Index].GetEventName()!=NULL) {
		CEventInfoData Info;

		m_EventInfo[Index]=Info;
		fChanged=true;
	}
	return fChanged;
}


bool CChannelPanel::CChannelEventInfo::IsEventEnabled(int Index) const
{
	if (Index<0 || Index>1)
		return false;
	return m_EventInfo[Index].GetEventName()!=NULL;
}


int CChannelPanel::CChannelEventInfo::FormatEventText(LPTSTR pszText,int MaxLength,int Index) const
{
	if (!IsEventEnabled(Index)) {
		pszText[0]='\0';
		return 0;
	}

	const CEventInfoData *pInfo=&m_EventInfo[Index];
	SYSTEMTIME stEnd;
	pInfo->GetEndTime(&stEnd);
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%d:%02d～%d:%02d %s%s%s"),
							 pInfo->m_stStartTime.wHour,pInfo->m_stStartTime.wMinute,
							 stEnd.wHour,stEnd.wMinute,
							 pInfo->GetEventName(),
							 pInfo->GetEventText()!=NULL?TEXT("\n\n"):TEXT(""),
							 pInfo->GetEventText()!=NULL?pInfo->GetEventText():TEXT(""));
}


void CChannelPanel::CChannelEventInfo::DrawChannelName(HDC hdc,const RECT *pRect)
{
	RECT rc=*pRect;

	if (m_hbmLogo!=NULL) {
		int LogoWidth,LogoHeight;
		LogoHeight=rc.bottom-rc.top-4;
		LogoWidth=LogoHeight*16/9;
		DrawUtil::DrawBitmap(hdc,rc.left,rc.top+(rc.bottom-rc.top-LogoHeight)/2,
							 LogoWidth,LogoHeight,m_hbmLogo,NULL,192);
		rc.left+=LogoWidth+3;
	}

	TCHAR szText[MAX_CHANNEL_NAME+16];
	if (m_ChannelInfo.GetChannelNo()!=0)
		::wsprintf(szText,TEXT("%d: %s"),m_ChannelInfo.GetChannelNo(),m_ChannelInfo.GetName());
	else
		::lstrcpy(szText,m_ChannelInfo.GetName());
	::DrawText(hdc,szText,-1,&rc,
			   DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
}


void CChannelPanel::CChannelEventInfo::DrawEventName(HDC hdc,const RECT *pRect,int Index)
{
	CEventInfoData *pInfo=&m_EventInfo[Index];

	if (pInfo->GetEventName()!=NULL) {
		TCHAR szText[256];
		SYSTEMTIME stEnd;

		pInfo->GetEndTime(&stEnd);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%02d:%02d～%02d:%02d %s"),
						  pInfo->m_stStartTime.wHour,pInfo->m_stStartTime.wMinute,
						  stEnd.wHour,stEnd.wMinute,
						  pInfo->GetEventName());
		::DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
				   DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
}
