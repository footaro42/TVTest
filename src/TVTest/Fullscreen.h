#ifndef FULLSCREEN_H
#define FULLSCREEN_H


#include "BasicWindow.h"
#include "View.h"


class CFullscreen : public CBasicWindow {
	CBasicWindow *m_pOwnerWindow;
	CVideoContainerWindow *m_pVideoContainer;
	CStatusView *m_pStatusView;
	CTitleBar *m_pTitleBar;
	CMediaViewer::ViewStretchMode m_StretchMode;
	bool m_fShowCursor;
	bool m_fMenu;
	bool m_fShowStatusView;
	bool m_fShowTitleBar;
	void ShowStatusView(bool fShow);
	void ShowTitleBar(bool fShow);
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	static HINSTANCE m_hinst;
	static CFullscreen *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CFullscreen();
	~CFullscreen();
	bool Create(CBasicWindow *pOwnerWindow,CVideoContainerWindow *pVideoContainer,CStatusView *pStatusView,CTitleBar *pTitleBar);
	void OnRButtonDown();
	void OnMouseMove();
	static bool Initialize(HINSTANCE hinst);
};


#endif
