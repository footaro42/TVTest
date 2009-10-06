// DtvEngine.cpp: CDtvEngine クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DtvEngine.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CDtvEngine 構築/消滅
//////////////////////////////////////////////////////////////////////

CDtvEngine::CDtvEngine(void)
	: m_pEventHandler(NULL)
	, m_wCurTransportStream(0U)
	, m_wCurService(SERVICE_INVALID)
	, m_CurServiceID(SID_INVALID)
	, m_SpecServiceID(SID_INVALID)
	, m_CurAudioStream(0)
	, m_BonSrcDecoder(this)
	, m_TsPacketParser(this)
	, m_TsAnalyzer(this)
	, m_TsDescrambler(this)
	, m_ProgManager(this)
	, m_MediaViewer(this)
	, m_MediaTee(this)
	, m_FileWriter(this)
	//, m_FileReader(this)
	, m_MediaBuffer(this)
	, m_MediaGrabber(this)
	, m_TsSelector(this)
	, m_bBuiled(false)
	, m_bIsFileMode(false)
	, m_bDescramble(true)
	, m_bBuffering(false)
	, m_bDescrambleCurServiceOnly(false)
	, m_bWriteCurServiceOnly(false)
	, m_WriteStream(CTsSelector::STREAM_ALL)
{
}


CDtvEngine::~CDtvEngine(void)
{
	CloseEngine();
}


const bool CDtvEngine::BuildEngine(CEventHandler *pEventHandler,
								   bool bDescramble, bool bBuffering)
{
	// 完全に暫定
	if (m_bBuiled)
		return true;

	/*
	グラフ構成図

	CBonSrcDecoder
		↓
	CTsPacketParser → EpgDataCap2.dll
		↓
	CTsAnalyzer
		↓
	CTsDescrambler
		↓
	CProgManager	// 削除予定(CTsAnalyzerに置き換え)
		↓
	CMediaTee─────┐
		↓             ↓
	CMediaBuffer  CMediaGrabber → プラグイン
		↓             ↓
	CMediaViewer  CTsSelector
		               ↓
		          CFileWriter
	*/

	Trace(TEXT("デコーダグラフを構築しています..."));

	// デコーダグラフ構築
	m_TsPacketParser.SetOutputDecoder(&m_TsAnalyzer);
	m_TsAnalyzer.SetOutputDecoder(&m_TsDescrambler);
	m_TsDescrambler.SetOutputDecoder(&m_ProgManager);
	m_TsDescrambler.EnableDescramble(bDescramble);
	m_bDescramble = bDescramble;
	m_ProgManager.SetOutputDecoder(&m_MediaTee);
	if (bBuffering) {
		m_MediaTee.SetOutputDecoder(&m_MediaBuffer, 0);
		m_MediaBuffer.SetOutputDecoder(&m_MediaViewer);
		m_MediaBuffer.Play();
	} else {
		m_MediaTee.SetOutputDecoder(&m_MediaViewer, 0);
	}
	m_bBuffering=bBuffering;
	m_MediaTee.SetOutputDecoder(&m_MediaGrabber, 1UL);
	m_MediaGrabber.SetOutputDecoder(&m_TsSelector);
	m_TsSelector.SetOutputDecoder(&m_FileWriter);

	// イベントハンドラ設定
	m_pEventHandler = pEventHandler;
	m_pEventHandler->m_pDtvEngine = this;

	m_bBuiled=true;

	return true;
}


const bool CDtvEngine::IsBuildComplete() const
{
	return m_bBuiled && IsSrcFilterOpen() && m_MediaViewer.IsOpen()
		&& (!m_bDescramble || m_TsDescrambler.IsBcasCardOpen());
}


const bool CDtvEngine::CloseEngine(void)
{
	//if (!m_bBuiled)
	//	return true;

	Trace(TEXT("DtvEngineを閉じています..."));

	//m_MediaViewer.Stop();

	ReleaseSrcFilter();

	Trace(TEXT("バッファのストリーミングを停止しています..."));
	m_MediaBuffer.Stop();

	Trace(TEXT("カードリーダを閉じています..."));
	m_TsDescrambler.CloseBcasCard();

	Trace(TEXT("メディアビューアを閉じています..."));
	m_MediaViewer.CloseViewer();

	// イベントハンドラ解除
	m_pEventHandler = NULL;

	m_bBuiled=false;

	Trace(TEXT("DtvEngineを閉じました。"));

	return true;
}


