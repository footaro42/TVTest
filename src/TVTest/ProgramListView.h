#ifndef PROGRAM_LIST_VIEW
#define PROGRAM_LIST_VIEW


#include "BasicWindow.h"
#include "EpgProgramList.h"


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

class CProgramListView : public CBasicWindow {
	CEpgProgramList *m_pProgramList;
	HFONT m_hfont;
	int m_FontHeight;
	int m_LineMargin;
	COLORREF m_crBackColor;
	COLORREF m_crTextColor;
	COLORREF m_crTitleBackColor;
	COLORREF m_crTitleTextColor;
	int m_TotalLines;
	CProgramItemList m_ItemList;
	int m_ScrollPos;
	static HINSTANCE m_hinst;
	static CProgramListView *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void DrawProgramList(HDC hdc,const RECT *prcPaint);
	bool UpdateListInfo(WORD TransportStreamID,WORD ServiceID);
	void CalcDimentions();
	void SetScrollBar();
public:
	static bool Initialize(HINSTANCE hinst);
	CProgramListView();
	~CProgramListView();
	void SetEpgProgramList(CEpgProgramList *pList) { m_pProgramList=pList; }
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool UpdateProgramList(WORD TransportStreamID,WORD ServiceID);
	bool OnProgramListChanged();
	void ClearProgramList();
	void SetColors(COLORREF crBackColor,COLORREF crTextColor,
						COLORREF crTitleBackColor,COLORREF crTitleTextColor);
};


#endif
