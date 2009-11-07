#include "stdafx.h"
#include "TVTest.h"
#include "EventInfoPopup.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif


#define EVENT_INFO_POPUP_CLASS APP_NAME TEXT(" Event Info")




//const LPCTSTR CEventInfoPopup::m_pszPropName=TEXT("EventInfoPopup");
HINSTANCE CEventInfoPopup::m_hinst=NULL;


bool CEventInfoPopup::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=::LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=EVENT_INFO_POPUP_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CEventInfoPopup::CEventInfoPopup()
	: m_hwndEdit(NULL)
	, m_BackColor(::GetSysColor(COLOR_3DFACE))
	, m_TextColor(::GetSysColor(COLOR_WINDOWTEXT))
	, m_TitleBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,
						  RGB(255,255,255),RGB(228,228,240))
	, m_TitleTextColor(RGB(80,80,80))
	, m_TitleLineMargin(1)
	, m_TitleHeight(0)
	, m_ButtonSize(14)
	, m_ButtonMargin(3)
	, m_pEventHandler(NULL)
{
	LOGFONT lf;
	DrawUtil::GetSystemFont(DrawUtil::FONT_MESSAGE,&lf);
	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
}


CEventInfoPopup::~CEventInfoPopup()
{
	Destroy();
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=NULL;
}


bool CEventInfoPopup::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,EVENT_INFO_POPUP_CLASS,NULL,m_hinst);
}


void CEventInfoPopup::SetEventInfo(const CEventInfoData *pEventInfo)
{
	if (m_EventInfo==*pEventInfo)
		return;

	m_EventInfo=*pEventInfo;

	TCHAR szText[2048];
	LPCTSTR pszVideo,pszAudio;

	static const struct {
		BYTE ComponentType;
		LPCTSTR pszText;
	} VideoComponentTypeList[] = {
		{0x01,TEXT("480i(4:3)")},
		{0x03,TEXT("480i(16:9)")},
		{0x04,TEXT("480i(>16:9)")},
		{0xA1,TEXT("480p(4:3)")},
		{0xA3,TEXT("480p(16:9)")},
		{0xA4,TEXT("480p(>16:9)")},
		{0xB1,TEXT("1080i(4:3)")},
		{0xB3,TEXT("1080i(16:9)")},
		{0xB4,TEXT("1080i(>16:9)")},
		{0xC1,TEXT("720p(4:3)")},
		{0xC3,TEXT("720p(16:9)")},
		{0xC4,TEXT("720p(>16:9)")},
		{0xD1,TEXT("240p(4:3)")},
		{0xD3,TEXT("240p(16:9)")},
		{0xD4,TEXT("240p(>16:9)")},
	};
	pszVideo=TEXT("?");
	for (int i=0;i<lengthof(VideoComponentTypeList);i++) {
		if (VideoComponentTypeList[i].ComponentType==m_EventInfo.m_ComponentType) {
			pszVideo=VideoComponentTypeList[i].pszText;
			break;
		}
	}

	static const struct {
		BYTE ComponentType;
		LPCTSTR pszText;
	} AudioComponentTypeList[] = {
		{0x01,TEXT("Mono")},
		{0x02,TEXT("Dual mono")},
		{0x03,TEXT("Stereo")},
		{0x07,TEXT("3+1")},
		{0x08,TEXT("3+2")},
		{0x09,TEXT("5.1ch")},
	};
	if (m_EventInfo.m_AudioComponentType==0x02
			&& m_EventInfo.m_fESMultiLangFlag) {
		pszAudio=TEXT("Mono 2カ国語");
	} else {
		pszAudio=TEXT("?");
		for (int i=0;i<lengthof(AudioComponentTypeList);i++) {
			if (AudioComponentTypeList[i].ComponentType==m_EventInfo.m_AudioComponentType) {
				pszAudio=AudioComponentTypeList[i].pszText;
				break;
			}
		}
	}

	TCHAR szAudioComponent[64];
	LPCTSTR p=m_EventInfo.GetAudioComponentTypeText();
	if (p!=NULL && *p!='\0') {
		szAudioComponent[0]=' ';
		szAudioComponent[1]='(';
		size_t i;
		for (i=2;*p!='\0' && i<lengthof(szAudioComponent)-2;i++) {
			if (*p=='\r' || *p=='\n') {
				szAudioComponent[i]='/';
				p++;
				if (*p=='\n')
					p++;
			} else {
				szAudioComponent[i]=*p++;
			}
		}
		szAudioComponent[i]=')';
		szAudioComponent[i+1]='\0';
	} else {
		szAudioComponent[0]='\0';
	}

	::wnsprintf(szText,lengthof(szText)-1,
		TEXT("%s%s%s%s映像: %s / 音声: %s%s"),
		NullToEmptyString(m_EventInfo.GetEventText()),
		m_EventInfo.GetEventText()!=NULL?TEXT("\r\n\r\n"):TEXT(""),
		NullToEmptyString(m_EventInfo.GetEventExtText()),
		m_EventInfo.GetEventExtText()!=NULL?TEXT("\r\n\r\n"):TEXT(""),
		pszVideo,
		pszAudio,
		szAudioComponent);
	szText[lengthof(szText)-1]='\0';

	LOGFONT lf;
	CHARFORMAT cf;
	HDC hdc=::GetDC(m_hwndEdit);
	m_Font.GetLogFont(&lf);
	CRichEditUtil::LogFontToCharFormat(hdc,&lf,&cf);
	cf.dwMask|=CFM_COLOR;
	cf.crTextColor=m_TextColor;
	::ReleaseDC(m_hwndEdit,hdc);
	::SendMessage(m_hwndEdit,WM_SETREDRAW,FALSE,0);
	::SetWindowText(m_hwndEdit,NULL);
	CRichEditUtil::AppendText(m_hwndEdit,szText,&cf);
	CRichEditUtil::DetectURL(m_hwndEdit,&cf);
	POINT pt={0,0};
	::SendMessage(m_hwndEdit,EM_SETSCROLLPOS,0,reinterpret_cast<LPARAM>(&pt));
	::SendMessage(m_hwndEdit,WM_SETREDRAW,TRUE,0);
	::InvalidateRect(m_hwndEdit,NULL,TRUE);

	CalcTitleHeight();
	RECT rc;
	GetClientRect(&rc);
	::MoveWindow(m_hwndEdit,0,m_TitleHeight,rc.right,max(rc.bottom-m_TitleHeight,0),TRUE);
	Invalidate();
}


