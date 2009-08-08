#ifndef PANEL_OPTIONS_H
#define PANEL_OPTIONS_H


#include "Options.h"
#include "Panel.h"


enum {
	PANEL_TAB_INFORMATION,
	PANEL_TAB_PROGRAMLIST,
	PANEL_TAB_CHANNEL,
	PANEL_TAB_CONTROL
};

#define PANEL_TAB_FIRST	PANEL_TAB_INFORMATION
#define PANEL_TAB_LAST	PANEL_TAB_CONTROL

class CPanelOptions : public COptions {
	CPanelFrame *m_pPanelFrame;
	bool m_fSnapAtMainWindow;
	int m_SnapMargin;
	bool m_fAttachToMainWindow;
	int m_Opacity;
	LOGFONT m_Font;
	LOGFONT m_CurSettingFont;
	int m_FirstTab;
	int m_LastTab;
	static CPanelOptions *GetThis(HWND hDlg);

public:
	CPanelOptions(CPanelFrame *pPanelFrame);
	~CPanelOptions();
// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CPanelOptions
	bool GetSnapAtMainWindow() const { return m_fSnapAtMainWindow; }
	void SetSnapAtMainWindow(bool fSnap);
	int GetSnapMargin() const { return m_SnapMargin; }
	bool SetSnapMargin(int Margin);
	bool GetAttachToMainWindow() const { return m_fAttachToMainWindow; }
	void SetAttachToMainWindow(bool fAttach);
	const LOGFONT *GetFont() const { return &m_Font; }
	int GetFirstTab() const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
