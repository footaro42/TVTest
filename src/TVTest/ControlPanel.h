#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H


#include <vector>
#include "PanelForm.h"
#include "DrawUtil.h"
#include "Theme.h"


class CControlPanelItem;

class CControlPanel : public CPanelForm::CPage
{
public:
	struct ThemeInfo {
		Theme::Style ItemStyle;
		Theme::Style OverItemStyle;
		COLORREF MarginColor;
	};

	static bool Initialize(HINSTANCE hinst);
	CControlPanel();
	~CControlPanel();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool AddItem(CControlPanelItem *pItem);
	CControlPanelItem *GetItem(int Index) const;
	bool UpdateItem(int Index);
	bool GetItemPosition(int Index,RECT *pRect) const;
	void UpdateLayout();
	/*
	void SetColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,
				   const Theme::GradientInfo *pOverBackGradient,COLORREF crOverText,
				   COLORREF crMargin);
	*/
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	int GetFontHeight() const { return m_FontHeight; }
	void SetSendMessageWindow(HWND hwnd);
	bool CheckRadioItem(int FirstID,int LastID,int CheckID);

	friend CControlPanelItem;

private:
	std::vector<CControlPanelItem*> m_ItemList;
	int m_MarginSize;
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	/*
	Theme::GradientInfo m_BackGradient;
	COLORREF m_crTextColor;
	Theme::GradientInfo m_OverBackGradient;
	COLORREF m_crOverTextColor;
	COLORREF m_crMarginColor;
	*/
	ThemeInfo m_Theme;
	DrawUtil::COffscreen m_Offscreen;
	HWND m_hwndMessage;
	int m_HotItem;
	bool m_fTrackMouseEvent;
	bool m_fOnButtonDown;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;

	static CControlPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

	void Draw(HDC hdc,const RECT &PaintRect);
	void SendCommand(int Command);
	bool CalcTextSize(LPCTSTR pszText,SIZE *pSize);
};

class ABSTRACT_CLASS(CControlPanelItem)
{
protected:
	RECT m_Position;
	int m_Command;
	bool m_fVisible;
	bool m_fEnable;
	bool m_fCheck;
	bool m_fBreak;
	CControlPanel *m_pControlPanel;

	bool CalcTextSize(LPCTSTR pszText,SIZE *pSize) const;
	void GetMenuPos(POINT *pPos) const;

public:
	CControlPanelItem();
	virtual ~CControlPanelItem()=0;
	void GetPosition(int *pLeft,int *pTop,int *pWidth,int *pHeight) const;
	bool SetPosition(int Left,int Top,int Width,int Height);
	void GetPosition(RECT *pRect) const;
	bool GetVisible() const { return m_fVisible; }
	void SetVisible(bool fVisible);
	bool GetEnable() const { return m_fEnable; }
	void SetEnable(bool fEnable);
	bool GetCheck() const { return m_fCheck; }
	void SetCheck(bool fCheck);
	bool GetBreak() const { return m_fBreak; }
	void SetBreak(bool fBreak);
	virtual void CalcSize(int Width,SIZE *pSize);
	virtual void Draw(HDC hdc,const RECT &Rect)=0;
	virtual void OnLButtonDown(int x,int y);
	virtual void OnRButtonDown(int x,int y) {}
	virtual void OnMouseMove(int x,int y) {}

	friend CControlPanel;
};


#endif
