#ifndef PROGRAM_GUIDE_H
#define PROGRAM_GUIDE_H


#include "BasicWindow.h"
#include "EpgProgramList.h"
#include "ChannelList.h"
#include "DriverManager.h"
#include "LogoManager.h"
#include "Theme.h"
#include "ProgramSearch.h"
#include "EventInfoPopup.h"
#include "StatusView.h"


class CProgramGuideServiceInfo;

class CProgramGuideServiceList
{
	CProgramGuideServiceInfo **m_ppServiceList;
	int m_NumServices;
	int m_ServiceListLength;

public:
	CProgramGuideServiceList();
	~CProgramGuideServiceList();
	int NumServices() const { return m_NumServices; }
	CProgramGuideServiceInfo *GetItem(int Index);
	bool Add(CProgramGuideServiceInfo *pInfo);
	void Clear();
};

class CProgramGuideEventHandler
{
protected:
	class CProgramGuide *m_pProgramGuide;

public:
	CProgramGuideEventHandler();
	virtual ~CProgramGuideEventHandler();
	virtual bool OnClose() { return true; }
	virtual void OnDestroy() {}
	virtual void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo) {}
	virtual bool OnBeginUpdate() { return true; }
	virtual void OnEndUpdate() {}
	virtual bool OnRefresh() { return true; }
	virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
	friend class CProgramGuide;
};

class CProgramGuideTool
{
public:
	enum { MAX_NAME=64, MAX_COMMAND=MAX_PATH*2 };

private:
	TCHAR m_szName[MAX_NAME];
	TCHAR m_szCommand[MAX_COMMAND];
	static LPTSTR GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName);
	static CProgramGuideTool *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CProgramGuideTool();
	CProgramGuideTool(const CProgramGuideTool &Tool);
	CProgramGuideTool(LPCTSTR pszName,LPCTSTR pszCommand);
	~CProgramGuideTool();
	CProgramGuideTool &operator=(const CProgramGuideTool &Tool);
	LPCTSTR GetName() const { return m_szName; }
	LPCTSTR GetCommand() const { return m_szCommand; }
	bool Execute(const CProgramGuideServiceInfo *pServiceInfo,int Program);
	bool ShowDialog(HWND hwndOwner);
};

class CProgramGuideToolList
{
	CProgramGuideTool **m_ppToolList;
	int m_NumTools;
	int m_ToolListLength;

public:
	CProgramGuideToolList();
	CProgramGuideToolList(const CProgramGuideToolList &List);
	~CProgramGuideToolList();
	CProgramGuideToolList &operator=(const CProgramGuideToolList &List);
	void Clear();
	bool Add(CProgramGuideTool *pTool);
	CProgramGuideTool *GetTool(int Index);
	const CProgramGuideTool *GetTool(int Index) const;
	int NumTools() const { return m_NumTools; }
};

