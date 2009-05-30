#ifndef PROGRAM_GUIDE_H
#define PROGRAM_GUIDE_H


#include "BasicWindow.h"
#include "EpgProgramList.h"
#include "ChannelList.h"
#include "DriverManager.h"


class CProgramGuideServiceInfo;

class CProgramGuideServiceList {
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

class CProgramGuideEventHandler {
protected:
	class CProgramGuide *m_pProgramGuide;

public:
	CProgramGuideEventHandler();
	virtual ~CProgramGuideEventHandler();
	virtual bool OnClose() { return true; }
	virtual void OnServiceTitleLButtonDown(LPCTSTR pszDriverFileName,const CServiceInfoData *pServiceInfo) {}
	virtual bool OnBeginUpdate() { return true; }
	virtual void OnEndUpdate() {}
	virtual bool OnRefresh() { return true; }
	virtual bool OnKeyDown(UINT KeyCode,UINT Flags) { return false; }
	friend class CProgramGuide;
};

class CProgramGuideTool {
public:
	enum { MAX_NAME=64, MAX_COMMAND=MAX_PATH*2 };

private:
	TCHAR m_szName[MAX_NAME];
	TCHAR m_szCommand[MAX_COMMAND];
	static LPTSTR GetCommandFileName(LPCTSTR *ppszCommand,LPTSTR pszFileName);
	static CProgramGuideTool *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

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

class CProgramGuideToolList {
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

class CProgramGuide : public CBasicWindow {
	CEpgProgramList *m_pProgramList;
	CProgramGuideServiceList m_ServiceList;
	int m_LinesPerHour;
	HFONT m_hfont;
	HFONT m_hfontTitle;
	int m_FontHeight;
	int m_LineMargin;
	int m_ItemWidth;
	int m_ItemMargin;
	int m_TextLeftMargin;
	int m_ServiceNameHeight;
	int m_TimeBarWidth;
	HFONT m_hfontTime;
	POINT m_ScrollPos;
	CChannelList m_ChannelList;
	CTuningSpaceList m_TuningSpaceList;
	int m_CurrentTuningSpace;
	TCHAR m_szDriverFileName[MAX_PATH];
	const CDriverManager *m_pDriverManager;
	SYSTEMTIME m_stFirstTime;
	SYSTEMTIME m_stLastTime;
	int m_Day;
	int m_Hours;
	struct {
		bool fValid;
		int Service;
		int Program;
	} m_CurItem;
	bool m_fUpdating;
	CProgramGuideEventHandler *m_pEventHandler;
	enum { NUM_COLORS=19 };
	COLORREF m_ColorList[NUM_COLORS];
	CProgramGuideToolList m_ToolList;
	bool UpdateList();
	bool SetTuningSpace(int Space);
	void CalcLayout();
	void DrawProgramList(int Service,HDC hdc,const RECT *pRect,const RECT *pPaintRect);
	void DrawServiceName(int Service,HDC hdc,const RECT *pRect);
	void DrawTimeBar(HDC hdc,const RECT *pRect);
	void GetProgramGuideRect(RECT *pRect);
	void Scroll(int XOffset,int YOffset);
	void SetScrollBar();
	void SetTitleBar();
	bool HitTest(int x,int y,int *pServiceIndex,int *pProgramIndex);
	static HINSTANCE m_hinst;
	static CProgramGuide *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

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
		COLOR_CHANNELNAMEBACK,
		COLOR_CHANNELNAMETEXT,
		COLOR_TIMEBACK,
		COLOR_TIMETEXT,
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
		COLOR_LAST=COLOR_CONTENT_LAST
	};
	enum { MIN_LINES_PER_HOUR=8, MAX_LINES_PER_HOUR=50 };
	enum { MIN_ITEM_WIDTH=100, MAX_ITEM_WIDTH=500 };
	static bool Initialize(HINSTANCE hinst);
	CProgramGuide();
	~CProgramGuide();
	bool SetEpgProgramList(CEpgProgramList *pList);
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool UpdateProgramGuide();
	bool SetTuningSpaceList(LPCTSTR pszDriverFileName,const CTuningSpaceList *pList,int Space);
	const CChannelList *GetChannelList() const { return &m_ChannelList; }
	bool UpdateChannelList();
	bool SetDriverList(const CDriverManager *pDriverManager);
	bool SetTimeRange(const SYSTEMTIME *pFirstTime,const SYSTEMTIME *pLastTime);
	bool GetTimeRange(SYSTEMTIME *pFirstTime,SYSTEMTIME *pLastTime);
	bool SetViewDay(int Day);
	int GetViewDay() const { return m_Day; }
	int GetLinesPerHour() const { return m_LinesPerHour; }
	int GetItemWidth() const { return m_ItemWidth; }
	bool SetUIOptions(int LinesPerHour,int ItemWidth);
	bool SetColor(int Type,COLORREF Color);
	bool SetFont(const LOGFONT *pFont);
	bool SetEventHandler(CProgramGuideEventHandler *pEventHandler);
	CProgramGuideToolList *GetToolList() { return &m_ToolList; }
};


#endif
