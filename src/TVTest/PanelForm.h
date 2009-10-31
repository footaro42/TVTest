#ifndef PANEL_FORM_H
#define PANEL_FORM_H


#include "BasicWindow.h"
#include "Theme.h"


class CPanelForm : public CBasicWindow
{
public:
	class CPage : public CBasicWindow {
	protected:
		static bool GetDefaultFont(LOGFONT *pFont);
		static HFONT CreateDefaultFont();
	public:
		CPage();
		virtual ~CPage()=0;
		virtual bool SetFont(const LOGFONT *pFont) { return true; }
	};

	class CEventHandler {
	public:
		virtual void OnSelChange() {}
		virtual void OnRButtonDown() {}
		virtual void OnTabRButtonDown(int x,int y) {}
		virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
		virtual void OnVisibleChange(bool fVisible) {}
	};

	struct TabInfo {
		int ID;
		bool fVisible;
	};

private:
	enum {MAX_WINDOWS=8};
	enum {TAB_MARGIN=3};
	class CWindowInfo {
	public:
		CPage *m_pWindow;
		int m_ID;
		LPTSTR m_pszTitle;
		bool m_fVisible;
		CWindowInfo(CPage *pWindow,int ID,LPCTSTR pszTitle);
		~CWindowInfo();
	};
	CWindowInfo *m_pWindowList[MAX_WINDOWS];
	int m_NumWindows;
	int m_TabOrder[MAX_WINDOWS];
	COLORREF m_crBackColor;
	COLORREF m_crMarginColor;
	Theme::GradientInfo m_TabBackGradient;
	COLORREF m_crTabTextColor;
	COLORREF m_crTabBorderColor;
	Theme::GradientInfo m_CurTabBackGradient;
	COLORREF m_crCurTabTextColor;
	COLORREF m_crCurTabBorderColor;
	HFONT m_hfont;
	int m_TabHeight;
	int m_TabWidth;
	int m_ClientMargin;
	int m_CurTab;
	CEventHandler *m_pEventHandler;

	static const LPCTSTR m_pszClassName;
	static HINSTANCE m_hinst;
	static CPanelForm *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool SetCurTab(int Index);
	void CalcTabSize();
	int HitTest(int x,int y) const;

public:
	static bool Initialize(HINSTANCE hinst);
	CPanelForm();
	~CPanelForm();
// CBasicWindow
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetVisible(bool fVisible);
// CPanelForm
	bool AddWindow(CPage *pWindow,int ID,LPCTSTR pszTitle);
	int NumPages() const { return m_NumWindows; }
	CPage *GetPageByIndex(int Index);
	CPage *GetPageByID(int ID);
	int IDToIndex(int ID) const;
	int GetCurPageID() const;
	bool SetCurPageByID(int ID);
	bool SetTabVisible(int ID,bool fVisible);
	bool GetTabVisible(int ID) const;
	bool SetTabOrder(const int *pOrder);
	bool GetTabInfo(int Index,TabInfo *pInfo) const;
	void SetEventHandler(CEventHandler *pHandler);
	void SetBackColors(COLORREF crBack,COLORREF crMargin);
	void SetTabColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,COLORREF crBorder);
	void SetCurTabColors(const Theme::GradientInfo *pBackGradient,COLORREF crText,COLORREF crBorder);
	bool SetTabFont(const LOGFONT *pFont);
	bool SetPageFont(const LOGFONT *pFont);
};


#endif
