#include "stdafx.h"
#include "TVTest.h"
#include "Splitter.h"


#define SPLITTER_WINDOW_CLASS APP_NAME TEXT(" Splitter")




HINSTANCE CSplitter::m_hinst=NULL;


bool CSplitter::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=0;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=(HBRUSH)(COLOR_3DFACE+1);
		wc.lpszMenuName=NULL;
		wc.lpszClassName=SPLITTER_WINDOW_CLASS;
		if (RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CSplitter::CSplitter()
{
	m_Style=0;
	m_BarWidth=4;
	m_BarPos=0;
	for (int i=0;i<lengthof(m_PaneList);i++) {
		m_PaneList[i].pWindow=NULL;
		m_PaneList[i].ID=-1;
		m_PaneList[i].fVisible=false;
		m_PaneList[i].MinSize=0;
	}
	m_FixedPane=-1;
}


CSplitter::~CSplitter()
{
}


bool CSplitter::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 SPLITTER_WINDOW_CLASS,NULL,m_hinst);
}


void CSplitter::SetSplitter()
{
	RECT rc;
	int BarPos;

	GetClientRect(&rc);
	if (m_PaneList[0].pWindow==NULL || !m_PaneList[0].fVisible
			|| m_PaneList[1].pWindow==NULL || !m_PaneList[1].fVisible) {
		if (m_PaneList[0].pWindow!=NULL && m_PaneList[0].fVisible)
			m_PaneList[0].pWindow->SetPosition(0,0,rc.right,rc.bottom);
		else if (m_PaneList[1].pWindow!=NULL && m_PaneList[1].fVisible)
			m_PaneList[1].pWindow->SetPosition(0,0,rc.right,rc.bottom);
		return;
	}
	BarPos=m_BarPos;
	if ((m_Style&STYLE_HORZ)!=0) {
		if (rc.bottom-BarPos-m_BarWidth<m_PaneList[1].MinSize)
			BarPos=rc.bottom-m_BarWidth-m_PaneList[1].MinSize;
		if (BarPos<m_PaneList[0].MinSize)
			BarPos=m_PaneList[0].MinSize;
		m_PaneList[0].pWindow->SetPosition(0,0,rc.right,BarPos);
		m_PaneList[1].pWindow->SetPosition(0,BarPos+m_BarWidth,
										rc.right,rc.bottom-BarPos-m_BarWidth);
	} else {
		if (rc.right-BarPos-m_BarWidth<m_PaneList[1].MinSize)
			BarPos=rc.right-m_BarWidth-m_PaneList[1].MinSize;
		if (BarPos<m_PaneList[0].MinSize)
			BarPos=m_PaneList[0].MinSize;
		m_PaneList[0].pWindow->SetPosition(0,0,BarPos,rc.bottom);
		m_PaneList[1].pWindow->SetPosition(BarPos+m_BarWidth,0,
										rc.right-BarPos-m_BarWidth,rc.bottom);
	}
}


