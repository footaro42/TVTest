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
	HBITMAP m_hbmLogo;
	bool m_fEdge;
	bool m_fShowCursor;
	static CViewWindow *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CViewWindow();
	~CViewWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetVideoContainer(CVideoContainerWindow *pVideoContainer);
	void SetMessageWindow(HWND hwnd);
	bool SetLogo(HBITMAP hbm);
	void SetEdge(bool fEdge);
	void ShowCursor(bool fShow);
	int GetVerticalEdgeWidth() const;
	int GetHorizontalEdgeHeight() const;
	static bool Initialize(HINSTANCE hinst);
};


#endif
