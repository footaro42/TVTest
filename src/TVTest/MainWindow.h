#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "View.h"
#include "ChannelManager.h"
#include "Splitter.h"
#include "TitleBar.h"
#include "StatusView.h"
#include "Settings.h"
#include "NotificationBar.h"
#include "Panel.h"


#define MAIN_WINDOW_CLASS		APP_NAME TEXT(" Window")
#define FULLSCREEN_WINDOW_CLASS	APP_NAME TEXT(" Fullscreen")

#define MAIN_TITLE_TEXT APP_NAME

#define WM_APP_SERVICEUPDATE	WM_APP
#define WM_APP_CHANNELCHANGE	(WM_APP+1)
#define WM_APP_IMAGESAVE		(WM_APP+2)
#define WM_APP_TRAYICON			(WM_APP+3)
#define WM_APP_EXECUTE			(WM_APP+4)
#define WM_APP_QUERYPORT		(WM_APP+5)
#define WM_APP_FILEWRITEERROR	(WM_APP+6)
#define WM_APP_VIDEOSIZECHANGED	(WM_APP+7)
#define WM_APP_EMMPROCESSED		(WM_APP+8)
#define WM_APP_ECMERROR			(WM_APP+9)
#define WM_APP_EPGFILELOADED	(WM_APP+10)
#define WM_APP_TVTESTACTIVE		(WM_APP+11)

enum {
	PANE_ID_VIEW=1,
	PANE_ID_PANEL
};


class CFullscreen : public CBasicWindow {
	CSplitter m_Splitter;
	CViewWindow m_ViewWindow;
	CVideoContainerWindow *m_pVideoContainer;
	CViewWindow *m_pViewWindow;
	CTitleBar m_TitleBar;
	CPanel m_Panel;
	class CPanelEventHandler : public CPanel::CEventHandler {
	public:
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

	bool OnCreate();
	void OnMouseCommand(int Command);
	void OnLButtonDoubleClick();
	void ShowStatusView(bool fShow);
	void ShowTitleBar(bool fShow);
	void ShowSideBar(bool fShow);
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	static CFullscreen *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CFullscreen();
	~CFullscreen();
	bool Create(HWND hwndOwner,CVideoContainerWindow *pVideoContainer,CViewWindow *pViewWindow);
	void ShowPanel(bool fShow);
	bool IsPanelVisible() const { return m_fShowPanel; }
	void OnRButtonDown();
	void OnMButtonDown();
	void OnMouseMove();
	static bool Initialize();
};

class CMainWindow : public CBasicWindow {
	enum { UPDATE_TIMER_INTERVAL=500 };

	static const BYTE VolumeNormalizeLevelList[];
	struct ZoomRateInfo {
		WORD Num,Denom;
	};
	static const ZoomRateInfo ZoomRateList[];

	CSplitter m_Splitter;
	CVideoContainerWindow m_VideoContainer;
	CViewWindow m_ViewWindow;
	CTitleBar m_TitleBar;
	bool m_fFullscreen;
	CFullscreen m_Fullscreen;
	bool m_fMaximize;
	bool m_fAlwaysOnTop;
	bool m_fShowStatusBar;
	bool m_fShowTitleBar;
	bool m_fCustomTitleBar;
	bool m_fShowSideBar;
	static int m_ThinFrameWidth;
	static bool m_fThinFrameCreate;
	bool m_fThinFrame;
	bool m_fStandby;
	bool m_fStandbyInit;
	bool m_fMinimizeInit;
	bool m_fSrcFilterReleased;
	CChannelSpec m_RestoreChannelSpec;
	bool m_fRestorePreview;
	bool m_fRestoreFullscreen;
	bool m_fProgramGuideUpdating;
	int m_ProgramGuideUpdateStartChannel;
	bool m_fRecordingStopOnEventEnd;
	bool m_fExitOnRecordingStop;
	POINT m_ptDragStartPos;
	RECT m_rcDragStart;
	bool m_fClosing;
	int m_WheelCount;
	int m_PrevWheelMode;
	DWORD m_PrevWheelTime;
	bool m_fWheelChannelChanging;
	BOOL m_fScreenSaverActive;
	BOOL m_fLowPowerActiveOriginal;
	BOOL m_fPowerOffActiveOriginal;
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
	int m_VideoSizeChangedTimerCount;
	bool m_fShowRecordRemainTime;
	unsigned int m_ProgramListUpdateTimerCount;
	bool m_fViewerBuildError;

	class CPreviewManager {
		bool m_fPreview;
		CVideoContainerWindow *m_pVideoContainer;
	public:
		CPreviewManager(CVideoContainerWindow *pVideoContainer);
		bool EnablePreview(bool fEnable);
		bool IsPreviewEnabled() const { return m_fPreview; }
		bool BuildMediaViewer();
		bool CloseMediaViewer();
	};
	CPreviewManager m_PreviewManager;

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
	CTimer m_ChannelPanelTimer;

