#ifndef VIEW_H
#define VIEW_H


#include "BasicWindow.h"
#include "DtvEngine.h"


class CVideoContainerWindow : public CBasicWindow {
	CDtvEngine *m_pDtvEngine;
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	static HINSTANCE m_hinst;
	static CVideoContainerWindow *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CVideoContainerWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID,CDtvEngine *pDtvEngine);
	CDtvEngine *GetDtvEngine() { return m_pDtvEngine; }
	const CDtvEngine *GetDtvEngine() const { return m_pDtvEngine; }
	static bool Initialize(HINSTANCE hinst);
};

class CViewWindow : public CBasicWindow {
	static HINSTANCE m_hinst;
	CVideoContainerWindow *m_pVideoContainer;
	HWND m_hwndMessage;
	static CViewWindow *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CViewWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetVideoContainer(CVideoContainerWindow *pVideoContainer);
	void SetMessageWindow(HWND hwnd);
	static bool Initialize(HINSTANCE hinst);
};


#endif
