#include "stdafx.h"
#include "BonTsEngine/TsDescrambler.h"
#include "BonTsEngine/Multi2Decoder.h"
#include "TVTest.h"
#include "AppMain.h"
#include "GeneralOptions.h"
#include "DialogUtil.h"
#include "Util.h"
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
{
	m_DefaultDriverType=DEFAULT_DRIVER_LAST;
	m_szDefaultDriverName[0]='\0';
	m_szMpeg2DecoderName[0]='\0';
	m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
	m_CardReaderType=CCardReader::READER_SCARD;
	m_fTemporaryNoDescramble=false;
	m_fResident=false;
	m_fKeepSingleTask=false;
	//m_fDescrambleUseSSE2=CTsDescrambler::IsSSE2Available();
	m_fDescrambleUseSSE2=false;
	m_fDescrambleCurServiceOnly=false;
	m_fEnableEmmProcess=false;
	m_fPacketBuffering=false;
	m_PacketBufferLength=40000;
	m_PacketBufferPoolPercentage=50;
}


CGeneralOptions::~CGeneralOptions()
{
}


bool CGeneralOptions::Apply(DWORD Flags)
{
	CAppMain &AppMain=GetAppClass();
	CCoreEngine *pCoreEngine=AppMain.GetCoreEngine();
	CMainWindow *pMainWindow=AppMain.GetMainWindow();

	/*
	if ((Flags&(UPDATE_DECODER | UPDATE_RENDERER))!=0) {
		if (pCoreEngine->m_DtvEngine.m_MediaViewer.IsOpen()) {
			CStatusView *pStatusView=pMainWindow->GetStatusView();

			pCoreEngine->m_DtvEngine.SetTracer(pStatusView);
			pMainWindow->BuildMediaViewer();
			pCoreEngine->m_DtvEngine.SetTracer(NULL);
			pStatusView->SetSingleText(NULL);
		}
	}
	*/

	if ((Flags&UPDATE_CARDREADER)!=0) {
		if (!pCoreEngine->SetCardReaderType(m_CardReaderType)) {
			AppMain.AddLog(pCoreEngine->GetLastErrorText());
			pMainWindow->ShowErrorMessage(pCoreEngine);
			pCoreEngine->SetCardReaderType(CCardReader::READER_NONE);
		}
	}

	if ((Flags&UPDATE_RESIDENT)!=0) {
		pMainWindow->SetResident(m_fResident);
	}

	if ((Flags&UPDATE_DESCRAMBLECURONLY)!=0) {
		if (!AppMain.GetRecordManager()->IsRecording())
			pCoreEngine->m_DtvEngine.SetDescrambleCurServiceOnly(m_fDescrambleCurServiceOnly);
	}

	if ((Flags&UPDATE_ENABLEEMMPROCESS)!=0) {
		pCoreEngine->m_DtvEngine.m_TsDescrambler.EnableEmmProcess(m_fEnableEmmProcess);
	}

	if ((Flags&UPDATE_PACKETBUFFERING)!=0) {
		if (m_fPacketBuffering) {
			pCoreEngine->SetPacketBufferLength(m_PacketBufferLength);
			pCoreEngine->SetPacketBufferPoolPercentage(m_PacketBufferPoolPercentage);
		}
		pCoreEngine->SetPacketBuffering(m_fPacketBuffering);
	}

	return true;
}


