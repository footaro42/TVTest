#include "stdafx.h"
#include "TVTest.h"
#include "View.h"
#include "DrawUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define VIEW_WINDOW_CLASS				APP_NAME TEXT("TVTest View")
#define VIDEO_CONTAINER_WINDOW_CLASS	APP_NAME TEXT("TVTest Video Container")




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
		wc.hbrBackground=CreateSolidBrush(RGB(0,0,0));
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
	return reinterpret_cast<CVideoContainerWindow*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
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
			pThis->m_pDtvEngine->m_MediaViewer.RepaintVideo(hwnd,ps.hdc);
			pThis->m_pDtvEngine->m_MediaViewer.GetDestRect(&rcDest);
			hbr=static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
			::GetClientRect(hwnd,&rc);
			DrawUtil::FillBorder(ps.hdc,&rc,&rcDest,&ps.rcPaint,hbr);
			/*
			if (ps.rcPaint.top<rcDest.top) {
				rc.left=ps.rcPaint.left;
				rc.top=ps.rcPaint.top;
				rc.right=ps.rcPaint.right;
				rc.bottom=min(rcDest.top,ps.rcPaint.bottom);
				::FillRect(ps.hdc,&rc,hbr);
			}
			if (ps.rcPaint.top<rcDest.bottom && ps.rcPaint.bottom>rcDest.top) {
				rc.top=max(rcDest.top,ps.rcPaint.top);
				rc.bottom=min(rcDest.bottom,ps.rcPaint.bottom);
				if (ps.rcPaint.left<rcDest.left) {
					rc.left=ps.rcPaint.left;
					rc.right=min(rcDest.left,ps.rcPaint.right);
					::FillRect(ps.hdc,&rc,hbr);
				}
				if (ps.rcPaint.right>rcDest.right) {
					rc.left=max(rcDest.right,ps.rcPaint.left);
					rc.right=ps.rcPaint.right;
					::FillRect(ps.hdc,&rc,hbr);
				}
			}
			if (ps.rcPaint.bottom>rcDest.bottom) {
				rc.left=ps.rcPaint.left;
				rc.top=max(rcDest.bottom,ps.rcPaint.top);
				rc.right=ps.rcPaint.right;
				rc.bottom=ps.rcPaint.bottom;
				::FillRect(ps.hdc,&rc,hbr);
			}
			*/
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
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
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

		wc.style=CS_DBLCLKS;
		wc.lpfnWndProc=WndProc;
		wc.cbClsExtra=0;
		wc.cbWndExtra=0;
		wc.hInstance=hinst;
		wc.hIcon=NULL;
		wc.hCursor=LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground=CreateSolidBrush(RGB(0,0,0));
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


CViewWindow *CViewWindow::GetThis(HWND hwnd)
{
	return reinterpret_cast<CViewWindow*>(GetWindowLongPtr(hwnd,GWLP_USERDATA));
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
					&& pThis->m_pVideoContainer->GetParent()==hwnd)
				pThis->m_pVideoContainer->SetPosition(0,0,LOWORD(lParam),HIWORD(lParam));
		}
		return 0;

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
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