class CProgramGuide : public CBasicWindow
{
public:
	enum {
		DAY_TODAY,
		DAY_TOMORROW,
		DAY_DAYAFTERTOMORROW,
		DAY_2DAYSAFTERTOMORROW,
		DAY_3DAYSAFTERTOMORROW,
		DAY_4DAYSAFTERTOMORROW,
		DAY_5DAYSAFTERTOMORROW,
		DAY_LAST=DAY_5DAYSAFTERTOMORROW
	};
	enum {
		COLOR_BACK,
		COLOR_TEXT,
		COLOR_CHANNELNAMETEXT,
		COLOR_CURCHANNELNAMETEXT,
		COLOR_TIMETEXT,
		COLOR_TIMELINE,
		COLOR_CURTIMELINE,
		COLOR_CONTENT_NEWS,
		COLOR_CONTENT_SPORTS,
		COLOR_CONTENT_INFORMATION,
		COLOR_CONTENT_DRAMA,
		COLOR_CONTENT_MUSIC,
		COLOR_CONTENT_VARIETY,
		COLOR_CONTENT_MOVIE,
		COLOR_CONTENT_ANIME,
		COLOR_CONTENT_DOCUMENTARY,
		COLOR_CONTENT_THEATER,
		COLOR_CONTENT_EDUCATION,
		COLOR_CONTENT_WELFARE,
		COLOR_CONTENT_OTHER,
		COLOR_CONTENT_FIRST=COLOR_CONTENT_NEWS,
		COLOR_CONTENT_LAST=COLOR_CONTENT_OTHER,
		COLOR_LAST=COLOR_CONTENT_LAST,
		NUM_COLORS=COLOR_LAST+1
	};
	enum { MIN_LINES_PER_HOUR=8, MAX_LINES_PER_HOUR=50 };
	enum { MIN_ITEM_WIDTH=100, MAX_ITEM_WIDTH=500 };
	enum { TIME_BAR_BACK_COLORS=8 };
	class CFrame {
		CProgramGuide *m_pProgramGuide;
	public:
		CFrame();
		virtual ~CFrame() = 0;
		virtual void SetCaption(LPCTSTR pszCaption) {}
		virtual void OnDateChanged() {}
		virtual void OnSpaceChanged() {}
		virtual bool OnCommand(int Command) { return false; }
		virtual bool SetAlwaysOnTop(bool fTop) { return false; }
		virtual bool GetAlwaysOnTop() const { return false; }
		friend CProgramGuide;
	};

private:
	CEpgProgramList *m_pProgramList;
	CProgramGuideServiceList m_ServiceList;
	int m_LinesPerHour;
	HFONT m_hfont;
	HFONT m_hfontTitle;
	HFONT m_hfontTime;
	int m_FontHeight;
	int m_LineMargin;
	int m_ItemWidth;
	int m_ItemMargin;
	int m_TextLeftMargin;
	int m_ServiceNameHeight;
	int m_TimeBarWidth;
	POINT m_ScrollPos;
	bool m_fDragScroll;
	HCURSOR m_hDragCursor1;
	HCURSOR m_hDragCursor2;
	struct {
		POINT StartCursorPos;
		POINT StartScrollPos;
	} m_DragInfo;
	//HWND m_hwndToolTip;
	CEventInfoPopup m_EventInfoPopup;
	CEventInfoPopupManager m_EventInfoPopupManager;
	class CEventInfoPopupHandler : public CEventInfoPopupManager::CEventHandler
								 , public CEventInfoPopup::CEventHandler
	{
		CProgramGuide *m_pProgramGuide;
	public:
		CEventInfoPopupHandler(CProgramGuide *pProgramGuide);
	// CEventInfoPopupManager::CEventHandler
		bool HitTest(int x,int y,LPARAM *pParam);
		bool GetEventInfo(LPARAM Param,const CEventInfoData **ppInfo);
		bool OnShow(const CEventInfoData *pInfo);
	// CEventInfoPopup::CEventHandler
		bool OnMenuPopup(HMENU hmenu);
		void OnMenuSelected(int Command);
	};
	friend CEventInfoPopupHandler;
	CEventInfoPopupHandler m_EventInfoPopupHandler;
	bool m_fShowToolTip;
	CChannelList m_ChannelList;
	CTuningSpaceList m_TuningSpaceList;
	int m_CurrentTuningSpace;
	WORD m_CurrentTransportStreamID;
	WORD m_CurrentServiceID;
	TCHAR m_szDriverFileName[MAX_PATH];
	const CDriverManager *m_pDriverManager;
	SYSTEMTIME m_stFirstTime;
	SYSTEMTIME m_stLastTime;
	SYSTEMTIME m_stCurTime;
	int m_Day;
	int m_Hours;
	struct {
		bool fValid;
		int Service;
		int Program;
	} m_CurItem;
	bool m_fUpdating;
	CProgramGuideEventHandler *m_pEventHandler;
	CFrame *m_pFrame;
	CLogoManager *m_pLogoManager;
	COLORREF m_ColorList[NUM_COLORS];
	Theme::GradientInfo m_ChannelNameBackGradient;
	Theme::GradientInfo m_CurChannelNameBackGradient;
	Theme::GradientInfo m_TimeBarMarginGradient;
	Theme::GradientInfo m_TimeBarBackGradient[TIME_BAR_BACK_COLORS];
	CProgramGuideToolList m_ToolList;
	int m_WheelScrollLines;

	CProgramSearch m_ProgramSearch;
	class CProgramSearchEventHandler : public CProgramSearch::CEventHandler {
		CProgramGuide *m_pProgramGuide;
	public:
		CProgramSearchEventHandler(CProgramGuide *pProgramGuide);
		bool Search(LPCTSTR pszKeyword);
	};
	friend class CProgramSearchEventHandler;
	CProgramSearchEventHandler m_ProgramSearchEventHandler;

