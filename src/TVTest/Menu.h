#ifndef MENU_H
#define MENU_H


#include "ChannelList.h"
#include "EpgProgramList.h"


class CMainMenu {
	HMENU m_hmenu;
	bool m_fPopup;
public:
	enum {
		SUBMENU_ZOOM=0,
		SUBMENU_ASPECTRATIO=1,
		SUBMENU_CHANNEL=5,
		SUBMENU_SERVICE=6,
		SUBMENU_SPACE=7,
		SUBMENU_VOLUME=9,
		SUBMENU_STEREOMODE=10,
		SUBMENU_IMAGESIZE=19,
		SUBMENU_PLUGIN=28
	};
	CMainMenu();
	~CMainMenu();
	bool Create(HINSTANCE hinst);
	void Destroy();
	bool Popup(UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
	bool PopupSubMenu(int SubMenu,UINT Flags,int x,int y,HWND hwnd);
	void EnableItem(int ID,bool fEnable);
	void CheckItem(int ID,bool fCheck);
	void CheckRadioItem(int FirstID,int LastID,int CheckID);
	HMENU GetSubMenu(int SubMenu);
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


#endif
