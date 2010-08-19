#include "stdafx.h"
#include "TVTest.h"
#include "ChannelPanel.h"
#include "StdUtil.h"
#include "DrawUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define CHANNEL_LEFT_MARGIN		2
#define CHANNEL_RIGHT_MARGIN	2
#define EVENT_LEFT_MARGIN	8
#define CHEVRON_WIDTH	10
#define CHEVRON_HEIGHT	10

#define EXPAND_INCREASE_EVENTS 4



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
	, m_EventNameMargin(1)
	, m_EventNameLines(2)
	, m_ItemHeight(0)
	, m_ExpandedItemHeight(0)

	, m_hbmChevron(NULL)
	, m_EventsPerChannel(2)
	, m_ExpandEvents(m_EventsPerChannel+EXPAND_INCREASE_EVENTS)
	, m_ScrollPos(0)
	, m_CurChannel(-1)
	, m_pEventHandler(NULL)
	, m_fDetailToolTip(false)
	, m_pLogoManager(NULL)
{
	LOGFONT lf;
	GetDefaultFont(&lf);
	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_ChannelFont.Create(&lf);

	::ZeroMemory(&m_UpdatedTime,sizeof(SYSTEMTIME));

	m_Theme.ChannelNameStyle.Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.ChannelNameStyle.Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.ChannelNameStyle.Gradient.Color1=RGB(128,128,128);
	m_Theme.ChannelNameStyle.Gradient.Color2=RGB(128,128,128);
	m_Theme.ChannelNameStyle.Border.Type=Theme::BORDER_NONE;
	m_Theme.ChannelNameStyle.TextColor=RGB(255,255,255);
	m_Theme.CurChannelNameStyle=m_Theme.ChannelNameStyle;
	m_Theme.EventStyle[0].Gradient.Type=Theme::GRADIENT_NORMAL;
	m_Theme.EventStyle[0].Gradient.Direction=Theme::DIRECTION_VERT;
	m_Theme.EventStyle[0].Gradient.Color1=RGB(0,0,0);
	m_Theme.EventStyle[0].Gradient.Color2=RGB(0,0,0);
	m_Theme.EventStyle[0].Border.Type=Theme::BORDER_NONE;
	m_Theme.EventStyle[0].TextColor=RGB(255,255,255);
	m_Theme.EventStyle[1]=m_Theme.EventStyle[0];
	m_Theme.CurChannelEventStyle[0]=m_Theme.EventStyle[0];
	m_Theme.CurChannelEventStyle[1]=m_Theme.CurChannelEventStyle[0];
	m_Theme.MarginColor=RGB(0,0,0);
}


