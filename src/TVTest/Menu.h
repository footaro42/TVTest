#ifndef TVTEST_MENU_H
#define TVTEST_MENU_H


#include "Accelerator.h"
#include "ChannelList.h"
#include "EpgProgramList.h"
#include "PointerArray.h"
#include "Theme.h"
#include "LogoManager.h"


class CMainMenu
{
	HMENU m_hmenu;
	bool m_fPopup;
	int m_PopupMenu;

public:
	// サブメニュー項目の位置
	enum {
		SUBMENU_ZOOM			= 0,
		SUBMENU_ASPECTRATIO		= 1,
		SUBMENU_CHANNEL			= 5,
		SUBMENU_SERVICE			= 6,
		SUBMENU_SPACE			= 7,
		SUBMENU_CHANNELHISTORY	= 8,
		SUBMENU_VOLUME			= 10,
		SUBMENU_AUDIO			= 11,
		SUBMENU_BAR				= 25,
		SUBMENU_PLUGIN			= 26,
		SUBMENU_FILTERPROPERTY	= 28
	};
	CMainMenu();
	~CMainMenu();
	bool Create(HINSTANCE hinst);
	void Destroy();
	bool Popup(UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
	bool PopupSubMenu(int SubMenu,UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
	void EnableItem(int ID,bool fEnable);
	void CheckItem(int ID,bool fCheck);
	void CheckRadioItem(int FirstID,int LastID,int CheckID);
	HMENU GetSubMenu(int SubMenu);
	bool SetAccelerator(CAccelerator *pAccelerator);
};

class CChannelMenu
{
	HMENU m_hmenu;
	CEpgProgramList *m_pProgramList;
	CLogoManager *m_pLogoManager;
	const CChannelList *m_pChannelList;
	int m_CurChannel;
	HFONT m_hfont;
	HFONT m_hfontCurrent;
	int m_TextHeight;
	int m_ChannelNameWidth;
	int m_EventNameWidth;
	int m_LogoWidth;
	int m_LogoHeight;
	enum { MENU_MARGIN=2, MENU_LOGO_MARGIN=3 };
	void CreateFont(HDC hdc);

public:
	CChannelMenu(CEpgProgramList *pProgramList,CLogoManager *pLogoManager);
	~CChannelMenu();
	bool Create(const CChannelList *pChannelList,int CurChannel,bool fUpdateProgramList);
	void Destroy();
	bool Popup(UINT Flags,int x,int y,HWND hwnd);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
};

class CPopupMenu
{
	HMENU m_hmenu;

public:
	CPopupMenu();
	CPopupMenu(HINSTANCE hinst,LPCTSTR pszName);
	CPopupMenu(HINSTANCE hinst,int ID);
	~CPopupMenu();
	HMENU GetPopupHandle();
	bool EnableItem(int ID,bool fEnable);
	bool CheckItem(int ID,bool fCheck);
	bool CheckRadioItem(int FirstID,int LastID,int CheckID);
	bool Popup(HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON);
	bool Popup(HMENU hmenu,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true);
	bool Popup(HINSTANCE hinst,LPCTSTR pszName,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true);
	bool Popup(HINSTANCE hinst,int ID,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true) {
		return Popup(hinst,MAKEINTRESOURCE(ID),hwnd,pPos,Flags,fToggle);
	}
};

class CDropDownMenu
{
	class CItem {
		int m_Command;
		LPTSTR m_pszText;
		int m_Width;
	public:
		CItem(int Command,LPCTSTR pszText);
		~CItem();
		int GetCommand() const { return m_Command; }
		LPCTSTR GetText() const { return m_pszText; }
		bool SetText(LPCTSTR pszText);
		int GetWidth(HDC hdc);
		bool IsSeparator() const { return m_Command<0; }
		void Draw(HDC hdc,const RECT *pRect);
	};
	CPointerVector<CItem> m_ItemList;
	HFONT m_hfont;
	HWND m_hwnd;
	HWND m_hwndMessage;
	RECT m_ItemMargin;
	RECT m_WindowMargin;
	COLORREF m_TextColor;
	COLORREF m_BackColor;
	COLORREF m_HighlightTextColor;
	COLORREF m_HighlightBackColor;
	Theme::BorderType m_BorderType;
	int m_ItemHeight;
	int m_HotItem;
	bool m_fTrackMouseEvent;

	bool GetItemRect(int Index,RECT *pRect) const;
	int HitTest(int x,int y) const;
	void UpdateItem(int Index) const;
	void Draw(HDC hdc,const RECT *pPaintRect);

	static HINSTANCE m_hinst;
	static CDropDownMenu *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	CDropDownMenu();
	~CDropDownMenu();
	void Clear();
	bool AppendItem(int Command,LPCTSTR pszText);
	bool InsertItem(int Index,int Command,LPCTSTR pszText);
	bool AppendSeparator() { return AppendItem(-1,NULL); }
	bool InsertSeparator(int Index) { return InsertItem(Index,-1,NULL); }
	bool DeleteItem(int Command);
	bool SetItemText(int Command,LPCTSTR pszText);
	int CommandToIndex(int Command) const;
	bool Show(HWND hwndOwner,HWND hwndMessage,const POINT *pPos,int CurItem=-1,UINT Flags=0);
	bool Hide();
	bool GetPosition(RECT *pRect);
};


#endif
