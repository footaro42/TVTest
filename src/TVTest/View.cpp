#include "stdafx.h"
#include "TVTest.h"
#include "View.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define VIEW_WINDOW_CLASS				APP_NAME TEXT(" View")
#define VIDEO_CONTAINER_WINDOW_CLASS	APP_NAME TEXT(" Video Container")

#define EDGE_SIZE	1




HINSTANCE CVideoContainerWindow::m_hinst=NULL;


CVideoContainerWindow::CVideoContainerWindow()
	: m_pDtvEngine(NULL)
{
}


bool CVideoContainerWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=NULL;
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=VIDEO_CONTAINER_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CVideoContainerWindow *CVideoContainerWindow::GetThis(HWND hwnd)
{
	return static_cast<CVideoContainerWindow*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CVideoContainerWindow::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		OnCreate(hwnd,lParam);
		return 0;

	case WM_PAINT:
		{
			CVideoContainerWindow *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			RECT rcDest,rc;
			HBRUSH hbr;

			::BeginPaint(hwnd,&ps);
			hbr=static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
			if (!pThis->m_pDtvEngine->m_MediaViewer.GetDestRect(&rcDest)
					|| ::IsRectEmpty(&rcDest)) {
				::FillRect(ps.hdc,&ps.rcPaint,hbr);
			} else {
				pThis->m_pDtvEngine->m_MediaViewer.RepaintVideo(hwnd,ps.hdc);
				::GetClientRect(hwnd,&rc);
				if (!::EqualRect(&rc,&rcDest))
					DrawUtil::FillBorder(ps.hdc,&rc,&rcDest,&ps.rcPaint,hbr);
			}
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_MOVE:
		{
			CVideoContainerWindow *pThis=GetThis(hwnd);
			CVideoRenderer::RendererType Renderer=
				pThis->m_pDtvEngine->m_MediaViewer.GetVideoRendererType();

			if (Renderer!=CVideoRenderer::RENDERER_VMR7
					&& Renderer!=CVideoRenderer::RENDERER_VMR9)
				break;
		}
	case WM_SIZE:
		{
			CVideoContainerWindow *pThis=GetThis(hwnd);

			pThis->m_pDtvEngine->SetViewSize(LOWORD(lParam),HIWORD(lParam));
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			POINT pt;

			pt.x=GET_X_LPARAM(lParam);
			pt.y=GET_Y_LPARAM(lParam);
			::MapWindowPoints(hwnd,::GetParent(hwnd),&pt,1);
			return ::SendMessage(::GetParent(hwnd),uMsg,wParam,
														MAKELPARAM(pt.x,pt.y));
		}

	case WM_DESTROY:
		{
			CVideoContainerWindow *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


bool CVideoContainerWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 VIDEO_CONTAINER_WINDOW_CLASS,NULL,m_hinst);
}


bool CVideoContainerWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID,CDtvEngine *pDtvEngine)
{
	m_pDtvEngine=pDtvEngine;
	if (!Create(hwndParent,Style,ExStyle,ID)) {
		m_pDtvEngine=NULL;
		return false;
	}
	return true;
}




HINSTANCE CViewWindow::m_hinst=NULL;


bool CViewWindow::Initialize(HINSTANCE hinst)
{
	if (m_hinst==NULL) {
		WNDCLASS wc;

		wc.style=CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=NULL;
		wc.lpszMenuName=NULL;
		wc.lpszClassName=VIEW_WINDOW_CLASS;
		if (::RegisterClass(&wc)==0)
			return false;
		m_hinst=hinst;
	}
	return true;
}


CViewWindow::CViewWindow()
{
	m_pVideoContainer=NULL;
	m_hwndMessage=NULL;
	m_hbmLogo=NULL;
	m_fEdge=false;
}


CViewWindow::~CViewWindow()
{
	if (m_hbmLogo)
		::DeleteObject(m_hbmLogo);
}


bool CViewWindow::Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID)
{
	return CreateBasicWindow(hwndParent,Style,ExStyle,ID,
							 VIEW_WINDOW_CLASS,NULL,m_hinst);
}


void CViewWindow::SetVideoContainer(CVideoContainerWindow *pVideoContainer)
{
	m_pVideoContainer=pVideoContainer;
	if (pVideoContainer!=NULL && m_hwnd!=NULL
			&& m_pVideoContainer->GetParent()==m_hwnd) {
		RECT rc;

		GetClientRect(&rc);
		pVideoContainer->SetPosition(&rc);
	}
}


