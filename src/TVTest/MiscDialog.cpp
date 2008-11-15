#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MiscDialog.h"
#include "resource.h"




BOOL CALLBACK CAboutDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			HBITMAP hbm;
			RECT rc;

			hbm=::LoadBitmap(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDB_LOGO));
			::SendDlgItemMessage(hDlg,IDC_ABOUT_LOGO,STM_SETIMAGE,IMAGE_BITMAP,
												reinterpret_cast<LPARAM>(hbm));
			::GetWindowRect(hDlg,&rc);
			::MoveWindow(hDlg,
						(GetSystemMetrics(SM_CXSCREEN)-(rc.right-rc.left))/2,
						(GetSystemMetrics(SM_CYSCREEN)-(rc.bottom-rc.top))/2,
						rc.right-rc.left,rc.bottom-rc.top,FALSE);
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_ABOUT_LOGO))
			return reinterpret_cast<BOOL>(::GetStockObject(WHITE_BRUSH));
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_ABOUT_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL)
				::DeleteObject(hbm);
		}
		return TRUE;
	}
	return FALSE;
}


bool CAboutDialog::Show(HWND hwndOwner)
{
	return ::DialogBox(GetAppClass().GetResourceInstance(),
					   MAKEINTRESOURCE(IDD_ABOUT),hwndOwner,DlgProc)==IDOK;
}
