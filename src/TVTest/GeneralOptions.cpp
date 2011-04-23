#include "stdafx.h"
#include "BonTsEngine/TsDescrambler.h"
#include "BonTsEngine/Multi2Decoder.h"
#include "TVTest.h"
#include "AppMain.h"
#include "GeneralOptions.h"
#include "DriverManager.h"
#include "DialogUtil.h"
#include "MessageDialog.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




static void FillRandomData(BYTE *pData,size_t Size)
{
	for (size_t i=0;i<Size;i++)
		pData[i]=::rand()&0xFF;
}




CGeneralOptions::CGeneralOptions()
	: m_DefaultDriverType(DEFAULT_DRIVER_LAST)
	, m_VideoRendererType(CVideoRenderer::RENDERER_DEFAULT)
	, m_CardReaderType(CCoreEngine::CARDREADER_SCARD)
	, m_fTemporaryNoDescramble(false)
	, m_fResident(false)
	, m_fKeepSingleTask(false)
#ifdef _M_AMD64
	, m_fDescrambleUseSSE2(CTsDescrambler::IsSSE2Available())
#else
	// SSE2 の方が遅いプロセッサもあるのでデフォルトは false にしておく
	, m_fDescrambleUseSSE2(false)
#endif
	, m_fDescrambleCurServiceOnly(false)
	, m_fEnableEmmProcess(false)
{
	m_szDriverDirectory[0]='\0';
	m_szDefaultDriverName[0]='\0';
	m_szLastDriverName[0]='\0';
}


CGeneralOptions::~CGeneralOptions()
{
	Destroy();
}


bool CGeneralOptions::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();
	CCoreEngine *pCoreEngine=AppMain.GetCoreEngine();

	if ((Flags&UPDATE_CARDREADER)!=0) {
		if (!pCoreEngine->SetCardReaderType(m_CardReaderType)) {
			AppMain.AddLog(pCoreEngine->GetLastErrorText());
			AppMain.GetUICore()->GetSkin()->ShowErrorMessage(pCoreEngine);
		}
	}

	if ((Flags&UPDATE_RESIDENT)!=0) {
		AppMain.GetUICore()->SetResident(m_fResident);
	}

	if ((Flags&UPDATE_DESCRAMBLECURONLY)!=0) {
		if (!AppMain.GetRecordManager()->IsRecording())
			pCoreEngine->m_DtvEngine.SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
	}

	if ((Flags&UPDATE_ENABLEEMMPROCESS)!=0) {
		pCoreEngine->m_DtvEngine.m_TsDescrambler.EnableEmmProcess(m_fEnableEmmProcess);
	}

	return true;
}


bool CGeneralOptions::Read(CSettings *pSettings)
{
	int Value;

	TCHAR szDirectory[MAX_PATH];
	if (pSettings->Read(TEXT("DriverDirectory"),szDirectory,lengthof(szDirectory))
			&& szDirectory[0]!='\0') {
		::lstrcpy(m_szDriverDirectory,szDirectory);
		GetAppClass().GetCoreEngine()->SetDriverDirectory(szDirectory);
	}
	if (pSettings->Read(TEXT("DefaultDriverType"),&Value)
			&& Value>=DEFAULT_DRIVER_NONE && Value<=DEFAULT_DRIVER_CUSTOM)
		m_DefaultDriverType=(DefaultDriverType)Value;
	pSettings->Read(TEXT("DefaultDriver"),
					m_szDefaultDriverName,lengthof(m_szDefaultDriverName));
	pSettings->Read(TEXT("Driver"),
					m_szLastDriverName,lengthof(m_szLastDriverName));
	TCHAR szDecoder[MAX_MPEG2_DECODER_NAME];
	if (pSettings->Read(TEXT("Mpeg2Decoder"),szDecoder,lengthof(szDecoder)))
		m_Mpeg2DecoderName.Set(szDecoder);
	TCHAR szRenderer[16];
	if (pSettings->Read(TEXT("Renderer"),szRenderer,lengthof(szRenderer))) {
		if (szRenderer[0]=='\0') {
			m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
		} else {
			CVideoRenderer::RendererType Renderer=CVideoRenderer::ParseName(szRenderer);
			if (Renderer!=CVideoRenderer::RENDERER_UNDEFINED)
				m_VideoRendererType=Renderer;
		}
	}
	bool fNoDescramble;
	if (pSettings->Read(TEXT("NoDescramble"),&fNoDescramble) && fNoDescramble)	// Backward compatibility
		m_CardReaderType=CCoreEngine::CARDREADER_NONE;
	if (pSettings->Read(TEXT("CardReader"),&Value)
			&& Value>=CCoreEngine::CARDREADER_NONE && Value<=CCoreEngine::CARDREADER_LAST)
		m_CardReaderType=(CCoreEngine::CardReaderType)Value;
	pSettings->Read(TEXT("Resident"),&m_fResident);
	pSettings->Read(TEXT("KeepSingleTask"),&m_fKeepSingleTask);
	pSettings->Read(TEXT("DescrambleSSE2"),&m_fDescrambleUseSSE2);
	pSettings->Read(TEXT("DescrambleCurServiceOnly"),&m_fDescrambleCurServiceOnly);
	pSettings->Read(TEXT("ProcessEMM"),&m_fEnableEmmProcess);
	return true;
}


