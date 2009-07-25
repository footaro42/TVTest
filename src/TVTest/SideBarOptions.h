#ifndef SIDE_BAR_OPTIONS_H
#define SIDE_BAR_OPTIONS_H


#include <vector>
#include "Options.h"
#include "SideBar.h"


class CSideBarOptions : public COptions {
	enum { ITEM_SEPARATOR=0 };
	CSideBar *m_pSideBar;
	std::vector<int> m_ItemList;
	bool m_fShowPopup;
	bool m_fShowToolTips;
	HIMAGELIST m_himlIcons;
	void ApplyItemList() const;
	void SetItemList(HWND hwndList,const int *pList,int NumItems);
	static CSideBarOptions *GetThis(HWND hDlg);

public:
	CSideBarOptions(CSideBar *pSideBar);
	~CSideBarOptions();
	// COptions
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	// CSideBarOptions
	bool ApplySideBarOptions();
	bool ShowPopup() const { return m_fShowPopup; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