void CEventInfoPopup::CalcTitleHeight()
{
	HDC hdc;
	HFONT hfontOld;
	TEXTMETRIC tm;
	int FontHeight;
	RECT rc;

	hdc=::GetDC(m_hwnd);
	if (hdc==NULL)
		return;
	hfontOld=static_cast<HFONT>(::SelectObject(hdc,m_TitleFont.GetHandle()));
	::GetTextMetrics(hdc,&tm);
	//FontHeight=tm.tmHeight+tm.tmInternalLeading;
	FontHeight=tm.tmHeight;
	m_TitleLineHeight=FontHeight+m_TitleLineMargin;
	GetClientRect(&rc);
	m_TitleHeight=(DrawUtil::CalcWrapTextLines(hdc,m_EventInfo.GetEventName(),rc.right)+1)*m_TitleLineHeight;
	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


bool CEventInfoPopup::Show(const CEventInfoData *pEventInfo,const RECT *pPos)
{
	if (pEventInfo==NULL)
		return false;
	bool fExists=m_hwnd!=NULL;
	if (!fExists) {
		if (!Create(NULL,WS_POPUP | WS_CLIPCHILDREN | WS_THICKFRAME,WS_EX_TOPMOST | WS_EX_NOACTIVATE,0))
			return false;
	}
	if (pPos!=NULL) {
		if (!GetVisible())
			SetPosition(pPos);
	} else if (!IsVisible() || m_EventInfo!=*pEventInfo) {
		POINT pt;
		int Width=320,Height=320;

		::GetCursorPos(&pt);
		pt.y+=16;
		HMONITOR hMonitor=::MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST);
		if (hMonitor!=NULL) {
			MONITORINFO mi;

			mi.cbSize=sizeof(mi);
			if (::GetMonitorInfo(hMonitor,&mi)) {
				if (pt.x+Width>mi.rcMonitor.right)
					pt.x=mi.rcMonitor.right-Width;
				if (pt.y+Height>mi.rcMonitor.bottom) {
					pt.y=mi.rcMonitor.bottom-Height;
					pt.x+=16;
				}
			}
		}
		::SetWindowPos(m_hwnd,HWND_TOPMOST,pt.x,pt.y,Width,Height,
					   SWP_NOACTIVATE);
	}
	SetEventInfo(pEventInfo);
	::ShowWindow(m_hwnd,SW_SHOWNA);
	return true;
}


