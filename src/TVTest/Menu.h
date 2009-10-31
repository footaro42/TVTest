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
//#ifndef TVH264
		SUBMENU_BAR=25,
		SUBMENU_PLUGIN=26
/*
#else
		SUBMENU_BAR=26,
		SUBMENU_PLUGIN=27
#endif
*/
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
	int m_CurChannel;
	HFONT m_hfont;
	HFONT m_hfontCurrent;
	int m_TextHeight;
	int m_ChannelNameWidth;
	int m_EventNameWidth;
	enum { MENU_MARGIN=2 };
	void CreateFont(HDC hdc);
public:
	CChannelMenu(CEpgProgramList *pProgramList);
	~CChannelMenu();
	bool Create(const CChannelList *pChannelList,int CurChannel,bool fUpdateProgramList);
	void Destroy();
	bool Popup(UINT Flags,int x,int y,HWND hwnd);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
};

class CPopupMenu {
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


#endif
