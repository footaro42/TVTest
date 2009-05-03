#ifndef ICON_MENU_H
#define ICON_MENU_H


class CIconMenu {
	enum {
		ICON_MARGIN=1,
		TEXT_MARGIN=3
	};
	HMENU m_hmenu;
	HIMAGELIST m_hImageList;
	int m_CheckItem;
public:
	CIconMenu();
	~CIconMenu();
	bool Initialize(HMENU hmenu,HINSTANCE hinst,LPCTSTR pszImageName,int IconWidth,COLORREF crMask);
	void Finalize();
	bool OnInitMenuPopup(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnMeasureItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	bool OnDrawItem(HWND hwnd,WPARAM wParam,LPARAM lParam);
	void SetCheckItem(int Item) { m_CheckItem=Item; }
	int GetCheckItem() const { return m_CheckItem; }
};


#endif