	bool UpdateList();
	bool SetTuningSpace(int Space);
	void CalcLayout();
	void DrawProgramList(int Service,HDC hdc,const RECT *pRect,const RECT *pPaintRect);
	void DrawServiceName(int Service,HDC hdc,const RECT *pRect);
	void DrawTimeBar(HDC hdc,const RECT *pRect,bool fRight);
	void Draw(HDC hdc,const RECT *pPaintRect);
	int GetCurTimeLinePos() const;
	void GetProgramGuideRect(RECT *pRect) const;
	void Scroll(int XOffset,int YOffset);
	void SetScrollBar();
	void SetCaption();
	//void SetToolTip();
	bool HitTest(int x,int y,int *pServiceIndex,int *pProgramIndex);

	static HINSTANCE m_hinst;
	static CProgramGuide *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	CProgramGuide();
	~CProgramGuide();
	bool SetEpgProgramList(CEpgProgramList *pList);
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void Clear();
	bool UpdateProgramGuide();
	bool SetTuningSpaceList(LPCTSTR pszDriverFileName,const CTuningSpaceList *pList,int Space);
	const CChannelList *GetChannelList() const { return &m_ChannelList; }
	bool UpdateChannelList();
	bool SetDriverList(const CDriverManager *pDriverManager);
	void SetCurrentService(WORD TSID,WORD ServiceID);
	void ClearCurrentService() { SetCurrentService(0,0); }
	int GetCurrentTuningSpace() const { return m_CurrentTuningSpace; }
	bool GetTuningSpaceName(int Space,LPTSTR pszName,int MaxName) const;
	bool EnumDriver(int *pIndex,LPTSTR pszName,int MaxName) const;
	bool SetTimeRange(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime);
	bool GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool GetCurrentTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime) const;
	bool SetViewDay(int Day);
	int GetViewDay() const { return m_Day; }
	int GetLinesPerHour() const { return m_LinesPerHour; }
	int GetItemWidth() const { return m_ItemWidth; }
	bool SetUIOptions(int LinesPerHour,int ItemWidth);
	bool SetColor(int Type,COLORREF Color);
	void SetBackColors(const Theme::GradientInfo *pChannelBackGradient,
					   const Theme::GradientInfo *pCurChannelBackGradient,
					   const Theme::GradientInfo *pTimeBarMarginGradient,
					   const Theme::GradientInfo *pTimeBarBackGradient);
	bool SetFont(const LOGFONT *pFont);
	bool SetEventInfoFont(const LOGFONT *pFont);
	bool SetShowToolTip(bool fShow);
	bool GetShowToolTip() const { return m_fShowToolTip; }
	bool SetEventHandler(CProgramGuideEventHandler *pEventHandler);
	bool SetFrame(CFrame *pFrame);
	bool SetLogoManager(CLogoManager *pLogoManager);
	CProgramGuideToolList *GetToolList() { return &m_ToolList; }
	int GetWheelScrollLines() const { return m_WheelScrollLines; }
	void SetWheelScrollLines(int Lines) { m_WheelScrollLines=Lines; }
	bool GetDragScroll() const { return m_fDragScroll; }
	bool SetDragScroll(bool fDragScroll);
	CProgramSearch *GetProgramSearch() { return &m_ProgramSearch; }
	bool OnClose();
};

class CProgramGuideFrame : public CBasicWindow, public CProgramGuide::CFrame
{
	CProgramGuide *m_pProgramGuide;
	CStatusView m_StatusView[2];
	CAeroGlass m_AeroGlass;
	SIZE m_StatusMargin;
	bool m_fAlwaysOnTop;

// CProgramGuide::CFrame
	void SetCaption(LPCTSTR pszCaption);
	void OnDateChanged();
	void OnSpaceChanged();
	bool OnCommand(int Command);

	static HINSTANCE m_hinst;
	static CProgramGuideFrame *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	static bool Initialize(HINSTANCE hinst);
	CProgramGuideFrame(CProgramGuide *pProgramGuide);
	~CProgramGuideFrame();
	CProgramGuide *GetProgramGuide() { return m_pProgramGuide; }
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	void SetStatusColor(const Theme::GradientInfo *pBackGradient,COLORREF crText,
						const Theme::GradientInfo *pHighlightBackGradient,COLORREF crHighlightText);
	void SetStatusBorderType(Theme::BorderType Type);
// CProgramGuide::CFrame
	bool SetAlwaysOnTop(bool fTop);
	bool GetAlwaysOnTop() const { return m_fAlwaysOnTop; }
};


#endif
