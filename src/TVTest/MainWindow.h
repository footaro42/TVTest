#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H


#include "BasicWindow.h"


class CMainWindow : public CBasicWindow {
	bool m_fFullscreen;
	bool m_fMaximize;
	bool m_fAlwaysOnTop;
	bool m_fShowStatusBar;
	bool m_fShowTitleBar;
	bool m_fEnablePreview;
	bool m_fStandby;
	bool m_fSrcFilterReleased;
	bool m_fRestoreFullscreen;
	bool m_fProgramGuideUpdating;
	int m_ProgramGuideUpdateStartChannel;
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
	bool SetFullscreen(bool fFullscreen);
	bool GetFullscreen() const { return m_fFullscreen; }
	HWND GetVideoHostWindow() const;
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
	bool SetStandby(bool fStandby);
	bool ConfirmExit();
	bool SetVolume(int Volume,bool fOSD=true);
	int CalcZoomRate();
	void SetZoomRate(int ZoomNum,int ZoomDenom=100);
	void SetMaximizeStatus(bool fMaximize) { m_fMaximize=fMaximize; }
	bool GetMaximizeStatus() const { return m_fMaximize; }
	void OnChannelChange();
	void OnMouseWheel(WPARAM wParam,LPARAM lParam,bool fStatus);
	void PopupMenu(const POINT *pPos=NULL);
	void SendCommand(int Command) { OnCommand(m_hwnd,Command,NULL,0); }
	void PostCommand(int Command) { PostMessage(WM_COMMAND,Command,0); }
	bool BeginProgramGuideUpdate();
	void EndProgramGuideUpdate();
	void BeginProgramListUpdateTimer();
	static bool Initialize();
};


#endif
