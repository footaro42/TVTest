#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "CaptionPanel.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define IDC_EDIT	1000




const LPCTSTR CCaptionPanel::m_pszClassName=APP_NAME TEXT(" Caption Panel");
const LPCTSTR CCaptionPanel::m_pszPropName=TEXT("CaptionPanel");
HINSTANCE CCaptionPanel::m_hinst=NULL;


bool CCaptionPanel::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=m_pszClassName;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CCaptionPanel::CCaptionPanel()
	: m_BackColor(RGB(0,0,0))
	, m_TextColor(RGB(255,255,255))
	, m_hbrBack(NULL)
	, m_hfont(NULL)
	, m_hwndEdit(NULL)
	, m_pOldEditProc(NULL)
	, m_fEnable(true)
	, m_fAutoScroll(true)
	, m_Language(0)
{
}


CCaptionPanel::~CCaptionPanel()
{
	Clear();
	if (m_hbrBack!=NULL)
		::DeleteObject(m_hbrBack);
	if (m_hfont!=NULL)
		::DeleteObject(m_hfont);
}


bool CCaptionPanel::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 m_pszClassName,TEXT("字幕"),m_hinst);
}


void CCaptionPanel::SetVisible(bool fVisible)
{
	if (m_hwnd!=NULL) {
		if (fVisible && !m_CaptionList.empty()) {
			int Length=0;
			for (std::deque<LPTSTR>::iterator itr=m_CaptionList.begin();itr!=m_CaptionList.end();itr++)
				Length+=::lstrlen(*itr);
			LPTSTR pszText=new TCHAR[Length+1],p;
			p=pszText;
			for (std::deque<LPTSTR>::iterator itr=m_CaptionList.begin();itr!=m_CaptionList.end();itr++) {
				::lstrcpy(p,*itr);
				p+=::lstrlen(*itr);
			}
			AppendText(pszText);
			delete [] pszText;
			ClearCaptionList();
		}
		CPanelForm::CPage::SetVisible(fVisible);
	}
}


void CCaptionPanel::SetColor(COLORREF BackColor,COLORREF TextColor)
{
	m_BackColor=BackColor;
	m_TextColor=TextColor;
	if (m_hbrBack!=NULL) {
		::DeleteObject(m_hbrBack);
		m_hbrBack=NULL;
	}
	if (m_hwnd!=NULL) {
		m_hbrBack=::CreateSolidBrush(m_BackColor);
		::InvalidateRect(m_hwndEdit,NULL,TRUE);
	}
}


bool CCaptionPanel::SetFont(const LOGFONT *pFont)
{
	HFONT hfont=::CreateFontIndirect(pFont);

	if (hfont==NULL)
		return false;
	if (m_hfont!=NULL)
		::DeleteObject(m_hfont);
	m_hfont=hfont;
	if (m_hwnd!=NULL) {
		SetWindowFont(m_hwndEdit,m_hfont,TRUE);
	}
	return true;
}


void CCaptionPanel::Clear()
{
	CBlockLock Lock(&m_Lock);

	ClearCaptionList();
	if (m_hwndEdit!=NULL) {
		::SetWindowText(m_hwndEdit, TEXT(""));
		m_fClearLast=true;
		m_fContinue=false;
	}
}


void CCaptionPanel::ClearCaptionList()
{
	if (!m_CaptionList.empty()) {
		for (std::deque<LPTSTR>::iterator itr=m_CaptionList.begin();itr!=m_CaptionList.end();itr++)
			delete [] *itr;
		m_CaptionList.clear();
	}
}


void CCaptionPanel::AppendText(LPCTSTR pszText)
{
	DWORD SelStart,SelEnd;

	::SendMessage(m_hwndEdit,EM_GETSEL,
				  reinterpret_cast<WPARAM>(&SelStart),
				  reinterpret_cast<LPARAM>(&SelEnd));
	::SendMessage(m_hwndEdit,EM_SETSEL,::GetWindowTextLength(m_hwndEdit),-1);
	::SendMessage(m_hwndEdit,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(pszText));
	::SendMessage(m_hwndEdit,EM_SETSEL,SelStart,SelEnd);
	if (m_fAutoScroll)
		::SendMessage(m_hwndEdit,WM_VSCROLL,MAKEWPARAM(SB_BOTTOM,0),0);
}


