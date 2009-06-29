// MediaViewer.cpp: CMediaViewer クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <Dvdmedia.h>
#include "MediaViewer.h"
#include "StdUtil.h"
#ifdef USE_GRABBER_FILTER
#include "../Grabber.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib,"quartz.lib")


//const CLSID CLSID_NullRenderer = {0xc1f400a4, 0x3f08, 0x11d3, {0x9f, 0x0b, 0x00, 0x60, 0x08, 0x03, 0x9e, 0x37}};
EXTERN_C const CLSID CLSID_NullRenderer;

#define LOCK_TIMEOUT 2000


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CMediaViewer::CMediaViewer(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 0UL)
	, m_bInit(false)
	, m_pMediaControl(NULL)

	, m_pFilterGraph(NULL)

	, m_pSrcFilter(NULL)
	, m_pBonSrcFilterClass(NULL)

	, m_pMpeg2SeqFilter(NULL)
	, m_pMpeg2SeqClass(NULL)

	, m_pAacDecFilter(NULL)
	, m_pAacDecClass(NULL)

	/*
	, m_pPcmSelFilter(NULL)
	, m_pPcmSelClass(NULL)
	*/

	, m_pMpeg2DecFilter(NULL)

	, m_pVideoRenderer(NULL)

	, m_pszMpeg2DecoderName(NULL)

	, m_pMp2DemuxFilter(NULL)
	, m_pMp2DemuxVideoMap(NULL)
	, m_pMp2DemuxAudioMap(NULL)

	, m_wVideoEsPID(PID_INVALID)
	, m_wAudioEsPID(PID_INVALID)

#ifdef USE_VIDEO_RATE_KEEPER
	, m_pVideoRateKeeper(NULL)
#endif

	, m_wVideoWindowX(0)
	, m_wVideoWindowY(0)
	, m_VideoInfo()
	, m_hOwnerWnd(NULL)
#ifdef _DEBUG
	, m_dwRegister(0)
#endif

	, m_VideoRendererType(CVideoRenderer::RENDERER_UNDEFINED)
	, m_ForceAspectX(0)
	, m_ForceAspectY(0)
	, m_PanAndScan(0)
	, m_ViewStretchMode(STRETCH_KEEPASPECTRATIO)
	, m_bIgnoreDisplayExtension(false)
	, m_bUseAudioRendererClock(true)
	, m_bAdjustAudioStreamTime(false)
	, m_pAudioStreamCallback(NULL)
	, m_pAudioStreamCallbackParam(NULL)
	, m_pImageMixer(NULL)
#ifdef USE_GRABBER_FILTER
	, m_bGrabber(false)
	, m_pGrabber(NULL)
#endif
{
	// COMライブラリ初期化
	//::CoInitialize(NULL);
}

CMediaViewer::~CMediaViewer()
{
	CloseViewer();

	// COMライブラリ開放
	//::CoUninitialize();
}


void CMediaViewer::Reset(void)
{
	TRACE(TEXT("CMediaViewer::Reset()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	Lock.TryLock(LOCK_TIMEOUT);

	/*
	bool fResume=false;

	if (m_pMediaControl) {
		OAFilterState fs;

		if (m_pMediaControl->GetState(1000,&fs)==S_OK && fs==State_Running) {
			//Stop();
			fResume=true;
		}
	}
	*/

	Flush();
	//Stop();

	SetVideoPID(PID_INVALID);
	SetAudioPID(PID_INVALID);

	if (m_pAacDecClass)
		m_pAacDecClass->ResetDecoder();

	/*
	if (fResume)
		Play();
	*/
}

const bool CMediaViewer::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if(dwInputIndex >= GetInputNum())return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;
	*/
	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// フィルタグラフに入力
	if (m_pBonSrcFilterClass && pTsPacket->GetPID()!=0x1FFF) {
		return m_pBonSrcFilterClass->InputMedia(pTsPacket);
	}

	return false;
}

const bool CMediaViewer::OpenViewer(HWND hOwnerHwnd, HWND hMessageDrainHwnd,
			CVideoRenderer::RendererType RendererType,
			LPCWSTR pszMpeg2Decoder, LPCWSTR pszAudioDevice)
{
	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT)) {
		SetError(TEXT("タイムアウトエラーです。"));
		return false;
	}

	if (m_bInit) {
		SetError(TEXT("既にフィルタグラフが構築されています。"));
		return false;
	}

	TRACE(TEXT("CMediaViewer::OpenViewer() フィルタグラフ作成開始\n"));

	HRESULT hr=S_OK;

	IPin *pOutput=NULL;
	IPin *pOutputVideo=NULL;
	IPin *pOutputAudio=NULL;

	try {
		// フィルタグラフマネージャを構築する
		hr=::CoCreateInstance(CLSID_FilterGraph,NULL,CLSCTX_INPROC_SERVER,
				IID_IGraphBuilder,reinterpret_cast<LPVOID*>(&m_pFilterGraph));
		if (hr != S_OK) {
			throw CBonException(hr,TEXT("フィルタグラフマネージャを作成できません。"));
		}
#ifdef _DEBUG
		AddToRot(m_pFilterGraph, &m_dwRegister);
#endif

		// IMediaControlインタフェースのクエリー
		hr=m_pFilterGraph->QueryInterface(IID_IMediaControl, reinterpret_cast<LPVOID*>(&m_pMediaControl));
		if (hr != S_OK) {
			throw CBonException(hr,TEXT("メディアコントロールを取得できません。"));
		}

		Trace(TEXT("ソースフィルタの接続中..."));

		/* CBonSrcFilter */
		{
			// インスタンス作成
			m_pSrcFilter = CBonSrcFilter::CreateInstance(NULL, &hr, &m_pBonSrcFilterClass);
			if (m_pSrcFilter==NULL || hr!=S_OK)
				throw CBonException(hr,TEXT("ソースフィルタを作成できません。"));
			m_pBonSrcFilterClass->SetOutputWhenPaused(RendererType==CVideoRenderer::RENDERER_DEFAULT);
			// フィルタグラフに追加
			hr=m_pFilterGraph->AddFilter(m_pSrcFilter, L"BonSrcFilter");
			if (hr != S_OK)
				throw CBonException(hr,TEXT("ソースフィルタをフィルタグラフに追加できません。"));
			// 出力ピンを取得
			pOutput = DirectShowUtil::GetFilterPin(m_pSrcFilter,PINDIR_OUTPUT);
			if (pOutput==NULL)
				throw CBonException(TEXT("ソースフィルタの出力ピンを取得できません。"));
		}

		Trace(TEXT("MPEG-2 Demultiplexerフィルタの接続中..."));

		/* MPEG-2 Demultiplexer */
		{
			CMediaType MediaTypeVideo;
			CMediaType MediaTypeAudio;
			IMpeg2Demultiplexer *pMpeg2Demuxer;

			hr=::CoCreateInstance(CLSID_MPEG2Demultiplexer,NULL,
					CLSCTX_INPROC_SERVER,IID_IBaseFilter,
					reinterpret_cast<LPVOID*>(&m_pMp2DemuxFilter));
			if (hr!=S_OK)
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerフィルタを作成できません。"),
									TEXT("MPEG-2 Demultiplexerフィルタがインストールされているか確認してください。"));
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								m_pMp2DemuxFilter,L"Mpeg2Demuxer",&pOutput);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerをフィルタグラフに追加できません。"));
			// この時点でpOutput==NULLのはずだが念のため
			SAFE_RELEASE(pOutput);