const bool CDtvEngine::ResetEngine(void)
{
	if (!m_bBuiled)
		return false;

	// デコーダグラフリセット
	ResetStatus();
	/*if (m_bIsFileMode)
		m_FileReader.ResetGraph();
	else*/
		m_BonSrcDecoder.ResetGraph();

	return true;
}


const bool CDtvEngine::OpenSrcFilter_BonDriver(HMODULE hBonDriverDll)
{
	ReleaseSrcFilter();
	// ソースフィルタを開く
	Trace(TEXT("チューナを開いています..."));
	if (!m_BonSrcDecoder.OpenTuner(hBonDriverDll)) {
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}
	m_MediaBuffer.SetFileMode(false);
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);
	Trace(TEXT("ストリームの再生を開始しています..."));
	if (!m_BonSrcDecoder.Play()) {
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}
	//ResetEngine();
	ResetStatus();

	m_bIsFileMode = false;
	return true;
}


#if 0
const bool CDtvEngine::OpenSrcFilter_File(LPCTSTR lpszFileName)
{
	ReleaseSrcFilter();
	// ファイルを開く
	if (!m_FileReader.OpenFile(lpszFileName)) {
		return false;
	}
	m_MediaBuffer.SetFileMode(true);
	m_FileReader.SetOutputDecoder(&m_TsPacketParser);
	if (!m_FileReader.StartReadAnsync()) {
		return false;
	}
	ResetEngine();

	m_bIsFileMode = true;
	return true;
}


const bool CDtvEngine::PlayFile(LPCTSTR lpszFileName)
{
	// ※グラフ全体の排他制御を実装しないとまともに動かない！！

	// 再生中の場合は閉じる
	m_FileReader.StopReadAnsync();

	// スレッド終了を待つ
	while(m_FileReader.IsAnsyncReadBusy()){
		::Sleep(1UL);
		}

	m_FileReader.CloseFile();

	// デバイスから再生中の場合はグラフを再構築する
	if(!m_bIsFileMode){
		m_BonSrcDecoder.Stop();
		m_BonSrcDecoder.SetOutputDecoder(NULL);
		m_FileReader.SetOutputDecoder(&m_TsPacketParser);
		}

	try{
		// グラフリセット
		m_FileReader.Reset();

		// ファイルオープン
		if(!m_FileReader.OpenFile(lpszFileName))throw 0UL;

		// 非同期再生開始
		if(!m_FileReader.StartReadAnsync())throw 1UL;
		}
	catch(const DWORD dwErrorStep){
		// エラー発生
		StopFile();
		return false;
		}

	m_bIsFileMode = true;

	return true;
}

void CDtvEngine::StopFile(void)
{
	// ※グラフ全体の排他制御を実装しないとまともに動かない！！

	m_bIsFileMode = false;

	// 再生中の場合は閉じる
	m_FileReader.StopReadAnsync();

	// スレッド終了を待つ
	while(m_FileReader.IsAnsyncReadBusy()){
		::Sleep(1UL);
		}

	m_FileReader.CloseFile();

	// グラフを再構築する
	m_FileReader.SetOutputDecoder(NULL);
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);
	m_BonSrcDecoder.Play();
}
#endif


const bool CDtvEngine::ReleaseSrcFilter()
{
	// ソースフィルタを開放する
	/*if (m_bIsFileMode) {
		m_FileReader.StopReadAnsync();
		m_FileReader.CloseFile();
		m_FileReader.SetOutputDecoder(NULL);
	} else */{
		if (m_BonSrcDecoder.IsOpen()) {
			m_BonSrcDecoder.CloseTuner();
			m_BonSrcDecoder.SetOutputDecoder(NULL);
		}
	}
	return true;
}


