#include "stdafx.h"
#include "TVTest.h"
#include "DisplayMenu.h"
#include "AppMain.h"
#include "Settings.h"




const LPCTSTR CChannelDisplayMenu::m_pszWindowClass=APP_NAME TEXT(" Channel Display");
HINSTANCE CChannelDisplayMenu::m_hinst=NULL;


bool CChannelDisplayMenu::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszWindowClass;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CChannelDisplayMenu::CChannelDisplayMenu(CEpgProgramList *pEpgProgramList)
	: m_TunerAreaBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_HORZ,RGB(80,80,80),RGB(64,64,64))
	, m_ChannelAreaBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_HORZ,RGB(80,80,80),RGB(64,64,64))
	, m_TunerBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,RGB(32,32,32),RGB(32,32,32))
	, m_TunerTextColor(RGB(255,255,255))
	, m_CurTunerBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,RGB(128,128,128),RGB(96,96,96))
	, m_CurTunerTextColor(RGB(255,255,255))
	, m_ChannelTextColor(RGB(255,255,255))
	, m_CurChannelBackGradient(Theme::GRADIENT_NORMAL,Theme::DIRECTION_VERT,RGB(128,128,128),RGB(96,96,96))
	, m_CurChannelTextColor(RGB(255,255,255))
	, m_fAutoFontSize(true)
	, m_hwndTunerScroll(NULL)
	, m_hwndChannelScroll(NULL)
	, m_TotalTuningSpaces(0)
	, m_CurTuner(-1)
	, m_CurChannel(-1)
	, m_pEpgProgramList(pEpgProgramList)
	, m_pEventHandler(NULL)
{
	m_ChannelBackGradient[0].Type=Theme::GRADIENT_NORMAL;
	m_ChannelBackGradient[0].Direction=Theme::DIRECTION_VERT;
	m_ChannelBackGradient[0].Color1=RGB(16,16,16);
	m_ChannelBackGradient[0].Color2=RGB(16,16,16);
	m_ChannelBackGradient[1].Type=Theme::GRADIENT_NORMAL;
	m_ChannelBackGradient[1].Direction=Theme::DIRECTION_VERT;
	m_ChannelBackGradient[1].Color1=RGB(48,48,48);
	m_ChannelBackGradient[1].Color2=RGB(48,48,48);
}


CChannelDisplayMenu::~CChannelDisplayMenu()
{
	Clear();
}


bool CChannelDisplayMenu::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,m_pszWindowClass,NULL,m_hinst);
}


void CChannelDisplayMenu::Clear()
{
	m_TunerList.DeleteAll();
	m_TotalTuningSpaces=0;
	m_CurTuner=-1;
	m_CurChannel=-1;
}


bool CChannelDisplayMenu::SetDriverManager(CDriverManager *pDriverManager)
{
	Clear();
	LoadSettings();
	for (int i=0;i<pDriverManager->NumDrivers();i++) {
		CDriverInfo *pDriverInfo=pDriverManager->GetDriverInfo(i);
		const TunerInfo *pTunerInfo=NULL;

		LPCTSTR pszFileName=::PathFindFileName(pDriverInfo->GetFileName());
		for (size_t j=0;j<m_TunerInfoList.size();j++) {
			const TunerInfo &Info=m_TunerInfoList[j];
			LPCTSTR p=Info.DriverMasks;
			while (*p!='\0') {
				if (::PathMatchSpec(pszFileName,p)) {
					pTunerInfo=&Info;
					goto End;
				}
				p+=::lstrlen(p)+1;
			}
		}
	End:
		bool fUseDriverChannel=pTunerInfo!=NULL && pTunerInfo->fUseDriverChannel;
		pDriverInfo->LoadTuningSpaceList(
			fUseDriverChannel?
				CDriverInfo::LOADTUNINGSPACE_DEFAULT:
				CDriverInfo::LOADTUNINGSPACE_NOLOADDRIVER);
		CTuner *pTuner=new CTuner(pDriverInfo);
		if (pTunerInfo!=NULL) {
			if (pTunerInfo->szDisplayName[0]!='\0')
				pTuner->SetDisplayName(pTunerInfo->szDisplayName);
			if (pTunerInfo->szIconFile[0]!='\0') {
				HICON hico=::ExtractIcon(GetAppClass().GetInstance(),
										 pTunerInfo->szIconFile,pTunerInfo->Index);
				if (hico!=NULL && hico!=(HICON)1)
					pTuner->SetIcon(hico);
			}
		}
		m_TunerList.Add(pTuner);
		m_TotalTuningSpaces+=pTuner->NumSpaces();
	}
	if (m_hwnd!=NULL) {
		Layout();
		Invalidate();
	}
	return true;
}


void CChannelDisplayMenu::SetEventHandler(CEventHandler *pEventHandler)
{
	m_pEventHandler=pEventHandler;
}


bool CChannelDisplayMenu::SetSelect(LPCTSTR pszDriverFileName,const CChannelInfo *pChannelInfo)
{
	int TunerIndex=0;
	for (int i=0;i<m_TunerList.Length();i++) {
		const CTuner *pTuner=m_TunerList[i];
		if (::lstrcmpi(pTuner->GetDriverFileName(),pszDriverFileName)==0) {
			int Space=0,Channel=-1;
			if (pChannelInfo!=NULL) {
				for (int j=0;j<pTuner->NumSpaces();j++) {
					const CChannelList *pChannelList=pTuner->GetTuningSpaceInfo(j)->GetChannelList();
					if (pChannelList!=NULL) {
						Channel=pChannelList->Find(pChannelInfo->GetSpace(),
												   pChannelInfo->GetChannelIndex(),
												   pChannelInfo->GetServiceID());
						if (Channel>=0) {
							Space=j;
							break;
						}
					}
				}
			}
			SetCurTuner(TunerIndex+Space,true);
			if (Channel>=0)
				SetCurChannel(Channel);
			return true;
		}
		TunerIndex+=pTuner->NumSpaces();
	}
	return false;
}