CCaptionPanel *CCaptionPanel::GetThis(HWND hwnd)
{
	return static_cast<CCaptionPanel*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CCaptionPanel::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CCaptionPanel *pThis=static_cast<CCaptionPanel*>(OnCreate(hwnd,lParam));

			if (pThis->m_hbrBack==NULL)
				pThis->m_hbrBack=::CreateSolidBrush(pThis->m_BackColor);
			if (pThis->m_hfont==NULL)
				pThis->m_hfont=CreateDefaultFont();
			pThis->m_hwndEdit=CreateWindowEx(0,TEXT("EDIT"),TEXT(""),
				WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
				0,0,0,0,hwnd,(HMENU)IDC_EDIT,m_hinst,NULL);
			SetWindowFont(pThis->m_hwndEdit,pThis->m_hfont,FALSE);
			::SetProp(pThis->m_hwndEdit,m_pszPropName,pThis);
			pThis->m_pOldEditProc=SubclassWindow(pThis->m_hwndEdit,EditWndProc);

			pThis->m_fClearLast=true;
			pThis->m_fContinue=false;

			GetAppClass().GetCoreEngine()->m_DtvEngine.m_CaptionDecoder.SetCaptionHandler(pThis);
		}
		return 0;

	case WM_SIZE:
		{
			CCaptionPanel *pThis=GetThis(hwnd);

			::MoveWindow(pThis->m_hwndEdit,0,0,LOWORD(lParam),HIWORD(lParam),TRUE);
		}
		return 0;

	case WM_CTLCOLORSTATIC:
		{
			CCaptionPanel *pThis=GetThis(hwnd);

			HDC hdc=reinterpret_cast<HDC>(wParam);

			::SetTextColor(hdc,pThis->m_TextColor);
			::SetBkColor(hdc,pThis->m_BackColor);
			return reinterpret_cast<LRESULT>(pThis->m_hbrBack);
		}

	case WM_APP:
		{
			CCaptionPanel *pThis=GetThis(hwnd);
			LPTSTR pszText=reinterpret_cast<LPTSTR>(lParam);

			if (pszText!=NULL) {
				if (pThis->m_fEnable) {
					if (pThis->GetVisible()) {
						::SendMessage(pThis->m_hwndEdit,WM_SETREDRAW,FALSE,0);
						pThis->AppendText(pszText);
						::SendMessage(pThis->m_hwndEdit,WM_SETREDRAW,TRUE,0);
					} else {
						// 非表示の場合はキューに溜める
						if (pThis->m_CaptionList.size()>=MAX_QUEUE_TEXT) {
							delete [] pThis->m_CaptionList.front();
							pThis->m_CaptionList.pop_front();
							::SetWindowText(pThis->m_hwndEdit,TEXT(""));
						}
						pThis->m_CaptionList.push_back(pszText);
						pszText=NULL;
					}
				}
				delete [] pszText;
			}
		}
		return 0;

	case WM_DESTROY:
		{
			CCaptionPanel *pThis=GetThis(hwnd);

			GetAppClass().GetCoreEngine()->m_DtvEngine.m_CaptionDecoder.SetCaptionHandler(NULL);

			pThis->ClearCaptionList();
			SubclassWindow(pThis->m_hwndEdit,pThis->m_pOldEditProc);
			pThis->m_hwndEdit=NULL;
			pThis->m_pOldEditProc=NULL;
			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


LRESULT CALLBACK CCaptionPanel::EditWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	CCaptionPanel *pThis=(CCaptionPanel*)::GetProp(hwnd,m_pszPropName);

	if (pThis==NULL)
		return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
	switch (uMsg) {
	case WM_RBUTTONDOWN:
		{
			HMENU hmenu=::CreatePopupMenu();

			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,1,TEXT("コピー(&C)"));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,2,TEXT("すべて選択(&A)"));
			::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED,3,TEXT("クリア(&C)"));
			::AppendMenu(hmenu,MFT_SEPARATOR,0,NULL);
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED | (pThis->m_fEnable?MFS_CHECKED:MFS_UNCHECKED),4,TEXT("字幕表示有効(&E)"));
			::AppendMenu(hmenu,MFT_STRING | MFS_ENABLED | (pThis->m_fAutoScroll?MFS_CHECKED:MFS_UNCHECKED),5,TEXT("自動スクロール(&S)"));
			POINT pt;
			::GetCursorPos(&pt);
			switch (::TrackPopupMenu(hmenu,TPM_RIGHTBUTTON | TPM_RETURNCMD,pt.x,pt.y,0,hwnd,NULL)) {
			case 1:
				{
					DWORD Start,End;

					::SendMessage(hwnd,WM_SETREDRAW,FALSE,0);
					::SendMessage(hwnd,EM_GETSEL,(WPARAM)&Start,(LPARAM)&End);
					if (Start==End)
						::SendMessage(hwnd,EM_SETSEL,0,-1);
					::SendMessage(hwnd,WM_COPY,0,0);
					if (Start==End)
						::SendMessage(hwnd,EM_SETSEL,Start,End);
					::SendMessage(hwnd,WM_SETREDRAW,TRUE,0);
				}
				break;
			case 2:
				::SendMessage(hwnd,EM_SETSEL,0,-1);
				break;
			case 3:
				::SetWindowText(hwnd,TEXT(""));
				break;
			case 4:
				pThis->m_Lock.Lock();
				pThis->m_fEnable=!pThis->m_fEnable;
				pThis->m_fClearLast=false;
				pThis->m_fContinue=false;
				pThis->m_Lock.Unlock();
				break;
			case 5:
				pThis->m_fAutoScroll=!pThis->m_fAutoScroll;
				break;
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


void CCaptionPanel::OnLanguageUpdate(CCaptionDecoder *pDecoder)
{
}


void CCaptionPanel::OnCaption(CCaptionDecoder *pDecoder,BYTE Language, LPCTSTR pszText)
{
	CBlockLock Lock(&m_Lock);

	if ((Language==m_Language || Language==0xFF) && m_hwnd!=NULL && m_fEnable) {
		int Length=::lstrlen(pszText);

		if (Length>0) {
			if (Length==2 && pszText[0]=='\r' && pszText[1]=='\n') {
				if (m_fClearLast || m_fContinue)
					return;
				m_fClearLast=true;
			} else {
				m_fClearLast=false;
			}

			LPTSTR pszBuff=new TCHAR[Length+1];
			int DstLength=Length;

			if (m_fContinue
					&& Length>=2 && pszText[0]=='\r' && pszText[1]=='\n') {
				::lstrcpy(pszBuff,pszText+2);
				DstLength-=2;
			} else {
				::lstrcpy(pszBuff,pszText);
			}
			m_fContinue=
#ifdef UNICODE
				Length>1 && pszText[Length-1]==L'→';
#else
				Length>2 && pszText[Length-2]=="→"[0] && pszText[Length-1]=="→"[1];
#endif
			if (m_fContinue)
				pszBuff[DstLength-(3-sizeof(TCHAR))]='\0';
			::PostMessage(m_hwnd,WM_APP,0,(LPARAM)pszBuff);
		}
	}
}