const bool CDtvEngine::IsSrcFilterOpen() const
{
	/*
	if (m_bIsFileMode)
		return m_FileReader.IsOpen();
	*/
	return m_BonSrcDecoder.IsOpen();
}


const bool CDtvEngine::EnablePreview(const bool bEnable)
{
	if (!m_MediaViewer.IsOpen())
		return false;

	bool bOK;

	if (bEnable) {
		// プレビュー有効
		bOK = m_MediaViewer.Play();
	} else {
		// プレビュー無効
		bOK = m_MediaViewer.Stop();
	}

	return bOK;
}


const bool CDtvEngine::SetViewSize(const int x,const int y)
{
	// ウィンドウサイズを設定する
	return m_MediaViewer.SetViewSize(x,y);
}


const bool CDtvEngine::SetVolume(const float fVolume)
{
	// オーディオボリュームを設定する( -100.0(無音) < fVolume < 0(最大) )
	return m_MediaViewer.SetVolume(fVolume);
}


const bool CDtvEngine::GetVideoSize(WORD *pwWidth,WORD *pwHeight)
{
	return m_MediaViewer.GetVideoSize(pwWidth,pwHeight);
}


const bool CDtvEngine::GetVideoAspectRatio(BYTE *pbyAspectRateX,BYTE *pbyAspectRateY)
{
	return m_MediaViewer.GetVideoAspectRatio(pbyAspectRateX,pbyAspectRateY);
}


const BYTE CDtvEngine::GetAudioChannelNum()
{
	return m_MediaViewer.GetAudioChannelNum();
}


const int CDtvEngine::GetAudioStreamNum(const WORD wService)
{
	return m_ProgManager.GetAudioEsNum(wService);
}


const bool CDtvEngine::SetAudioStream(int StreamIndex)
{
	if (StreamIndex < 0 || StreamIndex >= GetAudioStreamNum(m_wCurService))
		return false;

	WORD AudioPID;

	if (!m_ProgManager.GetAudioEsPID(&AudioPID, StreamIndex, m_wCurService))
		return false;

	if (!m_MediaViewer.SetAudioPID(AudioPID))
		return false;

	m_CurAudioStream = StreamIndex;

	return true;
}


const int CDtvEngine::GetAudioStream() const
{
	return m_CurAudioStream;
}


const BYTE CDtvEngine::GetAudioComponentType()
{
	return m_ProgManager.GetAudioComponentType(m_CurAudioStream, m_wCurService);
}


const bool CDtvEngine::SetStereoMode(int iMode)
{
	return m_MediaViewer.SetStereoMode(iMode);
}


const WORD CDtvEngine::GetEventID(bool bNext)
{
	return m_ProgManager.GetEventID(m_wCurService, bNext);
}


const int CDtvEngine::GetEventName(LPTSTR pszName, int MaxLength, bool bNext)
{
	return m_ProgManager.GetEventName(m_wCurService, pszName, MaxLength, bNext);
}


const int CDtvEngine::GetEventText(LPTSTR pszText, int MaxLength, bool bNext)
{
	return m_ProgManager.GetEventText(m_wCurService, pszText, MaxLength, bNext);
}


const bool CDtvEngine::GetEventTime(SYSTEMTIME *pStartTime, SYSTEMTIME *pEndTime, bool bNext)
{
	SYSTEMTIME stStart;

	if (!m_ProgManager.GetEventStartTime(m_wCurService, &stStart, bNext))
		return false;
	if (pStartTime)
		*pStartTime = stStart;
	if (pEndTime) {
		DWORD Duration = m_ProgManager.GetEventDuration(m_wCurService, bNext);
		if (Duration == 0)
			return false;

		FILETIME ft;
		ULARGE_INTEGER Time;

		SystemTimeToFileTime(&stStart, &ft);
		Time.LowPart=ft.dwLowDateTime;
		Time.HighPart=ft.dwHighDateTime;
		Time.QuadPart+=(ULONGLONG)Duration*10000000ULL;
		ft.dwLowDateTime=Time.LowPart;
		ft.dwHighDateTime=Time.HighPart;
		FileTimeToSystemTime(&ft, pEndTime);
	}
	return true;
}