bool CChannelDisplayMenu::SetFont(const LOGFONT *pFont,bool fAutoSize)
{
	if (!m_Font.Create(pFont))
		return false;
	m_fAutoFontSize=fAutoSize;
	if (m_hwnd!=NULL) {
		Layout();
		Invalidate();
	}
	return true;
}


bool CChannelDisplayMenu::IsMessageNeed(const MSG *pmsg) const
{
	if (pmsg->message==WM_KEYDOWN || pmsg->message==WM_KEYUP) {
		switch (pmsg->wParam) {
		case VK_LEFT:
		case VK_RIGHT:
		case VK_UP:
		case VK_DOWN:
		case VK_RETURN:
		case VK_SPACE:
		case VK_PRIOR:
		case VK_NEXT:
		case VK_HOME:
		case VK_END:
		case VK_ESCAPE:
			return true;
		}
	}
	return false;
}


void CChannelDisplayMenu::LoadSettings()
{
	TCHAR szIniFileName[MAX_PATH];
	CSettings Settings;

	GetAppClass().GetAppDirectory(szIniFileName);
	::PathAppend(szIniFileName,TEXT("Tuner.ini"));
	if (Settings.Open(szIniFileName,TEXT("TunerSettings"),CSettings::OPEN_READ)) {
		m_TunerInfoList.clear();
		for (int i=0;;i++) {
			TCHAR szName[32],*p;
			TunerInfo Info;

			::wsprintf(szName,TEXT("Tuner%d_Driver"),i);
			if (!Settings.Read(szName,Info.DriverMasks,lengthof(Info.DriverMasks)-1))
				break;
			p=Info.DriverMasks;
			while (*p!='\0') {
				if (*p=='|')
					*p='\0';
				p++;
			}
			*(p+1)='\0';
			::wsprintf(szName,TEXT("Tuner%d_Name"),i);
			if (!Settings.Read(szName,Info.szDisplayName,lengthof(Info.szDisplayName)))
				Info.szDisplayName[0]='\0';
			::wsprintf(szName,TEXT("Tuner%d_Icon"),i);
			if (!Settings.Read(szName,Info.szIconFile,lengthof(Info.szIconFile)))
				Info.szIconFile[0]='\0';
			if (::PathIsRelative(Info.szIconFile)) {
				TCHAR szPath[MAX_PATH];

				GetAppClass().GetAppDirectory(szPath);
				::PathAppend(szPath,Info.szIconFile);
				::PathCanonicalize(Info.szIconFile,szPath);
			}
			Info.Index=0;
			p=Info.szIconFile;
			while (*p!='\0') {
				if (*p=='|') {
					*p='\0';
					Info.Index=::StrToInt(p+1);
					break;
				}
				p++;
			}
			::wsprintf(szName,TEXT("Tuner%d_UseDriverChannel"),i);
			if (!Settings.Read(szName,&Info.fUseDriverChannel))
				Info.fUseDriverChannel=false;
			m_TunerInfoList.push_back(Info);
		}
		Settings.Close();
	}
}