void CViewWindow::SetMessageWindow(HWND hwnd)
{
	m_hwndMessage=hwnd;
}


bool CViewWindow::SetLogo(HBITMAP hbm)
{
	if (hbm==NULL && m_hbmLogo==NULL)
		return true;
	if (m_hbmLogo)
		::DeleteObject(m_hbmLogo);
	m_hbmLogo=hbm;
	if (m_hwnd) {
		Invalidate();
		Update();
	}
	return true;
}


void CViewWindow::SetEdge(bool fEdge)
{
	if (m_fEdge!=fEdge) {
		m_fEdge=fEdge;
		if (m_hwnd)
			Invalidate();
	}
}


int CViewWindow::GetVerticalEdgeWidth() const
{
	return EDGE_SIZE;
}


int CViewWindow::GetHorizontalEdgeHeight() const
{
	return EDGE_SIZE;
}


CViewWindow *CViewWindow::GetThis(HWND hwnd)
{
	return static_cast<CViewWindow*>(GetBasicWindow(hwnd));
}


LRESULT CALLBACK CViewWindow::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			//CViewWindow *pThis=dynamic_cast<CViewWindow*>(OnCreate(hwnd,lParam));
			OnCreate(hwnd,lParam);
		}
		return 0;

	case WM_SIZE:
		{
			CViewWindow *pThis=GetThis(hwnd);

			if (pThis->m_pVideoContainer!=NULL
					&& pThis->m_pVideoContainer->GetParent()==hwnd) {
				int x=0,y=0,Width=LOWORD(lParam),Height=HIWORD(lParam);

				if (pThis->m_fEdge) {
					x=EDGE_SIZE;
					y=EDGE_SIZE;
					Width-=EDGE_SIZE*2;
					if (Width<0)
						Width=0;
					Height-=EDGE_SIZE*2;
					if (Height<0)
						Height=0;
				}
				pThis->m_pVideoContainer->SetPosition(x,y,Width,Height);
			}
		}
		return 0;

	case WM_PAINT:
		{
			CViewWindow *pThis=GetThis(hwnd);
			PAINTSTRUCT ps;
			RECT rcClient;
			HBRUSH hbr=static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));

			::BeginPaint(hwnd,&ps);
			::GetClientRect(hwnd,&rcClient);
			if (pThis->m_hbmLogo) {
				RECT rcImage;
				BITMAP bm;
				HDC hdcMemory;
				HBITMAP hbmOld;

				::GetObject(pThis->m_hbmLogo,sizeof(BITMAP),&bm);
				rcImage.left=(rcClient.right-bm.bmWidth)/2;
				rcImage.top=(rcClient.bottom-bm.bmHeight)/2;
				rcImage.right=rcImage.left+bm.bmWidth;
				rcImage.bottom=rcImage.top+bm.bmHeight;
				hdcMemory=::CreateCompatibleDC(ps.hdc);
				hbmOld=static_cast<HBITMAP>(::SelectObject(hdcMemory,pThis->m_hbmLogo));
				::BitBlt(ps.hdc,rcImage.left,rcImage.top,bm.bmWidth,bm.bmHeight,
						 hdcMemory,0,0,SRCCOPY);
				::SelectObject(hdcMemory,hbmOld);
				::DeleteDC(hdcMemory);
				DrawUtil::FillBorder(ps.hdc,&rcClient,&rcImage,&ps.rcPaint,hbr);
			} else {
				::FillRect(ps.hdc,&ps.rcPaint,hbr);
			}
			if (pThis->m_fEdge)
				::DrawEdge(ps.hdc,&rcClient,BDR_SUNKENINNER,BF_RECT);
			::EndPaint(hwnd,&ps);
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case WM_MOUSEMOVE:
		{
			CViewWindow *pThis=GetThis(hwnd);

			if (pThis->m_hwndMessage!=NULL) {
				POINT pt;

				pt.x=GET_X_LPARAM(lParam);
				pt.y=GET_Y_LPARAM(lParam);
				::MapWindowPoints(hwnd,pThis->m_hwndMessage,&pt,1);
				return ::SendMessage(pThis->m_hwndMessage,uMsg,wParam,
														MAKELPARAM(pt.x,pt.y));
			}
		}
		break;

	case WM_SETCURSOR:
		if (LOWORD(lParam)==HTCLIENT) {
			::SetCursor(::LoadCursor(NULL,IDC_ARROW));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CViewWindow *pThis=GetThis(hwnd);

			pThis->OnDestroy();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}
