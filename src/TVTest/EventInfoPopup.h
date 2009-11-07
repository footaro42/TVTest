#ifndef EVENT_INFO_POPUP_H
#define EVENT_INFO_POPUP_H


#include "BasicWindow.h"
#include "EpgProgramList.h"
#include "Theme.h"
#include "DrawUtil.h"
#include "RichEditUtil.h"


class CEventInfoPopup : protected CBasicWindow
{
public:
	class __declspec(novtable) CEventHandler
	{
	protected:
		CEventInfoPopup *m_pPopup;
	public:
		enum { COMMAND_FIRST=100 };

		CEventHandler();
		virtual ~CEventHandler();
		virtual bool OnMenuPopup(HMENU hmenu) { return true; }
		virtual void OnMenuSelected(int Command) {}
		friend class CEventInfoPopup;
	};
	friend class CEventInfoHandler;

	CEventInfoPopup();
	~CEventInfoPopup();
	bool Show(const CEventInfoData *pEventInfo,const RECT *pPos=NULL);
	bool Hide();
	bool IsVisible();
	bool IsHandle(HWND hwnd) const { return m_hwnd==hwnd; }
	void SetColor(COLORREF BackColor,COLORREF TextColor);
	void SetTitleColor(Theme::GradientInfo *pBackGradient,COLORREF TextColor);
	bool SetFont(const LOGFONT *pFont);
	void SetEventHandler(CEventHandler *pEventHandler);
	bool IsSelected() const;
	LPTSTR GetSelectedText() const;

	static bool Initialize(HINSTANCE hinst);

private:
	CEventInfoData m_EventInfo;
	HWND m_hwndEdit;
	CRichEditUtil m_RichEditUtil;
	COLORREF m_BackColor;
	COLORREF m_TextColor;
	Theme::GradientInfo m_TitleBackGradient;
	COLORREF m_TitleTextColor;
	DrawUtil::CFont m_Font;
	DrawUtil::CFont m_TitleFont;
	int m_TitleLineMargin;
	int m_TitleLineHeight;
	int m_TitleHeight;
	int m_ButtonSize;
	int m_ButtonMargin;
	CEventHandler *m_pEventHandler;

	//static const LPCTSTR m_pszPropName;
	static HINSTANCE m_hinst;

	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle,int ID);
	void SetEventInfo(const CEventInfoData *pEventInfo);
	void CalcTitleHeight();
	void GetCloseButtonRect(RECT *pRect) const;
	static CEventInfoPopup *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};

class CEventInfoPopupManager
{
public:
	class __declspec(novtable) CEventHandler
	{
	protected:
		CEventInfoPopup *m_pPopup;
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual bool HitTest(int x,int y,LPARAM *pParam)=0;
		virtual bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo)=0;
		virtual bool OnShow(const CEventInfoData *pInfo) { return true; }
		friend class CEventInfoPopupManager;
	};

	CEventInfoPopupManager(CEventInfoPopup *pPopup);
	~CEventInfoPopupManager();
	bool Initialize(HWND hwnd,CEventHandler *pEventHandler);
	void Finalize();

private:
	static const LPCTSTR m_pszPropName;
	CEventInfoPopup *m_pPopup;
	HWND m_hwnd;
	WNDPROC m_pOldWndProc;
	CEventHandler *m_pEventHandler;
	bool m_fTrackMouseEvent;
	LPARAM m_HitTestParam;

	static LRESULT CALLBACK HookWndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