CChannelPanel::~CChannelPanel()
{
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


bool CChannelPanel::UpdateEvents(CChannelEventInfo *pInfo,const SYSTEMTIME *pTime)
{
	const WORD TransportStreamID=pInfo->GetTransportStreamID();
	const WORD ServiceID=pInfo->GetServiceID();
	int NumEvents;
	SYSTEMTIME st;
	CEventInfoData EventInfo;
	bool fChanged=false;

	if (pTime!=NULL)
		st=*pTime;
	else
		::GetLocalTime(&st);
	NumEvents=pInfo->IsExpanded()?m_ExpandEvents:m_EventsPerChannel;
	for (int i=0;i<NumEvents;i++) {
		if (m_pProgramList->GetEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)) {
			if (pInfo->SetEventInfo(i,&EventInfo))
				fChanged=true;
		} else {
			if (i==0) {
				if (pInfo->SetEventInfo(0,NULL))
					fChanged=true;
				if (m_EventsPerChannel==0)
					break;
				i++;
			}
			if (m_pProgramList->GetNextEventInfo(TransportStreamID,ServiceID,&st,&EventInfo)
					&& DiffSystemTime(&EventInfo.m_stStartTime,&st)<12*60*60*1000) {
				if (pInfo->SetEventInfo(i,&EventInfo))
					fChanged=true;
			} else {
				for (;i<m_EventsPerChannel;i++) {
					if (pInfo->SetEventInfo(i,NULL))
						fChanged=true;
				}
				break;
			}
		}
		EventInfo.GetEndTime(&st);
	}
	return fChanged;
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

			if (fSetEvent && m_pProgramList!=NULL)
				UpdateEvents(pEventInfo,&stCurrent);
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
				Invalidate(&rc);
				Update();
			}
			*/
		}
	}
	if (m_hwnd!=NULL) {
		SetScrollBar();
		SetTooltips();
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
			if (UpdateEvents(m_ChannelList[i],&stCurrent))
				fChanged=true;
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
				if (UpdateEvents(pEventInfo) && m_hwnd!=NULL) {
					RECT rc;

					GetItemRect(i,&rc);
					Invalidate(&rc);
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


bool CChannelPanel::SetTheme(const ThemeInfo *pTheme)
{
	if (pTheme==NULL)
		return false;
	m_Theme=*pTheme;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}


bool CChannelPanel::GetTheme(ThemeInfo *pTheme) const
{
	if (pTheme==NULL)
		return false;
	*pTheme=m_Theme;
	return true;
}


/*
bool CChannelPanel::SetColors(const Theme::GradientInfo *pChannelBackGradient,COLORREF ChannelTextColor,
	const Theme::GradientInfo *pCurChannelBackGradient,COLORREF CurChannelTextColor,
	const Theme::GradientInfo *pEventBackGradient1,COLORREF EventTextColor1,
	const Theme::GradientInfo *pEventBackGradient2,COLORREF EventTextColor2,
	const Theme::GradientInfo *pCurEventBackGradient1,COLORREF CurEventTextColor1,
	const Theme::GradientInfo *pCurEventBackGradient2,COLORREF CurEventTextColor2,
	COLORREF MarginColor)
{
	m_ChannelBackGradient=*pChannelBackGradient;
	m_ChannelTextColor=ChannelTextColor;
	m_CurChannelBackGradient=*pCurChannelBackGradient;
	m_CurChannelTextColor=CurChannelTextColor;
	m_EventBackGradient[0]=*pEventBackGradient1;
	m_EventBackGradient[1]=*pEventBackGradient2;
	m_EventTextColor[0]=EventTextColor1;
	m_EventTextColor[1]=EventTextColor2;
	m_CurChannelEventBackGradient[0]=*pCurChannelEventBackGradient1;
	m_CurChannelEventBackGradient[1]=*pCurChannelEventBackGradient2;
	m_CurChannelEventTextColor[0]=CurChannelEventTextColor1;
	m_CurChannelEventTextColor[1]=CurChannelEventTextColor2;
	m_MarginColor=MarginColor;
	if (m_hwnd!=NULL)
		Invalidate();
	return true;
}
*/


bool CChannelPanel::SetFont(const LOGFONT *pFont)
{
	if (!m_Font.Create(pFont))
		return false;
	LOGFONT lf=*pFont;
	lf.lfWeight=FW_BOLD;
	m_ChannelFont.Create(&lf);
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
			if (fDetail) {
				m_Tooltip.Destroy();
				m_EventInfoPopupManager.Initialize(m_hwnd,&m_EventInfoPopupHandler);
			} else {
				m_EventInfoPopupManager.Finalize();
				CreateTooltip();
			}
		}
	}
}


bool CChannelPanel::SetEventsPerChannel(int Events)
{
	if (Events<1 || Events>4)
		return false;
	if (m_EventsPerChannel!=Events) {
		m_EventsPerChannel=Events;
		m_ExpandEvents=Events+EXPAND_INCREASE_EVENTS;
		for (int i=0;i<m_ChannelList.Length();i++) {
			CChannelEventInfo *pInfo=m_ChannelList[i];

			pInfo->SetMaxEvents(pInfo->IsExpanded()?m_ExpandEvents:m_EventsPerChannel);
		}
		if (m_hwnd!=NULL)
			CalcItemHeight();
		UpdateChannelList();
		if (m_hwnd!=NULL) {
			m_ScrollPos=0;
			SetScrollBar();
			Invalidate();
		}
	}
	return true;
}