void CChannelDisplayMenu::Layout()
{
	RECT rc;
	GetClientRect(&rc);

	if (m_fAutoFontSize) {
		LOGFONT lf;
		m_Font.GetLogFont(&lf);
		lf.lfHeight=max(min(rc.bottom/24,rc.right/32),12);
		lf.lfWidth=0;
		m_Font.Create(&lf);
	}

	HDC hdc=::GetDC(m_hwnd);
	HFONT hfontOld=SelectFont(hdc,m_Font.GetHandle());
	TEXTMETRIC tm;
	::GetTextMetrics(hdc,&tm);
	m_FontHeight=tm.tmHeight/*+tm.tmInternalLeading*/;

	m_TunerItemWidth=m_FontHeight*10;
	m_TunerItemHeight=max(32,m_FontHeight)+8;
	m_VisibleTunerItems=(rc.bottom-32*2)/m_TunerItemHeight;
	if (m_VisibleTunerItems<1)
		m_VisibleTunerItems=1;
	else if (m_VisibleTunerItems>m_TotalTuningSpaces)
		m_VisibleTunerItems=m_TotalTuningSpaces;
	m_TunerItemLeft=8;
	m_TunerItemTop=max((rc.bottom-m_VisibleTunerItems*m_TunerItemHeight)/2,0);
	m_TunerAreaWidth=m_TunerItemLeft+m_TunerItemWidth+8;
	int ScrollWidth=::GetSystemMetrics(SM_CXVSCROLL);
	if (m_TotalTuningSpaces>m_VisibleTunerItems) {
		SCROLLINFO si;

		if (m_TunerScrollPos>m_TotalTuningSpaces-m_VisibleTunerItems)
			m_TunerScrollPos=m_TotalTuningSpaces-m_VisibleTunerItems;
		si.cbSize=sizeof(si);
		si.fMask=SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin=0;
		si.nMax=m_TotalTuningSpaces-1;
		si.nPage=m_VisibleTunerItems;
		si.nPos=m_TunerScrollPos;
		::SetScrollInfo(m_hwndTunerScroll,SB_CTL,&si,TRUE);
		m_TunerItemWidth-=ScrollWidth;
		::MoveWindow(m_hwndTunerScroll,
					 m_TunerItemLeft+m_TunerItemWidth,m_TunerItemTop,
					 ScrollWidth,m_VisibleTunerItems*m_TunerItemHeight,TRUE);
		::ShowWindow(m_hwndTunerScroll,SW_SHOW);
	} else {
		m_TunerScrollPos=0;
		::ShowWindow(m_hwndTunerScroll,SW_HIDE);
	}

	m_ChannelNameWidth=0;
	int NumChannels=0;
	if (m_CurTuner>=0) {
		const CTuningSpaceInfo *pTuningSpace=GetTuningSpaceInfo(m_CurTuner);
		if (pTuningSpace!=NULL) {
			NumChannels=pTuningSpace->NumChannels();
			if (NumChannels>0) {
				const CChannelList *pChannelList=pTuningSpace->GetChannelList();
				for (int i=0;i<NumChannels;i++) {
					LPCTSTR pszName=pChannelList->GetName(i);
					SIZE sz;
					if (::GetTextExtentPoint32(hdc,pszName,::lstrlen(pszName),&sz)
							&& sz.cx>m_ChannelNameWidth)
						m_ChannelNameWidth=sz.cx;
				}
				if (m_ChannelNameWidth>m_FontHeight*12)
					m_ChannelNameWidth=m_FontHeight*12;
			}
		}
	}
	m_ChannelNameWidth+=8*2;
	m_ChannelItemLeft=m_TunerAreaWidth+8;
	m_ChannelItemWidth=max(rc.right-m_ChannelItemLeft-8,m_ChannelNameWidth+80);
	m_ChannelItemHeight=m_FontHeight*2;
	m_VisibleChannelItems=max((rc.bottom-16*2)/m_ChannelItemHeight,1);
	if (m_VisibleChannelItems>NumChannels)
		m_VisibleChannelItems=NumChannels;
	m_ChannelItemTop=max((rc.bottom-m_VisibleChannelItems*m_ChannelItemHeight)/2,0);
	if (NumChannels>m_VisibleChannelItems) {
		SCROLLINFO si;

		if (m_ChannelScrollPos>NumChannels-m_VisibleChannelItems)
			m_ChannelScrollPos=NumChannels-m_VisibleChannelItems;
		si.cbSize=sizeof(si);
		si.fMask=SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin=0;
		si.nMax=NumChannels-1;
		si.nPage=m_VisibleChannelItems;
		si.nPos=m_ChannelScrollPos;
		::SetScrollInfo(m_hwndChannelScroll,SB_CTL,&si,TRUE);
		m_ChannelItemWidth-=ScrollWidth;
		::MoveWindow(m_hwndChannelScroll,
					 m_ChannelItemLeft+m_ChannelItemWidth,m_ChannelItemTop,
					 ScrollWidth,m_VisibleChannelItems*m_ChannelItemHeight,TRUE);
		::ShowWindow(m_hwndChannelScroll,SW_SHOW);
	} else {
		m_ChannelScrollPos=0;
		::ShowWindow(m_hwndChannelScroll,SW_HIDE);
	}

	::SelectObject(hdc,hfontOld);
	::ReleaseDC(m_hwnd,hdc);
}


const CTuningSpaceInfo *CChannelDisplayMenu::GetTuningSpaceInfo(int Index) const
{
	int TunerIndex=0;
	for (int i=0;i<m_TunerList.Length();i++) {
		const CTuner *pTuner=m_TunerList[i];

		if (Index>=TunerIndex && Index<TunerIndex+pTuner->NumSpaces())
			return pTuner->GetTuningSpaceInfo(Index-TunerIndex);
		TunerIndex+=pTuner->NumSpaces();
	}
	return NULL;
}


const CChannelDisplayMenu::CTuner *CChannelDisplayMenu::GetTuner(int Index,int *pSpace) const
{
	int TunerIndex=0;
	for (int i=0;i<m_TunerList.Length();i++) {
		const CTuner *pTuner=m_TunerList[i];

		if (Index>=TunerIndex && Index<TunerIndex+pTuner->NumSpaces()) {
			if (pSpace!=NULL)
				*pSpace=Index-TunerIndex;
			return pTuner;
		}
		TunerIndex+=pTuner->NumSpaces();
	}
	return NULL;
}


void CChannelDisplayMenu::GetTunerItemRect(int Index,RECT *pRect) const
{
	pRect->left=m_TunerItemLeft;
	pRect->top=m_TunerItemTop+(Index-m_TunerScrollPos)*m_TunerItemHeight;
	pRect->right=pRect->left+m_TunerItemWidth;
	pRect->bottom=pRect->top+m_TunerItemHeight;
}


void CChannelDisplayMenu::GetChannelItemRect(int Index,RECT *pRect) const
{
	pRect->left=m_ChannelItemLeft;
	pRect->top=m_ChannelItemTop+(Index-m_ChannelScrollPos)*m_ChannelItemHeight;
	pRect->right=pRect->left+m_ChannelItemWidth;
	pRect->bottom=pRect->top+m_ChannelItemHeight;
}


void CChannelDisplayMenu::UpdateTunerItem(int Index) const
{
	RECT rc;
	GetTunerItemRect(Index,&rc);
	::InvalidateRect(m_hwnd,&rc,TRUE);
}