bool CEventInfoPopup::Hide()
{
	if (m_hwnd!=NULL)
		::ShowWindow(m_hwnd,SW_HIDE);
	return true;
}


bool CEventInfoPopup::IsVisible()
{
	return m_hwnd!=NULL && GetVisible();
}


void CEventInfoPopup::SetColor(COLORREF BackColor,COLORREF TextColor)
{
	m_BackColor=BackColor;
	m_TextColor=TextColor;
	if (m_hwnd!=NULL) {
		::SendMessage(m_hwndEdit,EM_SETBKGNDCOLOR,0,m_BackColor);
		//::InvalidateRect(m_hwndEdit,NULL,TRUE);
	}
}


void CEventInfoPopup::SetTitleColor(Theme::GradientInfo *pBackGradient,COLORREF TextColor)
{
	m_TitleBackGradient=*pBackGradient;
	m_TitleTextColor=TextColor;
	if (m_hwnd!=NULL) {
		RECT rc;

		GetClientRect(&rc);
		rc.bottom=m_TitleHeight;
		::InvalidateRect(m_hwnd,&rc,TRUE);
	}
}


bool CEventInfoPopup::SetFont(const LOGFONT *pFont)
{
	LOGFONT lf=*pFont;

	m_Font.Create(&lf);
	lf.lfWeight=FW_BOLD;
	m_TitleFont.Create(&lf);
	if (m_hwnd!=NULL) {
		CalcTitleHeight();
		RECT rc;
		GetClientRect(&rc);
		::MoveWindow(m_hwndEdit,0,m_TitleHeight,rc.right,max(rc.bottom-m_TitleHeight,0),TRUE);
		Invalidate();

		::SendMessage(m_hwndEdit,WM_SETFONT,reinterpret_cast<WPARAM>(m_Font.GetHandle()),TRUE);
	}
	return true;
}


void CEventInfoPopup::SetEventHandler(CEventHandler *pEventHandler)
{
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=NULL;
	if (pEventHandler!=NULL)
		pEventHandler->m_pPopup=this;
	m_pEventHandler=pEventHandler;
}


bool CEventInfoPopup::IsSelected() const
{
	return CRichEditUtil::IsSelected(m_hwndEdit);
}


LPTSTR CEventInfoPopup::GetSelectedText() const
{
	return CRichEditUtil::GetSelectedText(m_hwndEdit);
}


