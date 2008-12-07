#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "View.h"
#include "ChannelManager.h"


class CFullscreen : public CBasicWindow {
	CVideoContainerWindow *m_pVideoContainer;
	CViewWindow *m_pViewWindow;
	bool m_fShowCursor;
	bool m_fMenu;
	bool m_fShowStatusView;
	bool m_fShowTitleBar;
	void ShowStatusView(bool fShow);
	void ShowTitleBar(bool fShow);
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	static CFullscreen *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CFullscreen();
	~CFullscreen();
	bool Create(HWND hwndOwner,CVideoContainerWindow *pVideoContainer,CViewWindow *m_pViewWindow);
	void OnRButtonDown();
	void OnMouseMove();
	HWND GetHandle() const { return m_hwnd; }
	static bool Initialize();
};

class CMainWindow : public CBasicWindow {
	CVideoContainerWindow m_VideoContainer;
	CViewWindow m_ViewWindow;
	bool m_fFullscreen;
	CFullscreen *m_pFullscreen;
	bool m_fMaximize;
	bool m_fAlwaysOnTop;
	bool m_fShowStatusBar;
	bool m_fShowTitleBar;
	bool m_fEnablePreview;
	bool m_fStandby;
	bool m_fStandbyInit;
	bool m_fMinimizeInit;
	bool m_fSrcFilterReleased;
	CChannelSpec m_RestoreChannelSpec;
	bool m_fRestoreFullscreen;
	bool m_fProgramGuideUpdating;
	int m_ProgramGuideUpdateStartChannel;
	bool m_fExitOnRecordingStop;
	POINT m_ptDragStartPos;
	RECT m_rcDragStart;
	bool m_fClosing;
	bool OnCreate();
	void OnCommand(HWND hwnd,int id,HWND hwndCtl,UINT codeNotify);
	void OnTimer(HWND hwnd,UINT id);
	BOOL OnAppCommand(HWND hwnd,HWND hwndFrom,int nCommand,UINT uDevice,DWORD dwKeys);
	bool OnExecute(LPCTSTR pszCmdLine);
	void CheckZoomMenu();
	void ShowChannelOSD();
	bool SetPreview(bool fPreview);
	static CMainWindow *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	enum {
		TIMER_ID_UPDATE=1,
		TIMER_ID_OSD,
		TIMER_ID_DISPLAY,
		TIMER_ID_WHEELCHANNELCHANGE,
		TIMER_ID_WHEELCHANNELCHANGE_DONE,
		TIMER_ID_PROGRAMLISTUPDATE,
		TIMER_ID_PROGRAMGUIDEUPDATE
	};
	CMainWindow();
	bool Create(HWND hwndParent,DWORD Style,DWORD ExStyle=0,int ID=0);
	bool Show(int CmdShow);
	bool BuildMediaViewer();
	bool SetFullscreen(bool fFullscreen);
	bool GetFullscreen() const { return m_fFullscreen; }
	HWND GetVideoHostWindow() const;
	int ShowMessage(LPCTSTR pszText,LPCTSTR pszCaption=NULL,UINT Type=MB_OK | MB_ICONEXCLAMATION) const;
	void AdjustWindowSize(int Width,int Height);
	void SetAlwaysOnTop(bool fTop);
	bool GetAlwaysOnTop() const { return m_fAlwaysOnTop; }
	void SetStatusBarVisible(bool fVisible);
	bool GetStatusBarVisible() const { return m_fShowStatusBar; }
	void SetTitleBarVisible(bool fVisible);
	bool GetTitleBarVisible() const { return m_fShowTitleBar; }
	void SetTitleText();
	bool EnablePreview(bool fEnable);
	bool IsPreview() const { return m_fEnablePreview; }
	void SetWindowVisible();
	bool SetStandby(bool fStandby);
	bool GetStandby() const { return m_fStandby; }
	bool InitStandby();
	bool InitMinimize();
	bool IsMinimizeToTray() const;
	bool OpenTuner();
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
	void OnChannelChange();
	void OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fStatus);
	void PopupMenu(const POINT *pPos=NULL);
	void SendCommand(int Command) { OnCommand(m_hwnd,Command,NULL,0); }
	void PostCommand(int Command) { PostMessage(WM_COMMAND,Command,0); }
	bool CommandLineRecord(LPCTSTR pszFileName,DWORD Delay,DWORD Duration);
	bool BeginProgramGuideUpdate();
	void EndProgramGuideUpdate(bool fRelease=true);
	void BeginProgramListUpdateTimer();
	bool SetViewWindowEdge(bool fEdge);
	bool GetExitOnRecordingStop() const { return m_fExitOnRecordingStop; }
	void SetExitOnRecordingStop(bool fExit) { m_fExitOnRecordingStop=fExit; }
	static bool Initialize();
};


#endif
