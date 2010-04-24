#ifndef TITLE_BAR_H
#define TITLE_BAR_H


#include "BasicWindow.h"
#include "Theme.h"
#include "DrawUtil.h"


class CTitleBar : public CBasicWindow
{
public:
	class ABSTRACT_DECL CEventHandler {
	protected:
		class CTitleBar *m_pTitleBar;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual bool OnClose() { return false; }
		virtual bool OnMinimize() { return false; }
		virtual bool OnMaximize() { return false; }
		virtual bool OnFullscreen() { return false; }
		virtual void OnMouseLeave() {}
		virtual void OnLabelLButtonDown(int x,int y) {}
		virtual void OnLabelLButtonDoubleClick(int x,int y) {}
		virtual void OnLabelRButtonDown(int x,int y) {}
		virtual void OnIconLButtonDown(int x,int y) {}
		virtual void OnIconLButtonDoubleClick(int x,int y) {}
		friend class CTitleBar;
	};

	static bool Initialize(HINSTANCE hinst);
	CTitleBar();
	~CTitleBar();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetVisible(bool fVisible);
// CTitleBar
	bool SetLabel(LPCTSTR pszLabel);
	LPCTSTR GetLabel() const { return m_pszLabel; }
	bool SetMaximizeMode(bool fMaximize);
	bool SetEventHandler(CEventHandler *pHandler);
	void SetColor(const Theme::GradientInfo *pBackGradient,COLORREF crText,
				  const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightText);
	void SetBorderType(Theme::BorderType Type);
	bool SetFont(const LOGFONT *pFont);
	void SetIcon(HICON hIcon);

private:
	enum {
		ITEM_LABEL,
		ITEM_MINIMIZE,
		ITEM_MAXIMIZE,
		ITEM_FULLSCREEN,
		ITEM_CLOSE,
		ITEM_BUTTON_FIRST=ITEM_MINIMIZE,
		ITEM_LAST=ITEM_CLOSE
	};
	DrawUtil::CFont m_Font;
	int m_FontHeight;
	Theme::GradientInfo m_BackGradient;
	COLORREF m_crTextColor;
	Theme::GradientInfo m_HighlightBackGradient;
	COLORREF m_crHighlightTextColor;
	Theme::BorderType m_BorderType;
	HBITMAP m_hbmIcons;
	HWND m_hwndToolTip;
	LPTSTR m_pszLabel;
	HICON m_hIcon;
	int m_HotItem;
	int m_ClickItem;
	bool m_fTrackMouseEvent;
	bool m_fMaximized;
	CEventHandler *m_pEventHandler;

	bool GetItemRect(int Item,RECT *pRect) const;
	bool UpdateItem(int Item);
	int HitTest(int x,int y) const;
	void SetToolTip();
	static HINSTANCE m_hinst;
	static CTitleBar *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
