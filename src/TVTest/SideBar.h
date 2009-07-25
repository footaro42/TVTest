#ifndef SIDE_BAR_H
#define SIDE_BAR_H


#include <vector>
#include "BasicWindow.h"
#include "Command.h"


class CSideBar : public CBasicWindow {
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

	class CEventHandler {
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
	HWND m_hwndToolTip;
	bool m_fShowToolTips;
	HBITMAP m_hbmIcons;
	COLORREF m_IconTransparentColor;
	bool m_fVertical;
	COLORREF m_BackColor1;
	COLORREF m_BackColor2;
	COLORREF m_ForeColor;
	COLORREF m_HighlightBackColor1;
	COLORREF m_HighlightBackColor2;
	COLORREF m_HighlightForeColor;
	std::vector<SideBarItem> m_ItemList;
	int m_HotItem;
	int m_ClickItem;
	bool m_fTrackMouseEvent;
	CEventHandler *m_pEventHandler;
	const CCommandList *m_pCommandList;

	void GetItemRect(int Item,RECT *pRect) const;
	void UpdateItem(int Item) const;
	int HitTest(int x,int y) const;
	void SetToolTip();

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
	void SetColor(COLORREF crBack1,COLORREF crBack2,COLORREF crFore,COLORREF crHighlightBack1,COLORREF crHighlightBack2,COLORREF crHighlightFore);
	void ShowToolTips(bool fShow);
	void SetEventHandler(CEventHandler *pHandler);
	const CCommandList *GetCommandList() const { return m_pCommandList; }
};


#endif