CEventInfoPopup *CEventInfoPopup::GetThis(HWND hwnd)
{
	return static_cast<CEventInfoPopup*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CEventInfoPopup::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CEventInfoPopup *pThis=static_cast<CEventInfoPopup*>(OnCreate(hwnd,lParam));

			pThis->m_RichEditUtil.LoadRichEditLib();
			pThis->m_hwndEdit=::CreateWindowEx(0,TEXT("RichEdit20W"),TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_NOHIDESEL,0,0,0,0,
				hwnd,(HMENU)1,m_hinst,NULL);
			::SendMessage(pThis->m_hwndEdit,WM_SETFONT,reinterpret_cast<WPARAM>(pThis->m_Font.GetHandle()),FALSE);
			::SendMessage(pThis->m_hwndEdit,EM_SETEVENTMASK,0,ENM_MOUSEEVENTS | ENM_LINK);
			::SendMessage(pThis->m_hwndEdit,EM_SETBKGNDCOLOR,0,pThis->m_BackColor);
		}
		return 0;

	case WM_SIZE:
		{
			CEventInfoPopup *pThis=GetThis(hwnd);

			pThis->CalcTitleHeight();
			::MoveWindow(pThis->m_hwndEdit,0,pThis->m_TitleHeight,
						 LOWORD(lParam),max(HIWORD(lParam)-pThis->m_TitleHeight,0),TRUE);
		}
		return 0;

	case WM_PAINT:
		{
			CEventInfoPopup *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			RECT rc;
			HFONT hfontOld;
			int OldBkMode;
			COLORREF OldTextColor;
			TCHAR szText[64];
			int Length;

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rc);
			rc.bottom=pThis->m_TitleHeight;
			Theme::FillGradient(ps.hdc,&rc,&pThis->m_TitleBackGradient);
			hfontOld=static_cast<HFONT>(::SelectObject(ps.hdc,pThis->m_TitleFont.GetHandle()));
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			OldTextColor=::SetTextColor(ps.hdc,pThis->m_TitleTextColor);
			if (pThis->m_EventInfo.m_fValidStartTime) {
				Length=::wsprintf(szText,TEXT("%d/%d/%d(%s) %d:%02d"),
					pThis->m_EventInfo.m_stStartTime.wYear,
					pThis->m_EventInfo.m_stStartTime.wMonth,
					pThis->m_EventInfo.m_stStartTime.wDay,
					GetDayOfWeekText(pThis->m_EventInfo.m_stStartTime.wDayOfWeek),
					pThis->m_EventInfo.m_stStartTime.wHour,
					pThis->m_EventInfo.m_stStartTime.wMinute);
				SYSTEMTIME stEnd;
				if (pThis->m_EventInfo.m_DurationSec>0
						&& pThis->m_EventInfo.GetEndTime(&stEnd))
					Length+=::wsprintf(szText+Length,TEXT("～%d:%02d"),stEnd.wHour,stEnd.wMinute);
				::TextOut(ps.hdc,0,0,szText,Length);
			}
			rc.top+=pThis->m_TitleLineHeight;
			DrawUtil::DrawWrapText(ps.hdc,pThis->m_EventInfo.GetEventName(),&rc,pThis->m_TitleLineHeight);
			::SelectObject(ps.hdc,hfontOld);
			::SetBkMode(ps.hdc,OldBkMode);
			::SetTextColor(ps.hdc,OldTextColor);
			pThis->GetCloseButtonRect(&rc);
			::DrawFrameControl(ps.hdc,&rc,DFC_CAPTION,DFCS_CAPTIONCLOSE | DFCS_MONO);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) {
			CEventInfoPopup *pThis=GetThis(hwnd);

			pThis->Hide();
		}
		return 0;

	case WM_ACTIVATEAPP:
		if (wParam==0) {
			CEventInfoPopup *pThis=GetThis(hwnd);

			pThis->Hide();
		}
		return 0;

	case WM_NCHITTEST:
		{
			CEventInfoPopup *pThis=GetThis(hwnd);
			POINT pt;
			RECT rc;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::ScreenToClient(hwnd,&pt);
			pThis->GetCloseButtonRect(&rc);
			if (::PtInRect(&rc,pt))
				return HTCLOSE;
			::GetClientRect(hwnd,&rc);
			rc.bottom=pThis->m_TitleHeight;
			if (::PtInRect(&rc,pt))
				return HTCAPTION;
		}
		break;

	case WM_NCLBUTTONDOWN:
		if (wParam==HTCLOSE) {
			::SendMessage(hwnd,WM_CLOSE,0,0);
			return 0;
		}
		break;

	case WM_MOUSEWHEEL:
		return ::SendMessage(GetThis(hwnd)->m_hwndEdit,uMsg,wParam,lParam);

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case EN_MSGFILTER:
			if (reinterpret_cast<MSGFILTER*>(lParam)->msg==WM_RBUTTONDOWN) {
				CEventInfoPopup *pThis=GetThis(hwnd);
				HMENU hmenu=::CreatePopupMenu();
				POINT pt;

				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("コピー(&C)"));
				::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,2,TEXT("すべて選択(&A)"));
				if (pThis->m_pEventHandler!=NULL)
					pThis->m_pEventHandler->OnMenuPopup(hmenu);
				::GetCursorPos(&pt);
				int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
				switch (Command) {
				case 1:
					if (::SendMessage(pThis->m_hwndEdit,EM_SELECTIONTYPE,0,0)==SEL_EMPTY) {
						CRichEditUtil::CopyAllText(pThis->m_hwndEdit);
					} else {
						::SendMessage(pThis->m_hwndEdit,WM_COPY,0,0);
					}
					break;
				case 2:
					CRichEditUtil::SelectAll(pThis->m_hwndEdit);
					break;
				default:
					if (Command>=CEventHandler::COMMAND_FIRST)
						pThis->m_pEventHandler->OnMenuSelected(Command);
					break;
				}
				::DestroyMenu(hmenu);
			}
			return 0;

		case EN_LINK:
			{
				ENLINK *penl=reinterpret_cast<ENLINK*>(lParam);

				if (penl->msg==WM_LBUTTONUP)
					CRichEditUtil::HandleLinkClick(penl);
			}
			return 0;
		}
		break;

	case WM_CLOSE:
		{
			CEventInfoPopup *pThis=GetThis(hwnd);

			pThis->Hide();
		}
		return 0;

	case WM_DESTROY:
		{
			CEventInfoPopup *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


/*
LRESULT CALLBACK CEventInfoPopup::EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CEventInfoPopup *pThis=(CEventInfoPopup*)::GetProp(hwnd,m_pszPropName);

	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	switch (uMsg) {
	case WM_RBUTTONDOWN:
		{
			HMENU hmenu=::CreatePopupMenu();

			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("コピー(&C)"));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,2,TEXT("すべて選択(&A)"));
			POINT pt;
			::GetCursorPos(&pt);
			int Command=::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL);
			if (Command==1) {
				DWORD Start,End;

				::SendMessage(hwnd,WM_SETREDRAW,FALSE,0);
				::SendMessage(hwnd,EM_GETSEL,(WPARAM)&Start,(LPARAM)&End);
				if (Start==End)
					::SendMessage(hwnd,EM_SETSEL,0,-1);
				::SendMessage(hwnd,WM_COPY,0,0);
				if (Start==End)
					::SendMessage(hwnd,EM_SETSEL,Start,End);
				::SendMessage(hwnd,WM_SETREDRAW,TRUE,0);
			} else if (Command==2) {
				::SendMessage(hwnd,EM_SETSEL,0,-1);
			}
		}
		return 0;

	case WM_RBUTTONUP:
		return 0;

	case WM_NCDESTROY:
		::RemoveProp(hwnd,m_pszPropName);
		break;
	}
	return ::CallWindowProc(pThis->m_pOldEditProc,hwnd,uMsg,wParam,lParam);
}
*/


void CEventInfoPopup::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	GetClientRect(&rc);
	rc.right-=m_ButtonMargin;
	rc.left=rc.right-m_ButtonSize;
	rc.top=m_ButtonMargin;
	rc.bottom=rc.top+m_ButtonSize;
	*pRect=rc;
}