#if 0
			/*
				このコードは元々のBonTestからあるコードで意図は分からないが
				やらなくても問題無さそう
			*/
			// IReferenceClockインタフェースのクエリー
			IReferenceClock *pMp2DemuxRefClock;
			hr=m_pMp2DemuxFilter->QueryInterface(IID_IReferenceClock,
						reinterpret_cast<LPVOID*>(&pMp2DemuxRefClock));
			if (hr != S_OK)
				throw CBonException(hr,TEXT("IReferenceClockを取得できません。"));
			// リファレンスクロック選択
			hr=m_pMp2DemuxFilter->SetSyncSource(pMp2DemuxRefClock);
			pMp2DemuxRefClock->Release();
			if (hr != S_OK)
				throw CBonException(hr,TEXT("リファレンスクロックを設定できません。"))
#endif

			// IMpeg2Demultiplexerインタフェースのクエリー
			hr=m_pMp2DemuxFilter->QueryInterface(IID_IMpeg2Demultiplexer,
									reinterpret_cast<LPVOID*>(&pMpeg2Demuxer));
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerインターフェースを取得できません。"),
									TEXT("互換性のないスプリッタの優先度がMPEG-2 Demultiplexerより高くなっている可能性があります。"));

			// 映像メディアフォーマット設定
			MediaTypeVideo.InitMediaType();
			MediaTypeVideo.SetType(&MEDIATYPE_Video);
			MediaTypeVideo.SetSubtype(&MEDIASUBTYPE_MPEG2_VIDEO);
			MediaTypeVideo.SetVariableSize();
			MediaTypeVideo.SetTemporalCompression(TRUE);
			MediaTypeVideo.SetSampleSize(0);
			MediaTypeVideo.SetFormatType(&FORMAT_MPEG2Video);
			 // フォーマット構造体確保
			MPEG2VIDEOINFO *pVideoInfo = (MPEG2VIDEOINFO *)MediaTypeVideo.AllocFormatBuffer(sizeof(MPEG2VIDEOINFO));
			if (!pVideoInfo)
				throw CBonException(TEXT("メモリが確保できません。"));
			::ZeroMemory(pVideoInfo, sizeof(MPEG2VIDEOINFO));
			// ビデオヘッダ設定
			VIDEOINFOHEADER2 &VideoHeader = pVideoInfo->hdr;
			//::SetRect(&VideoHeader.rcSource, 0, 0, 720, 480);
			VideoHeader.bmiHeader.biWidth = 720;
			VideoHeader.bmiHeader.biHeight = 480;
			// 映像出力ピン作成
			hr=pMpeg2Demuxer->CreateOutputPin(&MediaTypeVideo,L"Video",&pOutputVideo);
			if (hr != S_OK) {
				pMpeg2Demuxer->Release();
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerの映像出力ピンを作成できません。"));
			}
			// 音声メディアフォーマット設定	
			MediaTypeAudio.InitMediaType();
			MediaTypeAudio.SetType(&MEDIATYPE_Audio);
			MediaTypeAudio.SetSubtype(&MEDIASUBTYPE_NULL);
			MediaTypeAudio.SetVariableSize();
			MediaTypeAudio.SetTemporalCompression(TRUE);
			MediaTypeAudio.SetSampleSize(0);
			MediaTypeAudio.SetFormatType(&FORMAT_None);
			// 音声出力ピン作成
			hr=pMpeg2Demuxer->CreateOutputPin(&MediaTypeAudio,L"Audio",&pOutputAudio);
			pMpeg2Demuxer->Release();
			if (hr != S_OK)
				throw CBonException(hr,TEXT("MPEG-2 Demultiplexerの音声出力ピンを作成できません。"));
			// 映像出力ピンのIMPEG2PIDMapインタフェースのクエリー
			hr=pOutputVideo->QueryInterface(IID_IMPEG2PIDMap,(void**)&m_pMp2DemuxVideoMap);
			if (hr != S_OK)
				throw CBonException(hr,TEXT("映像出力ピンのIMPEG2PIDMapを取得できません。"));
			// 音声出力ピンのIMPEG2PIDMapインタフェースのクエリ
			hr=pOutputAudio->QueryInterface(IID_IMPEG2PIDMap,(void**)&m_pMp2DemuxAudioMap);
			if (hr != S_OK)
				throw CBonException(hr,TEXT("音声出力ピンのIMPEG2PIDMapを取得できません。"));
		}

		Trace(TEXT("MPEG-2シーケンスフィルタの接続中..."));

		/* CMpeg2SequenceFilter */
		{
			// インスタンス作成
			m_pMpeg2SeqFilter = CMpeg2SequenceFilter::CreateInstance(NULL, &hr,&m_pMpeg2SeqClass);
			if((!m_pMpeg2SeqFilter) || (hr != S_OK))
				throw CBonException(hr,TEXT("MPEG-2シーケンスフィルタを作成できません。"));
			m_pMpeg2SeqClass->SetRecvCallback(OnMpeg2VideoInfo,this);
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
					m_pMpeg2SeqFilter,L"Mpeg2SequenceFilter",&pOutputVideo);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("MPEG-2シーケンスフィルタをフィルタグラフに追加できません。"));
		}

		Trace(TEXT("AACデコーダの接続中..."));

#if 1
		/* CAacDecFilter */
		{
			// CAacDecFilterインスタンス作成
			m_pAacDecFilter=CAacDecFilter::CreateInstance(NULL,&hr,&m_pAacDecClass);
			if (!m_pAacDecFilter || hr!=S_OK)
				throw CBonException(hr,TEXT("AACデコーダフィルタを作成できません。"));
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								m_pAacDecFilter,L"AacDecFilter",&pOutputAudio);
			if (FAILED(hr))
				throw CBonException(hr,TEXT("AACデコーダフィルタをフィルタグラフに追加できません。"));

			m_pAacDecClass->SetAdjustStreamTime(m_bAdjustAudioStreamTime);
			if (m_pAudioStreamCallback)
				m_pAacDecClass->SetStreamCallback(m_pAudioStreamCallback,
												  m_pAudioStreamCallbackParam);
		}