CSplitter *CSplitter::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSplitter*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CSplitter::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			CSplitter *pThis=dynamic_cast<CSplitter*>(OnCreate(hwnd,lParam));
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			RECT rc;

			pThis->m_Style=pcs->style;
			pThis->GetClientRect(&rc);
			if ((pThis->m_Style&STYLE_HORZ)!=0)
				pThis->m_BarPos=(rc.bottom-pThis->m_BarWidth)/2;
			else
				pThis->m_BarPos=(rc.right-pThis->m_BarWidth)/2;
			for (int i=0;i<lengthof(pThis->m_PaneList);i++) {
				pThis->m_PaneList[i].pWindow=NULL;
				pThis->m_PaneList[i].ID=-1;
				pThis->m_PaneList[i].fVisible=false;
				pThis->m_PaneList[i].MinSize=0;
			}
		}
		return 0;

	case WM_SIZE:
		{
			CSplitter *pThis=GetThis(hwnd);

			if (pThis->m_FixedPane==pThis->m_PaneList[1].ID && pThis->m_PaneList[1].pWindow!=NULL) {
				int Width,Height;

				pThis->m_PaneList[1].pWindow->GetPosition(NULL,NULL,&Width,&Height);
				if ((pThis->m_Style&STYLE_HORZ)!=0)
					pThis->m_BarPos=HIWORD(lParam)-pThis->m_BarWidth-Height;
				else
					pThis->m_BarPos=LOWORD(lParam)-pThis->m_BarWidth-Width;
			}
			pThis->SetSplitter();
		}
		return 0;

	case WM_LBUTTONDOWN:
		{
			CSplitter *pThis=GetThis(hwnd);

			if ((pThis->m_Style&STYLE_FIXED)==0
					&& ((pThis->m_PaneList[0].pWindow!=NULL && pThis->m_PaneList[0].fVisible)
					|| (pThis->m_PaneList[1].pWindow!=NULL && pThis->m_PaneList[1].fVisible)))
				SetCapture(hwnd);
		}
		return 0;

	case WM_LBUTTONUP:
		if (GetCapture()==hwnd)
			ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
		{
			CSplitter *pThis=GetThis(hwnd);
			LPCTSTR pszCursor;

			if ((pThis->m_Style&STYLE_FIXED)==0
					&& pThis->m_PaneList[0].pWindow!=NULL && pThis->m_PaneList[0].fVisible
					&& pThis->m_PaneList[1].pWindow!=NULL && pThis->m_PaneList[1].fVisible)
				pszCursor=(pThis->m_Style&STYLE_HORZ)!=0?IDC_SIZENS:IDC_SIZEWE;
			else
				pszCursor=IDC_ARROW;
			SetCursor(LoadCursor(NULL,pszCursor));
			if (GetCapture()==hwnd) {
				pThis->m_BarPos=((pThis->m_Style&STYLE_HORZ)!=0?
					(SHORT)HIWORD(lParam):(SHORT)LOWORD(lParam))-pThis->m_BarWidth/2;
				pThis->SetSplitter();
			}
		}
		return 0;

	case WM_COMMAND:
	case WM_NOTIFY:
		return ::SendMessage(::GetParent(hwnd),uMsg,wParam,lParam);

	case WM_DESTROY:
		{
			CSplitter *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CSplitter::SetBarPos(int Pos)
{
	m_BarPos=Pos;
	SetSplitter();
	return true;
}


int CSplitter::GetBarPos() const
{
	return m_BarPos;
}


bool CSplitter::SetPane(int Index,CBasicWindow *pWindow,int ID)
{
	if (Index<0 || Index>1)
		return false;
	m_PaneList[Index].pWindow=pWindow;
	m_PaneList[Index].ID=ID;
	m_PaneList[Index].fVisible=pWindow!=NULL && pWindow->GetVisible();
	if (pWindow!=NULL && pWindow->GetParent()!=m_hwnd)
		pWindow->SetParent(this);
	SetSplitter();
	return true;
}


CBasicWindow *CSplitter::GetPane(int Index) const
{
	if (Index<0 || Index>1)
		return NULL;
	return m_PaneList[Index].pWindow;
}


int CSplitter::IDToIndex(int ID) const
{
	int i;

	for (i=0;i<lengthof(m_PaneList);i++) {
		if (m_PaneList[i].ID==ID)
			return i;
	}
	return -1;
}


bool CSplitter::SetPaneVisible(int ID,bool fVisible)
{
	int Index=IDToIndex(ID);

	if (Index<0 || m_PaneList[Index].pWindow==NULL)
		return false;
	if (m_PaneList[Index].fVisible!=fVisible) {
		m_PaneList[Index].fVisible=fVisible;
		m_PaneList[Index].pWindow->SetVisible(fVisible);
		SetSplitter();
	}
	return true;
}


bool CSplitter::SetMinSize(int ID,int Size)
{
	int Index=IDToIndex(ID);

	if (Index<0 || Size<0)
		return false;
	m_PaneList[Index].MinSize=Size;
	if (Index==0) {
	 	if (m_BarPos<m_PaneList[0].MinSize)
			SetSplitter();
	} else {
		RECT rc;

		GetClientRect(&rc);
		if (((m_Style&STYLE_HORZ)!=0?rc.bottom:rc.right)-m_BarPos-m_BarWidth<
														m_PaneList[1].MinSize)
			SetSplitter();
	}
	return true;
}


int CSplitter::GetPaneSize(int ID) const
{
	int Index=IDToIndex(ID);
	RECT rc;

	if (Index<0)
		return 0;
	if (Index==0)
		return m_BarPos;
	GetClientRect(&rc);
	rc.right-=m_BarPos+m_BarWidth;
	return max(rc.right,0);
}


bool CSplitter::SetPaneSize(int ID,int Size)
{
	int Index=IDToIndex(ID);
	RECT rc;

	if (Index<0)
		return false;
	if (Index==0) {
		m_BarPos=Size;
	} else {
		GetClientRect(&rc);
		m_BarPos=rc.right-Size-m_BarWidth;
	}
	SetSplitter();
	return true;
}


bool CSplitter::SetFixedPane(int ID)
{
	m_FixedPane=ID;
	return true;
}


bool CSplitter::SetStyle(DWORD Style)
{
	m_Style=Style;
	SetSplitter();
	return true;
}


bool CSplitter::SwapPane()
{
	PaneInfo Pane;

	m_BarPos=GetPaneSize(m_PaneList[1].ID);
	Pane=m_PaneList[0];
	m_PaneList[0]=m_PaneList[1];
	m_PaneList[1]=Pane;
	SetSplitter();
	return true;
}
