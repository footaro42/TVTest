#ifndef TOOLTIP_H
#define TOOLTIP_H


class CToolTip
{
	HWND m_hwndToolTips;
	HWND m_hwndOwner;

public:
	CToolTip();
	~CToolTip();
	bool Initialize(HWND hwnd);
	void Finalize();
	enum {
		ICON_NONE,
		ICON_INFO,
		ICON_WARNING,
		ICON_ERROR
	};
	bool Show(LPCTSTR pszText,LPCTSTR pszTitle,const POINT *pPos,int Icon=ICON_NONE);
	bool Hide();
};


#endif