bool CGeneralOptions::Read(CSettings *pSettings)
{
	int Value;

	if (pSettings->Read(TEXT("DefaultDriverType"),&Value)
			&& Value>=DEFAULT_DRIVER_NONE && Value<=DEFAULT_DRIVER_CUSTOM)
		m_DefaultDriverType=(DefaultDriverType)Value;
	pSettings->Read(TEXT("DefaultDriver"),
					m_szDefaultDriverName,lengthof(m_szDefaultDriverName));
	pSettings->Read(TEXT("Mpeg2Decoder"),
					m_szMpeg2DecoderName,lengthof(m_szMpeg2DecoderName));
	TCHAR szRenderer[16];
	if (pSettings->Read(TEXT("Renderer"),szRenderer,lengthof(szRenderer))) {
		if (szRenderer[0]=='\0') {
			m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
		} else {
			m_VideoRendererType=CVideoRenderer::ParseName(szRenderer);
			if (m_VideoRendererType==CVideoRenderer::RENDERER_UNDEFINED)
				m_VideoRendererType=CVideoRenderer::RENDERER_DEFAULT;
		}
	}
	bool fNoDescramble;
	if (pSettings->Read(TEXT("NoDescramble"),&fNoDescramble) && fNoDescramble)	// Backward compatibility
		m_CardReaderType=CCardReader::READER_NONE;
	if (pSettings->Read(TEXT("CardReader"),&Value)
			&& Value>=CCardReader::READER_NONE && Value<=CCardReader::READER_LAST)
		m_CardReaderType=(CCardReader::ReaderType)Value;
	pSettings->Read(TEXT("Resident"),&m_fResident);
	pSettings->Read(TEXT("KeepSingleTask"),&m_fKeepSingleTask);
	pSettings->Read(TEXT("DescrambleSSE2"),&m_fDescrambleUseSSE2);
	pSettings->Read(TEXT("DescrambleCurServiceOnly"),&m_fDescrambleCurServiceOnly);
	pSettings->Read(TEXT("ProcessEMM"),&m_fEnableEmmProcess);
	pSettings->Read(TEXT("PacketBuffering"),&m_fPacketBuffering);
	unsigned int BufferLength;
	if (pSettings->Read(TEXT("PacketBufferLength"),&BufferLength))
		m_PacketBufferLength=BufferLength;
	pSettings->Read(TEXT("PacketBufferPoolPercentage"),&m_PacketBufferPoolPercentage);
	return true;
}


bool CGeneralOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("DefaultDriverType"),(int)m_DefaultDriverType);
	pSettings->Write(TEXT("DefaultDriver"),m_szDefaultDriverName);
	pSettings->Write(TEXT("Mpeg2Decoder"),m_szMpeg2DecoderName);
	pSettings->Write(TEXT("Renderer"),
					 CVideoRenderer::EnumRendererName((int)m_VideoRendererType));
	pSettings->Write(TEXT("NoDescramble"),m_CardReaderType==CCardReader::READER_NONE);	// Backward compatibility
	pSettings->Write(TEXT("CardReader"),(int)m_CardReaderType);
	pSettings->Write(TEXT("Resident"),m_fResident);
	pSettings->Write(TEXT("KeepSingleTask"),m_fKeepSingleTask);
	pSettings->Write(TEXT("DescrambleSSE2"),m_fDescrambleUseSSE2);
	pSettings->Write(TEXT("DescrambleCurServiceOnly"),m_fDescrambleCurServiceOnly);
	pSettings->Write(TEXT("ProcessEMM"),m_fEnableEmmProcess);
	pSettings->Write(TEXT("PacketBuffering"),m_fPacketBuffering);
	pSettings->Write(TEXT("PacketBufferLength"),(unsigned int)m_PacketBufferLength);
	pSettings->Write(TEXT("PacketBufferPoolPercentage"),m_PacketBufferPoolPercentage);
	return true;
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


LPCTSTR CGeneralOptions::GetMpeg2DecoderName() const
{
	return m_szMpeg2DecoderName;
}


