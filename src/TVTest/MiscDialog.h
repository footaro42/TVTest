#ifndef MISC_DIALOG_H
#define MISC_DIALOG_H


#include "Dialog.h"
#include "Aero.h"
#include "DrawUtil.h"


class CAboutDialog : public CBasicDialog {
	CAeroGlass m_AeroGlass;
	CGdiPlus m_GdiPlus;
	CGdiPlus::CImage m_LogoImage;
	INT_PTR DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CAboutDialog();
	~CAboutDialog();
	bool Show(HWND hwndOwner);
};


#endif