bool CChannelPanel::ExpandChannel(int Channel,bool fExpand)
{
	if (Channel<0 || Channel>=m_ChannelList.Length())
		return false;
	CChannelEventInfo *pInfo=m_ChannelList[Channel];
	if (pInfo->IsExpanded()!=fExpand) {
		pInfo->Expand(fExpand);
		pInfo->SetMaxEvents(fExpand?m_ExpandEvents:m_EventsPerChannel);
		UpdateEvents(pInfo);
		if (m_hwnd!=NULL) {
			RECT rcClient,rc;

			GetClientRect(&rcClient);
			GetItemRect(Channel,&rc);
			if (fExpand) {
				rc.bottom=rcClient.bottom;
				Invalidate(&rc);
			} else {
				int Height=CalcHeight();

				if (m_ScrollPos>Height-rcClient.bottom) {
					m_ScrollPos=max(Height-rcClient.bottom,0);
					Invalidate();
				} else {
					rc.bottom=rcClient.bottom;
					Invalidate(&rc);
				}
			}
			SetScrollBar();
			SetTooltips();
		}
	}
	return true;
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
			if (pThis->m_fDetailToolTip)
				pThis->m_EventInfoPopupManager.Initialize(hwnd,&pThis->m_EventInfoPopupHandler);
			else
				pThis->CreateTooltip();
			pThis->m_hbmChevron=(HBITMAP)::LoadImage(m_hinst,MAKEINTRESOURCE(IDB_CHEVRON),
													 IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);
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
			int TotalHeight=pThis->CalcHeight();

			Max=max(TotalHeight-Height,0);
			if (pThis->m_ScrollPos>Max) {
				pThis->m_ScrollPos=Max;
				pThis->Invalidate();
				pThis->SetTooltips();
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
			int Height=pThis->CalcHeight();
			int Pos,Page;
			RECT rc;

			Pos=pThis->m_ScrollPos;
			pThis->GetClientRect(&rc);
			Page=rc.bottom;
			switch (LOWORD(wParam)) {
			case SB_LINEUP:		Pos-=pThis->m_FontHeight;	break;
			case SB_LINEDOWN:	Pos+=pThis->m_FontHeight;	break;
			case SB_PAGEUP:		Pos-=Page;					break;
			case SB_PAGEDOWN:	Pos+=Page;					break;
			case SB_THUMBTRACK:	Pos=HIWORD(wParam);			break;
			case SB_TOP:		Pos=0;						break;
			case SB_BOTTOM:		Pos=max(Height-Page,0);		break;
			default:	return 0;
			}
			pThis->SetScrollPos(Pos);
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CChannelPanel *pThis=GetThis(hwnd);
			HitType Type;
			int Channel;

			SetFocus(hwnd);
			Channel=pThis->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&Type);
			if (Channel>=0) {
				if (Type==HIT_CHEVRON)
					pThis->ExpandChannel(Channel,!pThis->m_ChannelList[Channel]->IsExpanded());
				else if (pThis->m_pEventHandler!=NULL)
					pThis->m_pEventHandler->OnChannelClick(pThis->m_ChannelList[Channel]->GetChannelInfo());
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

			if (y>=0 && y<pThis->CalcHeight()-pThis->m_ScrollPos)
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
				int Channel=LOWORD(pnmtdi->lParam),Event=HIWORD(pnmtdi->lParam);

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

			pThis->m_EventInfoPopupManager.Finalize();
			pThis->m_Tooltip.Destroy();
			::DeleteObject(pThis->m_hbmChevron);
			pThis->m_hbmChevron=NULL;
			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


void CChannelPanel::Draw(HDC hdc,const RECT *prcPaint)
{
	HFONT hfontOld;
	COLORREF crOldTextColor;
	int OldBkMode;
	HDC hdcMem;
	HBITMAP hbmOld;
	RECT rcClient,rc;

	hfontOld=static_cast<HFONT>(::GetCurrentObject(hdc,OBJ_FONT));
	crOldTextColor=::GetTextColor(hdc);
	OldBkMode=::SetBkMode(hdc,TRANSPARENT);

	hdcMem=::CreateCompatibleDC(hdc);
	hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMem,m_hbmChevron));

	GetClientRect(&rcClient);
	rc.top=-m_ScrollPos;
	for (int i=0;i<m_ChannelList.Length() && rc.top<prcPaint->bottom;i++) {
		CChannelEventInfo *pChannelInfo=m_ChannelList[i];
		const bool fCurrent=pChannelInfo->GetOriginalChannelIndex()==m_CurChannel;

		rc.bottom=rc.top+m_FontHeight+m_ChannelNameMargin*2;
		if (rc.bottom>prcPaint->top) {
			const Theme::Style &Style=
				fCurrent?m_Theme.CurChannelNameStyle:m_Theme.ChannelNameStyle;

			DrawUtil::SelectObject(hdc,m_ChannelFont);
			::SetTextColor(hdc,Style.TextColor);
			rc.left=0;
			rc.right=rcClient.right;
			Theme::DrawStyleBackground(hdc,&rc,&Style);
			rc.left=CHANNEL_LEFT_MARGIN;
			rc.right-=CHEVRON_WIDTH+CHANNEL_RIGHT_MARGIN;
			pChannelInfo->DrawChannelName(hdc,&rc);
			DrawUtil::DrawMonoColorDIB(hdc,
									   rc.right,rc.top+((rc.bottom-rc.top)-CHEVRON_HEIGHT)/2,
									   hdcMem,
									   pChannelInfo->IsExpanded()?CHEVRON_WIDTH:0,0,
									   CHEVRON_WIDTH,CHEVRON_HEIGHT,
									   Style.TextColor);
		}

		int NumEvents=
				pChannelInfo->IsExpanded()?m_ExpandEvents:m_EventsPerChannel;
		rc.left=0;
		rc.right=rcClient.right;
		for (int j=0;j<NumEvents;j++) {
			rc.top=rc.bottom;
			rc.bottom=rc.top+m_FontHeight*m_EventNameLines+m_EventNameMargin*2;
			if (rc.bottom>prcPaint->top) {
				const Theme::Style &Style=
					(fCurrent?m_Theme.CurChannelEventStyle:m_Theme.EventStyle)[j%2];

				DrawUtil::SelectObject(hdc,m_Font);
				::SetTextColor(hdc,Style.TextColor);
				Theme::DrawStyleBackground(hdc,&rc,&Style);
				RECT rcText;
				rcText.left=rc.left+EVENT_LEFT_MARGIN;
				rcText.top=rc.top+m_EventNameMargin;
				rcText.right=rc.right;
				rcText.bottom=rc.bottom-m_EventNameMargin;
				pChannelInfo->DrawEventName(hdc,&rcText,j);
			}
		}
		rc.top=rc.bottom;
	}

	if (rc.top<prcPaint->bottom) {
		rc.left=prcPaint->left;
		if (rc.top<prcPaint->top)
			rc.top=prcPaint->top;
		rc.right=prcPaint->right;
		rc.bottom=prcPaint->bottom;
		DrawUtil::Fill(hdc,&rc,m_Theme.MarginColor);
	}

	::SelectObject(hdcMem,hbmOld);
	::DeleteDC(hdcMem);

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
		int Height=CalcHeight();
		int Max=max(Height-rc.bottom,0);
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
		SetTooltips();
	}
}