#else
		/* CAacParserFilter */
		{
			IBaseFilter *m_pAacParserFilter;
			CAacParserFilter *m_pAacParserClass;
			// CAacParserFilterインスタンス作成
			m_pAacParserFilter=CAacParserFilter::CreateInstance(NULL,&hr,&m_pAacParserClass);
			if (!m_pAacParserFilter || hr!=S_OK)
				throw CBonException(hr,TEXT("AACパーサフィルタを作成できません。"));
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
						m_pAacParserFilter,L"AacParserFilter",&pOutputAudio);
			if (FAILED(hr))
				throw CBonException(TEXT("AACパーサフィルタをフィルタグラフに追加できません。"));
			m_pAacParserFilter->Release();
		}

		/* AACデコーダー */
		{
			CDirectShowFilterFinder FilterFinder;

			// 検索
			if(!FilterFinder.FindFilter(&MEDIATYPE_Audio,&MEDIASUBTYPE_AAC))
				throw CBonException(TEXT("AACデコーダが見付かりません。"),
									TEXT("AACデコーダがインストールされているか確認してください。"));

			WCHAR szAacDecoder[128];
			CLSID idAac;
			bool bConnectSuccess=false;
			IBaseFilter *m_pAacDecFilter=NULL;

			for (int i=0;i<FilterFinder.GetFilterCount();i++){
				if (FilterFinder.GetFilterInfo(i,&idAac,szAacDecoder,128)) {
					TRACE(TEXT("AacDecoder %d : %s\n"),i,szAacDecoder);
					if (!bConnectSuccess) {
						hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								idAac,szAacDecoder,&m_pAacDecFilter,
								&pOutputAudio);
						if (SUCCEEDED(hr)) {
							TRACE(TEXT("AAC decoder connected : %s\n"),szAacDecoder);
							bConnectSuccess=true;
							//break;
						}
					}
				}
			}
			// どれかのフィルタで接続できたか
			if (bConnectSuccess) {
				SAFE_RELEASE(m_pAacDecFilter);
				//m_pszAacDecoderName=StdUtil::strdup(szAacDecoder);
			} else {
				throw CBonException(TEXT("AACデコーダフィルタをフィルタグラフに追加できません。"),
									TEXT("設定で有効なAACデコーダが選択されているか確認してください。"));
			}
		}
#endif

#if 0
		Trace(TEXT("PCMセレクトフィルタの接続中..."));

		/* CPcmSelectFilter */
		{
			// インスタンス作成
			m_pPcmSelFilter=CPcmSelectFilter::CreateInstance(NULL,&hr,&m_pPcmSelClass);
			if (!m_pPcmSelFilter || hr!=S_OK)
				throw CBonException(TEXT(hr,"PCMセレクトフィルタを作成できません。"));
			// フィルタの追加と接続
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
								m_pPcmSelFilter,L"PcmSelFilter",&pOutputAudio);
			if (FAILED(hr))
				throw CBonException(TEXT("PCMセレクトフィルタをフィルタグラフに追加できません。"));
		}
#endif

		Trace(TEXT("MPEG-2デコーダの接続中..."));

		/* Mpeg2デコーダー */
		{
			CDirectShowFilterFinder FilterFinder;

			// 検索
			if(!FilterFinder.FindFilter(&MEDIATYPE_Video,&MEDIASUBTYPE_MPEG2_VIDEO))
				throw CBonException(TEXT("MPEG-2デコーダが見付かりません。"),
									TEXT("MPEG-2デコーダがインストールされているか確認してください。"));

			WCHAR szMpeg2Decoder[128];
			CLSID idMpeg2Vid;
			bool bConnectSuccess=false;

			for (int i=0;i<FilterFinder.GetFilterCount();i++){
				if (FilterFinder.GetFilterInfo(i,&idMpeg2Vid,szMpeg2Decoder,128)) {
					if (pszMpeg2Decoder!=NULL && pszMpeg2Decoder[0]!='\0'
							&& ::lstrcmpi(szMpeg2Decoder,pszMpeg2Decoder)!=0)
						continue;
					hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
							idMpeg2Vid,szMpeg2Decoder,&m_pMpeg2DecFilter,
							&pOutputVideo,NULL,true);
					if (SUCCEEDED(hr)) {
						bConnectSuccess=true;
						break;
					}
				} else {
					// フィルタ情報取得失敗
				}
			}
			// どれかのフィルタで接続できたか
			if (bConnectSuccess) {
				m_pszMpeg2DecoderName=StdUtil::strdup(szMpeg2Decoder);
			} else {
				throw CBonException(hr,TEXT("MPEG-2デコーダフィルタをフィルタグラフに追加できません。"),
									TEXT("設定で有効なMPEG-2デコーダが選択されているか確認してください。"));
			}
		}

#ifdef USE_GRABBER_FILTER
		// グラバをテスト実装したがいまいちうまくいかないので保留
		if (m_bGrabber) {
			m_pGrabber=new CGrabber();
			IBaseFilter *pGrabberFilter;

			Trace(TEXT("グラバフィルタを接続中..."));

			m_pGrabber->Init();
			pGrabberFilter=m_pGrabber->GetGrabberFilter();
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
									pGrabberFilter,L"Grabber",&pOutputVideo);
			if (FAILED(hr)) {
				delete m_pGrabber;
				m_pGrabber=NULL;
			}
		}
#endif

#ifdef USE_VIDEO_RATE_KEEPER
		CRateKeeperFilter::CreateInstance(NULL,&hr,&m_pVideoRateKeeper);
		if (SUCCEEDED(hr)) {
			hr=DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
						m_pVideoRateKeeper,L"Video Rate Keeper",&pOutputVideo);
			if (FAILED(hr)) {
				SAFE_RELEASE(m_pVideoRateKeeper);
			}
		}
