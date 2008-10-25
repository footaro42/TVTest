#ifndef MISC_DIALOG_H
#define MISC_DIALOG_H


class CAboutDialog {
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	bool Show(HWND hwndOwner);
};


#endif