	bool OnCreate(const CREATESTRUCT *pcs);
	void OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify);
	void OnTimer(HWND hwnd,UINT id);
	bool OnExecute(LPCTSTR pszCmdLine);
	void CheckZoomMenu();
	void ShowChannelOSD();
	void SetWindowVisible();
	void ShowFloatingWindows(bool fShow);
	bool OpenTuner();
	void SetTuningSpaceMenu();
	void SetChannelMenu();
	void SetNetworkRemoconChannelMenu(HMENU hmenu);
	bool ProcessTunerSelectMenu(int Command);
	void RefreshChannelPanel();
	static CMainWindow *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static DWORD WINAPI ExitWatchThread(LPVOID lpParameter);

public:
	enum {
		TIMER_ID_UPDATE=1,
		TIMER_ID_OSD,
		TIMER_ID_DISPLAY,
		TIMER_ID_WHEELCHANNELCHANGE,
		TIMER_ID_WHEELCHANNELCHANGE_DONE,
		TIMER_ID_PROGRAMLISTUPDATE,
		TIMER_ID_PROGRAMGUIDEUPDATE,
		TIMER_ID_CHANNELPANELUPDATE,
		TIMER_ID_VIDEOSIZECHANGED
	};
	enum { COMMAND_FROM_MOUSE=8 };

	CMainWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool Show(int CmdShow);
	bool BuildMediaViewer();
	bool IsMediaViewerBuildError() const { return m_fViewerBuildError; }
	bool CloseMediaViewer();
	bool SetFullscreen(bool fFullscreen);
	bool GetFullscreen() const { return m_fFullscreen; }
	HWND GetVideoHostWindow() const;
	int ShowMessage(LPCTSTR pszText,LPCTSTR pszCaption=NULL,UINT Type=MB_OK | MB_ICONEXCLAMATION) const;
	void ShowErrorMessage(LPCTSTR pszText);
	void ShowErrorMessage(const CBonErrorHandler *pErrorHandler,LPCTSTR pszTitle=NULL);
	void ShowNotificationBar(LPCTSTR pszText,CNotificationBar::MessageType Type=CNotificationBar::MESSAGE_INFO);
	void AdjustWindowSize(int Width,int Height);
	bool ReadSettings(CSettings *pSettings);
	bool WriteSettings(CSettings *pSettings);
	void SetAlwaysOnTop(bool fTop);
	bool GetAlwaysOnTop() const { return m_fAlwaysOnTop; }
	void SetStatusBarVisible(bool fVisible);
	bool GetStatusBarVisible() const { return m_fShowStatusBar; }
	void SetTitleBarVisible(bool fVisible);
	bool GetTitleBarVisible() const { return m_fShowTitleBar; }
	void SetCustomTitleBar(bool fCustom);
	bool GetCustomTitleBar() const { return m_fCustomTitleBar; }
	void SetThinFrame(bool fThinFrame);
	bool GetThinFrame() const { return m_fThinFrame; }
	void SetTitleText();
	void SetSideBarVisible(bool fVisible);
	bool GetSideBarVisible() const { return m_fShowSideBar; }
	bool EnablePreview(bool fEnable);
	bool IsPreview() const;
	bool SetResident(bool fResident);
	bool GetResident() const;
	bool SetStandby(bool fStandby);
	bool GetStandby() const { return m_fStandby; }
	bool InitStandby();
	bool InitMinimize();
	bool IsMinimizeToTray() const;
	bool ConfirmExit();
	int GetVolume() const;
	bool SetVolume(int Volume,bool fOSD=true);
	bool GetMute() const;
	bool SetMute(bool fMute);
	int GetStereoMode() const;
	bool SetStereoMode(int StereoMode);
	bool SwitchAudio();
	int CalcZoomRate();
	bool CalcZoomRate(int *pNum,int *pDenom);
	bool SetZoomRate(int ZoomNum,int ZoomDenom=100);
	void SetMaximizeStatus(bool fMaximize) { m_fMaximize=fMaximize; }
	bool GetMaximizeStatus() const { return m_fMaximize; }
	void OnChannelListUpdated();
	void OnChannelChanged();
	void OnDriverChanged();
	void OnTunerOpened();
	void OnTunerClosed();
	void OnServiceChanged();
	void OnRecordingStart();
	void OnRecordingStop();
	void OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fHorz,bool fStatus);
	void PopupMenu(const POINT *pPos=NULL);
	HMENU CreateTunerSelectMenu();
	void SendCommand(int Command) { OnCommand(m_hwnd,Command,NULL,0); }
	void PostCommand(int Command) { PostMessage(WM_COMMAND,Command,0); }
	bool DoCommand(LPCWSTR pszCommand);
	bool CommandLineRecord(LPCTSTR pszFileName,DWORD Delay,DWORD Duration);
	bool BeginProgramGuideUpdate();
	void OnProgramGuideUpdateEnd(bool fRelease=true);
	void EndProgramGuideUpdate(bool fRelease=true);
	void BeginProgramListUpdateTimer();
	void UpdatePanel();
	void BeginChannelPanelUpdateTimer();
	bool SetLogo(LPCTSTR pszFileName);
	bool SetViewWindowEdge(bool fEdge);
	bool GetRecordingStopOnEventEnd() const { return m_fRecordingStopOnEventEnd; }
	bool SetRecordingStopOnEventEnd(bool fEnable);
	bool GetExitOnRecordingStop() const { return m_fExitOnRecordingStop; }
	void SetExitOnRecordingStop(bool fExit) { m_fExitOnRecordingStop=fExit; }
	void SetDisplayStatus();
	void ResetDisplayStatus();
	bool IsWheelChannelChanging() const { return m_fWheelChannelChanging; }
	CStatusView *GetStatusView() const;
	CSplitter &GetSplitter() { return m_Splitter; }
	CTitleBar &GetTitleBar() { return m_TitleBar; }
	bool UpdateProgramInfo();
	static bool Initialize();
};


#endif
