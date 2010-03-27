#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "MiscDialog.h"
#include "DialogUtil.h"
#include "resource.h"




CAboutDialog::CAboutDialog()
{
}



CAboutDialog::~CAboutDialog()
{
	m_LogoImage.Free();
	m_GdiPlus.Finalize();
}


INT_PTR CAboutDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			AdjustDialogPos(GetParent(hDlg),hDlg);

			{
				HWND hwndLogo=::GetDlgItem(hDlg,IDC_ABOUT_LOGO);
				RECT rc;

				::GetWindowRect(hwndLogo,&rc);
				::SetRect(&rc,rc.right-rc.left,0,0,0);
				if (m_AeroGlass.ApplyAeroGlass(hDlg,&rc)) {
					m_GdiPlus.Initialize();
					m_LogoImage.LoadFromResource(GetAppClass().GetResourceInstance(),
						MAKEINTRESOURCE(IDB_LOGO32),TEXT("PNG"));
					::ShowWindow(hwndLogo,SW_HIDE);
				} else {
					HBITMAP hbm=::LoadBitmap(GetAppClass().GetResourceInstance(),
											 MAKEINTRESOURCE(IDB_LOGO));
					::SendMessage(hwndLogo,STM_SETIMAGE,
								  IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
				}
			}

			::SetDlgItemText(hDlg,IDC_ABOUT_VERSION,ABOUT_VERSION_TEXT
#ifdef VERSION_PLATFORM
				TEXT(" (") VERSION_PLATFORM TEXT(")")
#endif
			);
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

	case WM_PAINT:
		if (m_GdiPlus.IsInitialized()) {
			PAINTSTRUCT ps;

			::BeginPaint(hDlg,&ps);
			{
				CGdiPlus::CCanvas Canvas(ps.hdc);
				CGdiPlus::CBrush Brush(::GetSysColor(COLOR_3DFACE));
				RECT rc,rcClient;

				::GetWindowRect(::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO),&rc);
				::OffsetRect(&rc,-rc.left,-rc.top);
				Canvas.Clear(0,0,0,0);
				::GetClientRect(hDlg,&rcClient);
				rcClient.left=rc.right;
				m_GdiPlus.FillRect(&Canvas,&Brush,&rcClient);
				m_GdiPlus.DrawImage(&Canvas,&m_LogoImage,
									(rc.right-m_LogoImage.GetWidth())/2,
									(rc.bottom-m_LogoImage.GetHeight())/2);
			}
			::EndPaint(hDlg,&ps);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_ABOUT_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL) {
				::DeleteObject(hbm);
			} else {
				m_LogoImage.Free();
				m_GdiPlus.Finalize();
			}
		}
		return TRUE;
	}
	return FALSE;
}


bool CAboutDialog::Show(HWND hwndOwner)
{
	return ShowDialog(hwndOwner,GetAppClass().GetResourceInstance(),
					  MAKEINTRESOURCE(IDD_ABOUT))==IDOK;
}