#endif

		Trace(TEXT("映像レンダラの構築中..."));

		if (!CVideoRenderer::CreateRenderer(RendererType,&m_pVideoRenderer)) {
			throw CBonException(TEXT("映像レンダラを作成できません。"),
								TEXT("設定で有効なレンダラが選択されているか確認してください。"));
		}
		if (!m_pVideoRenderer->Initialize(m_pFilterGraph,pOutputVideo,
										  hOwnerHwnd,hMessageDrainHwnd)) {
			throw CBonException(m_pVideoRenderer->GetLastErrorException());
		}
		m_VideoRendererType=RendererType;

		Trace(TEXT("音声レンダラの構築中..."));

		// 音声レンダラ構築
		{
			IBaseFilter *pAudioRenderer;
			bool fOK = false;

			if (pszAudioDevice != NULL && pszAudioDevice[0] != '\0') {
				CDirectShowDeviceEnumerator DevEnum;

				if (DevEnum.CreateFilter(CLSID_AudioRendererCategory,
										 pszAudioDevice, &pAudioRenderer))
					fOK = true;
			}
			if (!fOK) {
				hr = ::CoCreateInstance(CLSID_DSoundRender, NULL,
									CLSCTX_INPROC_SERVER, IID_IBaseFilter,
									reinterpret_cast<void**>(&pAudioRenderer));
				fOK = SUCCEEDED(hr);
			}
			if (fOK) {
				hr = DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
							pAudioRenderer, L"Audio Renderer", &pOutputAudio);
				if (SUCCEEDED(hr)) {
#ifdef _DEBUG
					if (pszAudioDevice != NULL && pszAudioDevice[0] != '\0')
						TRACE(TEXT("音声デバイス %s を接続\n"), pszAudioDevice);
#endif
					if (m_bUseAudioRendererClock) {
						IMediaFilter *pMediaFilter;

						if (SUCCEEDED(m_pFilterGraph->QueryInterface(IID_IMediaFilter,
								reinterpret_cast<LPVOID*>(&pMediaFilter)))) {
							IReferenceClock *pReferenceClock;

							if (SUCCEEDED(pAudioRenderer->QueryInterface(IID_IReferenceClock,
									reinterpret_cast<LPVOID*>(&pReferenceClock)))) {
								pMediaFilter->SetSyncSource(pReferenceClock);
								pReferenceClock->Release();
								TRACE(TEXT("グラフのクロックに音声レンダラを選択\n"));
							}
							pMediaFilter->Release();
						}
					}
					fOK = true;
				} else {
					fOK = false;
				}
				pAudioRenderer->Release();
				if (!fOK) {
					hr = m_pFilterGraph->Render(pOutputAudio);
					if (FAILED(hr))
						throw CBonException(hr, TEXT("音声レンダラを接続できません。"),
							TEXT("設定で有効なサウンドデバイスが選択されているか確認してください。"));
				}
			} else {
				// 音声デバイスが無い?
				// Nullレンダラを繋げておく
				hr = ::CoCreateInstance(CLSID_NullRenderer, NULL,
										CLSCTX_INPROC_SERVER, IID_IBaseFilter,
									reinterpret_cast<void**>(&pAudioRenderer));
				if (SUCCEEDED(hr)) {
					hr = DirectShowUtil::AppendFilterAndConnect(m_pFilterGraph,
						pAudioRenderer, L"Null Audio Renderer", &pOutputAudio);
					pAudioRenderer->Release();
					if (FAILED(hr)) {
						throw CBonException(hr, TEXT("Null音声レンダラを接続できません。"));
					}
					TRACE(TEXT("Nullレンダラを接続\n"));
				}
			}
		}

		/*
			デフォルトでMPEG-2 Demultiplexerがグラフのクロックに
			設定されるらしいが、一応設定しておく
		*/
		if (!m_bUseAudioRendererClock) {
			IMediaFilter *pMediaFilter;

			if (SUCCEEDED(m_pFilterGraph->QueryInterface(IID_IMediaFilter,
								reinterpret_cast<LPVOID*>(&pMediaFilter)))) {
				IReferenceClock *pReferenceClock;

				if (SUCCEEDED(m_pMp2DemuxFilter->QueryInterface(IID_IReferenceClock,
							reinterpret_cast<LPVOID*>(&pReferenceClock)))) {
					pMediaFilter->SetSyncSource(pReferenceClock);
					pReferenceClock->Release();
					TRACE(TEXT("グラフのクロックにMPEG-2 Demultiplexerを選択\n"));
				}
				pMediaFilter->Release();
			}
		}

		// オーナウィンドウ設定
		m_hOwnerWnd = hOwnerHwnd;
		RECT rc;
		::GetClientRect(hOwnerHwnd, &rc);
		m_wVideoWindowX = (WORD)rc.right;
		m_wVideoWindowY = (WORD)rc.bottom;

		m_bInit=true;

		ULONG PID;
		if (m_wVideoEsPID != PID_INVALID) {
			PID = m_wVideoEsPID;
			if (FAILED(m_pMp2DemuxVideoMap->MapPID(1, &PID, MEDIA_ELEMENTARY_STREAM)))
				m_wVideoEsPID = PID_INVALID;
		}
		if (m_wAudioEsPID != PID_INVALID) {
			PID = m_wAudioEsPID;
			if (FAILED(m_pMp2DemuxAudioMap->MapPID(1, &PID, MEDIA_ELEMENTARY_STREAM)))
				m_wAudioEsPID = PID_INVALID;
		}
	} catch (CBonException &Exception) {
		SetError(Exception);
		if (Exception.GetErrorCode()!=0) {
			TCHAR szText[MAX_ERROR_TEXT_LEN+32];
			int Length;

			Length=::AMGetErrorText(Exception.GetErrorCode(),szText,MAX_ERROR_TEXT_LEN);
			::wsprintf(szText+Length,TEXT("\nエラーコード(HRESULT) 0x%08X"),Exception.GetErrorCode());
			SetErrorSystemMessage(szText);
		}
		CloseViewer();
		TRACE(TEXT("フィルタグラフ構築失敗 : %s\n"), GetLastErrorText());
		return false;
	}

	SAFE_RELEASE(pOutputVideo);
	SAFE_RELEASE(pOutputAudio);

	ClearError();

	TRACE(TEXT("フィルタグラフ構築成功\n"));
	return true;
}

void CMediaViewer::CloseViewer(void)
{
	CTryBlockLock Lock(&m_DecoderLock);
	Lock.TryLock(LOCK_TIMEOUT);

	/*
	if (!m_bInit)
		return;
	*/

	Flush();
	Stop();

	// COMインスタンスを開放する
	if (m_pVideoRenderer!=NULL) {
		m_pVideoRenderer->Finalize();
	}

	if (m_pImageMixer!=NULL) {
		delete m_pImageMixer;
		m_pImageMixer=NULL;
	}

#ifdef USE_GRABBER_FILTER
	if (m_pGrabber!=NULL) {
		m_pFilterGraph->RemoveFilter(m_pGrabber->GetGrabberFilter());
		delete m_pGrabber;
		m_pGrabber=NULL;
	}
#endif

	if (m_pszMpeg2DecoderName!=NULL) {
		delete [] m_pszMpeg2DecoderName;
		m_pszMpeg2DecoderName=NULL;
	}

#ifdef USE_VIDEO_RATE_KEEPER
	SAFE_RELEASE(m_pVideoRateKeeper);
#endif

	SAFE_RELEASE(m_pMpeg2DecFilter);

	/*
	SAFE_RELEASE(m_pPcmSelFilter);
	m_pPcmSelClass=NULL;
	*/
	SAFE_RELEASE(m_pAacDecFilter);
	m_pAacDecClass=NULL;

	SAFE_RELEASE(m_pMpeg2SeqFilter);
	m_pMpeg2SeqClass=NULL;

	SAFE_RELEASE(m_pMp2DemuxAudioMap);
	SAFE_RELEASE(m_pMp2DemuxVideoMap);
	SAFE_RELEASE(m_pMp2DemuxFilter);

	SAFE_RELEASE(m_pSrcFilter);
	m_pBonSrcFilterClass=NULL;

	SAFE_RELEASE(m_pMediaControl);

#ifdef DEBUG
	if(m_dwRegister!=0){
		RemoveFromRot(m_dwRegister);
		m_dwRegister = 0;
	}
#endif

#ifdef DEBUG
	if (m_pFilterGraph)
		TRACE(TEXT("FilterGraph RefCount = %d\n"),DirectShowUtil::GetRefCount(m_pFilterGraph));
#endif
	SAFE_RELEASE(m_pFilterGraph);

	if (m_pVideoRenderer!=NULL) {
		delete m_pVideoRenderer;
		m_pVideoRenderer=NULL;
	}

	m_bInit=false;
}