bool CGeneralOptions::SetMpeg2DecoderName(LPCTSTR pszDecoderName)
{
	if (pszDecoderName==NULL)
		m_szMpeg2DecoderName[0]='\0';
	else
		::lstrcpy(m_szMpeg2DecoderName,pszDecoderName);
	return true;
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


CCardReader::ReaderType CGeneralOptions::GetCardReaderType() const
{
	return m_CardReaderType;
}


bool CGeneralOptions::SetCardReaderType(CCardReader::ReaderType CardReader)
{
	if (CardReader<CCardReader::READER_NONE || CardReader>CCardReader::READER_LAST)
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


bool CGeneralOptions::GetPacketBuffering() const
{
	return m_fPacketBuffering;
}


bool CGeneralOptions::SetPacketBuffering(bool fBuffering)
{
	m_fPacketBuffering=fBuffering;
	return true;
}


DWORD CGeneralOptions::GetPacketBufferLength() const
{
	return m_PacketBufferLength;
}


int CGeneralOptions::GetPacketBufferPoolPercentage() const
{
	return m_PacketBufferPoolPercentage;
}


BOOL CALLBACK CGeneralOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CGeneralOptions *pThis=static_cast<CGeneralOptions*>(OnInitDialog(hDlg,lParam));
			CAppMain &AppMain=GetAppClass();

			// Driver
			::CheckRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
									IDC_OPTIONS_DEFAULTDRIVER_CUSTOM,
							   (int)pThis->m_DefaultDriverType+IDC_OPTIONS_DEFAULTDRIVER_NONE);
			EnableDlgItems(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								IDC_OPTIONS_DEFAULTDRIVER_BROWSE,
						   pThis->m_DefaultDriverType==DEFAULT_DRIVER_CUSTOM);

			TCHAR szDirectory[MAX_PATH];
			const CDriverManager *pDriverManager=AppMain.GetDriverManager();
			DlgComboBox_LimitText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,MAX_PATH-1);
			AppMain.GetAppDirectory(szDirectory);
			/*
			pDriverManager->Find(szDirectory);
			AppMain.UpdateDriverMenu();
			*/
			for (int i=0;i<pDriverManager->NumDrivers();i++) {
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
									  pDriverManager->GetDriverInfo(i)->GetFileName());
			}
			::SetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,pThis->m_szDefaultDriverName);

			// MPEG-2 decoder
			CDirectShowFilterFinder FilterFinder;
			int Count=0;
			if (FilterFinder.FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_MPEG2_VIDEO)) {
				for (int i=0;i<FilterFinder.GetFilterCount();i++) {
					WCHAR szFilterName[MAX_MPEG2_DECODER_NAME];

					if (FilterFinder.GetFilterInfo(i,NULL,szFilterName,lengthof(szFilterName))) {
						DlgComboBox_AddString(hDlg,IDC_OPTIONS_DECODER,szFilterName);
						Count++;
					}
				}
			}
			int Sel;
			if (pThis->m_szMpeg2DecoderName[0]=='\0') {
				Sel=0;
			} else {
				Sel=DlgComboBox_FindStringExact(hDlg,IDC_OPTIONS_DECODER,-1,
												pThis->m_szMpeg2DecoderName)+1;
			}
			DlgComboBox_InsertString(hDlg,IDC_OPTIONS_DECODER,0,
				Count>0?TEXT("デフォルト"):TEXT("<デコーダが見付かりません>"));
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_DECODER,Sel);

			// Card reader
			LPCTSTR pszRenderer;
			DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,TEXT("デフォルト"));
			for (int i=1;(pszRenderer=CVideoRenderer::EnumRendererName(i))!=NULL;i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_RENDERER,pszRenderer);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_RENDERER,pThis->m_VideoRendererType);

			static const LPCTSTR pszCardReaderList[] = {
				TEXT("なし (スクランブル解除しない)"),
				TEXT("スマートカードリーダ"),
				TEXT("HDUS内蔵カードリーダ")
			};
			for (int i=0;i<lengthof(pszCardReaderList);i++)
				DlgComboBox_AddString(hDlg,IDC_OPTIONS_CARDREADER,pszCardReaderList[i]);
			DlgComboBox_SetCurSel(hDlg,IDC_OPTIONS_CARDREADER,
				pThis->m_fTemporaryNoDescramble?0:pThis->m_CardReaderType);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_RESIDENT,pThis->m_fResident);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_KEEPSINGLETASK,pThis->m_fKeepSingleTask);
			if (CTsDescrambler::IsSSE2Available())
				DlgCheckBox_Check(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2,pThis->m_fDescrambleUseSSE2);
			else
				EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2,
									IDC_OPTIONS_DESCRAMBLEBENCHMARK,false);
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY,pThis->m_fDescrambleCurServiceOnly);
			if (pThis->m_CardReaderType==CCardReader::READER_NONE)
				EnableDlgItems(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2,
									IDC_OPTIONS_ENABLEEMMPROCESS,false);

			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS,pThis->m_fEnableEmmProcess);

			// Buffering
			DlgCheckBox_Check(hDlg,IDC_OPTIONS_ENABLEBUFFERING,pThis->m_fPacketBuffering);
			EnableDlgItems(hDlg,IDC_OPTIONS_BUFFERING_FIRST,IDC_OPTIONS_BUFFERING_LAST,
					pThis->m_fPacketBuffering);
			SetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERSIZE,pThis->m_PacketBufferLength,FALSE);
			DlgUpDown_SetRange(hDlg,IDC_OPTIONS_BUFFERSIZE_UD,1,INT_MAX);
			SetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE,
						  pThis->m_PacketBufferPoolPercentage,TRUE);
			DlgUpDown_SetRange(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE_UD,0,100);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
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
			if (::MessageBox(hDlg,TEXT("ベンチマークテストを開始します。\n終了するまで操作は行わないようにしてください。\n結果はばらつきがありますので、数回実行してください。"),
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
			Again:
				StartTime=::timeGetTime();
				for (int i=0;i<BENCHMARK_COUNT;i++) {
					if (i%10000==0) {
						FillRandomData(ScrambleKey,sizeof(ScrambleKey));
						Multi2Decoder.SetScrambleKey(ScrambleKey);
					}
					Multi2Decoder.Decode(Data,sizeof(Data),2);
				}
				NormalTime+=DiffTime(StartTime,::timeGetTime());
				StartTime=::timeGetTime();
				for (int i=0;i<BENCHMARK_COUNT;i++) {
					if (i%10000==0) {
						FillRandomData(ScrambleKey,sizeof(ScrambleKey));
						Multi2Decoder.SetScrambleKey(ScrambleKey);
					}
					Multi2Decoder.DecodeSSE2(Data,sizeof(Data),2);
				}
				SSE2Time+=DiffTime(StartTime,::timeGetTime());
				BenchmarkCount+=BENCHMARK_COUNT;
				if (NormalTime<1000 && SSE2Time<1000)
					goto Again;

				::SetCursor(hcurOld);
				int Percentage=(int)(NormalTime*100/SSE2Time)-100;
				TCHAR szText[256];
				::wsprintf(szText,TEXT("%lu 回の実行に掛かった時間\nSSE2不使用 : %lu ms\nSSE2使用 : %lu ms\nSSE2で高速化される割合 : %d %%\n\n%s"),
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

		case IDC_OPTIONS_ENABLEBUFFERING:
			EnableDlgItemsSyncCheckBox(hDlg,IDC_OPTIONS_BUFFERING_FIRST,
									   IDC_OPTIONS_BUFFERING_LAST,
									   IDC_OPTIONS_ENABLEBUFFERING);
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CGeneralOptions *pThis=GetThis(hDlg);

				pThis->m_DefaultDriverType=(DefaultDriverType)
					(GetCheckedRadioButton(hDlg,IDC_OPTIONS_DEFAULTDRIVER_NONE,
										   IDC_OPTIONS_DEFAULTDRIVER_CUSTOM)-
					IDC_OPTIONS_DEFAULTDRIVER_NONE);
				::GetDlgItemText(hDlg,IDC_OPTIONS_DEFAULTDRIVER,
								 pThis->m_szDefaultDriverName,lengthof(pThis->m_szDefaultDriverName));

				TCHAR szDecoder[MAX_MPEG2_DECODER_NAME];
				int Sel=DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_DECODER);
				if (Sel>0)
					DlgComboBox_GetLBString(hDlg,IDC_OPTIONS_DECODER,Sel,szDecoder);
				else
					szDecoder[0]='\0';
				if (::lstrcmpi(szDecoder,pThis->m_szMpeg2DecoderName)!=0) {
					::lstrcpy(pThis->m_szMpeg2DecoderName,szDecoder);
					pThis->SetUpdateFlag(UPDATE_DECODER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				CVideoRenderer::RendererType Renderer=(CVideoRenderer::RendererType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_RENDERER);
				if (Renderer!=pThis->m_VideoRendererType) {
					pThis->m_VideoRendererType=Renderer;
					pThis->SetUpdateFlag(UPDATE_RENDERER);
					SetGeneralUpdateFlag(UPDATE_GENERAL_BUILDMEDIAVIEWER);
				}

				CCardReader::ReaderType CardReader=(CCardReader::ReaderType)
					DlgComboBox_GetCurSel(hDlg,IDC_OPTIONS_CARDREADER);
				if ((pThis->m_fTemporaryNoDescramble && CardReader!=CCardReader::READER_NONE)
						|| (!pThis->m_fTemporaryNoDescramble && CardReader!=pThis->m_CardReaderType)) {
					pThis->m_CardReaderType=CardReader;
					pThis->m_fTemporaryNoDescramble=false;
					pThis->SetUpdateFlag(UPDATE_CARDREADER);
				}

				bool fResident=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_RESIDENT);
				if (fResident!=pThis->m_fResident) {
					pThis->m_fResident=fResident;
					pThis->SetUpdateFlag(UPDATE_RESIDENT);
				}

				pThis->m_fKeepSingleTask=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_KEEPSINGLETASK);

				pThis->m_fDescrambleUseSSE2=
					DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DESCRAMBLEUSESSE2);

				bool fCurOnly=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_DESCRAMBLECURSERVICEONLY);
				if (fCurOnly!=pThis->m_fDescrambleCurServiceOnly) {
					pThis->m_fDescrambleCurServiceOnly=fCurOnly;
					pThis->SetUpdateFlag(UPDATE_DESCRAMBLECURONLY);
				}

				bool fEmm=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEEMMPROCESS);
				if (fEmm!=pThis->m_fEnableEmmProcess) {
					pThis->m_fEnableEmmProcess=fEmm;
					pThis->SetUpdateFlag(UPDATE_ENABLEEMMPROCESS);
				}

				bool fBuffering=DlgCheckBox_IsChecked(hDlg,IDC_OPTIONS_ENABLEBUFFERING);
				DWORD BufferLength=::GetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERSIZE,NULL,FALSE);
				int PoolPercentage=::GetDlgItemInt(hDlg,IDC_OPTIONS_BUFFERPOOLPERCENTAGE,NULL,TRUE);
				if (fBuffering!=pThis->m_fPacketBuffering
					|| (fBuffering
						&& (BufferLength!=pThis->m_PacketBufferLength
							|| PoolPercentage!=pThis->m_PacketBufferPoolPercentage)))
					pThis->SetUpdateFlag(UPDATE_PACKETBUFFERING);
				pThis->m_fPacketBuffering=fBuffering;
				pThis->m_PacketBufferLength=BufferLength;
				pThis->m_PacketBufferPoolPercentage=PoolPercentage;
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CGeneralOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}


CGeneralOptions *CGeneralOptions::GetThis(HWND hDlg)
{
	return static_cast<CGeneralOptions*>(GetOptions(hDlg));
}
