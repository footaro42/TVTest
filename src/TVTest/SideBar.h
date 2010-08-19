#ifndef SIDE_BAR_H
#define SIDE_BAR_H


#include <vector>
#include "BasicWindow.h"
#include "Command.h"
#include "Theme.h"
#include "Tooltip.h"


class CSideBar : public CBasicWindow
{
public:
	enum {
		ITEM_SEPARATOR=0
	};

	struct SideBarItem {
		int Command;
		int Icon;
		unsigned int Flags;
	};
	enum {
		ITEM_FLAG_DISABLED	=0x0001,
		ITEM_FLAG_CHECKED	=0x0002
	};

	struct ThemeInfo {
		Theme::Style ItemStyle;
		Theme::Style HighlightItemStyle;
		Theme::BorderInfo Border;
	};

	class ABSTRACT_CLASS(CEventHandler) {
	protected:
		CSideBar *m_pSideBar;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnCommand(int Command) {}
		virtual void OnRButtonDown(int x,int y) {}
		virtual void OnMouseLeave() {}
		friend class CSideBar;
	};

protected:
	CTooltip m_Tooltip;
	bool m_fShowTooltips;
	HBITMAP m_hbmIcons;
	COLORREF m_IconTransparentColor;
	bool m_fVertical;
	/*
	Theme::GradientInfo m_BackGradient;
	COLORREF m_ForeColor;
	Theme::GradientInfo m_HighlightBackGradient;
	COLORREF m_HighlightForeColor;
	Theme::BorderInfo m_BorderInfo;
	*/
	ThemeInfo m_Theme;
	std::vector<SideBarItem> m_ItemList;
	int m_HotItem;
	int m_ClickItem;
	bool m_fTrackMouseEvent;
	CEventHandler *m_pEventHandler;
	const CCommandList *m_pCommandList;

	void GetItemRect(int Item,RECT *pRect) const;
	void UpdateItem(int Item) const;
	int HitTest(int x,int y) const;
	void UpdateTooltipsRect();
	void Draw(HDC hdc,const RECT &PaintRect);

	static HINSTANCE m_hinst;
	static CSideBar *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	CSideBar(const CCommandList *pCommandList);
	~CSideBar();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
// CSideBar
	int GetBarWidth() const;
	bool SetIconImage(HBITMAP hbm,COLORREF crTransparent);
	void DeleteAllItems();
	bool AddItem(const SideBarItem *pItem);
	bool AddItems(const SideBarItem *pItemList,int NumItems);
	/*
	void SetColor(const Theme::GradientInfo *pBackGradient,COLORREF crFore,
				  const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightFore);
	void SetBorder(const Theme::BorderInfo *pInfo);
	*/
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	void ShowToolTips(bool fShow);
	void SetVertical(bool fVertical);
	void SetEventHandler(CEventHandler *pHandler);
	const CCommandList *GetCommandList() const { return m_pCommandList; }
};


#endif
