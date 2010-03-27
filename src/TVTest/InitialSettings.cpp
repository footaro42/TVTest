#include "stdafx.h"
#include <shlobj.h>
#include "TVTest.h"
#include "AppMain.h"
#include "InitialSettings.h"
#include "DirectShowFilter/DirectShowUtil.h"
#include "DialogUtil.h"
#include "DrawUtil.h"
#include "Aero.h"
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
	// ...と問題が出る環境もあるみたい
	{
		OSVERSIONINFO osvi;

		osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
		GetVersionEx(&osvi);
		if (osvi.dwMajorVersion>=6)
			m_VideoRenderer=CVideoRenderer::RENDERER_EVR;
	}
#endif
	m_CardReader=
#ifndef TVH264_FOR_1SEG
		CCoreEngine::CARDREADER_SCARD;
#else
		CCoreEngine::CARDREADER_NONE;
#endif
	if (!::SHGetSpecialFolderPath(NULL,m_szRecordFolder,CSIDL_MYVIDEO,FALSE)
			&& !::SHGetSpecialFolderPath(NULL,m_szRecordFolder,CSIDL_PERSONAL,FALSE))
		m_szRecordFolder[0]='\0';
}


CInitialSettings::~CInitialSettings()
{
	m_LogoImage.Free();
	m_GdiPlus.Finalize();
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


INT_PTR CALLBACK CInitialSettings::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CInitialSettings *pThis=reinterpret_cast<CInitialSettings*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			AdjustDialogPos(NULL,hDlg);
			{
				HWND hwndLogo=::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO);
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

			// Driver
			{
				int NormalDriverCount=0;

				DlgComboBox_LimitText(hDlg,IDC_INITIALSETTINGS_DRIVER,MAX_PATH-1);
				for (int i=0;i<pThis->m_pDriverManager->NumDrivers();i++) {
					const CDriverInfo *pDriverInfo=pThis->m_pDriverManager->GetDriverInfo(i);
					int Index;

					if (CCoreEngine::IsNetworkDriverFileName(pDriverInfo->GetFileName())) {
						Index=i;
					} else {
						Index=NormalDriverCount++;
					}
					DlgComboBox_InsertString(hDlg,IDC_INITIALSETTINGS_DRIVER,
											 Index,pDriverInfo->GetFileName());
				}
				if (pThis->m_pDriverManager->NumDrivers()>0) {
					DlgComboBox_GetLBString(hDlg,IDC_INITIALSETTINGS_DRIVER,
											0,pThis->m_szDriverFileName);
					::SetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,pThis->m_szDriverFileName);
				}
			}

			// MPEG-2 or H.264 decoder
			{
				CDirectShowFilterFinder FilterFinder;
				WCHAR szFilterName[MAX_DECODER_NAME];
				int Sel=0,Count=0;

				if (FilterFinder.FindFilter(&MEDIATYPE_Video,
#ifndef TVH264
											&MEDIASUBTYPE_MPEG2_VIDEO
#else
											&MEDIASUBTYPE_H264
#endif
											)) {
					for (int i=0;i<FilterFinder.GetFilterCount();i++) {
						if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
							int Index=(int)DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,szFilterName);
							if (::lstrcmpi(szFilterName,pThis->m_szMpeg2DecoderName)==0)
								Sel=Index;
							Count++;
						}
					}
				}
				DlgComboBox_InsertString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,
					0,Count>0?TEXT("デフォルト"):TEXT("<デコーダが見付かりません>"));
				DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,Sel);
			}

			// Video renderer
			{
				LPCTSTR pszName;

				DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,TEXT("デフォルト"));
				for (int i=1;(pszName=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
					DlgComboBox_AddString(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,pszName);
				DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER,
									  pThis->m_VideoRenderer);
			}

			// Card reader
			{
				static const LPCTSTR pszCardReaderList[] = {
					TEXT("なし(スクランブル解除しない)"),
					TEXT("スマートカードリーダ"),
					TEXT("HDUS内蔵カードリーダ"),
				};
				CCardReader *pCardReader;

				SetComboBoxList(hDlg,IDC_INITIALSETTINGS_CARDREADER,
								&pszCardReaderList[0],lengthof(pszCardReaderList));
				pThis->m_CardReader=CCoreEngine::CARDREADER_NONE;
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_SCARD);
				if (pCardReader!=NULL) {
					if (pCardReader->NumReaders()>0)
						pThis->m_CardReader=CCoreEngine::CARDREADER_SCARD;
					delete pCardReader;
				}
				if (pThis->m_CardReader==CCoreEngine::CARDREADER_NONE) {
					pCardReader=CCardReader::CreateCardReader(CCardReader::READER_HDUS);
					if (pCardReader!=NULL) {
						if (pCardReader->Open()) {
							pThis->m_CardReader=CCoreEngine::CARDREADER_HDUS;
							pCardReader->Close();
						}
						delete pCardReader;
					}
				}
				DlgComboBox_SetCurSel(hDlg,IDC_INITIALSETTINGS_CARDREADER,pThis->m_CardReader);
			}

			// Record folder
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
				CCoreEngine::CardReaderType ReaderType=CCoreEngine::CARDREADER_NONE;
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
						ReaderType=CCoreEngine::CARDREADER_SCARD;
					}
					delete pCardReader;
				}
				pCardReader=CCardReader::CreateCardReader(CCardReader::READER_HDUS);
				if (pCardReader!=NULL) {
					if (pCardReader->Open()) {
						::lstrcat(szText,TEXT("\"HDUS内蔵カードリーダ\"\n"));
						pCardReader->Close();
						if (ReaderType==CCoreEngine::CARDREADER_NONE)
							ReaderType=CCoreEngine::CARDREADER_HDUS;
					}
					delete pCardReader;
				}
				::SetCursor(::LoadCursor(NULL,IDC_ARROW));
				if (ReaderType==CCoreEngine::CARDREADER_NONE) {
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
				TCHAR szMpeg2Decoder[MAX_DECODER_NAME];
				CVideoRenderer::RendererType VideoRenderer;

				int SelDecoder=(int)DlgComboBox_GetCurSel(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER);
				if (SelDecoder>0) {
					DlgComboBox_GetLBString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,SelDecoder,szMpeg2Decoder);
				} else if (DlgComboBox_GetCount(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER)>1) {
					DlgComboBox_GetLBString(hDlg,IDC_INITIALSETTINGS_MPEG2DECODER,1,szMpeg2Decoder);
				} else {
					::MessageBox(hDlg,
						TEXT("デコーダが見付からないため、再生を行うことができません。\n")
						TEXT("映像を再生するにはデコーダをインストールしてください。"),
						TEXT("お知らせ"),
						MB_OK | MB_ICONINFORMATION);
					szMpeg2Decoder[0]='\0';
				}
				VideoRenderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_INITIALSETTINGS_VIDEORENDERER);
