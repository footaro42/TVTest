#ifndef TITLE_BAR_H
#define TITLE_BAR_H


#include "BasicWindow.h"


class CTitleBarEventHandler {
protected:
	class CTitleBar *m_pTitleBar;
public:
	CTitleBarEventHandler();
	virtual ~CTitleBarEventHandler();
	virtual bool OnClose() { return false; }
	virtual bool OnMinimize() { return false; }
	virtual bool OnMaximize() { return false; }
	virtual bool OnFullscreen() { return false; }
	virtual void OnMouseLeave() {}
	virtual void OnLabelLButtonDown(int x,int y) {}
	virtual void OnLabelRButtonDown(int x,int y) {}
	friend class CTitleBar;
};

class CTitleBar : public CBasicWindow {
	enum {
		ITEM_LABEL,
		ITEM_MINIMIZE,
		ITEM_MAXIMIZE,
		ITEM_FULLSCREEN,
		ITEM_CLOSE,
		ITEM_BUTTON_FIRST=ITEM_MINIMIZE,
		ITEM_LAST=ITEM_CLOSE
	};
	HFONT m_hfont;
	int m_FontHeight;
	COLORREF m_crBackColor1;
	COLORREF m_crBackColor2;
	COLORREF m_crTextColor;
	COLORREF m_crHighlightBackColor1;
	COLORREF m_crHighlightBackColor2;
	COLORREF m_crHighlightTextColor;
	HBITMAP m_hbmIcons;
	HWND m_hwndToolTip;
	LPTSTR m_pszLabel;
	int m_HotItem;
	int m_ClickItem;
	bool m_fTrackMouseEvent;
	bool m_fMaximized;
	CTitleBarEventHandler *m_pEventHandler;
	bool GetItemRect(int Item,RECT *pRect) const;
	bool UpdateItem(int Item);
	int HitTest(int x,int y) const;
	void SetToolTip();
	static HINSTANCE m_hinst;
	static CTitleBar *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	static bool Initialize(HINSTANCE hinst);
	CTitleBar();
	~CTitleBar();
	// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetVisible(bool fVisible);
	// CTitleBar
	bool SetLabel(LPCTSTR pszLabel);
	bool SetMaximizeMode(bool fMaximize);
	bool SetEventHandler(CTitleBarEventHandler *pHandler);
	void SetColor(COLORREF crBack1,COLORREF crBack2,COLORREF crText,
		COLORREF crHighlightBack1,COLORREF crHighlightBack2,COLORREF crHighlightText);
	//bool SetFont(const LOGFONT *pFont);
};


#endif
