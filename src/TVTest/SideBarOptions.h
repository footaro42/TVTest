#ifndef SIDE_BAR_OPTIONS_H
#define SIDE_BAR_OPTIONS_H


#include <vector>
#include "Options.h"
#include "SideBar.h"
#include "ZoomOptions.h"


class CSideBarOptions : public COptions
{
public:
	enum PlaceType {
		PLACE_LEFT,
		PLACE_RIGHT,
		PLACE_TOP,
		PLACE_BOTTOM,
		PLACE_FIRST=PLACE_LEFT,
		PLACE_LAST=PLACE_BOTTOM
	};

protected:
	enum { ITEM_SEPARATOR=0 };
	CSideBar *m_pSideBar;
	const CZoomOptions *m_pZoomOptions;
	std::vector<int> m_ItemList;
	bool m_fShowPopup;
	bool m_fShowToolTips;
	PlaceType m_Place;
	HIMAGELIST m_himlIcons;

	HBITMAP CreateImage();
	void ApplyItemList() const;
	void SetItemList(HWND hwndList,const int *pList,int NumItems);
	static CSideBarOptions *GetThis(HWND hDlg);

public:
	CSideBarOptions(CSideBar *pSideBar,const CZoomOptions *pZoomOptions);
	~CSideBarOptions();
// COptions
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
// CSideBarOptions
	bool ApplySideBarOptions();
	bool SetSideBarImage();
	bool ShowPopup() const { return m_fShowPopup; }
	PlaceType GetPlace() const { return m_Place; }
	bool SetPlace(PlaceType Place);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
