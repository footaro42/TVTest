#ifndef PROGRAM_LIST_PANEL_H
#define PROGRAM_LIST_PANEL_H


#include "PanelForm.h"
#include "EpgProgramList.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "EventInfoPopup.h"


class CProgramItemInfo;

class CProgramItemList
{
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

class CProgramListPanel : public CPanelForm::CPage
{
public:
	struct ThemeInfo {
		Theme::Style EventNameStyle;
		Theme::Style CurEventNameStyle;
		Theme::Style EventTextStyle;
		Theme::Style CurEventTextStyle;
		COLORREF MarginColor;
	};

	static bool Initialize(HINSTANCE hinst);
	CProgramListPanel();
	~CProgramListPanel();
	void SetEpgProgramList(CEpgProgramList *pList) { m_pProgramList=pList; }
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool UpdateProgramList(WORD TransportStreamID,WORD ServiceID);
	bool OnProgramListChanged();
	void ClearProgramList();
	void SetCurrentEventID(int EventID);
	/*
	void SetColors(const Theme::GradientInfo *pEventBackGradient,COLORREF EventTextColor,
		const Theme::GradientInfo *pCurEventBackGradient,COLORREF CurEventTextColor,
		const Theme::GradientInfo *pTitleBackGradient,COLORREF TitleTextColor,
		const Theme::GradientInfo *pCurTitleBackGradient,COLORREF CurTitleTextColor,
		COLORREF MarginColor);
	*/
	bool SetTheme(const ThemeInfo *pTheme);
	bool GetTheme(ThemeInfo *pTheme) const;
	bool SetFont(const LOGFONT *pFont);
	bool SetEventInfoFont(const LOGFONT *pFont);

private:
	CEpgProgramList *m_pProgramList;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_TitleFont;
	int m_FontHeight;
	int m_LineMargin;
	int m_TitleMargin;
	/*
	Theme::GradientInfo m_EventBackGradient;
	COLORREF m_EventTextColor;
	Theme::GradientInfo m_CurEventBackGradient;
	COLORREF m_CurEventTextColor;
	Theme::GradientInfo m_TitleBackGradient;
	COLORREF m_TitleTextColor;
	Theme::GradientInfo m_CurTitleBackGradient;
	COLORREF m_CurTitleTextColor;
	COLORREF m_MarginColor;
	*/
	ThemeInfo m_Theme;
	int m_TotalLines;
	CProgramItemList m_ItemList;
	int m_CurEventID;
	int m_ScrollPos;
	//HWND m_hwndToolTip;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
	{
		CProgramListPanel *m_pPanel;
	public:
		CEventInfoPopupHandler(CProgramListPanel *pPanel);
		bool HitTest(int x,int y,LPARAM *pParam);
		bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
	};
	friend CEventInfoPopupHandler;
	CEventInfoPopupHandler m_EventInfoPopupHandler;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;
	static CProgramListPanel *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	void DrawProgramList(HDC hdc,const RECT *prcPaint);
	bool UpdateListInfo(WORD TransportStreamID,WORD ServiceID);
	void CalcDimensions();
	void SetScrollPos(int Pos);
	void SetScrollBar();
	void CalcFontHeight();
	int HitTest(int x,int y) const;
	//void SetToolTip();
};


#endif
