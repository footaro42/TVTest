#ifndef TVTEST_MENU_H
#define TVTEST_MENU_H


#include "Accelerator.h"
#include "ChannelList.h"
#include "EpgProgramList.h"
#include "PointerArray.h"
#include "Theme.h"
#include "LogoManager.h"
#include "DrawUtil.h"
#include "Tooltip.h"


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
	bool Show(UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
	bool PopupSubMenu(int SubMenu,UINT Flags,int x,int y,HWND hwnd,bool fToggle=true);
	void EnableItem(UINT ID,bool fEnable);
	void CheckItem(UINT ID,bool fCheck);
	void CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID);
	HMENU GetMenuHandle() const;
	HMENU GetSubMenu(int SubMenu) const;
	bool SetAccelerator(CAccelerator *pAccelerator);
};

class CMenuPainter
{
	CUxTheme m_UxTheme;
	bool m_fFlatMenu;

public:
	CMenuPainter();
	~CMenuPainter();
	void Initialize(HWND hwnd);
	void Finalize();
	void GetFont(LOGFONT *pFont);
	COLORREF GetTextColor(bool fHighlight=false);
	void DrawItemBackground(HDC hdc,const RECT &Rect,bool fHighlight=false);
	void GetItemMargins(MARGINS *pMargins);
	void GetMargins(MARGINS *pMargins);
	void GetBorderSize(SIZE *pSize);
	void DrawBackground(HDC hdc,const RECT &Rect);
	void DrawBorder(HDC hdc,const RECT &Rect);
	void DrawSeparator(HDC hdc,const RECT &Rect);
};

class CChannelMenu
{
	unsigned int m_Flags;
	HWND m_hwnd;
	HMENU m_hmenu;
	CEpgProgramList *m_pProgramList;
	CLogoManager *m_pLogoManager;
	const CChannelList *m_pChannelList;
	int m_CurChannel;
	UINT m_FirstCommand;
	UINT m_LastCommand;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_FontCurrent;
	int m_TextHeight;
	int m_ChannelNameWidth;
	int m_EventNameWidth;
	int m_LogoWidth;
	int m_LogoHeight;
	CMenuPainter m_MenuPainter;
	MARGINS m_Margins;
	int m_MenuLogoMargin;
	CTooltip m_Tooltip;

	int GetEventText(const CEventInfoData *pEventInfo,
					 LPTSTR pszText,int MaxLength) const;
	void CreateFont(HDC hdc);
	static void GetBaseTime(SYSTEMTIME *pTime);

public:
	enum {
		FLAG_SHOWEVENTINFO	=0x0001,
		FLAG_SHOWLOGO		=0x0002,
		FLAG_SHOWTOOLTIP	=0x0004,
		FLAG_SPACEBREAK		=0x0008,
		FLAG_SHARED			=0x1000
	};

	CChannelMenu(CEpgProgramList *pProgramList,CLogoManager *pLogoManager);
	~CChannelMenu();
	bool Create(const CChannelList *pChannelList,int CurChannel,UINT Command,
				HMENU hmenu,HWND hwnd,unsigned int Flags,int MaxRows=0);
	void Destroy();
	bool Show(UINT Flags,int x,int y);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnMenuSelect(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnUninitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam);
};

class CPopupMenu
{
	enum PopupMenuType {
		TYPE_RESOURCE,
		TYPE_CREATED,
		TYPE_ATTACHED
	};

