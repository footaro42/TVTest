#ifndef MISC_DIALOG_H
#define MISC_DIALOG_H


#include "Aero.h"
#include "DrawUtil.h"


class CAboutDialog {
	CAeroGlass m_AeroGlass;
	CGdiPlus m_GdiPlus;
	CGdiPlus::CImage m_LogoImage;
	static CAboutDialog *GetThis(HWND hDlg);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CAboutDialog();
	~CAboutDialog();
	bool Show(HWND hwndOwner);
};


#endif
