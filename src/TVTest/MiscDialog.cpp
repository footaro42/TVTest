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


CAboutDialog *CAboutDialog::GetThis(HWND hDlg)
{
	return static_cast<CAboutDialog*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CAboutDialog::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAboutDialog *pThis=reinterpret_cast<CAboutDialog*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			AdjustDialogPos(GetParent(hDlg),hDlg);

			{
				HWND hwndLogo=::GetDlgItem(hDlg,IDC_ABOUT_LOGO);
				RECT rc;

				::GetWindowRect(hwndLogo,&rc);
				::SetRect(&rc,rc.right-rc.left,0,0,0);
				if (pThis->m_AeroGlass.ApplyAeroGlass(hDlg,&rc)) {
					pThis->m_GdiPlus.Initialize();
					pThis->m_LogoImage.LoadFromResource(GetAppClass().GetResourceInstance(),
						MAKEINTRESOURCE(IDB_LOGO32),TEXT("PNG"));
					::ShowWindow(hwndLogo,SW_HIDE);
				} else {
					HBITMAP hbm=::LoadBitmap(GetAppClass().GetResourceInstance(),
											 MAKEINTRESOURCE(IDB_LOGO));
					::SendMessage(hwndLogo,STM_SETIMAGE,
								  IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
				}
			}
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
		{
			CAboutDialog *pThis=GetThis(hDlg);

			if (pThis->m_GdiPlus.IsInitialized()) {
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
					pThis->m_GdiPlus.FillRect(&Canvas,&Brush,&rcClient);
					pThis->m_GdiPlus.DrawImage(&Canvas,&pThis->m_LogoImage,
						(rc.right-pThis->m_LogoImage.Width())/2,
						(rc.bottom-pThis->m_LogoImage.Height())/2);
				}
				::EndPaint(hDlg,&ps);
				return TRUE;
			}
		}
		break;

	case WM_DESTROY:
		{
			CAboutDialog *pThis=GetThis(hDlg);
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_ABOUT_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL) {
				::DeleteObject(hbm);
			} else {
				pThis->m_LogoImage.Free();
				pThis->m_GdiPlus.Finalize();
			}
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}


bool CAboutDialog::Show(HWND hwndOwner)
{
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),
							MAKEINTRESOURCE(IDD_ABOUT),hwndOwner,DlgProc,
							reinterpret_cast<LPARAM>(this))==IDOK;
}
