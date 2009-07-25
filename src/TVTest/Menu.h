#ifndef MENU_H
#define MENU_H


#include "Accelerator.h"
#include "ChannelList.h"
#include "EpgProgramList.h"


class CMainMenu {
	HMENU m_hmenu;
	bool m_fPopup;
	int m_PopupMenu;
public:
	enum {
		SUBMENU_ZOOM=0,
		SUBMENU_ASPECTRATIO=1,
		SUBMENU_CHANNEL=5,
		SUBMENU_SERVICE=6,
		SUBMENU_SPACE=7,
		SUBMENU_CHANNELHISTORY=8,
		SUBMENU_VOLUME=10,
		SUBMENU_STEREOMODE=11,
		SUBMENU_BAR=25,
		SUBMENU_PLUGIN=26
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

class CChannelMenu {
	HMENU m_hmenu;
	CEpgProgramList *m_pProgramList;
	const CChannelList *m_pChannelList;
	HFONT m_hfont;
	int m_TextHeight;
	int m_ChannelNameWidth;
	int m_EventNameWidth;
	enum { MENU_MARGIN=4 };
public:
	CChannelMenu(CEpgProgramList *pProgramList);
	~CChannelMenu();
	bool Create(const CChannelList *pChannelList);
	void Destroy();
	bool Popup(UINT Flags,int x,int y,HWND hwnd);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
};

class CPopupMenu {
	HMENU m_hmenu;
public:
	CPopupMenu();
	~CPopupMenu();
	bool Popup(HMENU hmenu,UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
	bool Popup(HINSTANCE hinst,LPCTSTR pszName,UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
};


#endif