void CChannelDisplayMenu::UpdateChannelItem(int Index) const
{
	RECT rc;
	GetChannelItemRect(Index,&rc);
	::InvalidateRect(m_hwnd,&rc,TRUE);
}


int CChannelDisplayMenu::TunerItemHitTest(int x,int y) const
{
	if (x>=m_TunerItemLeft && x<m_TunerItemLeft+m_TunerItemWidth
			&& y>=m_TunerItemTop && y<m_TunerItemTop+m_VisibleTunerItems*m_TunerItemHeight) {
		const int Index=(y-m_TunerItemTop)/m_TunerItemHeight+m_TunerScrollPos;
		if (Index<m_TotalTuningSpaces)
			return Index;
	}
	return -1;
}


int CChannelDisplayMenu::ChannelItemHitTest(int x,int y) const
{
	if (m_CurTuner>=0
			&& x>=m_ChannelItemLeft && x<m_ChannelItemLeft+m_ChannelItemWidth
			&& y>=m_ChannelItemTop && y<m_ChannelItemTop+m_VisibleChannelItems*m_ChannelItemHeight) {
		const int Index=(y-m_ChannelItemTop)/m_ChannelItemHeight+m_ChannelScrollPos;
		const CTuningSpaceInfo *pTuningSpace=GetTuningSpaceInfo(m_CurTuner);

		if (pTuningSpace!=NULL && Index<pTuningSpace->NumChannels())
			return Index;
	}
	return -1;
}


bool CChannelDisplayMenu::SetCurTuner(int Index,bool fUpdate)
{
	if (Index<-1 || Index>=m_TotalTuningSpaces)
		return false;
	if (Index!=m_CurTuner || fUpdate) {
		if (Index!=m_CurTuner) {
			m_CurTuner=Index;
			if (Index>=0) {
				if (Index<m_TunerScrollPos)
					SetTunerScrollPos(Index,false);
				else if (Index>=m_TunerScrollPos+m_VisibleTunerItems)
					SetTunerScrollPos(Index-(m_VisibleTunerItems-1),false);
			}
		}
		m_CurChannel=-1;
		m_ChannelScrollPos=0;
		::GetLocalTime(&m_EpgBaseTime);
		m_EpgBaseTime.wSecond=0;
		m_EpgBaseTime.wMilliseconds=0;
		Layout();
		Invalidate();
	}
	return true;
}


bool CChannelDisplayMenu::SetCurChannel(int Index)
{
	const CTuningSpaceInfo *pTuningSpace=GetTuningSpaceInfo(m_CurTuner);

	if (pTuningSpace==NULL || Index<-1 || Index>=pTuningSpace->NumChannels())
		return false;
	if (Index!=m_CurChannel) {
		if (m_CurChannel>=0)
			UpdateChannelItem(m_CurChannel);
		m_CurChannel=Index;
		if (Index>=0) {
			UpdateChannelItem(Index);

			int ScrollPos=m_ChannelScrollPos;
			if (Index<ScrollPos)
				ScrollPos=Index;
			else if (Index>=ScrollPos+m_VisibleChannelItems)
				ScrollPos=Index-(m_VisibleChannelItems-1);
			if (ScrollPos!=m_ChannelScrollPos) {
				Update();
				SetChannelScrollPos(ScrollPos,true);
			}
		}
	}
	return true;
}


void CChannelDisplayMenu::SetTunerScrollPos(int Pos,bool fScroll)
{
	if (Pos<0)
		Pos=0;
	else if (Pos>m_TotalTuningSpaces-m_VisibleTunerItems)
		Pos=m_TotalTuningSpaces-m_VisibleTunerItems;
	if (Pos!=m_TunerScrollPos) {
		SCROLLINFO si;

		si.cbSize=sizeof(si);
		si.fMask=SIF_POS;
		si.nPos=Pos;
		::SetScrollInfo(m_hwndTunerScroll,SB_CTL,&si,TRUE);
		if (fScroll) {
			RECT rc;
			::SetRect(&rc,m_TunerItemLeft,m_TunerItemTop,
					  m_TunerItemLeft+m_TunerItemWidth,
					  m_TunerItemTop+m_TunerItemHeight*m_VisibleTunerItems);
			::ScrollWindowEx(m_hwnd,0,(m_TunerScrollPos-Pos)*m_TunerItemHeight,
							 &rc,&rc,NULL,NULL,SW_INVALIDATE);
		}
		m_TunerScrollPos=Pos;
	}
}


void CChannelDisplayMenu::SetChannelScrollPos(int Pos,bool fScroll)
{
	int NumChannels=0;
	if (m_CurTuner>=0) {
		const CTuningSpaceInfo *pTuningSpace=GetTuningSpaceInfo(m_CurTuner);
		if (pTuningSpace!=NULL)
			NumChannels=pTuningSpace->NumChannels();
	}
	if (Pos<0)
		Pos=0;
	else if (Pos>NumChannels-m_VisibleChannelItems)
		Pos=NumChannels-m_VisibleChannelItems;
	if (Pos!=m_ChannelScrollPos) {
		SCROLLINFO si;

		si.cbSize=sizeof(si);
		si.fMask=SIF_POS;
		si.nPos=Pos;
		::SetScrollInfo(m_hwndChannelScroll,SB_CTL,&si,TRUE);
		if (fScroll) {
			RECT rc;
			::SetRect(&rc,m_ChannelItemLeft,m_ChannelItemTop,
					  m_ChannelItemLeft+m_ChannelItemWidth,
					  m_ChannelItemTop+m_ChannelItemHeight*m_VisibleChannelItems);
			::ScrollWindowEx(m_hwnd,0,(m_ChannelScrollPos-Pos)*m_ChannelItemHeight,
							 &rc,&rc,NULL,NULL,SW_INVALIDATE);
		}
		m_ChannelScrollPos=Pos;
	}
}