void CChannelPanel::SetScrollBar()
{
	SCROLLINFO si;
	RECT rc;

	si.cbSize=sizeof(SCROLLINFO);
	si.fMask=SIF_PAGE | SIF_RANGE | SIF_POS | SIF_DISABLENOSCROLL;
	si.nMin=0;
	si.nMax=CalcHeight();
	GetClientRect(&rc);
	si.nPage=rc.bottom;
	si.nPos=m_ScrollPos;
	::SetScrollInfo(m_hwnd,SB_VERT,&si,TRUE);
}


void CChannelPanel::CalcItemHeight()
{
	HDC hdc;

	hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	m_FontHeight=m_Font.GetHeight(hdc);
	int ChannelNameHeight=m_FontHeight+m_ChannelNameMargin*2;
	int EventNameHeight=m_FontHeight*m_EventNameLines+m_EventNameMargin*2;
	m_ItemHeight=EventNameHeight*m_EventsPerChannel+ChannelNameHeight;
	m_ExpandedItemHeight=EventNameHeight*m_ExpandEvents+ChannelNameHeight;
	::ReleaseDC(m_hwnd,hdc);
}


int CChannelPanel::CalcHeight() const
{
	int Height;

	Height=0;
	for (int i=0;i<m_ChannelList.Length();i++) {
		if (m_ChannelList[i]->IsExpanded())
			Height+=m_ExpandedItemHeight;
		else
			Height+=m_ItemHeight;
	}
	return Height;
}