	HMENU m_hmenu;
	PopupMenuType m_Type;

public:
	CPopupMenu();
	CPopupMenu(HINSTANCE hinst,LPCTSTR pszName);
	CPopupMenu(HINSTANCE hinst,UINT ID);
	CPopupMenu(HMENU hmenu);
	~CPopupMenu();
	bool Create();
	bool IsCreated() const { return m_hmenu!=NULL; }
	bool Load(HINSTANCE hinst,LPCTSTR pszName);
	bool Load(HINSTANCE hinst,UINT ID) { return Load(hinst,MAKEINTRESOURCE(ID)); }
	bool Attach(HMENU hmenu);
	void Destroy();
	int GetItemCount() const;
	void Clear();
	HMENU GetPopupHandle() const;
	bool Append(UINT ID,LPCTSTR pszText,UINT Flags=MF_ENABLED);
	bool AppendUnformatted(UINT ID,LPCTSTR pszText,UINT Flags=MF_ENABLED);
	bool Append(HMENU hmenu,LPCTSTR pszText,UINT Flags=MF_ENABLED);
	bool AppendSeparator();
	bool EnableItem(UINT ID,bool fEnable);
	bool CheckItem(UINT ID,bool fCheck);
	bool CheckRadioItem(UINT FirstID,UINT LastID,UINT CheckID,UINT Flags=MF_BYCOMMAND);
	HMENU GetSubMenu(int Pos) const;
	bool Show(HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON);
	bool Show(HMENU hmenu,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true);
	bool Show(HINSTANCE hinst,LPCTSTR pszName,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true);
	bool Show(HINSTANCE hinst,int ID,HWND hwnd,const POINT *pPos=NULL,UINT Flags=TPM_RIGHTBUTTON,bool fToggle=true) {
		return Show(hinst,MAKEINTRESOURCE(ID),hwnd,pPos,Flags,fToggle);
	}
};

class CIconMenu
{
	enum {
		ICON_MARGIN=1,
		TEXT_MARGIN=3
	};
	HMENU m_hmenu;
	HIMAGELIST m_hImageList;
	UINT m_FirstID;
	UINT m_LastID;
	int m_CheckItem;

public:
	CIconMenu();
	~CIconMenu();
	bool Initialize(HMENU hmenu,HINSTANCE hinst,LPCTSTR pszImageName,
					int IconWidth,COLORREF crMask,
					UINT FirstID,UINT LastID);
	void Finalize();
	bool OnInitMenuPopup(HWND hwnd,HMENU hmenu);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	void SetCheckItem(int Item) { m_CheckItem=Item; }
	int GetCheckItem() const { return m_CheckItem; }
};

class CDropDownMenu
{
public:
	class CItem {
	public:
		CItem(int Command,LPCTSTR pszText);
		virtual ~CItem();
		int GetCommand() const { return m_Command; }
		LPCTSTR GetText() const { return m_pszText; }
		bool SetText(LPCTSTR pszText);
		bool IsSeparator() const { return m_Command<0; }
		virtual int GetWidth(HDC hdc);
		virtual void Draw(HDC hdc,const RECT *pRect);

	protected:
		int m_Command;
		LPTSTR m_pszText;
		int m_Width;
	};

	static bool Initialize(HINSTANCE hinst);

	CDropDownMenu();
	~CDropDownMenu();
	void Clear();
	bool AppendItem(CItem *pItem);
	bool InsertItem(int Index,CItem *pItem);
	bool AppendSeparator();
	bool InsertSeparator(int Index);
	bool DeleteItem(int Command);
	bool SetItemText(int Command,LPCTSTR pszText);
	int CommandToIndex(int Command) const;
	bool Show(HWND hwndOwner,HWND hwndMessage,const POINT *pPos,int CurItem=-1,UINT Flags=0);
	bool Hide();
	bool GetPosition(RECT *pRect);

private:
	CPointerVector<CItem> m_ItemList;
	HWND m_hwnd;
	HWND m_hwndMessage;
	MARGINS m_ItemMargin;
	MARGINS m_WindowMargin;
	DrawUtil::CFont m_Font;
	CMenuPainter m_MenuPainter;
	int m_ItemWidth;
	int m_ItemHeight;
	int m_MaxRows;
	int m_HotItem;
	bool m_fTrackMouseEvent;

	bool GetItemRect(int Index,RECT *pRect) const;
	int HitTest(int x,int y) const;
	void UpdateItem(int Index) const;
	void Draw(HDC hdc,const RECT *pPaintRect);

	static HINSTANCE m_hinst;
	static CDropDownMenu *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