CEventInfoPopup::CEventHandler::CEventHandler()
	: m_pPopup(NULL)
{
}


CEventInfoPopup::CEventHandler::~CEventHandler()
{
	if (m_pPopup!=NULL)
		m_pPopup->m_pEventHandler=NULL;
}




const LPCTSTR CEventInfoPopupManager::m_pszPropName=TEXT("EventInfoPopup");


CEventInfoPopupManager::CEventInfoPopupManager(CEventInfoPopup *pPopup)
	: m_pPopup(pPopup)
	, m_hwnd(NULL)
	, m_pOldWndProc(NULL)
	, m_pEventHandler(NULL)
	, m_HitTestParam(-1)
{
}


CEventInfoPopupManager::~CEventInfoPopupManager()
{
	Finalize();
}


bool CEventInfoPopupManager::Initialize(HWND hwnd,CEventHandler *pEventHandler)
{
	if (hwnd==NULL)
		return false;
	m_hwnd=hwnd;
	m_pOldWndProc=(WNDPROC)::SetWindowLongPtr(hwnd,GWLP_WNDPROC,(LONG_PTR)HookWndProc);
	m_pEventHandler=pEventHandler;
	if (m_pEventHandler!=NULL)
		m_pEventHandler->m_pPopup=m_pPopup;
	m_fTrackMouseEvent=false;
	::SetProp(hwnd,m_pszPropName,this);
	return true;
}