void CChannelPanel::GetItemRect(int Index,RECT *pRect)
{
	int y;

	y=-m_ScrollPos;
	for (int i=0;i<Index;i++)
		y+=m_ChannelList[i]->IsExpanded()?m_ExpandedItemHeight:m_ItemHeight;
	GetClientRect(pRect);
	pRect->top=y;
	pRect->bottom=y+(m_ChannelList[Index]->IsExpanded()?
											m_ExpandedItemHeight:m_ItemHeight);
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
			if (pType!=NULL) {
				if (x>=rc.right-(CHEVRON_WIDTH+CHANNEL_RIGHT_MARGIN))
					*pType=HIT_CHEVRON;
				else
					*pType=HIT_CHANNELNAME;
			}
			return i;
		}
		int NumEvents=m_ChannelList[i]->IsExpanded()?
											m_ExpandEvents:m_EventsPerChannel;
		for (int j=0;j<NumEvents;j++) {
			rc.top=rc.bottom;
			rc.bottom=rc.top+m_EventNameLines*m_FontHeight+m_EventNameMargin*2;
			if (::PtInRect(&rc,pt)) {
				if (pType!=NULL)
					*pType=(HitType)(HIT_EVENT1+j);
				return i;
			}
		}
		rc.top=rc.bottom;
	}
	return -1;
}


bool CChannelPanel::CreateTooltip()
{
	if (!m_Tooltip.Create(m_hwnd))
		return false;
	m_Tooltip.SetMaxWidth(256);
	m_Tooltip.SetPopDelay(30*1000);
	SetTooltips();
	return true;
}


void CChannelPanel::SetTooltips()
{
	if (m_Tooltip.IsCreated()) {
		int NumTools=m_Tooltip.NumTools();
		int ToolCount;
		RECT rc;

		GetClientRect(&rc);
		rc.top=-m_ScrollPos;
		ToolCount=0;
		for (int i=0;i<m_ChannelList.Length();i++) {
			rc.top+=m_FontHeight+m_ChannelNameMargin*2;
			int NumEvents=m_ChannelList[i]->IsExpanded()?
											m_ExpandEvents:m_EventsPerChannel;
			for (int j=0;j<NumEvents;j++) {
				rc.bottom=rc.top+m_EventNameLines*m_FontHeight+m_EventNameMargin*2;
				if (ToolCount<NumTools)
					m_Tooltip.SetToolRect(ToolCount,rc);
				else
					m_Tooltip.AddTool(ToolCount,rc,LPSTR_TEXTCALLBACK,i);
				ToolCount++;
				rc.top=rc.bottom;
			}
		}
		if (NumTools>ToolCount) {
			for (int i=NumTools-1;i>=ToolCount;i--) {
				m_Tooltip.DeleteTool(i);
			}
		}
	}
}