bool CGeneralOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("DriverDirectory"),m_szDriverDirectory);
	pSettings->Write(TEXT("DefaultDriverType"),(int)m_DefaultDriverType);
	pSettings->Write(TEXT("DefaultDriver"),m_szDefaultDriverName);
	pSettings->Write(TEXT("Driver"),GetAppClass().GetCoreEngine()->GetDriverFileName());
	pSettings->Write(TEXT("Mpeg2Decoder"),m_Mpeg2DecoderName.GetSafe());
	pSettings->Write(TEXT("Renderer"),
					 CVideoRenderer::EnumRendererName((int)m_VideoRendererType));
	pSettings->Write(TEXT("NoDescramble"),m_CardReaderType==CCoreEngine::CARDREADER_NONE);	// Backward compatibility
	pSettings->Write(TEXT("CardReader"),(int)m_CardReaderType);
	pSettings->Write(TEXT("Resident"),m_fResident);
	pSettings->Write(TEXT("KeepSingleTask"),m_fKeepSingleTask);
	pSettings->Write(TEXT("DescrambleSSE2"),m_fDescrambleUseSSE2);
	pSettings->Write(TEXT("DescrambleCurServiceOnly"),m_fDescrambleCurServiceOnly);
	pSettings->Write(TEXT("ProcessEMM"),m_fEnableEmmProcess);
	return true;
}


bool CGeneralOptions::Create(HWND hwndOwner)
{
	return CreateDialogWindow(hwndOwner,
							  GetAppClass().GetResourceInstance(),MAKEINTRESOURCE(IDD_OPTIONS_GENERAL));
}


CGeneralOptions::DefaultDriverType CGeneralOptions::GetDefaultDriverType() const
{
	return m_DefaultDriverType;
}


LPCTSTR CGeneralOptions::GetDefaultDriverName() const
{
	return m_szDefaultDriverName;
}


bool CGeneralOptions::SetDefaultDriverName(LPCTSTR pszDriverName)
{
	if (pszDriverName==NULL)
		m_szDefaultDriverName[0]='\0';
	else
		::lstrcpy(m_szDefaultDriverName,pszDriverName);
	return true;
}


bool CGeneralOptions::GetFirstDriverName(LPTSTR pszDriverName) const
{
	switch (m_DefaultDriverType) {
	case DEFAULT_DRIVER_NONE:
		pszDriverName[0]='\0';
		break;
	case DEFAULT_DRIVER_LAST:
		::lstrcpy(pszDriverName,m_szLastDriverName);
		break;
	case DEFAULT_DRIVER_CUSTOM:
		::lstrcpy(pszDriverName,m_szDefaultDriverName);
		break;
	default:
		return false;
	}
	return true;
}


LPCTSTR CGeneralOptions::GetMpeg2DecoderName() const
{
	return m_Mpeg2DecoderName.Get();
}


bool CGeneralOptions::SetMpeg2DecoderName(LPCTSTR pszDecoderName)
{
	return m_Mpeg2DecoderName.Set(pszDecoderName);
}


CVideoRenderer::RendererType CGeneralOptions::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CGeneralOptions::SetVideoRendererType(CVideoRenderer::RendererType Renderer)
{
	if (CVideoRenderer::EnumRendererName((int)Renderer)==NULL)
		return false;
	m_VideoRendererType=Renderer;
	return true;
}


