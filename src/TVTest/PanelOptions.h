#ifndef PANEL_OPTIONS_H
#define PANEL_OPTIONS_H


#include "Options.h"
#include "Panel.h"
#include "PanelForm.h"


enum {
	PANEL_ID_INFORMATION,
	PANEL_ID_PROGRAMLIST,
	PANEL_ID_CHANNEL,
	PANEL_ID_CONTROL,
	PANEL_ID_CAPTION,
	NUM_PANELS
};

#define PANEL_ID_FIRST	PANEL_ID_INFORMATION
#define PANEL_ID_LAST	PANEL_ID_CAPTION

class CPanelOptions : public COptions
{
	CPanelFrame *m_pPanelFrame;
	bool m_fSnapAtMainWindow;
	int m_SnapMargin;
	bool m_fAttachToMainWindow;
	int m_Opacity;
	LOGFONT m_Font;
	LOGFONT m_CurSettingFont;
	bool m_fSpecCaptionFont;
	LOGFONT m_CaptionFont;
	LOGFONT m_CurSettingCaptionFont;
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
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