const bool CDtvEngine::GetVideoDecoderName(LPWSTR lpName,int iBufLen)
{
	return m_MediaViewer.GetVideoDecoderName(lpName, iBufLen);
}


const bool CDtvEngine::DisplayVideoDecoderProperty(HWND hWndParent)
{
	return m_MediaViewer.DisplayVideoDecoderProperty(hWndParent);
}


const bool CDtvEngine::SetChannel(const BYTE byTuningSpace, const WORD wChannel, const WORD ServiceID)
{
	TRACE(TEXT("CDtvEngine::SetChannel(%d, %d, %04x)\n"),
		  byTuningSpace, wChannel, ServiceID);

	CBlockLock Lock(&m_EngineLock);

	// サービスはPATが来るまで保留
	m_SpecServiceID = ServiceID;

	// チャンネル変更
	if (!m_BonSrcDecoder.SetChannel((DWORD)byTuningSpace, (DWORD)wChannel)) {
		m_SpecServiceID = SID_INVALID;
		SetError(m_BonSrcDecoder.GetLastErrorException());
		return false;
	}

	ResetStatus();

	return true;
}


const bool CDtvEngine::SetService(const WORD wService)
{
	CBlockLock Lock(&m_EngineLock);

	// サービス変更(wService==0xFFFFならPAT先頭サービス)

	if (wService == 0xFFFF || wService < m_ProgManager.GetServiceNum()) {
		WORD wServiceID;

		// 先頭PMTが到着するまで失敗にする
		if (!m_ProgManager.GetServiceID(&wServiceID, wService))
			return false;
		if (wService == 0xFFFF) {
			m_wCurService = 0;
		} else {
			m_wCurService = wService;
		}

		m_CurServiceID = wServiceID;
		m_SpecServiceID = wServiceID;

		WORD wVideoPID = CMediaViewer::PID_INVALID;
		WORD wAudioPID = CMediaViewer::PID_INVALID;

		m_ProgManager.GetVideoEsPID(&wVideoPID, m_wCurService);
		if (!m_ProgManager.GetAudioEsPID(&wAudioPID, m_CurAudioStream, m_wCurService)
				&& m_CurAudioStream != 0) {
			m_ProgManager.GetAudioEsPID(&wAudioPID, 0, m_wCurService);
			m_CurAudioStream = 0;
		}

		TRACE(TEXT("------- Service Select -------\n"));
		TRACE(TEXT("%d (ServiceID = %04X)\n"), m_wCurService, wServiceID);

#ifdef TVH264
		const BYTE VideoComponentTag = m_ProgManager.GetVideoComponentTag(m_wCurService);
		const bool b1Seg = VideoComponentTag >= 0x81 && VideoComponentTag <= 0x8F;
		m_MediaViewer.SetAdjustSampleTime(b1Seg);
		m_MediaViewer.EnableTBSFilter(b1Seg);
#endif

		m_MediaViewer.SetVideoPID(wVideoPID);
		m_MediaViewer.SetAudioPID(wAudioPID);

		if (m_bDescrambleCurServiceOnly)
			SetDescrambleService(wServiceID);

		if (m_bWriteCurServiceOnly)
			SetWriteService(wServiceID);

		return true;
	}

	return false;
}


const WORD CDtvEngine::GetService(void) const
{
	// サービス取得
	return m_wCurService;
}


const bool CDtvEngine::GetServiceID(WORD *pServiceID)
{
	// サービスID取得
	return m_ProgManager.GetServiceID(pServiceID, m_wCurService);
}


const bool CDtvEngine::SetServiceByID(const WORD ServiceID, const bool bReserve)
{
	CBlockLock Lock(&m_EngineLock);

	// bReserve == true の場合、まだPATが来ていなくてもエラーにしない

	WORD Index;

	Index = m_ProgManager.GetServiceIndexByID(ServiceID);
	if (Index == 0xFFFF) {
		if (!bReserve || m_wCurTransportStream != 0)
			return false;
	} else if (Index != 0xFFFF) {
		SetService(Index);
	}
	m_SpecServiceID = ServiceID;

	return true;
}


