#include "stdafx.h"
#include <shlwapi.h>
#include <shlobj.h>
#include "TVTest.h"
#include "AppMain.h"
#include "InitialSettings.h"
#include "DirectShowFilter/DirectShowUtil.h"
#include "DialogUtil.h"
#include "Help.h"
#include "resource.h"




CInitialSettings::CInitialSettings(const CDriverManager *pDriverManager)
{
	m_pDriverManager=pDriverManager;
	m_szDriverFileName[0]='\0';
	m_szMpeg2DecoderName[0]='\0';
	m_VideoRenderer=CVideoRenderer::RENDERER_DEFAULT;
#if 0
	// VistaではビデオレンダラのデフォルトをEVRにする
	{
		OSVERSIONINFO osvi;

		osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion>=6)
			m_VideoRenderer=CVideoRenderer::RENDERER_EVR;
	}
#endif
	m_CardReader=CCardReader::READER_SCARD;
	if (!::SHGetSpecialFolderPath(NULL,m_szRecordFolder,CSIDL_MYVIDEO,FALSE)
			&& !::SHGetSpecialFolderPath(NULL,m_szRecordFolder,CSIDL_PERSONAL,FALSE))
		m_szRecordFolder[0]='\0';
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
				CCardReader *pCardReader;

				for (int i=0;i<lengthof(pszCardReaderList);i++)
					::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_CARDREADER,
								CB_ADDSTRING,0,(LPARAM)pszCardReaderList[i]);
				pThis->m_CardReader=CCardReader::READER_NONE;
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_SCARD);
				if (pCardReader!=NULL) {
					if (pCardReader->NumReaders()>0)
						pThis->m_CardReader=CCardReader::READER_SCARD;
					delete pCardReader;
				}
				if (pThis->m_CardReader==CCardReader::READER_NONE) {
					pCardReader=CCardReader::CreateCardReader(CCardReader::READER_HDUS);
					if (pCardReader!=NULL) {
						if (pCardReader->Open()) {
							pThis->m_CardReader=CCardReader::READER_HDUS;
							pCardReader->Close();
						}
						delete pCardReader;
					}
				}
				::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_CARDREADER,
								CB_SETCURSEL,(WPARAM)pThis->m_CardReader,0);
			}
			::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,pThis->m_szRecordFolder);
			::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
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

		case IDC_INITIALSETTINGS_SEARCHCARDREADER:
			{
				CCardReader::ReaderType ReaderType=CCardReader::READER_NONE;
				CCardReader *pCardReader;
				TCHAR szText[1024];

				::SetCursor(::LoadCursor(NULL,IDC_WAIT));
				::lstrcpy(szText,TEXT("以下のカードリーダが見付かりました。\n"));
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_SCARD);
				if (pCardReader!=NULL) {
					if (pCardReader->NumReaders()>0) {
						for (int i=0;i<pCardReader->NumReaders();i++) {
							LPCTSTR pszReaderName=pCardReader->EnumReader(i);
							if (pszReaderName!=NULL) {
								::wsprintf(szText+::lstrlen(szText),
										   TEXT("\"%s\"\n"),pszReaderName);
							}
						}
						ReaderType=CCardReader::READER_SCARD;
					}
					delete pCardReader;
				}
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_HDUS);
				if (pCardReader!=NULL) {
					if (pCardReader->Open()) {
						::lstrcat(szText,TEXT("\"HDUS内蔵カードリーダ\"\n"));
						pCardReader->Close();
						if (ReaderType==CCardReader::READER_NONE)
							ReaderType=CCardReader::READER_HDUS;
					}
					delete pCardReader;
				}
				::SetCursor(::LoadCursor(NULL,IDC_ARROW));
				if (ReaderType==CCardReader::READER_NONE) {
					::MessageBox(hDlg,TEXT("カードリーダは見付かりませんでした。"),TEXT("検索結果"),MB_OK | MB_ICONINFORMATION);
				} else {
					DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_CARDREADER,(WPARAM)ReaderType);
					::MessageBox(hDlg,szText,TEXT("検索結果"),MB_OK | MB_ICONINFORMATION);
				}
			}
			return TRUE;

		case IDC_INITIALSETTINGS_RECORDFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,szFolder,MAX_PATH);
				if (BrowseFolderDialog(hDlg,szFolder,
										TEXT("録画ファイルの保存先フォルダ:")))
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,szFolder);
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

				TCHAR szRecordFolder[MAX_PATH];
				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_RECORDFOLDER,
								 szRecordFolder,lengthof(szRecordFolder));
				if (szRecordFolder[0]!='\0'
						&& !::PathIsDirectory(szRecordFolder)) {
					TCHAR szMessage[MAX_PATH+64];

					::wsprintf(szMessage,
						TEXT("録画ファイルの保存先フォルダ \"%s\" がありません。\n")
						TEXT("作成しますか?"),szRecordFolder);
					if (::MessageBox(hDlg,szMessage,TEXT("フォルダ作成の確認"),
										MB_YESNO | MB_ICONQUESTION)==IDYES) {
						int Result;

						Result=::SHCreateDirectoryEx(hDlg,szRecordFolder,NULL);
						if (Result!=ERROR_SUCCESS
								&& Result!=ERROR_ALREADY_EXISTS) {
							::MessageBox(hDlg,TEXT("フォルダが作成できません。"),
											NULL,MB_OK | MB_ICONEXCLAMATION);
						}
					}
				}
				::lstrcpy(pThis->m_szRecordFolder,szRecordFolder);
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
