#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "UISkin.h"
#include "View.h"
#include "ChannelManager.h"
#include "Layout.h"
#include "TitleBar.h"
#include "StatusView.h"
#include "Settings.h"
#include "NotificationBar.h"
#include "Panel.h"
#include "OSDManager.h"


#define MAIN_WINDOW_CLASS		APP_NAME TEXT(" Window")
#define FULLSCREEN_WINDOW_CLASS	APP_NAME TEXT(" Fullscreen")

#define MAIN_TITLE_TEXT APP_NAME

#define WM_APP_SERVICEUPDATE	(WM_APP+0)
#define WM_APP_CHANNELCHANGE	(WM_APP+1)
#define WM_APP_IMAGESAVE		(WM_APP+2)
#define WM_APP_TRAYICON			(WM_APP+3)
#define WM_APP_EXECUTE			(WM_APP+4)
#define WM_APP_QUERYPORT		(WM_APP+5)
#define WM_APP_FILEWRITEERROR	(WM_APP+6)
#define WM_APP_VIDEOSIZECHANGED	(WM_APP+7)
#define WM_APP_EMMPROCESSED		(WM_APP+8)
#define WM_APP_ECMERROR			(WM_APP+9)
#define WM_APP_EPGLOADED		(WM_APP+10)
#define WM_APP_CONTROLLERFOCUS	(WM_APP+11)

enum {
	CONTAINER_ID_VIEW=1,
	CONTAINER_ID_PANELSPLITTER,
	CONTAINER_ID_PANEL,
	CONTAINER_ID_STATUSSPLITTER,
	CONTAINER_ID_STATUS,
	CONTAINER_ID_TITLEBARSPLITTER,
	CONTAINER_ID_TITLEBAR,
	CONTAINER_ID_SIDEBARSPLITTER,
	CONTAINER_ID_SIDEBAR
};


class CBasicViewer
{
protected:
	CDtvEngine *m_pDtvEngine;
	bool m_fEnabled;
	CViewWindow m_ViewWindow;
	CVideoContainerWindow m_VideoContainer;
	CDisplayBase m_DisplayBase;

public:
	CBasicViewer(CDtvEngine *pDtvEngine);
	bool Create(HWND hwndParent,int ViewID,int ContainerID,HWND hwndMessage);
	bool EnableViewer(bool fEnable);
	bool IsViewerEnabled() const { return m_fEnabled; }
	bool BuildViewer();
	bool CloseViewer();
	CViewWindow &GetViewWindow() { return m_ViewWindow; }
	CVideoContainerWindow &GetVideoContainer() { return m_VideoContainer; }
	CDisplayBase &GetDisplayBase() { return m_DisplayBase; }
};

class CFullscreen : public CBasicWindow
{
	Layout::CLayoutBase m_LayoutBase;
	CViewWindow m_ViewWindow;
	CBasicViewer *m_pViewer;
	CTitleBar m_TitleBar;
	CPanel m_Panel;
	class CPanelEventHandler : public CPanel::CEventHandler {
		bool OnClose();
	};
	CPanelEventHandler m_PanelEventHandler;
	bool m_fShowCursor;
	bool m_fMenu;
	bool m_fShowStatusView;
	bool m_fShowTitleBar;
	bool m_fShowSideBar;
	bool m_fShowPanel;
	int m_PanelWidth;
	POINT m_LastCursorMovePos;
	enum {
		TIMER_ID_HIDECURSOR=1
	};
	enum {
		HIDE_CURSOR_DELAY=1000UL
	};

	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool OnCreate();
	void OnMouseCommand(int Command);
	void OnLButtonDoubleClick();
	void ShowCursor(bool fShow);
	void ShowStatusView(bool fShow);
	void ShowTitleBar(bool fShow);
	void ShowSideBar(bool fShow);
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CFullscreen();
	~CFullscreen();
	bool Create(HWND hwndOwner,CBasicViewer *pViewer);
	void ShowPanel(bool fShow);
	bool IsPanelVisible() const { return m_fShowPanel; }
	void OnRButtonDown();
	void OnMButtonDown();
	void OnMouseMove();
	static bool Initialize();
};