const unsigned __int64 CDtvEngine::GetPcrTimeStamp()
{
	// PCRタイムスタンプ取得
	unsigned __int64 TimeStamp;
	if (m_ProgManager.GetPcrTimeStamp(&TimeStamp, m_wCurService))
		return TimeStamp;
	return 0ULL;

}


const DWORD CDtvEngine::OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam)
{
	// デコーダからのイベントを受け取る(暫定)
	if (pDecoder == &m_ProgManager) {
		// プログラムマネージャからのイベント
		switch (dwEventID) {
		case CProgManager::EID_SERVICE_LIST_UPDATED :
			// サービスの構成が変化した
			{
				CBlockLock Lock(&m_EngineLock);
				WORD wTransportStream = m_ProgManager.GetTransportStreamID();
				bool bStreamChanged = m_wCurTransportStream != wTransportStream;

				if (bStreamChanged) {
					// ストリームIDが変わっているなら初期化
					TRACE(TEXT("CDtvEngine ■Stream Change!! %04X\n"), wTransportStream);

					bool bSetService = true;

					m_wCurService = SERVICE_INVALID;
					m_CurServiceID = SID_INVALID;
					m_CurAudioStream = 0;
					m_wCurTransportStream = wTransportStream;

					WORD Service = 0xFFFF;
					if (m_SpecServiceID != SID_INVALID) {
						// サービスが指定されている
						if (m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID) < 0) {
							// サービスがPATにない
							TRACE(TEXT("Service not found %d\n"), m_SpecServiceID);
							m_SpecServiceID = SID_INVALID;
						} else {
							Service = m_ProgManager.GetServiceIndexByID(m_SpecServiceID);
							if (Service == 0xFFFF) {
								// サービスはPATにあるが、まだPMTが来ていない
								bSetService = false;
							}
						}
					}
					if (!bSetService || !SetService(Service)) {
						m_MediaViewer.SetVideoPID(CMediaViewer::PID_INVALID);
						m_MediaViewer.SetAudioPID(CMediaViewer::PID_INVALID);
					}
				} else {
					// ストリームIDは同じだが、構成ESのPIDが変わった可能性がある
					TRACE(TEXT("CDtvEngine ■Stream Updated!! %04X\n"), wTransportStream);

					WORD Service = 0xFFFF;
					if (m_SpecServiceID != SID_INVALID) {
						if (m_TsAnalyzer.GetServiceIndexByID(m_SpecServiceID) < 0) {
							// サービスがPATにない
							TRACE(TEXT("Service not found %d\n"), m_SpecServiceID);
							m_SpecServiceID = SID_INVALID;
						} else {
							Service = m_ProgManager.GetServiceIndexByID(m_SpecServiceID);
							if (Service == 0xFFFF) {
								// サービスはPATにあるが、まだPMTが来ていない
								goto ServiceListUpdatedHandler;
							}
						}
					}
					if (Service == 0xFFFF && m_CurServiceID != SID_INVALID) {
						if (m_TsAnalyzer.GetServiceIndexByID(m_CurServiceID) < 0) {
							m_CurServiceID = SID_INVALID;
						} else {
							Service = m_ProgManager.GetServiceIndexByID(m_CurServiceID);
							if (Service == 0xFFFF)
								goto ServiceListUpdatedHandler;
						}
					}
					SetService(Service);
				}
			ServiceListUpdatedHandler:
				if (m_pEventHandler)
					m_pEventHandler->OnServiceListUpdated(&m_ProgManager, bStreamChanged);
			}
			return 0UL;

		case CProgManager::EID_SERVICE_INFO_UPDATED :
			// サービスの情報が更新された
#ifdef TVH264
			if (m_wCurService != SERVICE_INVALID) {
				const BYTE VideoComponentTag = m_ProgManager.GetVideoComponentTag(m_wCurService);
				const bool b1Seg = VideoComponentTag >= 0x81 && VideoComponentTag <= 0x8F;
				m_MediaViewer.SetAdjustSampleTime(b1Seg);
				m_MediaViewer.EnableTBSFilter(b1Seg);
			}
#endif
			if (m_pEventHandler)
				m_pEventHandler->OnServiceInfoUpdated(&m_ProgManager);
			return 0UL;

		case CProgManager::EID_PCR_TIMESTAMP_UPDATED :
			// タイムスタンプが更新された
			// Unused
			/*
			if (m_pEventHandler)
				m_pEventHandler->OnPcrTimeStampUpdated(&m_ProgManager);
			*/
			return 0UL;
		}
	}/* else if(pDecoder == &m_FileReader) {
		CFileReader *pFileReader = dynamic_cast<CFileReader *>(pDecoder);

		// ファイルリーダからのイベント
		switch(dwEventID){
		case CFileReader::EID_READ_ASYNC_START:
			// 非同期リード開始
			return 0UL;

		case CFileReader::EID_READ_ASYNC_END:
			// 非同期リード終了
			return 0UL;

		case CFileReader::EID_READ_ASYNC_POSTREAD:
			// 非同期リード後
			if (pFileReader->GetReadPos() >= pFileReader->GetFileSize()) {
				// 最初に巻き戻す(ループ再生)
				pFileReader->SetReadPos(0ULL);
				//pFileReader->Reset();
				ResetEngine();
			}
			return 0UL;
		}
	}*/ else if (pDecoder == &m_FileWriter) {
		switch (dwEventID) {
		case CFileWriter::EID_WRITE_ERROR:
			// 書き込みエラーが発生した
			if (m_pEventHandler)
				m_pEventHandler->OnFileWriteError(&m_FileWriter);
			return 0UL;
		}
	} else if (pDecoder == &m_MediaViewer) {
		switch (dwEventID) {
		case CMediaViewer::EID_VIDEO_SIZE_CHANGED:
			if (m_pEventHandler)
				m_pEventHandler->OnVideoSizeChanged(&m_MediaViewer);
			return 0UL;

		case CMediaViewer::EID_FILTER_GRAPH_FLUSH:
			//m_BonSrcDecoder.PurgeStream();
			return 0UL;
		}
	} else if (pDecoder == &m_TsDescrambler) {
		switch (dwEventID) {
		case CTsDescrambler::EID_EMM_PROCESSED:
			// EMM処理が行われた
			if (m_pEventHandler)
				m_pEventHandler->OnEmmProcessed(static_cast<const BYTE*>(pParam));
			return 0UL;
		}
	}

	return 0UL;
}


