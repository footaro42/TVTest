#ifndef STATUS_OPTIONS_H
#define STATUS_OPTIONS_H


#include "Options.h"
#include "StatusItems.h"


#define NUM_STATUS_ITEMS (STATUS_ITEM_LAST+1)

class CStatusOptions : public COptions
{
	CStatusView *m_pStatusView;
	struct StatusItemInfo {
		int ID;
		bool fVisible;
		int Width;
	};
	StatusItemInfo m_ItemList[NUM_STATUS_ITEMS];
	StatusItemInfo m_ItemListCur[NUM_STATUS_ITEMS];
	WNDPROC m_pOldListProc;
	int m_ItemHeight;
	int m_TextWidth;
	int m_DropInsertPos;
	UINT m_DragTimerID;
	bool m_fDragResize;
	LOGFONT m_lfItemFont;
	bool m_fShowTOTTime;

	void SetDefaultItemList();
	void InitListBox(HWND hDlg);
	void CalcTextWidth(HWND hDlg);
	void SetListHExtent(HWND hDlg);
	void DrawInsertMark(HWND hwndList,int Pos);
	bool GetItemPreviewRect(HWND hwndList,int Index,RECT *pRect);
	bool IsCursorResize(HWND hwndList,int x,int y);
	bool ApplyItemList();

	static CStatusOptions *GetThis(HWND hDlg);
	static LRESULT CALLBACK ItemListProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CStatusOptions(CStatusView *pStatusView);
	~CStatusOptions();
// COptions
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
// CStatusOptions
	bool ApplyOptions();
	bool GetShowTOTTime() { return m_fShowTOTTime; }
	void SetShowTOTTime(bool fShow) { m_fShowTOTTime=fShow; }
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
