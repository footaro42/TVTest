#ifndef BASIC_WINDOW_H
#define BASIC_WINDOW_H


class CBasicWindow {
protected:
	HWND m_hwnd;
	struct {
		int Left,Top;
		int Width,Height;
	} m_WindowPosition;
	bool CreateBasicWindow(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID,
						LPCTSTR pszClassName,LPCTSTR pszText,HINSTANCE hinst);
	static CBasicWindow *OnCreate(HWND hwnd,LPARAM lParam);
	void OnDestroy();
	static CBasicWindow *GetBasicWindow(HWND hwnd);
public:
	CBasicWindow();
	virtual ~CBasicWindow();
	virtual bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0)=0;
	void Destroy();
	bool SetPosition(int Left,int Top,int Width,int Height);
	bool SetPosition(const RECT *pPosition);
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	void GetPosition(RECT *pPosition) const;
	int GetWidth() const;
	int GetHeight() const;
	bool GetScreenPosition(RECT *pPosition) const;
	virtual void SetVisible(bool fVisible);
	bool GetVisible() const;
	bool GetMaximize() const;
	HWND GetHandle() const { return m_hwnd; }
	bool Invalidate(bool fErase=true);
	bool Update();
	bool GetClientRect(RECT *pRect) const;
	bool GetClientSize(SIZE *pSize) const;
	bool SetParent(HWND hwnd);
	bool SetParent(CBasicWindow *pWindow);
	HWND GetParent() const;
	bool MoveToMonitorInside();
	DWORD GetStyle() const;
	bool SetStyle(DWORD Style,bool fFrameChange=false);
	DWORD GetExStyle() const;
	bool SetExStyle(DWORD ExStyle,bool fFrameChange=false);
	LRESULT SendMessage(UINT Msg,WPARAM wParam,LPARAM lParam);
	bool PostMessage(UINT Msg,WPARAM wParam,LPARAM lParam);
};


#endif
