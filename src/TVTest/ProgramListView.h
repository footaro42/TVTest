#ifndef PROGRAM_LIST_VIEW
#define PROGRAM_LIST_VIEW


#include "InfoPanel.h"
#include "EpgProgramList.h"
#include "Theme.h"


class CProgramItemInfo;

class CProgramItemList {
	int m_NumItems;
	CProgramItemInfo **m_ppItemList;
	int m_ItemListLength;
	void SortSub(CProgramItemInfo **ppFirst,CProgramItemInfo **ppLast);
public:
	CProgramItemList();
	~CProgramItemList();
	int NumItems() const { return m_NumItems; }
	CProgramItemInfo *GetItem(int Index);
	const CProgramItemInfo *GetItem(int Index) const;
	bool Add(CProgramItemInfo *pItem);
	void Clear();
	void Sort();
	void Reserve(int NumItems);
	void Attach(CProgramItemList *pList);
};

class CProgramListView : public CInfoPanelPage {
	CEpgProgramList *m_pProgramList;
	HFONT m_hfont;
	int m_FontHeight;
	int m_LineMargin;
	Theme::GradientInfo m_EventBackGradient;
	COLORREF m_EventTextColor;
	Theme::GradientInfo m_CurEventBackGradient;
	COLORREF m_CurEventTextColor;
	Theme::GradientInfo m_TitleBackGradient;
	COLORREF m_TitleTextColor;
	Theme::GradientInfo m_CurTitleBackGradient;
	COLORREF m_CurTitleTextColor;
	COLORREF m_MarginColor;
	int m_TotalLines;
	CProgramItemList m_ItemList;
	int m_CurEventID;
	int m_ScrollPos;
	static HINSTANCE m_hinst;
	static CProgramListView *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void DrawProgramList(HDC hdc,const RECT *prcPaint);
	bool UpdateListInfo(WORD TransportStreamID,WORD ServiceID);
	void CalcDimentions();
	void SetScrollBar();
	void CalcFontHeight();

public:
	static bool Initialize(HINSTANCE hinst);
	CProgramListView();
	~CProgramListView();
	void SetEpgProgramList(CEpgProgramList *pList) { m_pProgramList=pList; }
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool UpdateProgramList(WORD TransportStreamID,WORD ServiceID);
	bool OnProgramListChanged();
	void ClearProgramList();
	void SetCurrentEventID(int EventID);
	void SetColors(const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
		const Theme::GradientInfo *pCurEventBackGradient,COLORREF CurEventTextColor,
		const Theme::GradientInfo *pTitleBackGradient,COLORREF TitleTextColor,
		const Theme::GradientInfo *pCurTitleBackGradient,COLORREF CurTitleTextColor,
		COLORREF MarginColor);
	bool SetFont(const LOGFONT *pFont);
};


#endif
