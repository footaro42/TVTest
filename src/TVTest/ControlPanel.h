#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H


#include "PanelForm.h"
#include "Theme.h"


class CControlPanelItem;

class CControlPanel : public CPanelForm::CPage
 {
	enum { MAX_ITEMS=16 };
	CControlPanelItem *m_pItemList[MAX_ITEMS];
	int m_NumItems;
	HFONT m_hfont;
	int m_FontHeight;
	Theme::GradientInfo m_BackGradient;
	COLORREF m_crTextColor;
	Theme::GradientInfo m_OverBackGradient;
	COLORREF m_crOverTextColor;
	COLORREF m_crMarginColor;
	HWND m_hwndMessage;
	int m_HotItem;
	bool m_fTrackMouseEvent;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;
	static CControlPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void SendCommand(int Command);

public:
	static bool Initialize(HINSTANCE hinst);
	CControlPanel();
	~CControlPanel();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool AddItem(CControlPanelItem *pItem);
	bool UpdateItem(int Index);
	bool GetItemPosition(int Index,RECT *pRect) const;
	void SetColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,
				   const Theme::GradientInfo *pOverBackGradient,COLORREF crOverText,
				   COLORREF crMargin);
	//bool SetFont(const LOGFONT *pFont);
	int GetFontHeight() const { return m_FontHeight; }
	void SetSendMessageWindow(HWND hwnd);
	bool CheckRadioItem(int FirstID,int LastID,int CheckID);
	friend CControlPanelItem;
};

class CControlPanelItem {
protected:
	struct {
		int Left,Top;
		int Width,Height;
	} m_Position;
	int m_Command;
	bool m_fVisible;
	bool m_fEnable;
	bool m_fCheck;
	CControlPanel *m_pControlPanel;
public:
	CControlPanelItem();
	virtual ~CControlPanelItem();
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	bool SetPosition(int Left,int Top,int Width,int Height);
	void GetPosition(RECT *pRect) const;
	bool GetVisible() const { return m_fVisible; }
	void SetVisible(bool fVisible);
	bool GetEnable() const { return m_fEnable; }
	void SetEnable(bool fEnable);
	bool GetCheck() const { return m_fCheck; }
	void SetCheck(bool fCheck);
	virtual void Draw(HDC hdc)=0;
	virtual bool Rayout(int Width,int Height) { return false; }
	virtual void OnLButtonDown(int x,int y);
	virtual void OnRButtonDown(int x,int y) {}
	virtual void OnMouseMove(int x,int y) {}
	friend CControlPanel;
};

class CControlPanelButton : public CControlPanelItem {
	LPTSTR m_pszText;
public:
	CControlPanelButton(int Command,LPCTSTR pszText,
										int Left,int Top,int Width,int Height);
	virtual ~CControlPanelButton();
	virtual void Draw(HDC hdc);
};


#endif
