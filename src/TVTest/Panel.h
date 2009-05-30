#ifndef PANEL_H
#define PANEL_H


#include "Splitter.h"


class CPanel : public CBasicWindow {
public:
	class CEventHandler {
	public:
		virtual ~CEventHandler() {}
		virtual bool OnFloating() { return false; }
		virtual bool OnClose() { return false; }
		virtual bool OnEnterSizeMove() { return false; }
		virtual bool OnMoving(RECT *pRect) { return false; }
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual void OnSizeChanged(int Width,int Height) {}
	};

private:
	int m_TitleMargin;
	int m_ButtonSize;
	HFONT m_hfont;
	int m_TitleHeight;
	CBasicWindow *m_pWindow;
	LPTSTR m_pszTitle;
	bool m_fShowTitle;
	COLORREF m_crTitleBackColor;
	COLORREF m_crTitleTextColor;
	CEventHandler *m_pEventHandler;
	POINT m_ptDragStartPos;
	POINT m_ptMovingWindowPos;
	void OnSize(int Width,int Height);
	void GetCloseButtonRect(RECT *pRect) const;
	static HINSTANCE m_hinst;
	static CPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	CPanel();
	~CPanel();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool SetWindow(CBasicWindow *pWindow,LPCTSTR pszTitle);
	void ShowTitle(bool fShow);
	void SetEventHandler(CEventHandler *pHandler);
	CBasicWindow *GetWindow() { return m_pWindow; }
	bool SetTitleColor(COLORREF crTitleBack,COLORREF crTitleText);
};

class CDropHelper : public CBasicWindow {
	int m_Opacity;
	static HINSTANCE m_hinst;
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	CDropHelper();
	~CDropHelper();
	bool Show(const RECT *pRect);
	bool Hide();
};

class CPanelFrameEventHandler {
public:
	virtual bool OnClose() { return true; }
	virtual bool OnMoving(RECT *pRect) { return false; }
	virtual bool OnEnterSizeMove() { return false; }
	virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
	virtual bool OnMouseWheel(WPARAM wParam,LPARAM lParam) { return false; }
	virtual void OnVisibleChange(bool fVisible) {}
	virtual bool OnFloatingChange(bool fFloating) { return true; }
	virtual bool OnActivate(bool fActive) { return false; }
};

class CPanelFrame : public CBasicWindow, public CPanel::CEventHandler {
	CSplitter *m_pSplitter;
	int m_PanelID;
	CPanel m_Panel;
	bool m_fFloating;
	int m_DockingWidth;
	int m_Opacity;
	CDropHelper m_DropHelper;
	enum DockingPlace {
		DOCKING_NONE,
		DOCKING_LEFT,
		DOCKING_RIGHT
	};
	DockingPlace m_DragDockingTarget;
	bool m_fDragMoving;
	CPanelFrameEventHandler *m_pEventHandler;
	bool Create(HWND hwndParent,DWORD Sytle,DWORD ExStyle=0,int ID=0);
	static HINSTANCE m_hinst;
	static CPanelFrame *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	// CPanel::CEventHandler
	bool OnFloating();
	bool OnClose();
	bool OnEnterSizeMove();
	bool OnMoving(RECT *pRect);
	bool OnKeyDown(UINT KeyCode,UINT Flags);
	void OnSizeChanged(int Width,int Height);

public:
	static bool Initialize(HINSTANCE hinst);
	CPanelFrame();
	~CPanelFrame();
	bool Create(HWND hwndOwner,CSplitter *pSplitter,int PanelID,CBasicWindow *pWindow,LPCTSTR pszTitle);
	CBasicWindow *GetWindow() { return m_Panel.GetWindow(); }
	bool SetFloating(bool fFloating);
	bool GetFloating() const { return m_fFloating; }
	void SetEventHandler(CPanelFrameEventHandler *pHandler);
	bool SetPanelVisible(bool fVisible,bool fNoActivate=false);
	int GetDockingWidth() const { return m_DockingWidth; }
	bool SetDockingWidth(int Width);
	bool SetTitleColor(COLORREF crTitleBack,COLORREF crTitleText);
	bool SetOpacity(int Opacity);
	int GetOpacity() const { return m_Opacity; }
};


#endif
