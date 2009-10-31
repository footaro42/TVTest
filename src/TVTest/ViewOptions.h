#ifndef VIEW_OPTIONS_H
#define VIEW_OPTIONS_H


#include "Options.h"
#include "MediaViewer.h"


class CViewOptions : public COptions
{
	bool m_fAdjustAspectResizing;
	bool m_fSnapAtWindowEdge;
	int m_SnapAtWindowEdgeMargin;
	bool m_fPanScanNoResizeWindow;
	CMediaViewer::ViewStretchMode m_FullscreenStretchMode;
	CMediaViewer::ViewStretchMode m_MaximizeStretchMode;
	bool m_fClientEdge;
	bool m_fMinimizeToTray;
	bool m_fDisablePreviewWhenMinimized;
	bool m_fRestorePlayStatus;
	bool m_fResetPanScanEventChange;
	bool m_fNoMaskSideCut;
	bool m_fIgnoreDisplayExtension;
	bool m_fNoScreenSaver;
	bool m_fNoMonitorLowPower;
	bool m_fNoMonitorLowPowerActiveOnly;
	bool m_fShowLogo;
	TCHAR m_szLogoFileName[MAX_PATH];
	static CViewOptions *GetThis(HWND hDlg);

public:
	CViewOptions();
// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CViewOptions
	bool GetAdjustAspectResizing() const { return m_fAdjustAspectResizing; }
	bool GetSnapAtWindowEdge() const { return m_fSnapAtWindowEdge; }
	int GetSnapAtWindowEdgeMargin() const { return m_SnapAtWindowEdgeMargin; }
	bool GetPanScanNoResizeWindow() const { return m_fPanScanNoResizeWindow; }
	CMediaViewer::ViewStretchMode GetFullscreenStretchMode() const { return m_FullscreenStretchMode; }
	CMediaViewer::ViewStretchMode GetMaximizeStretchMode() const { return m_MaximizeStretchMode; }
	bool GetClientEdge() const { return m_fClientEdge; }
	bool GetMinimizeToTray() const { return m_fMinimizeToTray; }
	bool GetDisablePreviewWhenMinimized() const { return m_fDisablePreviewWhenMinimized; }
	bool GetRestorePlayStatus() const { return m_fRestorePlayStatus; }
	bool GetResetPanScanEventChange() const { return m_fResetPanScanEventChange; }
	bool GetNoMaskSideCut() const { return m_fNoMaskSideCut; }
	bool GetIgnoreDisplayExtension() const { return m_fIgnoreDisplayExtension; }
	bool GetNoScreenSaver() const { return m_fNoScreenSaver; }
	bool GetNoMonitorLowPower() const { return m_fNoMonitorLowPower; }
	bool GetNoMonitorLowPowerActiveOnly() const { return m_fNoMonitorLowPowerActiveOnly; }
	bool GetShowLogo() const { return m_fShowLogo; }
	LPCTSTR GetLogoFileName() const { return m_szLogoFileName; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