class CMainWindow : public CBasicWindow, public CUISkin, public COSDManager::CEventHandler
{
	enum { UPDATE_TIMER_INTERVAL=500 };

	Layout::CLayoutBase m_LayoutBase;
	CBasicViewer m_Viewer;
	CTitleBar m_TitleBar;
	CFullscreen m_Fullscreen;
	bool m_fShowStatusBar;
	bool m_fShowTitleBar;
	bool m_fCustomTitleBar;
	bool m_fShowSideBar;
	int m_PanelPaneIndex;
	static int m_ThinFrameWidth;
	static bool m_fThinFrameCreate;
	bool m_fThinFrame;
	bool m_fStandbyInit;
	bool m_fMinimizeInit;
	bool m_fSrcFilterReleased;
	CChannelSpec m_RestoreChannelSpec;
	bool m_fRestorePreview;
	bool m_fRestoreFullscreen;
	bool m_fProgramGuideUpdating;
	int m_ProgramGuideUpdateStartChannel;
	bool m_fExitOnRecordingStop;
	POINT m_ptDragStartPos;
	RECT m_rcDragStart;
	bool m_fClosing;
	int m_WheelCount;
	int m_PrevWheelMode;
	DWORD m_PrevWheelTime;
	enum {
		ASPECTRATIO_DEFAULT,
		ASPECTRATIO_16x9,
		ASPECTRATIO_LETTERBOX,
		ASPECTRATIO_SUPERFRAME,
		ASPECTRATIO_SIDECUT,
		ASPECTRATIO_4x3
	};
	int m_AspectRatioType;
	DWORD m_AspectRatioResetTime;
	bool m_fFrameCut;
	int m_VideoSizeChangedTimerCount;
	unsigned int m_ProgramListUpdateTimerCount;
	int m_CurEventStereoMode;
	bool m_fAlertedLowFreeSpace;

	class CTimer {
		HWND m_hwnd;
		UINT m_ID;
	public:
		CTimer(UINT ID) : m_hwnd(NULL), m_ID(ID) {}
		bool Begin(HWND hwnd,DWORD Interval) {
			if (::SetTimer(hwnd,m_ID,Interval,NULL)==0) {
				m_hwnd=NULL;
				return false;
			}
			m_hwnd=hwnd;
			return true;
		}
		void End() {
			if (m_hwnd!=NULL) {
				::KillTimer(m_hwnd,m_ID);
				m_hwnd=NULL;
			}
		}
		bool IsEnabled() const { return m_hwnd!=NULL; }
	};
	CTimer m_ResetErrorCountTimer;

