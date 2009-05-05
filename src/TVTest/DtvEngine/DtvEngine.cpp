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

#define SID_INVALID 0xFFFF


//////////////////////////////////////////////////////////////////////
// CDtvEngine 構築/消滅
//////////////////////////////////////////////////////////////////////

CDtvEngine::CDtvEngine(void)
	: m_pEventHandler(NULL)
	, m_wCurTransportStream(0U)
	, m_wCurService(0xFFFF)
	, m_CurServiceID(SID_INVALID)
	, m_SpecServiceID(SID_INVALID)
	, m_CurAudioStream(0)
	, m_u64CurPcrTimeStamp(0UL)
	, m_BonSrcDecoder(this)
	, m_TsPacketParser(this)
	, m_TsAnalyzer(this)
	, m_TsDescrambler(this)
	, m_ProgManager(this)
	, m_MediaViewer(this)
	, m_MediaTee(this)
	, m_FileWriter(this)
	, m_FileReader(this)
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
	CTsPacketParser
		↓
	CTsAnalyzer
		↓
	CTsDescrambler
		↓
	CProgManager	// 削除予定(CTsAnalyzerに置き換え)
		↓
	CMediaTee─────┐
		↓             ↓
	CMediaBuffer  CMediaGrabber
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
	if (m_bIsFileMode)
		m_FileReader.ResetGraph();
	else
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


#if 0
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
	if (m_bIsFileMode) {
		m_FileReader.StopReadAnsync();
		m_FileReader.CloseFile();
		m_FileReader.SetOutputDecoder(NULL);
	} else {
		if (m_BonSrcDecoder.IsOpen()) {
			m_BonSrcDecoder.CloseTuner();
			m_BonSrcDecoder.SetOutputDecoder(NULL);
		}
	}
	return true;
}


const bool CDtvEngine::IsSrcFilterOpen() const
{
	if (m_bIsFileMode)
		return m_FileReader.IsOpen();
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
	if (StreamIndex == m_CurAudioStream)
		return true;
	if (StreamIndex < 0 || StreamIndex >= GetAudioStreamNum(m_wCurService))
		return false;

	WORD wAudioPID;

	if (!m_ProgManager.GetAudioEsPID(&wAudioPID, StreamIndex, m_wCurService))
		return false;

	if (!m_MediaViewer.SetAudioPID(wAudioPID))
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


const WORD CDtvEngine::GetEventID()
{
	return m_ProgManager.GetEventID(m_wCurService);
}


const int CDtvEngine::GetEventName(LPTSTR pszName, int MaxLength, bool fNext)
{
	return m_ProgManager.GetEventName(m_wCurService, pszName, MaxLength, fNext);
}


const int CDtvEngine::GetEventText(LPTSTR pszText, int MaxLength, bool fNext)
{
	return m_ProgManager.GetEventText(m_wCurService, pszText, MaxLength, fNext);
}


const bool CDtvEngine::GetEventTime(SYSTEMTIME *pStartTime, SYSTEMTIME *pEndTime, bool bNext)
{
	SYSTEMTIME stStart;

	if (!m_ProgManager.GetStartTime(m_wCurService, &stStart, bNext))
		return false;
	if (pStartTime)
		*pStartTime = stStart;
	if (pEndTime) {
		DWORD Duration = m_ProgManager.GetDuration(m_wCurService, bNext);
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


const bool CDtvEngine::SetChannel(const BYTE byTuningSpace, const WORD wChannel)
{
	// チャンネル変更
	m_SpecServiceID = SID_INVALID;
	if (!m_BonSrcDecoder.SetChannel((DWORD)byTuningSpace, (DWORD)wChannel)) {
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

		if (wService == 0xFFFF) {
			m_wCurService = 0;
		} else {
			m_wCurService = wService;
		}
		// 先頭PMTが到着するまで失敗にする
		if (!m_ProgManager.GetServiceID(&wServiceID, wService))
			return false;

		m_CurServiceID = wServiceID;

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


const bool CDtvEngine::SetServiceByID(const WORD ServiceID)
{
	WORD Index;

	m_SpecServiceID = ServiceID;
	Index = m_ProgManager.GetServiceIndexByID(ServiceID);
	if (Index != 0xFFFF)
		SetService(Index);
	return true;
}


const unsigned __int64 CDtvEngine::GetPcrTimeStamp() const
{
	// PCRタイムスタンプ取得
	return m_u64CurPcrTimeStamp;
}


const DWORD CDtvEngine::OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam)
{
	// デコーダからのイベントを受け取る(暫定)
	if (pDecoder == &m_ProgManager) {
		// プログラムマネージャからのイベント
		WORD wTransportStream = m_ProgManager.GetTransportStreamID();
		switch (dwEventID) {
		case CProgManager::EID_SERVICE_LIST_UPDATED :
			// サービスの構成が変化した
			m_CurAudioStream = 0;
			if (m_wCurTransportStream != wTransportStream) {
				// ストリームIDが変わっているなら初期化
				TRACE(TEXT("■Stream Change!! %04X\n"), wTransportStream);
				// この時点でまだサービスが全部きてないこともあるので、いったん保留
				WORD Service;
				if (m_SpecServiceID != SID_INVALID)
					Service = m_ProgManager.GetServiceIndexByID(m_SpecServiceID);
				else
					Service = 0xFFFF;
				m_CurServiceID = SID_INVALID;
				if (SetService(Service))
					m_wCurTransportStream = wTransportStream;
			} else {
				// ストリームIDは同じだが、構成ESのPIDが変わった可能性がある
				WORD Service;
				if (m_SpecServiceID != SID_INVALID
						|| m_CurServiceID != SID_INVALID) {
					Service = m_ProgManager.GetServiceIndexByID(
						m_SpecServiceID != SID_INVALID ? m_SpecServiceID : m_CurServiceID);
				} else {
					Service = 0xFFFF;
				}
				SetService(Service);
			}
			if (m_pEventHandler)
				m_pEventHandler->OnServiceListUpdated(&m_ProgManager);
			return 0UL;

		case CProgManager::EID_SERVICE_INFO_UPDATED :
			// サービス名が更新された
			if (m_pEventHandler)
				m_pEventHandler->OnServiceInfoUpdated(&m_ProgManager);
			return 0UL;

		case CProgManager::EID_PCR_TIMESTAMP_UPDATED :
			// タイムスタンプが更新された
			if (m_wCurService!=0xFFFF) {
				m_ProgManager.GetPcrTimeStamp(&m_u64CurPcrTimeStamp,m_wCurService);
			}
			// Unused
			/*
			if (m_pEventHandler)
				m_pEventHandler->OnPcrTimeStampUpdated(&m_ProgManager);
			*/
			return 0UL;
		}
	} else if(pDecoder == &m_FileReader) {
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
	} else if (pDecoder == &m_FileWriter) {
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


bool CDtvEngine::OpenBcasCard(CCardReader::ReaderType CardReaderType)
{
	// B-CASカードを開く
	if (CardReaderType!=CCardReader::READER_NONE) {
		Trace(TEXT("B-CASカードを開いています..."));
		if (!m_TsDescrambler.OpenBcasCard(CardReaderType)) {
			SetError(0,TEXT("B-CASカードの初期化に失敗しました。"),
					 TEXT("カードリーダが接続されているか、設定で有効なカードリーダが選択されているか確認してください。"));
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
	m_wCurService = 0xFFFF;
	m_CurServiceID = SID_INVALID;
}