void CEventInfoPopupManager::Finalize()
{
	if (m_hwnd!=NULL) {
		if (m_pOldWndProc!=NULL) {
			::SetWindowLongPtr(m_hwnd,GWLP_WNDPROC,(LONG_PTR)m_pOldWndProc);
			m_pOldWndProc=NULL;
		}
		::RemoveProp(m_hwnd,m_pszPropName);
		m_hwnd=NULL;
	}
}


LRESULT CALLBACK CEventInfoPopupManager::HookWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CEventInfoPopupManager *pThis=(CEventInfoPopupManager*)::GetProp(hwnd,m_pszPropName);

	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	switch (uMsg) {
	case WM_MOUSEMOVE:
		if (!pThis->m_fTrackMouseEvent) {
			TRACKMOUSEEVENT tme;

			tme.cbSize=sizeof(tme);
			tme.dwFlags=TME_HOVER | TME_LEAVE;
			tme.hwndTrack=hwnd;
			tme.dwHoverTime=1000;
			if (::TrackMouseEvent(&tme))
				pThis->m_fTrackMouseEvent=true;
		}
		if (pThis->m_pPopup->IsVisible() && pThis->m_pEventHandler!=NULL) {
			LPARAM Param;
			if (pThis->m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&Param)) {
				if (Param!=pThis->m_HitTestParam) {
					const CEventInfoData *pEventInfo;

					pThis->m_HitTestParam=Param;
					if (pThis->m_pEventHandler->GetEventInfo(pThis->m_HitTestParam,&pEventInfo)
							&& pThis->m_pEventHandler->OnShow(pEventInfo))
						pThis->m_pPopup->Show(pEventInfo);
				}
			} else {
				pThis->m_pPopup->Hide();
			}
		}
		break;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_NCLBUTTONDOWN:
	case WM_NCRBUTTONDOWN:
	case WM_NCMBUTTONDOWN:
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
	case WM_VSCROLL:
	case WM_HSCROLL:
		pThis->m_pPopup->Hide();
		break;

	case WM_MOUSELEAVE:
		if (pThis->m_pPopup->IsVisible()) {
			POINT pt;
			::GetCursorPos(&pt);
			HWND hwndCur=::WindowFromPoint(pt);
			if (!pThis->m_pPopup->IsHandle(hwndCur))
				pThis->m_pPopup->Hide();
		}
		pThis->m_fTrackMouseEvent=false;
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam)==WA_INACTIVE) {
			HWND hwndActive=reinterpret_cast<HWND>(lParam);
			if (!pThis->m_pPopup->IsHandle(hwndActive))
				pThis->m_pPopup->Hide();
		}
		break;

	case WM_MOUSEHOVER:
		if (pThis->m_pEventHandler!=NULL
				&& ::GetActiveWindow()==::GetForegroundWindow()) {
			bool fHit=false;
			pThis->m_HitTestParam=-1;
			if (pThis->m_pEventHandler->HitTest(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam),&pThis->m_HitTestParam)) {
				const CEventInfoData *pEventInfo;

				if (pThis->m_pEventHandler->GetEventInfo(pThis->m_HitTestParam,&pEventInfo)
						&& pThis->m_pEventHandler->OnShow(pEventInfo)) {
					pThis->m_pPopup->Show(pEventInfo);
					fHit=true;
				}
			}
			if (!fHit)
				pThis->m_pPopup->Hide();
		}
		pThis->m_fTrackMouseEvent=false;
		return 0;

	case WM_DESTROY:
		::CallWindowProc(pThis->m_pOldWndProc,hwnd,uMsg,wParam,lParam);
		pThis->Finalize();
		return 0;
	}
	return ::CallWindowProc(pThis->m_pOldWndProc,hwnd,uMsg,wParam,lParam);
}




CEventInfoPopupManager::CEventHandler::CEventHandler()
	: m_pPopup(NULL)
{
}


CEventInfoPopupManager::CEventHandler::~CEventHandler()
{
}