void CChannelDisplayMenu::GetCloseButtonRect(RECT *pRect) const
{
	RECT rc;

	GetClientRect(&rc);
	pRect->right=rc.right-1;
	pRect->left=pRect->right-14;
	pRect->top=1;
	pRect->bottom=pRect->top+14;
}


bool CChannelDisplayMenu::CloseButtonHitTest(int x,int y) const
{
	RECT rc;
	POINT pt;

	GetCloseButtonRect(&rc);
	pt.x=x;
	pt.y=y;
	return ::PtInRect(&rc,pt)!=FALSE;
}


void CChannelDisplayMenu::NotifyTunerSelect() const
{
	int Space;
	const CTuner *pTuner=GetTuner(m_CurTuner,&Space);
	if (pTuner==NULL)
		return;
	const CTuningSpaceInfo *pTuningSpace=pTuner->GetTuningSpaceInfo(Space);

	if (pTuningSpace!=NULL && pTuningSpace->NumChannels()>0) {
		const CChannelList *pChannelList=pTuningSpace->GetChannelList();

		for (int i=0;i<pChannelList->NumChannels();i++) {
			int ChannelSpace=pChannelList->GetSpace(i);
			if (i==0) {
				Space=ChannelSpace;
			} else {
				if (Space!=ChannelSpace) {
					Space=CEventHandler::SPACE_ALL;
					break;
				}
			}
		}
	} else {
		Space=CEventHandler::SPACE_NOTSPECIFIED;
	}
	m_pEventHandler->OnTunerSelect(pTuner->GetDriverFileName(),Space);
}


void CChannelDisplayMenu::NotifyChannelSelect() const
{
	const CTuner *pTuner=GetTuner(m_CurTuner);
	if (pTuner==NULL)
		return;
	const CTuningSpaceInfo *pTuningSpace=GetTuningSpaceInfo(m_CurTuner);

	m_pEventHandler->OnChannelSelect(pTuner->GetDriverFileName(),
		pTuningSpace->GetChannelList()->GetChannelInfo(m_CurChannel));
}