	class CDisplayBaseEventHandler : public CDisplayBase::CEventHandler {
		CMainWindow *m_pMainWindow;
		bool OnVisibleChange(bool fVisible);
	public:
		CDisplayBaseEventHandler(CMainWindow *pMainWindow);
	};
	CDisplayBaseEventHandler m_DisplayBaseEventHandler;
	friend CDisplayBaseEventHandler;

// CUISkin
	virtual HWND GetMainWindow() const { return m_hwnd; }
	virtual bool InitializeViewer();
	virtual bool FinalizeViewer();
	virtual bool EnableViewer(bool fEnable);
	virtual bool IsViewerEnabled() const;
	virtual bool SetZoomRate(int Rate,int Factor);
	virtual bool GetZoomRate(int *pRate,int *pFactor);
	virtual void OnVolumeChanged(bool fOSD);
	virtual void OnMuteChanged();
	virtual void OnStereoModeChanged();
	virtual void OnAudioStreamChanged();
	virtual bool OnStandbyChange(bool fStandby);
	virtual bool OnFullscreenChange(bool fFullscreen);
	virtual bool SetAlwaysOnTop(bool fTop);
	virtual void OnTunerChanged();
	virtual void OnTunerOpened();
	virtual void OnTunerClosed();
	virtual void OnChannelListChanged();
	virtual void OnChannelChanged(bool fSpaceChanged);
	virtual void OnServiceChanged();
	virtual void OnRecordingStarted();
	virtual void OnRecordingStopped();

// COSDManager::CEventHandler
	virtual bool GetOSDWindow(HWND *phwndParent,RECT *pRect,bool *pfForcePseudoOSD);
	virtual bool SetOSDHideTimer(DWORD Delay);

// CMainWindow
	LRESULT OnMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool OnCreate(const CREATESTRUCT *pcs);
	void OnSizeChanged(UINT State,int Width,int Height);
	bool OnSizeChanging(UINT Edge,RECT *pRect);
	void OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify);
	void OnTimer(HWND hwnd,UINT id);
	bool OnInitMenuPopup(HMENU hmenu);
	void AutoSelectStereoMode();
	bool OnExecute(LPCTSTR pszCmdLine);
	int GetZoomPercentage();
	void ShowChannelOSD();
	void SetWindowVisible();
	void ShowFloatingWindows(bool fShow);
	bool OpenTuner();
	void SetTitleText(bool fEvent);
	void RefreshChannelPanel();
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static DWORD WINAPI ExitWatchThread(LPVOID lpParameter);

public:
	enum { COMMAND_FROM_MOUSE=8 };

	CMainWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool Show(int CmdShow);
	void ShowNotificationBar(LPCTSTR pszText,
							 CNotificationBar::MessageType Type=CNotificationBar::MESSAGE_INFO,
							 DWORD Duration=0);
	void AdjustWindowSize(int Width,int Height);
	bool ReadSettings(CSettings *pSettings);
	bool WriteSettings(CSettings *pSettings);
	void SetStatusBarVisible(bool fVisible);
	bool GetStatusBarVisible() const { return m_fShowStatusBar; }
	void SetTitleBarVisible(bool fVisible);
	bool GetTitleBarVisible() const { return m_fShowTitleBar; }
	void SetCustomTitleBar(bool fCustom);
	bool GetCustomTitleBar() const { return m_fCustomTitleBar; }
	void SetThinFrame(bool fThinFrame);
	bool GetThinFrame() const { return m_fThinFrame; }
	void SetSideBarVisible(bool fVisible);
	bool GetSideBarVisible() const { return m_fShowSideBar; }
	int GetPanelPaneIndex() const;
	bool InitStandby();
	bool InitMinimize();
	bool IsMinimizeToTray() const;
	bool ConfirmExit();
	void OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz);
	void SendCommand(int Command) { SendMessage(WM_COMMAND,Command,0); }
	void PostCommand(int Command) { PostMessage(WM_COMMAND,Command,0); }
	bool CommandLineRecord(LPCTSTR pszFileName,DWORD Delay,DWORD Duration);
	bool BeginProgramGuideUpdate(bool fStandby=false);
	void OnProgramGuideUpdateEnd(bool fRelease=true);
	void EndProgramGuideUpdate(bool fRelease=true);
	void UpdatePanel();
	void ApplyColorScheme(const class CColorScheme *pColorScheme);
	bool SetLogo(LPCTSTR pszFileName);
	bool SetViewWindowEdge(bool fEdge);
	bool GetExitOnRecordingStop() const { return m_fExitOnRecordingStop; }
	void SetExitOnRecordingStop(bool fExit) { m_fExitOnRecordingStop=fExit; }
	CStatusView *GetStatusView() const;
	Layout::CLayoutBase &GetLayoutBase() { return m_LayoutBase; }
	CTitleBar &GetTitleBar() { return m_TitleBar; }
	bool UpdateProgramInfo();
	static bool Initialize();

// CUISkin
	virtual HWND GetVideoHostWindow() const;
};


#endif