bool CDtvEngine::BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	if (!m_MediaViewer.OpenViewer(hwndHost,hwndMessage,VideoRenderer,
								  pszMpeg2Decoder,pszAudioDevice)) {
		SetError(m_MediaViewer.GetLastErrorException());
		return false;
	}
	return true;
}


bool CDtvEngine::RebuildMediaViewer(HWND hwndHost,HWND hwndMessage,
	CVideoRenderer::RendererType VideoRenderer,LPCWSTR pszMpeg2Decoder,LPCWSTR pszAudioDevice)
{
	bool bOK;

	EnablePreview(false);
	m_MediaBuffer.SetOutputDecoder(NULL);
	m_MediaViewer.CloseViewer();
	bOK=m_MediaViewer.OpenViewer(hwndHost,hwndMessage,VideoRenderer,
								 pszMpeg2Decoder,pszAudioDevice);
	if (!bOK) {
		SetError(m_MediaViewer.GetLastErrorException());
	}
	if (m_bBuffering)
		m_MediaBuffer.SetOutputDecoder(&m_MediaViewer);

	return bOK;
}


bool CDtvEngine::CloseMediaViewer()
{
	m_MediaViewer.CloseViewer();
	return true;
}


bool CDtvEngine::ResetMediaViewer()
{
	if (!m_MediaViewer.IsOpen())
		return false;

	m_MediaViewer.Reset();

	if (m_bBuffering)
		m_MediaBuffer.Reset();

	CBlockLock Lock(&m_EngineLock);

	WORD VideoPID = CMediaViewer::PID_INVALID;
	WORD AudioPID = CMediaViewer::PID_INVALID;
	if (m_ProgManager.GetVideoEsPID(&VideoPID, m_wCurService))
		m_MediaViewer.SetVideoPID(VideoPID);
	if (m_ProgManager.GetAudioEsPID(&AudioPID, m_CurAudioStream, m_wCurService))
		m_MediaViewer.SetAudioPID(AudioPID);

	return true;
}