CChannelDisplayMenu *CChannelDisplayMenu::GetThis(HWND hwnd)
{
	return static_cast<CChannelDisplayMenu*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CChannelDisplayMenu::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CChannelDisplayMenu *pThis=static_cast<CChannelDisplayMenu*>(OnCreate(hwnd,lParam));

			if (!pThis->m_Font.IsCreated())
				pThis->m_Font.Create(/*DrawUtil::FONT_DEFAULT*/DrawUtil::FONT_MESSAGE);
			pThis->m_hwndTunerScroll=::CreateWindowEx(0,TEXT("SCROLLBAR"),TEXT(""),
				WS_CHILD | SBS_VERT,0,0,0,0,hwnd,NULL,m_hinst,NULL);
			pThis->m_hwndChannelScroll=::CreateWindowEx(0,TEXT("SCROLLBAR"),TEXT(""),
				WS_CHILD | SBS_VERT,0,0,0,0,hwnd,NULL,m_hinst,NULL);
			pThis->m_TunerScrollPos=0;
			pThis->m_ChannelScrollPos=0;
			pThis->m_CurTuner=-1;
			pThis->m_CurChannel=-1;
			pThis->m_LastCursorPos.x=-1;
			pThis->m_LastCursorPos.y=-1;
		}
		return 0;

	case WM_SIZE:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);

			pThis->Layout();
		}
		return 0;

	case WM_PAINT:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			RECT rcClient,rc;
			HFONT hfontOld;
			COLORREF OldTextColor;
			int OldBkMode;
			TCHAR szText[1024];

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rcClient);
			hfontOld=SelectFont(ps.hdc,pThis->m_Font.GetHandle());
			OldTextColor=::GetTextColor(ps.hdc);
			OldBkMode=::SetBkMode(ps.hdc,TRANSPARENT);
			if (ps.rcPaint.left<pThis->m_TunerAreaWidth) {
				rc.left=rcClient.left;
				rc.top=rcClient.top;
				rc.right=pThis->m_TunerAreaWidth;
				rc.bottom=rcClient.bottom;
				Theme::FillGradient(ps.hdc,&rc,&pThis->m_TunerAreaBackGradient);
				int TunerIndex=0;
				for (int i=0;i<pThis->m_TunerList.Length();i++) {
					const CTuner *pTuner=pThis->m_TunerList[i];

					for (int j=0;j<pTuner->NumSpaces();j++) {
						if (TunerIndex>=pThis->m_TunerScrollPos
								&& TunerIndex<pThis->m_TunerScrollPos+pThis->m_VisibleTunerItems) {
							const CTuningSpaceInfo *pTuningSpace=pTuner->GetTuningSpaceInfo(j);
							const Theme::GradientInfo *pGradient;
							COLORREF TextColor;
							if (TunerIndex==pThis->m_CurTuner) {
								pGradient=&pThis->m_CurTunerBackGradient;
								TextColor=pThis->m_CurTunerTextColor;
							} else {
								pGradient=&pThis->m_TunerBackGradient;
								TextColor=pThis->m_TunerTextColor;
							}
							pThis->GetTunerItemRect(TunerIndex,&rc);
							Theme::FillGradient(ps.hdc,&rc,pGradient);
							if (pTuner->GetIcon()!=NULL)
								::DrawIconEx(ps.hdc,
											 rc.left+8,
											 rc.top+((rc.bottom-rc.top)-32)/2,
											 pTuner->GetIcon(),
											 32,32,0,NULL,DI_NORMAL);
							::SetTextColor(ps.hdc,TextColor);
							if (pTuner->GetDisplayName()!=NULL) {
								::lstrcpyn(szText,pTuner->GetDisplayName(),lengthof(szText));
							} else {
								LPCTSTR pszDriver=pTuner->GetDriverFileName();
								if (::StrCmpNI(pszDriver,TEXT("BonDriver_"),10)==0)
									pszDriver+=10;
								::lstrcpy(szText,pszDriver);
								::PathRemoveExtension(szText);
							}
							if (pTuner->NumSpaces()>1) {
								if (pTuningSpace->GetName()!=NULL)
									::wsprintf(szText+::lstrlen(szText),TEXT(" [%s]"),pTuningSpace->GetName());
								else
									::wsprintf(szText+::lstrlen(szText),TEXT(" [%d]"),j+1);
							}
							::DrawText(ps.hdc,szText,-1,&rc,DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
						}
						TunerIndex++;
					}
				}
			}
			if (ps.rcPaint.right>pThis->m_TunerAreaWidth) {
				rc.left=pThis->m_TunerAreaWidth;
				rc.top=rcClient.top;
				rc.right=rcClient.right;
				rc.bottom=rcClient.bottom;
				Theme::FillGradient(ps.hdc,&rc,&pThis->m_ChannelAreaBackGradient);
				if (pThis->m_CurTuner>=0) {
					const CTuningSpaceInfo *pTuningSpace=pThis->GetTuningSpaceInfo(pThis->m_CurTuner);

					if (pTuningSpace!=NULL) {
						const CChannelList *pChannelList=pTuningSpace->GetChannelList();
						for (int i=pThis->m_ChannelScrollPos;i<pChannelList->NumChannels() && i<pThis->m_ChannelScrollPos+pThis->m_VisibleChannelItems;i++) {
							const CChannelInfo *pChannelInfo=pChannelList->GetChannelInfo(i);
							const Theme::GradientInfo *pGradient;
							COLORREF TextColor;
							if (i==pThis->m_CurChannel) {
								pGradient=&pThis->m_CurChannelBackGradient;
								TextColor=pThis->m_CurChannelTextColor;
							} else {
								pGradient=&pThis->m_ChannelBackGradient[i%2];
								TextColor=pThis->m_ChannelTextColor;
							}
							RECT rcItem;
							pThis->GetChannelItemRect(i,&rcItem);
							Theme::FillGradient(ps.hdc,&rcItem,pGradient);
							::SetTextColor(ps.hdc,TextColor);
							rc=rcItem;
							rc.right=rc.left+pThis->m_ChannelNameWidth;
							rc.left+=8;
							::DrawText(ps.hdc,pChannelInfo->GetName(),-1,&rc,
								DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
							rc=rcItem;
							rc.left+=pThis->m_ChannelNameWidth;
							rc.bottom=(rc.top+rc.bottom)/2;
							CEventInfoData EventInfo;
							if (pThis->m_pEpgProgramList->GetEventInfo(
									pChannelInfo->GetTransportStreamID(),
									pChannelInfo->GetServiceID(),
									&pThis->m_EpgBaseTime,&EventInfo)) {
								int Length=0;
								if (EventInfo.m_fValidStartTime)
									Length+=::wsprintf(szText,TEXT("%02d:%02d "),
										EventInfo.m_stStartTime.wHour,
										EventInfo.m_stStartTime.wMinute);
								if (EventInfo.GetEventName()!=NULL)
									Length+=::wsprintf(szText+Length,TEXT("%s"),
													   EventInfo.GetEventName());
								if (Length>0)
									::DrawText(ps.hdc,szText,Length,&rc,DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
								SYSTEMTIME st;
								if (EventInfo.m_fValidStartTime
										&& EventInfo.m_DurationSec>0
										&& EventInfo.GetEndTime(&st)
										&& pThis->m_pEpgProgramList->GetEventInfo(
											pChannelInfo->GetTransportStreamID(),
											pChannelInfo->GetServiceID(),
											&st,&EventInfo)) {
									rc.top=rc.bottom;
									rc.bottom=rcItem.bottom;
									Length=0;
									if (EventInfo.m_fValidStartTime)
										Length+=::wsprintf(szText,TEXT("%02d:%02d "),
											EventInfo.m_stStartTime.wHour,
											EventInfo.m_stStartTime.wMinute);
									if (EventInfo.GetEventName()!=NULL)
										Length+=::wsprintf(szText+Length,TEXT("%s"),
														   EventInfo.GetEventName());
									if (Length>0)
										::DrawText(ps.hdc,szText,Length,&rc,
											DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
								}
							}
						}
					}
				}
			}
			::SelectObject(ps.hdc,hfontOld);
			::SetTextColor(ps.hdc,OldTextColor);
			::SetBkMode(ps.hdc,OldBkMode);
			pThis->GetCloseButtonRect(&rc);
			::DrawFrameControl(ps.hdc,&rc,DFC_CAPTION,DFCS_CAPTIONCLOSE | DFCS_MONO);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_VSCROLL:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);
			HWND hwndScroll=reinterpret_cast<HWND>(lParam);
			int Pos;

			if (hwndScroll==pThis->m_hwndTunerScroll) {
				Pos=pThis->m_TunerScrollPos;
				switch (LOWORD(wParam)) {
				case SB_LINEUP:		Pos--;	break;
				case SB_LINEDOWN:	Pos++;	break;
				case SB_PAGEUP:		Pos-=pThis->m_VisibleTunerItems;	break;
				case SB_PAGEDOWN:	Pos+=pThis->m_VisibleTunerItems;	break;
				case SB_TOP:		Pos=0;	break;
				case SB_BOTTOM:		Pos=pThis->m_TotalTuningSpaces-pThis->m_VisibleTunerItems;	break;
				case SB_THUMBTRACK:	Pos=HIWORD(wParam);	break;
				default:	return 0;
				}
				pThis->SetTunerScrollPos(Pos,true);
			} else if (hwndScroll==pThis->m_hwndChannelScroll) {
				Pos=pThis->m_ChannelScrollPos;
				switch (LOWORD(wParam)) {
				case SB_LINEUP:		Pos--;	break;
				case SB_LINEDOWN:	Pos++;	break;
				case SB_PAGEUP:		Pos-=pThis->m_VisibleChannelItems;	break;
				case SB_PAGEDOWN:	Pos+=pThis->m_VisibleChannelItems;	break;
				case SB_TOP:		Pos=0;	break;
				case SB_BOTTOM:		Pos=1000;	break;
				case SB_THUMBTRACK:	Pos=HIWORD(wParam);	break;
				default:	return 0;
				}
				pThis->SetChannelScrollPos(Pos,true);
			}
		}
		return 0;

	case WM_MOUSEWHEEL:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			int Delta=GET_WHEEL_DELTA_WPARAM(wParam)<=0?1:-1;

			if (pThis->TunerItemHitTest(x,y)>=0) {
				pThis->SetTunerScrollPos(pThis->m_TunerScrollPos+Delta,true);
			} else if (pThis->ChannelItemHitTest(x,y)>=0) {
				pThis->SetChannelScrollPos(pThis->m_ChannelScrollPos+Delta,true);
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);

			::SetFocus(hwnd);
			if (pThis->m_pEventHandler!=NULL) {
				int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);

				if (pThis->CloseButtonHitTest(x,y)) {
					pThis->m_pEventHandler->OnClose();
					return 0;
				}
				if (pThis->m_CurTuner>=0) {
					if (pThis->TunerItemHitTest(x,y)==pThis->m_CurTuner) {
						pThis->NotifyTunerSelect();
						return 0;
					}
				}
				if (pThis->m_CurChannel>=0) {
					if (pThis->ChannelItemHitTest(x,y)==pThis->m_CurChannel) {
						pThis->NotifyChannelSelect();
					}
				}
			}
		}
		return 0;

	case WM_RBUTTONDOWN:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);

			if (pThis->m_pEventHandler!=NULL)
				pThis->m_pEventHandler->OnRButtonDown(GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam));
		}
		return 0;

	case WM_MOUSEMOVE:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);
			int x=GET_X_LPARAM(lParam),y=GET_Y_LPARAM(lParam);
			if (pThis->m_LastCursorPos.x==x && pThis->m_LastCursorPos.y==y)
				return 0;
			pThis->m_LastCursorPos.x=x;
			pThis->m_LastCursorPos.y=y;
			int Index;

			Index=pThis->TunerItemHitTest(x,y);
			if (Index>=0 && Index!=pThis->m_CurTuner) {
				pThis->SetCurTuner(Index);
			}
			if (Index<0) {
				Index=pThis->ChannelItemHitTest(x,y);
				if (Index!=pThis->m_CurChannel) {
					pThis->SetCurChannel(Index);
				}
			}
		}
		return 0;

	case WM_SETCURSOR:
		if (reinterpret_cast<HWND>(wParam)==hwnd
				&& LOWORD(lParam)==HTCLIENT) {
			CChannelDisplayMenu *pThis=GetThis(hwnd);
			DWORD Pos=::GetMessagePos();
			POINT pt;
			LPCTSTR pszCursor;

			pt.x=(SHORT)LOWORD(Pos);
			pt.y=(SHORT)HIWORD(Pos);
			::ScreenToClient(hwnd,&pt);
			if (pThis->TunerItemHitTest(pt.x,pt.y)>=0
					|| pThis->ChannelItemHitTest(pt.x,pt.y)>=0
					|| pThis->CloseButtonHitTest(pt.x,pt.y)) {
				pszCursor=IDC_HAND;
			} else {
				pszCursor=IDC_ARROW;
			}
			::SetCursor(::LoadCursor(NULL,pszCursor));
			return TRUE;
		}
		break;

	case WM_KEYDOWN:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);

			switch (wParam) {
			case VK_UP:
			case VK_DOWN:
				if (pThis->m_CurChannel>=0) {
					if (wParam==VK_DOWN || pThis->m_CurChannel>0)
						pThis->SetCurChannel(wParam==VK_UP?pThis->m_CurChannel-1:pThis->m_CurChannel+1);
				} else {
					if (wParam==VK_DOWN || pThis->m_CurTuner>0)
						pThis->SetCurTuner(wParam==VK_UP?pThis->m_CurTuner-1:pThis->m_CurTuner+1);
				}
				break;
			case VK_LEFT:
				if (pThis->m_CurChannel>=0) {
					pThis->SetCurChannel(-1);
				}
				break;
			case VK_RIGHT:
				if (pThis->m_CurTuner>=0 && pThis->m_CurChannel<0) {
					const CTuningSpaceInfo *pTuningSpace=pThis->GetTuningSpaceInfo(pThis->m_CurTuner);

					if (pTuningSpace!=NULL && pTuningSpace->NumChannels()>0) {
						pThis->SetCurChannel(0);
					}
				}
				break;
			case VK_RETURN:
			case VK_SPACE:
				if (pThis->m_CurChannel>=0) {
					pThis->NotifyChannelSelect();
				} else if (pThis->m_CurTuner>=0) {
					pThis->NotifyTunerSelect();
				}
				break;
			case VK_PRIOR:
			case VK_NEXT:
			case VK_HOME:
			case VK_END:
				{
					static const struct {
						WORD KeyCode;
						WORD Scroll;
					} KeyMap[] = {
						{VK_PRIOR,	SB_PAGEUP},
						{VK_NEXT,	SB_PAGEDOWN},
						{VK_HOME,	SB_TOP},
						{VK_END,	SB_BOTTOM},
					};
					int i;
					for (i=0;KeyMap[i].KeyCode!=wParam;i++);
					::SendMessage(hwnd,WM_VSCROLL,KeyMap[i].Scroll,
								  reinterpret_cast<LPARAM>(pThis->m_CurChannel>=0?pThis->m_hwndChannelScroll:pThis->m_hwndTunerScroll));
				}
				break;
			case VK_ESCAPE:
				if (pThis->m_pEventHandler!=NULL)
					pThis->m_pEventHandler->OnClose();
				break;
			}
		}
		return 0;

	case WM_SHOWWINDOW:
		if (wParam) {
			CChannelDisplayMenu *pThis=GetThis(hwnd);
			POINT pt;

			::GetCursorPos(&pt);
			::ScreenToClient(hwnd,&pt);
			pThis->m_LastCursorPos=pt;
		}
		break;

	case WM_DESTROY:
		{
			CChannelDisplayMenu *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}