CCoreEngine::CardReaderType CGeneralOptions::GetCardReaderType() const
{
	return m_CardReaderType;
}


bool CGeneralOptions::SetCardReaderType(CCoreEngine::CardReaderType CardReader)
{
	if (CardReader<CCoreEngine::CARDREADER_NONE || CardReader>CCoreEngine::CARDREADER_LAST)
		return false;
	m_CardReaderType=CardReader;
	return true;
}


void CGeneralOptions::SetTemporaryNoDescramble(bool fNoDescramble)
{
	m_fTemporaryNoDescramble=fNoDescramble;
}


bool CGeneralOptions::GetResident() const
{
	return m_fResident;
}


bool CGeneralOptions::GetKeepSingleTask() const
{
	return m_fKeepSingleTask;
}


bool CGeneralOptions::GetDescrambleCurServiceOnly() const
{
	return m_fDescrambleCurServiceOnly;
}


bool CGeneralOptions::GetEnableEmmProcess() const
{
	return m_fEnableEmmProcess;
}


INT_PTR CGeneralOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CAppMain &AppMain=GetAppClass();

			::SendDlgItemMessage(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,m_szDriverDirectory);

			// BonDriver
			::CheckRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
									IDC_OPTIONS_DEFAULTDRIVER_CUSTOM,
							   (int)m_DefaultDriverType+IDC_OPTIONS_DEFAULTDRIVER_NONE);
			EnableDlgItems(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
						   m_DefaultDriverType==DEFAULT_DRIVER_CUSTOM);

			const CDriverManager *pDriverManager=AppMain.GetDriverManager();
			DlgComboBox_LimitText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,MAX_PATH-1);
			/*
			TCHAR szDirectory[MAX_PATH];
			AppMain.GetDriverDirectory(szDirectory);
			pDriverManager->Find(szDirectory);
			AppMain.UpdateDriverMenu();
			*/
			for (int i=0;i<pDriverManager->NumDrivers();i++) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									  pDriverManager->GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,m_szDefaultDriverName);

			// MPEG-2 decoder
			CDirectShowFilterFinder FilterFinder;
			int Count=0;
			if (FilterFinder.FindFilter(&MEDIATYPE_Video,
#ifndef TVH264
										&MEDIASUBTYPE_MPEG2_VIDEO
#else
										&MEDIASUBTYPE_H264
#endif
					)) {
				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					WCHAR szFilterName[MAX_MPEG2_DECODER_NAME];

					if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
						DlgComboBox_AddString(hDlg,IDC_OPTIONS_DECODER,szFilterName);
						Count++;
					}
				}
			}
			int Sel=0;
			if (Count==0) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DECODER,TEXT("<デコーダが見付かりません>"));
			} else {
				CMediaViewer &MediaViewer=GetAppClass().GetCoreEngine()->m_DtvEngine.m_MediaViewer;
				TCHAR szText[32+MAX_MPEG2_DECODER_NAME];

				::lstrcpy(szText,TEXT("デフォルト"));
				if (!m_Mpeg2DecoderName.IsEmpty()) {
					Sel=(int)DlgComboBox_FindStringExact(hDlg,IDC_OPTIONS_DECODER,-1,
														 m_Mpeg2DecoderName.Get())+1;
				} else if (MediaViewer.IsOpen()) {
					::lstrcat(szText,TEXT(" ("));
					MediaViewer.GetVideoDecoderName(szText+::lstrlen(szText),MAX_MPEG2_DECODER_NAME);
					::lstrcat(szText,TEXT(")"));
				}
				DlgComboBox_InsertString(hDlg,IDC_OPTIONS_DECODER,0,szText);
			}
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_DECODER,Sel);

			// Renderer
			LPCTSTR pszRenderer;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,TEXT("デフォルト"));
			for (int i=1;(pszRenderer=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,pszRenderer);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RENDERER,m_VideoRendererType);

			// Card reader
			for (int i=0;i<=CCoreEngine::CARDREADER_LAST;i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_CARDREADER,
									  CCoreEngine::GetCardReaderSettingName((CCoreEngine::CardReaderType)i));
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_CARDREADER,
				(int)(m_fTemporaryNoDescramble?CCoreEngine::CARDREADER_NONE:m_CardReaderType));

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESIDENT,m_fResident);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_KEEPSINGLETASK,m_fKeepSingleTask);
			if (CTsDescrambler::IsSSE2Available())
				DlgCheckBox_Check(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2,m_fDescrambleUseSSE2);
			else
				EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2,
									IDC_OPTIONS_DESCRAMBLEBENCHMARK,false);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY,m_fDescrambleCurServiceOnly);
			if (m_CardReaderType==CCoreEngine::CARDREADER_NONE)
				EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2,
									IDC_OPTIONS_ENABLEEMMPROCESS,false);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS,m_fEnableEmmProcess);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_OPTIONS_DRIVERDIRECTORY_BROWSE:
			{
				TCHAR szDirectory[MAX_PATH];

				if (::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory,lengthof(szDirectory))>0) {
					if (::PathIsRelative(szDirectory)) {
						TCHAR szTemp[MAX_PATH];

						GetAppClass().GetAppDirectory(szTemp);
						::PathAppend(szTemp,szDirectory);
						::PathCanonicalize(szDirectory,szTemp);
					}
				} else {
					GetAppClass().GetAppDirectory(szDirectory);
				}
				if (BrowseFolderDialog(hDlg,szDirectory,TEXT("ドライバの検索フォルダを選択してください。")))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,szDirectory);
			}
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_NONE:
		case IDC_OPTIONS_DEFAULTDRIVER_LAST:
		case IDC_OPTIONS_DEFAULTDRIVER_CUSTOM:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									   IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
									   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM);
			return TRUE;

		case IDC_OPTIONS_DEFAULTDRIVER_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitDir[MAX_PATH];
				CFilePath FilePath;

				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName,lengthof(szFileName));
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
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,szFileName);
			}
			return TRUE;

		case IDC_OPTIONS_CARDREADER:
			if (HIWORD(wParam)==CBN_SELCHANGE) {
				bool fBcas=DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_CARDREADER)>0;

				if (CTsDescrambler::IsSSE2Available())
					EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2,
										IDC_OPTIONS_DESCRAMBLEBENCHMARK,fBcas);
				EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY,
									IDC_OPTIONS_ENABLEEMMPROCESS,fBcas);
			}
			return TRUE;

		case IDC_OPTIONS_DESCRAMBLEBENCHMARK:
			if (::MessageBox(hDlg,
					TEXT("ベンチマークテストを開始します。\n")
					TEXT("終了するまで操作は行わないようにしてください。\n")
					TEXT("結果はばらつきがありますので、数回実行してください。"),
					TEXT("ベンチマークテスト"),
					MB_OKCANCEL | MB_ICONINFORMATION)==IDOK) {
				static const DWORD BENCHMARK_COUNT=400000;
				HCURSOR hcurOld=::SetCursor(LoadCursor(NULL,IDC_WAIT));
				CMulti2Decoder Multi2Decoder;
				BYTE SystemKey[32],InitialCbc[8],ScrambleKey[16],Data[184];
				DWORD BenchmarkCount=0,StartTime,NormalTime,SSE2Time;

				FillRandomData(SystemKey,sizeof(SystemKey));
				FillRandomData(InitialCbc,sizeof(InitialCbc));
				FillRandomData(Data,sizeof(Data));
				Multi2Decoder.Initialize(SystemKey,InitialCbc);
				NormalTime=SSE2Time=0;
				do {
					StartTime=::timeGetTime();
					for (int i=0;i<BENCHMARK_COUNT;i++) {
						if (i%10000==0) {
							FillRandomData(ScrambleKey,sizeof(ScrambleKey));
							Multi2Decoder.SetScrambleKey(ScrambleKey);
						}
						Multi2Decoder.Decode(Data,sizeof(Data),2);
					}
					NormalTime+=TickTimeSpan(StartTime,::timeGetTime());
					StartTime=::timeGetTime();
					for (int i=0;i<BENCHMARK_COUNT;i++) {
						if (i%10000==0) {
							FillRandomData(ScrambleKey,sizeof(ScrambleKey));
							Multi2Decoder.SetScrambleKey(ScrambleKey);
						}
						Multi2Decoder.DecodeSSE2(Data,sizeof(Data),2);
					}
					SSE2Time+=TickTimeSpan(StartTime,::timeGetTime());
					BenchmarkCount+=BENCHMARK_COUNT;
				} while (NormalTime<1000 && SSE2Time<1000);

				::SetCursor(hcurOld);
				int Percentage;
				if (NormalTime>=SSE2Time)
					Percentage=(int)(NormalTime*100/SSE2Time)-100;
				else
					Percentage=-(int)((SSE2Time*100/NormalTime)-100);
				// 表示されるメッセージに深い意味はない
				TCHAR szText[256];
				::wsprintf(szText,
						   TEXT("%lu 回の実行に掛かった時間\n")
						   TEXT("SSE2不使用 : %lu ms\n")
						   TEXT("SSE2使用 : %lu ms\n")
						   TEXT("SSE2で高速化される割合 : %d %%\n")
						   TEXT("\n%s"),
						   BenchmarkCount,NormalTime,SSE2Time,Percentage,
						   Percentage<=5?
								TEXT("SSE2を無効にすることをお勧めします。"):
						   Percentage<10?TEXT("微妙…"):
								TEXT("SSE2を有効にすることをお勧めします。"));
				CMessageDialog MessageDialog;
				MessageDialog.Show(hDlg,CMessageDialog::TYPE_INFO,szText,
								   TEXT("ベンチマークテスト結果"),NULL,TEXT("ベンチマークテスト"));
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CVideoRenderer::RendererType Renderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RENDERER);
				if (Renderer!=m_VideoRendererType) {
					if (!CVideoRenderer::IsAvailable(Renderer)) {
						SettingError();
						::MessageBox(hDlg,TEXT("選択されたレンダラはこの環境で利用可能になっていません。"),
									 NULL,MB_OK | MB_ICONEXCLAMATION);
						return TRUE;
					}
					m_VideoRendererType=Renderer;
					SetUpdateFlag(UPDATE_RENDERER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				::GetDlgItemText(hDlg,IDC_OPTIONS_DRIVERDIRECTORY,
								 m_szDriverDirectory,lengthof(m_szDriverDirectory));
				m_DefaultDriverType=(DefaultDriverType)
					(GetCheckedRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
										   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM)-
					IDC_OPTIONS_DEFAULTDRIVER_NONE);
				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								 m_szDefaultDriverName,lengthof(m_szDefaultDriverName));

				TCHAR szDecoder[MAX_MPEG2_DECODER_NAME];
				int Sel=(int)DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_DECODER);
				if (Sel>0)
					DlgComboBox_GetLBString(hDlg,IDC_OPTIONS_DECODER,Sel,szDecoder);
				else
					szDecoder[0]='\0';
				if (::lstrcmpi(szDecoder,m_Mpeg2DecoderName.GetSafe())!=0) {
					m_Mpeg2DecoderName.Set(szDecoder);
					SetUpdateFlag(UPDATE_DECODER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				CCoreEngine::CardReaderType CardReader=(CCoreEngine::CardReaderType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_CARDREADER);
				if ((m_fTemporaryNoDescramble && CardReader!=CCoreEngine::CARDREADER_NONE)
						|| (!m_fTemporaryNoDescramble && CardReader!=m_CardReaderType)) {
					m_CardReaderType=CardReader;
					m_fTemporaryNoDescramble=false;
					SetUpdateFlag(UPDATE_CARDREADER);
				}

				bool fResident=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESIDENT);
				if (fResident!=m_fResident) {
					m_fResident=fResident;
					SetUpdateFlag(UPDATE_RESIDENT);
				}

				m_fKeepSingleTask=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_KEEPSINGLETASK);

				m_fDescrambleUseSSE2=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2);

				bool fCurOnly=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY);
				if (fCurOnly!=m_fDescrambleCurServiceOnly) {
					m_fDescrambleCurServiceOnly=fCurOnly;
					SetUpdateFlag(UPDATE_DESCRAMBLECURONLY);
				}

				bool fEmm=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS);
				if (fEmm!=m_fEnableEmmProcess) {
					m_fEnableEmmProcess=fEmm;
					SetUpdateFlag(UPDATE_ENABLEEMMPROCESS);
				}
			}
			return TRUE;
		}
		break;
	}

	return FALSE;
}