bool CChannelPanel::EventInfoPopupHitTest(int x,int y,LPARAM *pParam)
{
	if (m_fDetailToolTip) {
		HitType Type;
		int Channel=HitTest(x,y,&Type);

		if (Channel>=0 && Type>=HIT_EVENT1) {
			int Event=Type-HIT_EVENT1;
			if (m_ChannelList[Channel]->IsEventEnabled(Event)) {
				*pParam=MAKELONG(Channel,Event);
				return true;
			}
		}
	}
	return false;
}


bool CChannelPanel::GetEventInfoPopupEventInfo(LPARAM Param,const CEventInfoData **ppInfo)
{
	int Channel=LOWORD(Param),Event=HIWORD(Param);

	if (Channel<0 || Channel>=m_ChannelList.Length()
			|| !m_ChannelList[Channel]->IsEventEnabled(Event))
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
	, m_fExpanded(false)
{
}


CChannelPanel::CChannelEventInfo::~CChannelEventInfo()
{
}


bool CChannelPanel::CChannelEventInfo::SetEventInfo(int Index,const CEventInfoData *pInfo)
{
	if (Index<0)
		return false;
	bool fChanged=false;
	if (pInfo!=NULL) {
		if ((int)m_EventList.size()<=Index)
			m_EventList.resize(Index+1);
		if (m_EventList[Index]!=*pInfo) {
			m_EventList[Index]=*pInfo;
			fChanged=true;
		}
	} else {
		if (Index<(int)m_EventList.size()
				&& m_EventList[Index].m_fValidStartTime) {
			CEventInfoData Info;

			m_EventList[Index]=Info;
			fChanged=true;
		}
	}
	return fChanged;
}


void CChannelPanel::CChannelEventInfo::SetMaxEvents(int Events)
{
	if (Events>(int)m_EventList.size())
		m_EventList.resize(Events);
}


bool CChannelPanel::CChannelEventInfo::IsEventEnabled(int Index) const
{
	if (Index<0 || Index>=(int)m_EventList.size())
		return false;
	return m_EventList[Index].m_fValidStartTime;
}


int CChannelPanel::CChannelEventInfo::FormatEventText(LPTSTR pszText,int MaxLength,int Index) const
{
	if (!IsEventEnabled(Index)) {
		pszText[0]='\0';
		return 0;
	}

	const CEventInfoData &Info=m_EventList[Index];
	SYSTEMTIME stEnd;
	Info.GetEndTime(&stEnd);
	return StdUtil::snprintf(pszText,MaxLength,TEXT("%d:%02d～%d:%02d %s%s%s"),
							 Info.m_stStartTime.wHour,Info.m_stStartTime.wMinute,
							 stEnd.wHour,stEnd.wMinute,
							 NullToEmptyString(Info.GetEventName()),
							 Info.GetEventText()!=NULL?TEXT("\n\n"):TEXT(""),
							 NullToEmptyString(Info.GetEventText()));
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
	if (IsEventEnabled(Index)) {
		const CEventInfoData &Info=m_EventList[Index];
		TCHAR szText[256];
		SYSTEMTIME stEnd;

		Info.GetEndTime(&stEnd);
		StdUtil::snprintf(szText,lengthof(szText),TEXT("%02d:%02d～%02d:%02d "),
						  Info.m_stStartTime.wHour,Info.m_stStartTime.wMinute,
						  stEnd.wHour,stEnd.wMinute);
		if (Info.GetEventName()!=NULL)
			::lstrcat(szText,Info.GetEventName());
		::DrawText(hdc,szText,-1,const_cast<LPRECT>(pRect),
				   DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS);
	}
}
