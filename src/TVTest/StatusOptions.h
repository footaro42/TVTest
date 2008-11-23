#ifndef STATUS_OPTIONS_H
#define STATUS_OPTIONS_H


#include "Options.h"
#include "StatusView.h"


enum {
	STATUS_ITEM_CHANNEL,
	STATUS_ITEM_VIDEOSIZE,
	STATUS_ITEM_VOLUME,
	STATUS_ITEM_AUDIOCHANNEL,
	STATUS_ITEM_RECORD,
	STATUS_ITEM_CAPTURE,
	STATUS_ITEM_ERROR,
	STATUS_ITEM_SIGNALLEVEL,
	STATUS_ITEM_CLOCK,
	STATUS_ITEM_PROGRAMINFO,
	STATUS_ITEM_BUFFERING,
	STATUS_ITEM_TUNER,
	STATUS_ITEM_LAST=STATUS_ITEM_TUNER
};

#define NUM_STATUS_ITEMS (STATUS_ITEM_LAST+1)

class CStatusOptions : public COptions {
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
	void SetDefaultItemList();
	void InitListBox(HWND hDlg);
	void CalcTextWidth(HWND hDlg);
	void SetListHExtent(HWND hDlg);
	void DrawInsertMark(HWND hwndList,int Pos);
	bool GetItemPreviewRect(HWND hwndList,int Index,RECT *pRect);
	bool IsCursorResize(HWND hwndList,int x,int y);
	static LRESULT CALLBACK ItemListProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool ApplyItemList();
	static CStatusOptions *GetThis(HWND hDlg);
public:
	CStatusOptions(CStatusView *pStatusView);
	~CStatusOptions();
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool ApplyOptions();
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
