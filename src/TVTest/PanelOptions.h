#ifndef PANEL_OPTIONS_H
#define PANEL_OPTIONS_H


#include "Options.h"


class CPanelOptions : public COptions {
	bool m_fSnapAtMainWindow;
	int m_SnapMargin;
	bool m_fAttachToMainWindow;
	static CPanelOptions *GetThis(HWND hDlg);
public:
	CPanelOptions();
	~CPanelOptions();
	bool GetSnapAtMainWindow() const { return m_fSnapAtMainWindow; }
	void SetSnapAtMainWindow(bool fSnap);
	int GetSnapMargin() const { return m_SnapMargin; }
	bool SetSnapMargin(int Margin);
	bool GetAttachToMainWindow() const { return m_fAttachToMainWindow; }
	void SetAttachToMainWindow(bool fAttach);
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