CChannelDisplayMenu::CTuner::CTuner(const CDriverInfo *pDriverInfo)
	: m_pszDriverFileName(DuplicateString(pDriverInfo->GetFileName()))
	, m_pszTunerName(DuplicateString(pDriverInfo->GetTunerName()))
	, m_pszDisplayName(NULL)
	, m_hIcon(NULL)
{
	const CTuningSpaceList *pList=pDriverInfo->GetAvailableTuningSpaceList();

	if (pList!=NULL) {
		for (int i=0;i<pList->NumSpaces();i++) {
			const CTuningSpaceInfo *pSrcTuningSpace=pList->GetTuningSpaceInfo(i);
			if (pSrcTuningSpace==NULL)
				break;
			const CChannelList *pSrcChannelList=pSrcTuningSpace->GetChannelList();

			if (pSrcChannelList!=NULL && pSrcChannelList->NumEnableChannels()>0) {
				CTuningSpaceInfo *pTuningSpace;

				if (m_TuningSpaceList.Length()>0
						&& pSrcTuningSpace->GetType()==CTuningSpaceInfo::SPACE_TERRESTRIAL
						&& m_TuningSpaceList[m_TuningSpaceList.Length()-1]->GetType()==CTuningSpaceInfo::SPACE_TERRESTRIAL) {
					pTuningSpace=m_TuningSpaceList[m_TuningSpaceList.Length()-1];
					pTuningSpace->SetName(TEXT("’nã"));
				} else {
					pTuningSpace=new CTuningSpaceInfo;
					pTuningSpace->Create(NULL,pSrcTuningSpace->GetName());
					m_TuningSpaceList.Add(pTuningSpace);
				}
				for (int j=0;j<pSrcChannelList->NumChannels();j++) {
					const CChannelInfo *pChannelInfo=pSrcChannelList->GetChannelInfo(j);

					if (pChannelInfo->IsEnabled()) {
						pTuningSpace->GetChannelList()->AddChannel(*pChannelInfo);
					}
				}
			}
		}
	}
	if (m_TuningSpaceList.Length()==0) {
		CTuningSpaceInfo *pTuningSpace=new CTuningSpaceInfo;

		pTuningSpace->Create();
		m_TuningSpaceList.Add(pTuningSpace);
	}
}


