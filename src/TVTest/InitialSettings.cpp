#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "InitialSettings.h"
#include "DirectShowFilter/DirectShowUtil.h"
#include "Help.h"
#include "resource.h"




CInitialSettings::CInitialSettings(const CDriverManager *pDriverManager)
{
	m_pDriverManager=pDriverManager;
	m_szDriverFileName[0]='\0';
	m_szMpeg2DecoderName[0]='\0';
	m_VideoRenderer=CVideoRenderer::RENDERER_DEFAULT;
	// VistaではビデオレンダラのデフォルトをVMR9にする
	{
		OSVERSIONINFO osvi;

		osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion>=6)
			m_VideoRenderer=CVideoRenderer::RENDERER_VMR9;
	}
	m_CardReader=CCardReader::READER_SCARD;
}


CInitialSettings::~CInitialSettings()
{
}


bool CInitialSettings::GetDriverFileName(LPTSTR pszFileName,int MaxLength) const
{
	if (::lstrlen(m_szDriverFileName)>=MaxLength)
		return false;
	::lstrcpy(pszFileName,m_szDriverFileName);
	return true;
}


bool CInitialSettings::GetMpeg2DecoderName(LPTSTR pszDecoderName,int MaxLength) const
{
	if (::lstrlen(m_szMpeg2DecoderName)>=MaxLength)
		return false;
	::lstrcpy(pszDecoderName,m_szMpeg2DecoderName);
	return true;
}


CInitialSettings *CInitialSettings::GetThis(HWND hDlg)
{
	return static_cast<CInitialSettings*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CInitialSettings::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CInitialSettings *pThis=reinterpret_cast<CInitialSettings*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			{
				RECT rc;

				::GetWindowRect(hDlg,&rc);
				::OffsetRect(&rc,-rc.left,-rc.top);
				::MoveWindow(hDlg,(::GetSystemMetrics(SM_CXSCREEN)-rc.right)/2,
								  (::GetSystemMetrics(SM_CYSCREEN)-rc.bottom)/2,
								  rc.right,rc.bottom,FALSE);
			}
			{
				HBITMAP hbm=::LoadBitmap(GetAppClass().GetResourceInstance(),
													MAKEINTRESOURCE(IDB_LOGO));
				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_LOGO,STM_SETIMAGE,
									 IMAGE_BITMAP,reinterpret_cast<LPARAM>(hbm));
			}
			{
				bool fUDPDriverExists=false,fDriverFinded=false;

				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_DRIVER,CB_LIMITTEXT,MAX_PATH-1,0);
				for (int i=0;i<pThis->m_pDriverManager->NumDrivers();i++) {
					const CDriverInfo *pDriverInfo=pThis->m_pDriverManager->GetDriverInfo(i);

					::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_DRIVER,
						CB_ADDSTRING,0,
						reinterpret_cast<LPARAM>(pDriverInfo->GetFileName()));
					if (::lstrcmpi(pDriverInfo->GetFileName(),
								   TEXT("BonDriver_UDP.dll"))==0) {
						fUDPDriverExists=true;
					} else if (!fDriverFinded) {
						::lstrcpy(pThis->m_szDriverFileName,pDriverInfo->GetFileName());
						fDriverFinded=true;
					}
				}
				if (fUDPDriverExists && !fDriverFinded)
					::lstrcpy(pThis->m_szDriverFileName,TEXT("BonDriver_UDP.dll"));
				::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,pThis->m_szDriverFileName);
			}
			{
				CDirectShowFilterFinder FilterFinder;
				WCHAR szFilterName[128];
				int Sel=-1;

				if (pThis->m_szMpeg2DecoderName[0]=='\0')
					Sel=0;
				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,
									CB_ADDSTRING,0,(LPARAM)TEXT("デフォルト"));
				if (FilterFinder.FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_MPEG2_VIDEO)) {
					for (int i=0;i<FilterFinder.GetFilterCount();i++) {
						if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
							int Index=::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,
								CB_ADDSTRING,0,(LPARAM)szFilterName);
							if (::lstrcmpi(szFilterName,pThis->m_szMpeg2DecoderName)==0)
								Sel=Index;
						}
					}
				}
				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,CB_SETCURSEL,Sel,0);
			}
			{
				LPCTSTR pszName;

				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,
					CB_ADDSTRING,0,reinterpret_cast<LPARAM>(TEXT("デフォルト")));
				for (int i=1;(pszName=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
					::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,
						CB_ADDSTRING,0,reinterpret_cast<LPARAM>(pszName));
				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,
								CB_SETCURSEL,(WPARAM)pThis->m_VideoRenderer,0);
			}
			{
				static const LPCTSTR pszCardReaderList[] = {
					TEXT("なし(スクランブル解除しない)"),
					TEXT("スマートカードリーダ"),
					TEXT("HDUS内蔵カードリーダ")
				};

				for (int i=0;i<lengthof(pszCardReaderList);i++)
					::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_CARDREADER,
								CB_ADDSTRING,0,(LPARAM)pszCardReaderList[i]);
				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_CARDREADER,
								CB_SETCURSEL,(WPARAM)pThis->m_CardReader,0);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_INITIALSETTINGS_DRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,szFileName,lengthof(szFileName));
				FilePath.SetPath(szFileName);
				if (FilePath.GetDirectory(szInitDir)) {
					::lstrcpy(szFileName,FilePath.GetFileName());
				} else {
					GetAppClass().GetAppDirectory(szInitDir);
				}
				InitOpenFileName(&ofn);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=
					TEXT("BonDriver(BonDriver*.dll)\0BonDriver*.dll\0")
					TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrInitialDir=szInitDir;
				ofn.lpstrTitle=TEXT("BonDriverの選択");
				ofn.Flags=OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_EXPLORER;
				if (::GetOpenFileName(&ofn)) {
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,szFileName);
				}
			}
			return TRUE;

		case IDC_INITIALSETTINGS_HELP:
			GetAppClass().ShowHelpContent(HELP_ID_INITIALSETTINGS);
			return TRUE;

		case IDOK:
			{
				CInitialSettings *pThis=GetThis(hDlg);

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,
								 pThis->m_szDriverFileName,MAX_PATH);
				int Sel=::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,
															CB_GETCURSEL,0,0);
				if (Sel>0)
					::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,
						CB_GETLBTEXT,Sel,reinterpret_cast<LPARAM>(pThis->m_szMpeg2DecoderName));
				else
					pThis->m_szMpeg2DecoderName[0]='\0';
				pThis->m_VideoRenderer=(CVideoRenderer::RendererType)
					::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,CB_GETCURSEL,0,0);
				pThis->m_CardReader=(CCardReader::ReaderType)
					::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_CARDREADER,CB_GETCURSEL,0,0);
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO))
			return reinterpret_cast<BOOL>(::GetStockObject(WHITE_BRUSH));
		break;

	case WM_DESTROY:
		{
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_LOGO,
				STM_SETIMAGE,IMAGE_BITMAP,reinterpret_cast<LPARAM>((HBITMAP)NULL)));

			if (hbm!=NULL)
				::DeleteObject(hbm);
			::RemoveProp(hDlg,TEXT("This"));
		}
		return TRUE;
	}
	return FALSE;
}


bool CInitialSettings::ShowDialog(HWND hwndOwner)
{
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_INITIALSETTINGS),
							hwndOwner,DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}
