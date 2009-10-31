#ifndef PANEL_OPTIONS_H
#define PANEL_OPTIONS_H


#include "Options.h"
#include "Panel.h"
#include "PanelForm.h"


enum {
	PANEL_ID_INFORMATION,
	PANEL_ID_PROGRAMLIST,
	PANEL_ID_CHANNEL,
	PANEL_ID_CONTROL
#ifndef TVH264
	,PANEL_ID_CAPTION
#endif
	,NUM_PANELS
};

#define PANEL_ID_FIRST	PANEL_ID_INFORMATION
#ifndef TVH264
#define PANEL_ID_LAST	PANEL_ID_CAPTION
#else
#define PANEL_ID_LAST	PANEL_ID_CONTROL
#endif

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
	struct TabInfo {
		int ID;
		bool fVisible;
	};
	TabInfo m_TabList[NUM_PANELS];
	bool m_fChannelDetailToolTip;
	static CPanelOptions *GetThis(HWND hDlg);
	static LRESULT CALLBACK TabListProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);

public:
	CPanelOptions(CPanelFrame *pPanelFrame);
	~CPanelOptions();
// COptions
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
// CPanelOptions
	bool InitializePanelForm(CPanelForm *pPanelForm);
	bool GetSnapAtMainWindow() const { return m_fSnapAtMainWindow; }
	void SetSnapAtMainWindow(bool fSnap);
	int GetSnapMargin() const { return m_SnapMargin; }
	bool SetSnapMargin(int Margin);
	bool GetAttachToMainWindow() const { return m_fAttachToMainWindow; }
	void SetAttachToMainWindow(bool fAttach);
	const LOGFONT *GetFont() const { return &m_Font; }
	int GetFirstTab() const;
	bool GetChannelDetailToolTip() const { return m_fChannelDetailToolTip; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