bool CDtvEngine::OpenBcasCard(CCardReader::ReaderType CardReaderType)
{
	// B-CASカードを開く
	if (CardReaderType!=CCardReader::READER_NONE) {
		Trace(TEXT("B-CASカードを開いています..."));
		if (!m_TsDescrambler.OpenBcasCard(CardReaderType)) {
			TCHAR szText[256];

			if (m_TsDescrambler.GetLastErrorText()!=NULL)
				::wsprintf(szText,TEXT("B-CASカードの初期化に失敗しました。%s"),m_TsDescrambler.GetLastErrorText());
			else
				::lstrcpy(szText,TEXT("B-CASカードの初期化に失敗しました。"));
			SetError(0,szText,
					 TEXT("カードリーダが接続されているか、設定で有効なカードリーダが選択されているか確認してください。"),
					 m_TsDescrambler.GetLastErrorSystemMessage());
			return false;
		}
	} else if (m_TsDescrambler.IsBcasCardOpen()) {
		m_TsDescrambler.CloseBcasCard();
	}
	return true;
}


bool CDtvEngine::CloseBcasCard()
{
	if (m_TsDescrambler.IsBcasCardOpen())
		m_TsDescrambler.CloseBcasCard();
	return true;
}


bool CDtvEngine::SetDescramble(bool bDescramble)
{
	if (!m_bBuiled) {
		SetError(0,TEXT("内部エラー : DtvEngineが構築されていません。"));
		return false;
	}

	if (m_bDescramble != bDescramble) {
		m_TsDescrambler.EnableDescramble(bDescramble);
		m_bDescramble = bDescramble;
	}
	return true;
}


bool CDtvEngine::ResetBuffer()
{
	m_MediaBuffer.ResetBuffer();
	return true;
}


bool CDtvEngine::GetOriginalVideoSize(WORD *pWidth,WORD *pHeight)
{
	return m_MediaViewer.GetOriginalVideoSize(pWidth,pHeight);
}


bool CDtvEngine::SetDescrambleService(WORD ServiceID)
{
	return m_TsDescrambler.SetTargetServiceID(ServiceID);
}


bool CDtvEngine::SetDescrambleCurServiceOnly(bool bOnly)
{
	if (m_bDescrambleCurServiceOnly != bOnly) {
		WORD ServiceID = 0;

		m_bDescrambleCurServiceOnly = bOnly;
		if (bOnly)
			m_ProgManager.GetServiceID(&ServiceID, m_wCurService);
		SetDescrambleService(ServiceID);
	}
	return true;
}


bool CDtvEngine::SetWriteService(WORD ServiceID, DWORD Stream)
{
	return m_TsSelector.SetTargetServiceID(ServiceID, Stream);
}


bool CDtvEngine::SetWriteCurServiceOnly(bool bOnly, DWORD Stream)
{
	if (m_bWriteCurServiceOnly != bOnly || m_WriteStream != Stream) {
		m_bWriteCurServiceOnly = bOnly;
		m_WriteStream = Stream;
		if (bOnly) {
			WORD ServiceID = 0;

			m_ProgManager.GetServiceID(&ServiceID, m_wCurService);
			SetWriteService(ServiceID, Stream);
		} else {
			SetWriteService(0, Stream);
		}
	}
	return true;
}


CEpgDataInfo *CDtvEngine::GetEpgDataInfo(WORD ServiceID, bool bNext)
{
	return m_TsPacketParser.GetEpgDataInfo(ServiceID,bNext);
}


void CDtvEngine::SetTracer(CTracer *pTracer)
{
	CBonBaseClass::SetTracer(pTracer);
	m_MediaViewer.SetTracer(pTracer);
}


void CDtvEngine::ResetStatus()
{
	m_wCurTransportStream = 0;
	m_wCurService = SERVICE_INVALID;
	m_CurServiceID = SID_INVALID;
}