CChannelDisplayMenu::CTuner::~CTuner()
{
	Clear();
	delete [] m_pszDriverFileName;
	delete [] m_pszTunerName;
	delete [] m_pszDisplayName;
	if (m_hIcon!=NULL)
		::DestroyIcon(m_hIcon);
}


void CChannelDisplayMenu::CTuner::Clear()
{
	m_TuningSpaceList.DeleteAll();
}


LPCTSTR CChannelDisplayMenu::CTuner::GetDisplayName() const
{
	if (m_pszDisplayName!=NULL)
		return m_pszDisplayName;
	return m_pszTunerName;
}


void CChannelDisplayMenu::CTuner::SetDisplayName(LPCTSTR pszName)
{
	ReplaceString(&m_pszDisplayName,pszName);
}


int CChannelDisplayMenu::CTuner::NumSpaces() const
{
	return m_TuningSpaceList.Length();
}


const CTuningSpaceInfo *CChannelDisplayMenu::CTuner::GetTuningSpaceInfo(int Index) const
{
	return m_TuningSpaceList.Get(Index);
}


void CChannelDisplayMenu::CTuner::SetIcon(HICON hico)
{
	if (m_hIcon!=NULL)
		::DestroyIcon(m_hIcon);
	m_hIcon=hico;
}




CChannelDisplayMenu::CEventHandler::CEventHandler()
{
}


CChannelDisplayMenu::CEventHandler::~CEventHandler()
{
}