#ifndef TVH264
				// 相性の悪い組み合わせに対して注意を表示する
				static const struct {
					LPCTSTR pszDecoder;
					CVideoRenderer::RendererType Renderer;
					LPCTSTR pszMessage;
				} ConflictList[] = {
					{TEXT("CyberLink"),	CVideoRenderer::RENDERER_DEFAULT,
						TEXT("CyberLink のデコーダとデフォルトレンダラの組み合わせで、\n")
						TEXT("一部の番組で比率がおかしくなる現象が出る事があるため、\n")
						TEXT("レンダラをデフォルト以外にすることをお奨めします。\n")
						TEXT("現在の設定を変更しますか?")},
				};
				for (int i=0;i<lengthof(ConflictList);i++) {
					if (::StrCmpNI(szMpeg2Decoder,ConflictList[i].pszDecoder,::lstrlen(ConflictList[i].pszDecoder))==0
							&& VideoRenderer==ConflictList[i].Renderer) {

						if (::MessageBox(hDlg,ConflictList[i].pszMessage,TEXT("注意"),
										 MB_YESNO | MB_ICONINFORMATION)==IDYES)
							return TRUE;
						break;
					}
				}
#endif

				::GetDlgItemText(hDlg,IDC_INITIALSETTINGS_DRIVER,
								 pThis->m_szDriverFileName,MAX_PATH);
				if (SelDecoder>0)
					::lstrcpy(pThis->m_szMpeg2DecoderName,szMpeg2Decoder);
				else
					pThis->m_szMpeg2DecoderName[0]='\0';
				pThis->m_VideoRenderer=VideoRenderer;
				pThis->m_CardReader=(CCoreEngine::CardReaderType)
					DlgComboBox_GetCurSel(hDlg,IDC_INITIALSETTINGS_CARDREADER);

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

	case WM_PAINT:
		{
			CInitialSettings *pThis=GetThis(hDlg);

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
						(rc.right-pThis->m_LogoImage.GetWidth())/2,
						(rc.bottom-pThis->m_LogoImage.GetHeight())/2);
				}
				::EndPaint(hDlg,&ps);
				return TRUE;
			}
		}
		break;

	case WM_CTLCOLORSTATIC:
		if (reinterpret_cast<HWND>(lParam)==::GetDlgItem(hDlg,IDC_INITIALSETTINGS_LOGO))
			return reinterpret_cast<BOOL>(::GetStockObject(WHITE_BRUSH));
		break;

	case WM_DESTROY:
		{
			CInitialSettings *pThis=GetThis(hDlg);
			HBITMAP hbm=reinterpret_cast<HBITMAP>(::SendDlgItemMessage(hDlg,IDC_INITIALSETTINGS_LOGO,
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


bool CInitialSettings::ShowDialog(HWND hwndOwner)
{
	return ::DialogBoxParam(GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_INITIALSETTINGS),
							hwndOwner,DlgProc,reinterpret_cast<LPARAM>(this))==IDOK;
}