const bool CMediaViewer::IsOpen() const
{
	return m_bInit;
}

const bool CMediaViewer::Play(void)
{
	TRACE(TEXT("CMediaViewer::Play()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if(!m_pMediaControl)return false;

	// フィルタグラフを再生する

	//return m_pMediaControl->Run()==S_OK;

	if (m_pMediaControl->Run()!=S_OK) {
		int i;
		OAFilterState fs;

		for (i=0;i<20;i++) {
			if (m_pMediaControl->GetState(100,&fs)==S_OK && fs==State_Running)
				return true;
		}
		return false;
	}
	return true;
}

const bool CMediaViewer::Stop(void)
{
	TRACE(TEXT("CMediaViewer::Stop()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (!m_pMediaControl)
		return false;

	/*
	// 既にフィルタグラフがデッドロックしている場合、止めると戻ってこないので
	// 本当はデッドロックしないようにすべきだが...
	if (CheckHangUp(1000))
		return false;
	*/

	if (m_pBonSrcFilterClass)
		//m_pBonSrcFilterClass->Reset();
		m_pBonSrcFilterClass->Flush();

	// フィルタグラフを停止する
	return m_pMediaControl->Stop()==S_OK;
}

const bool CMediaViewer::Pause()
{
	TRACE(TEXT("CMediaViewer::Pause()\n"));

	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;

	if (!m_pMediaControl)
		return false;

	if (m_pBonSrcFilterClass)
		//m_pBonSrcFilterClass->Reset();
		m_pBonSrcFilterClass->Flush();

	if (m_pMediaControl->Pause()!=S_OK) {
		int i;
		OAFilterState fs;
		HRESULT hr;

		for (i=0;i<20;i++) {
			hr=m_pMediaControl->GetState(100,&fs);
			if ((hr==S_OK || hr==VFW_S_CANT_CUE) && fs==State_Paused)
				return true;
		}
		return false;
	}
	return true;
}

const bool CMediaViewer::Flush()
{
	TRACE(TEXT("CMediaViewer::Flush()\n"));

	/*
	CTryBlockLock Lock(&m_DecoderLock);
	if (!Lock.TryLock(LOCK_TIMEOUT))
		return false;
	*/

	if (!m_pBonSrcFilterClass)
		return false;

	m_pBonSrcFilterClass->Flush();
	return true;
}

const bool CMediaViewer::SetVideoPID(const WORD wPID)
{
	// 映像出力ピンにPIDをマッピングする

	if (wPID == m_wVideoEsPID)
		return true;

	TRACE(TEXT("CMediaViewer::SetVideoPID() %04X <- %04X\n"), wPID, m_wVideoEsPID);

	if (m_pMp2DemuxVideoMap) {
		ULONG TempPID;

		// 現在のPIDをアンマップ
		if (m_wVideoEsPID != PID_INVALID) {
			TempPID = m_wVideoEsPID;
			if (m_pMp2DemuxVideoMap->UnmapPID(1UL, &TempPID) != S_OK)
				return false;
		}

		// 新規にPIDをマップ
		if (wPID != PID_INVALID) {
			TempPID = wPID;
			if (m_pMp2DemuxVideoMap->MapPID(1UL, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
		}
	}

	m_wVideoEsPID = wPID;

	return true;
}

const bool CMediaViewer::SetAudioPID(const WORD wPID)
{
	// 音声出力ピンにPIDをマッピングする

	if (wPID == m_wAudioEsPID)
		return true;

	TRACE(TEXT("CMediaViewer::SetAudioPID() %04X <- %04X\n"), wPID, m_wAudioEsPID);

	if (m_pMp2DemuxAudioMap) {
		ULONG TempPID;

		// 現在のPIDをアンマップ
		if (m_wAudioEsPID != PID_INVALID) {
			TempPID = m_wAudioEsPID;
			if (m_pMp2DemuxAudioMap->UnmapPID(1UL, &TempPID) != S_OK)
				return false;
		}

		// 新規にPIDをマップ
		if (wPID != PID_INVALID) {
			TempPID = wPID;
			if (m_pMp2DemuxAudioMap->MapPID(1UL, &TempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)
				return false;
		}
	}

	m_wAudioEsPID = wPID;

	return true;
}

const WORD CMediaViewer::GetVideoPID(void) const
{
	return m_wVideoEsPID;
}

const WORD CMediaViewer::GetAudioPID(void) const
{
	return m_wAudioEsPID;
}

void CMediaViewer::OnMpeg2VideoInfo(const CMpeg2VideoInfo *pVideoInfo,const LPVOID pParam)
{
	// ビデオ情報の更新
	CMediaViewer *pThis=static_cast<CMediaViewer*>(pParam);

	CBlockLock Lock(&pThis->m_ResizeLock);

	//if (pThis->m_VideoInfo != *pVideoInfo) {
		// ビデオ情報の更新
		pThis->m_VideoInfo = *pVideoInfo;
		pThis->ResizeVideoWindow();
	//}
	pThis->SendDecoderEvent(EID_VIDEO_SIZE_CHANGED);

#ifdef USE_VIDEO_RATE_KEEPER
	if (pThis->m_pVideoRateKeeper)
		pThis->m_pVideoRateKeeper->SetRate(pVideoInfo->m_FrameRate.Num,
										   pVideoInfo->m_FrameRate.Denom,
										   10000000LL / 5LL);
#endif
}

const bool CMediaViewer::ResizeVideoWindow()
{
	// ウィンドウサイズを変更する
	if (m_pVideoRenderer && m_wVideoWindowX>0 && m_wVideoWindowY>0) {
		long WindowWidth,WindowHeight,VideoWidth,VideoHeight;

		WindowWidth = m_wVideoWindowX;
		WindowHeight = m_wVideoWindowY;
		if (m_ViewStretchMode!=STRETCH_FIT) {
			int AspectX,AspectY;
			double aspect_rate;
			double window_rate = (double)WindowWidth / (double)WindowHeight;

			if (m_ForceAspectX>0 && m_ForceAspectY>0) {
				AspectX = m_ForceAspectX;
				AspectY = m_ForceAspectY;
			} else if (m_VideoInfo.m_AspectRatioX>0 && m_VideoInfo.m_AspectRatioY>0) {
				AspectX = m_VideoInfo.m_AspectRatioX;
				AspectY = m_VideoInfo.m_AspectRatioY;
				if (m_bIgnoreDisplayExtension
						&& m_VideoInfo.m_DisplayWidth > 0
						&& m_VideoInfo.m_DisplayHeight > 0) {
					AspectX = AspectX * 3 * m_VideoInfo.m_OrigWidth / m_VideoInfo.m_DisplayWidth;
					AspectY = AspectY * 3 * m_VideoInfo.m_OrigHeight / m_VideoInfo.m_DisplayHeight;
				}
			} else {
				if (((m_VideoInfo.m_DisplayWidth==1920
							|| m_VideoInfo.m_DisplayWidth==1440)
						&& m_VideoInfo.m_DisplayHeight==1080)) {
					AspectX = 16;
					AspectY = 9;
				} else {
					AspectX = WindowWidth;
					AspectY = WindowHeight;
				}
			}
			aspect_rate = (double)AspectX / (double)AspectY;
			if ((m_ViewStretchMode==STRETCH_KEEPASPECTRATIO && aspect_rate>window_rate)
					|| (m_ViewStretchMode==STRETCH_CUTFRAME && aspect_rate<window_rate)) {
				VideoWidth = WindowWidth;
				VideoHeight = VideoWidth * AspectY  / AspectX;
			} else {
				VideoHeight = WindowHeight;
				VideoWidth = VideoHeight * AspectX / AspectY;
			}
		} else {
			VideoWidth = WindowWidth;
			VideoHeight = WindowHeight;
		}
		RECT rcSrc,rcDst,rcWindow;

		CalcSourceRect(&rcSrc);
		// 座標値がマイナスになるとマルチディスプレイでおかしくなる?
		/*
		rcDst.left=(WindowWidth-VideoWidth)/2;
		rcDst.top=(WindowHeight-VideoHeight)/2,
		rcDst.right=rcDst.left+VideoWidth;
		rcDst.bottom=rcDst.top+VideoHeight;
		*/
		if (WindowWidth<VideoWidth) {
			rcDst.left=0;
			rcDst.right=WindowWidth;
			rcSrc.left+=(VideoWidth-WindowWidth)*(rcSrc.right-rcSrc.left)/VideoWidth/2;
			rcSrc.right=m_VideoInfo.m_OrigWidth-rcSrc.left;
		} else {
			rcDst.left=(WindowWidth-VideoWidth)/2;
			rcDst.right=rcDst.left+VideoWidth;
		}
		if (WindowHeight<VideoHeight) {
			rcDst.top=0;
			rcDst.bottom=WindowHeight;
			rcSrc.top+=(VideoHeight-WindowHeight)*(rcSrc.bottom-rcSrc.top)/VideoHeight/2;
			rcSrc.bottom=m_VideoInfo.m_OrigHeight-rcSrc.top;
		} else {
			rcDst.top=(WindowHeight-VideoHeight)/2,
			rcDst.bottom=rcDst.top+VideoHeight;
		}
		rcWindow.left=0;
		rcWindow.top=0;
		rcWindow.right=WindowWidth;
		rcWindow.bottom=WindowHeight;
		return m_pVideoRenderer->SetVideoPosition(
			m_VideoInfo.m_OrigWidth,m_VideoInfo.m_OrigHeight,&rcSrc,&rcDst,&rcWindow);
	}
	return false;
}

const bool CMediaViewer::SetViewSize(const int x,const int y)
{
	CBlockLock Lock(&m_ResizeLock);

	// ウィンドウサイズを設定する
	if (x>0 && y>0) {
		m_wVideoWindowX = x;
		m_wVideoWindowY = y;
		return ResizeVideoWindow();
	}
	return false;
}

const bool CMediaViewer::SetVolume(const float fVolume)
{
	// オーディオボリュームをdBで設定する( -100.0(無音) < fVolume < 0(最大) )
	IBasicAudio *pBasicAudio;
	bool fOK=false;

	if (m_pFilterGraph) {
		if (SUCCEEDED(m_pFilterGraph->QueryInterface(IID_IBasicAudio,
								reinterpret_cast<LPVOID *>(&pBasicAudio)))) {
			long lVolume = (long)(fVolume * 100.0f);

			if (lVolume>=-10000 && lVolume<=0) {
					TRACE(TEXT("Volume Control = %d\n"),lVolume);
				if (SUCCEEDED(pBasicAudio->put_Volume(lVolume)))
					fOK=true;
			}
			pBasicAudio->Release();
		}
	}
	return fOK;
}

const bool CMediaViewer::GetVideoSize(WORD *pwWidth,WORD *pwHeight)
{
	if (m_bIgnoreDisplayExtension)
		return GetOriginalVideoSize(pwWidth, pwHeight);

	CBlockLock Lock(&m_ResizeLock);

	// ビデオのサイズを取得する
	/*
	if (m_pMpeg2SeqClass)
		return m_pMpeg2SeqClass->GetVideoSize(pwWidth,pwHeight);
	*/
	if (m_VideoInfo.m_DisplayWidth > 0 && m_VideoInfo.m_DisplayHeight > 0) {
		if (pwWidth)
			*pwWidth = m_VideoInfo.m_DisplayWidth;
		if (pwHeight)
			*pwHeight = m_VideoInfo.m_DisplayHeight;
		return true;
	}
	return false;
}

const bool CMediaViewer::GetVideoAspectRatio(BYTE *pbyAspectRatioX,BYTE *pbyAspectRatioY)
{
	CBlockLock Lock(&m_ResizeLock);

	// ビデオのアスペクト比を取得する
	/*
	if (m_pMpeg2SeqClass)
		return m_pMpeg2SeqClass->GetAspectRatio(pbyAspectRatioX,pbyAspectRatioY);
	*/
	if (m_VideoInfo.m_AspectRatioX > 0 && m_VideoInfo.m_AspectRatioY > 0) {
		if (pbyAspectRatioX)
			*pbyAspectRatioX = m_VideoInfo.m_AspectRatioX;
		if (pbyAspectRatioY)
			*pbyAspectRatioY = m_VideoInfo.m_AspectRatioY;
		return true;
	}
	return false;
}

const BYTE CMediaViewer::GetAudioChannelNum()
{
	// オーディオの入力チャンネル数を取得する
	if (m_pAacDecClass)
		return m_pAacDecClass->GetCurrentChannelNum();
	return 0;
}

const bool CMediaViewer::SetStereoMode(const int iMode)
{
	// ステレオ出力チャンネルの設定
	/*
	if(m_pPcmSelClass){
		m_pPcmSelClass->SetStereoMode(iMode);
		return true;
		}
	*/
	if (m_pAacDecClass)
		return m_pAacDecClass->SetStereoMode(iMode);
	return false;
}

const int CMediaViewer::GetStereoMode() const
{
	if (m_pAacDecClass)
		return m_pAacDecClass->GetStereoMode();
	return CAacDecFilter::STEREOMODE_STEREO;
}

const bool CMediaViewer::GetVideoDecoderName(LPWSTR lpName,int iBufLen)
{
	// 選択されているビデオデコーダー名の取得
	if (m_pszMpeg2DecoderName!=NULL) {
		::lstrcpynW(lpName,m_pszMpeg2DecoderName,iBufLen);
		return true;
	}
	if (iBufLen>0)
		lpName[0]='\0';
	return false;
}

const bool CMediaViewer::DisplayVideoDecoderProperty(HWND hWndParent)
{
	if (m_pMpeg2DecFilter)
		return DirectShowUtil::ShowPropertyPage(m_pMpeg2DecFilter,hWndParent);
	return false;
}

const bool CMediaViewer::DisplayVideoRandererProperty(HWND hWndParent)
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->ShowProperty(hWndParent);
	return false;
}


#ifdef _DEBUG

HRESULT CMediaViewer::AddToRot(IUnknown *pUnkGraph, DWORD *pdwRegister) const
{
	// デバッグ用
	IMoniker * pMoniker;
	IRunningObjectTable *pROT;
	if(FAILED(::GetRunningObjectTable(0, &pROT)))return E_FAIL;

	WCHAR wsz[256];
	wsprintfW(wsz, L"FilterGraph %08p pid %08x", (DWORD_PTR)pUnkGraph, ::GetCurrentProcessId());

	HRESULT hr = ::CreateItemMoniker(L"!", wsz, &pMoniker);

	if(SUCCEEDED(hr)){
		hr = pROT->Register(0, pUnkGraph, pMoniker, pdwRegister);
		pMoniker->Release();
		}

	pROT->Release();

	return hr;
}

void CMediaViewer::RemoveFromRot(const DWORD dwRegister) const
{
	// デバッグ用
	IRunningObjectTable *pROT;

	if(SUCCEEDED(::GetRunningObjectTable(0, &pROT))){
		pROT->Revoke(dwRegister);
		pROT->Release();
		}
}

#endif	// _DEBUG




const bool CMediaViewer::ForceAspectRatio(int AspectX,int AspectY)
{
	m_ForceAspectX=AspectX;
	m_ForceAspectY=AspectY;
	return true;
}


const bool CMediaViewer::GetForceAspectRatio(int *pAspectX,int *pAspectY) const
{
	if (pAspectX)
		*pAspectX=m_ForceAspectX;
	if (pAspectY)
		*pAspectY=m_ForceAspectY;
	return true;
}


const bool CMediaViewer::GetEffectiveAspectRatio(BYTE *pAspectX, BYTE *pAspectY)
{
	if (m_ForceAspectX != 0 && m_ForceAspectY != 0) {
		if (pAspectX)
			*pAspectX = m_ForceAspectX;
		if (pAspectY)
			*pAspectY = m_ForceAspectY;
		return true;
	}
	BYTE AspectX, AspectY;
	if (!GetVideoAspectRatio(&AspectX, &AspectY))
		return false;
	if (m_bIgnoreDisplayExtension
			&& (m_VideoInfo.m_DisplayWidth != m_VideoInfo.m_OrigWidth
				|| m_VideoInfo.m_DisplayHeight != m_VideoInfo.m_OrigHeight)) {
		if (m_VideoInfo.m_DisplayWidth == 0
				|| m_VideoInfo.m_DisplayHeight == 0)
			return false;
		AspectX = AspectX * 3 * m_VideoInfo.m_OrigWidth / m_VideoInfo.m_DisplayWidth;
		AspectY = AspectY * 3 * m_VideoInfo.m_OrigHeight / m_VideoInfo.m_DisplayHeight;
		if (AspectX % 4 == 0 && AspectY % 4 == 0) {
			AspectX /= 4;
			AspectY /= 4;
		}
	}
	if (pAspectX)
		*pAspectX = AspectX;
	if (pAspectY)
		*pAspectY = AspectY;
	return true;
}


const bool CMediaViewer::SetPanAndScan(int AspectX,int AspectY,BYTE PanScanFlags)
{
	if (m_ForceAspectX!=AspectX || m_ForceAspectY!=AspectY
			|| m_PanAndScan!=PanScanFlags) {
		CBlockLock Lock(&m_ResizeLock);

		m_ForceAspectX=AspectX;
		m_ForceAspectY=AspectY;
		m_PanAndScan=PanScanFlags;
		ResizeVideoWindow();
	}
	return true;
}


const bool CMediaViewer::SetViewStretchMode(ViewStretchMode Mode)
{
	if (m_ViewStretchMode!=Mode) {
		CBlockLock Lock(&m_ResizeLock);

		m_ViewStretchMode=Mode;
		return ResizeVideoWindow();
	}
	return true;
}


const bool CMediaViewer::SetIgnoreDisplayExtension(bool bIgnore)
{
	if (bIgnore != m_bIgnoreDisplayExtension) {
		CBlockLock Lock(&m_ResizeLock);

		m_bIgnoreDisplayExtension = bIgnore;
		if (m_VideoInfo.m_DisplayWidth != m_VideoInfo.m_OrigWidth
				|| m_VideoInfo.m_DisplayHeight != m_VideoInfo.m_OrigHeight)
			ResizeVideoWindow();
	}
	return true;
}


const bool CMediaViewer::GetOriginalVideoSize(WORD *pWidth,WORD *pHeight)
{
	CBlockLock Lock(&m_ResizeLock);

	/*
	if (m_pMpeg2SeqClass)
		return m_pMpeg2SeqClass->GetOriginalVideoSize(pWidth,pHeight);
	*/
	if (m_VideoInfo.m_OrigWidth > 0 && m_VideoInfo.m_OrigHeight > 0) {
		if (pWidth)
			*pWidth = m_VideoInfo.m_OrigWidth;
		if (pHeight)
			*pHeight = m_VideoInfo.m_OrigHeight;
		return true;
	}
	return false;
}


const bool CMediaViewer::GetCroppedVideoSize(WORD *pWidth,WORD *pHeight)
{
	RECT rc;

	if (!GetSourceRect(&rc))
		return false;
	if (pWidth)
		*pWidth = (WORD)(rc.right - rc.left);
	if (pHeight)
		*pHeight = (WORD)(rc.bottom - rc.top);
	return true;
}


const bool CMediaViewer::GetSourceRect(RECT *pRect)
{
	CBlockLock Lock(&m_ResizeLock);

	if (!pRect)
		return false;
	return CalcSourceRect(pRect);
}


const bool CMediaViewer::CalcSourceRect(RECT *pRect)
{
	long SrcX,SrcY,SrcWidth,SrcHeight;

	if (m_VideoInfo.m_OrigWidth==0 || m_VideoInfo.m_OrigHeight==0)
		return false;
	if (m_PanAndScan&PANANDSCAN_HORZ_NONE) {
		SrcWidth=m_VideoInfo.m_OrigWidth;
		SrcX=0;
	} else if (m_PanAndScan&PANANDSCAN_HORZ_CUT) {
		SrcWidth=m_VideoInfo.m_OrigWidth*12/16;
		SrcX=(m_VideoInfo.m_OrigWidth-SrcWidth)/2;
	} else if (m_bIgnoreDisplayExtension) {
		SrcWidth=m_VideoInfo.m_OrigWidth;
		SrcX=0;
	} else {
		SrcWidth=m_VideoInfo.m_DisplayWidth;
		SrcX=m_VideoInfo.m_PosX;
	}
	if (m_PanAndScan&PANANDSCAN_VERT_NONE) {
		SrcHeight=m_VideoInfo.m_OrigHeight;
		SrcY=0;
	} else if (m_PanAndScan&PANANDSCAN_VERT_CUT) {
		SrcHeight=m_VideoInfo.m_OrigHeight*9/12;
		SrcY=(m_VideoInfo.m_OrigHeight-SrcHeight)/2;
	} else if (m_bIgnoreDisplayExtension) {
		SrcHeight=m_VideoInfo.m_OrigHeight;
		SrcY=0;
	} else {
		SrcHeight=m_VideoInfo.m_DisplayHeight;
		SrcY=m_VideoInfo.m_PosY;
	}
	pRect->left=SrcX;
	pRect->top=SrcY;
	pRect->right=SrcX+SrcWidth;
	pRect->bottom=SrcY+SrcHeight;
	return true;
}


const bool CMediaViewer::GetDestRect(RECT *pRect)
{
	if (m_pVideoRenderer && pRect) {
		if (m_pVideoRenderer->GetDestPosition(pRect))
			return true;
	}
	return false;
}


const bool CMediaViewer::GetDestSize(WORD *pWidth,WORD *pHeight)
{
	RECT rc;

	if (!GetDestRect(&rc))
		return false;
	if (pWidth)
		*pWidth=(WORD)(rc.right-rc.left);
	if (pHeight)
		*pHeight=(WORD)(rc.bottom-rc.top);
	return true;
}


bool CMediaViewer::SetVisible(bool fVisible)
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->SetVisible(fVisible);
	return false;
}


const void CMediaViewer::HideCursor(bool bHide)
{
	if (m_pVideoRenderer)
		m_pVideoRenderer->ShowCursor(!bHide);
}


const bool CMediaViewer::GetCurrentImage(BYTE **ppDib)
{
	/*
	CTryBlockLock Lock(&m_CriticalLock);
	if (!Lock.TryLock(1000))
		return false;
	*/

	bool fOK=false;

	if (m_pVideoRenderer) {
		void *pBuffer;

		if (m_pVideoRenderer->GetCurrentImage(&pBuffer)) {
			fOK=true;
			*ppDib=static_cast<BYTE*>(pBuffer);
		}
	}
	return fOK;
}


#ifdef USE_GRABBER_FILTER

bool CMediaViewer::SetGrabber(bool bGrabber)
{
	m_bGrabber=bGrabber;
	return true;
}


void *CMediaViewer::DoCapture(DWORD WaitTime)
{
	void *pDib;

	if (m_pGrabber==NULL)
		return NULL;
	if (!m_pGrabber->SetCapture(true))
		return NULL;
	if (!m_pGrabber->WaitCapture(WaitTime)) {
		m_pGrabber->SetCapture(false);
		return NULL;
	}
	pDib=m_pGrabber->GetCaptureBitmap();
	m_pGrabber->SetCapture(false);
	return pDib;
}

#endif


bool CMediaViewer::SetDownMixSurround(bool bDownMix)
{
	if (m_pAacDecClass)
		return m_pAacDecClass->SetDownMixSurround(bDownMix);
	return false;
}


bool CMediaViewer::GetDownMixSurround() const
{
	if (m_pAacDecClass)
		return m_pAacDecClass->GetDownMixSurround();
	return false;
}


bool CMediaViewer::SetAudioNormalize(bool bNormalize,float Level)
{
	if (m_pAacDecClass==NULL)
		return false;
	return m_pAacDecClass->SetNormalize(bNormalize,Level);
}


CVideoRenderer::RendererType CMediaViewer::GetVideoRendererType() const
{
	return m_VideoRendererType;
}


bool CMediaViewer::SetUseAudioRendererClock(bool bUse)
{
	m_bUseAudioRendererClock = bUse;
	return true;
}


bool CMediaViewer::SetAdjustAudioStreamTime(bool bAdjust)
{
	m_bAdjustAudioStreamTime = bAdjust;
	if (m_pAacDecClass == NULL)
		return true;
	return m_pAacDecClass->SetAdjustStreamTime(bAdjust);
}


bool CMediaViewer::SetAudioStreamCallback(CAacDecFilter::StreamCallback pCallback, void *pParam)
{
	m_pAudioStreamCallback=pCallback;
	m_pAudioStreamCallbackParam=pParam;
	if (m_pAacDecClass == NULL)
		return true;
	return m_pAacDecClass->SetStreamCallback(pCallback, pParam);
}


const bool CMediaViewer::RepaintVideo(HWND hwnd,HDC hdc)
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->RepaintVideo(hwnd,hdc);
	return false;
}


const bool CMediaViewer::DisplayModeChanged()
{
	if (m_pVideoRenderer)
		return m_pVideoRenderer->DisplayModeChanged();
	return false;
}


const bool CMediaViewer::DrawText(LPCTSTR pszText,int x,int y,
								  HFONT hfont,COLORREF crColor,int Opacity)
{
	IBaseFilter *pRenderer;
	int Width,Height;

	if (m_pVideoRenderer==NULL || !IsDrawTextSupported())
		return false;
	pRenderer=m_pVideoRenderer->GetRendererFilter();
	if (pRenderer==NULL)
		return false;
	if (m_pImageMixer==NULL) {
		m_pImageMixer=CImageMixer::CreateImageMixer(m_VideoRendererType,pRenderer);
		if (m_pImageMixer==NULL)
			return false;
	}
	if (!m_pImageMixer->GetMapSize(&Width,&Height))
		return false;
	m_ResizeLock.Lock();
	if (m_VideoInfo.m_OrigWidth==0 || m_VideoInfo.m_OrigHeight==0)
		return false;
	x=x*Width/m_VideoInfo.m_OrigWidth;
	y=y*Height/m_VideoInfo.m_OrigHeight;
	m_ResizeLock.Unlock();
	return m_pImageMixer->SetText(pszText,x,y,hfont,crColor,Opacity);
}


const bool CMediaViewer::IsDrawTextSupported() const
{
	return CImageMixer::IsSupported(m_VideoRendererType);
}


const bool CMediaViewer::ClearOSD()
{
	if (m_pVideoRenderer==NULL)
		return false;
	if (m_pImageMixer!=NULL)
		m_pImageMixer->Clear();
	return true;
}


bool CMediaViewer::SetAudioOnly(bool bOnly)
{
	if (m_pMpeg2SeqClass==NULL)
		return false;
	return m_pMpeg2SeqClass->SetDeliverSamples(!bOnly);
}


bool CMediaViewer::CheckHangUp(DWORD TimeOut)
{
	if (!m_DecoderLock.TryLock(TimeOut))
		return true;
	m_DecoderLock.Unlock();
	return false;
}
