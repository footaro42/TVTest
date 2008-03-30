// MediaViewer.cpp: CMediaViewer クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaViewer.h"
#include <Dvdmedia.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// マクロ定義
//////////////////////////////////////////////////////////////////////

#define SAFE_RELEASE(pOblect)	if((pOblect)){(pOblect)->Release(); (pOblect) = NULL;}


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CMediaViewer::CMediaViewer(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler)
	, m_pFilterGraph(NULL)
	, m_pMediaControl(NULL)

	, m_pBonSrcFilter(NULL)
	, m_pAacDecFilter(NULL)
	, m_pMp2DemuxFilter(NULL)

	, m_pMp2DemuxInterface(NULL)
	, m_pMp2DemuxRefClock(NULL)

	, m_pBonSrcOutputPin(NULL)

	, m_pMp2DemuxInputPin(NULL)
	, m_pMp2DemuxVideoPin(NULL)
	, m_pMp2DemuxAudioPin(NULL)

	, m_pMp2DemuxVideoMap(NULL)
	, m_pMp2DemuxAudioMap(NULL)

	, m_pAacDecInputPin(NULL)
	, m_pAacDecOutputPin(NULL)

	, m_pVideoWindow(NULL)
	
	, m_wVideoEsPID(0xFFFFU)
	, m_wAudioEsPID(0xFFFFU)
{
	// COMライブラリ初期化
	::CoInitialize(NULL);
}

CMediaViewer::~CMediaViewer()
{
	CloseViewer();

	// COMライブラリ開放
	::CoUninitialize();
}

void CMediaViewer::Reset(void)
{
	Stop();

	CMediaDecoder::Reset();

	SetVideoPID(0xFFFFU);
	SetAudioPID(0xFFFFU);

	Play();
}

const DWORD CMediaViewer::GetInputNum(void) const
{
	return 1UL;
}

const DWORD CMediaViewer::GetOutputNum(void) const
{
	return 0UL;
}

const bool CMediaViewer::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if(dwInputIndex > GetInputNum())return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;
	
	// フィルタグラフに入力
	if(m_pBonSrcFilter){
		return m_pBonSrcFilter->InputMedia(pTsPacket);
		}

	return false;
}

const bool CMediaViewer::OpenViewer(HWND hOwnerHwnd)
{
	if(m_pFilterGraph)return false;

	HRESULT hReturn;

	try{
		// フィルタグラフマネージャを構築する
		if(::CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, reinterpret_cast<LPVOID *>(&m_pFilterGraph)) != S_OK)throw 1UL;

		AddToRot(m_pFilterGraph, &m_dwRegister);

		// IMediaControlインタフェースのクエリー
		if(m_pFilterGraph->QueryInterface(IID_IMediaControl, reinterpret_cast<LPVOID *>(&m_pMediaControl)) != S_OK)throw 1UL;

		// CBonSrcFilterインスタンス作成
		m_pBonSrcFilter = static_cast<CBonSrcFilter *>(CBonSrcFilter::CreateInstance(NULL, &hReturn));
		if((!m_pBonSrcFilter) || (hReturn != S_OK))throw 1UL;
		m_pBonSrcFilter->AddRef();
		
		// CBonSrcFilterをフィルタグラフに追加
		if(m_pFilterGraph->AddFilter(static_cast<IBaseFilter *>(m_pBonSrcFilter), L"BonSrcFilter") != S_OK)throw 1UL;

		// CBonSrcFilterの出力ピンを取得
		if(!(m_pBonSrcOutputPin = m_pBonSrcFilter->GetPin(0)))throw 1UL;

		// MPEG-2 Demultiplexerインスタンス作成
		if(::CoCreateInstance(CLSID_MPEG2Demultiplexer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, reinterpret_cast<LPVOID *>(&m_pMp2DemuxFilter)) != S_OK)throw 1UL;

		// IMpeg2Demultiplexerインタフェースのクエリー
		if(m_pMp2DemuxFilter->QueryInterface(IID_IMpeg2Demultiplexer, reinterpret_cast<LPVOID *>(&m_pMp2DemuxInterface)) != S_OK)throw 1UL;

		// MPEG-2 Demultiplexerをフィルタグラフに追加
		if(m_pFilterGraph->AddFilter(m_pMp2DemuxFilter, L"Mpeg2Demuxer") != S_OK)throw 1UL;

		// MPEG-2 Demultiplexerの入力ピンを取得
		if(m_pMp2DemuxFilter->FindPin(L"MPEG-2 Stream", &m_pMp2DemuxInputPin) != S_OK)throw 1UL;
		
		// ピンを接続する(CBonSrcFilter [TS] → [MPEG-2 Stream] MPEG-2 Demultiplexer)
		if(m_pFilterGraph->ConnectDirect(m_pBonSrcOutputPin, m_pMp2DemuxInputPin, NULL) != S_OK)throw 1UL;
		
		// IReferenceClockインタフェースのクエリー
		if(m_pMp2DemuxFilter->QueryInterface(IID_IReferenceClock, reinterpret_cast<LPVOID *>(&m_pMp2DemuxRefClock)) != S_OK)throw 1UL;

		// リファレンスクロック選択
		if(m_pMp2DemuxFilter->SetSyncSource(m_pMp2DemuxRefClock) != S_OK)throw 1UL;
		
		// 映像メディアフォーマット設定		
		CMediaType MediaType;
		MediaType.InitMediaType();
		MediaType.SetType(&MEDIATYPE_Video);
		MediaType.SetSubtype(&MEDIASUBTYPE_MPEG2_VIDEO);
		MediaType.SetVariableSize();
		MediaType.SetTemporalCompression(TRUE);
		MediaType.SetSampleSize(0);
		MediaType.SetFormatType(&FORMAT_MPEG2Video);
		
		 // フォーマット構造体確保
		MPEG2VIDEOINFO *pVideoInfo = reinterpret_cast<MPEG2VIDEOINFO *>(MediaType.AllocFormatBuffer(sizeof(MPEG2VIDEOINFO)));
		if(!pVideoInfo)return false;
		::ZeroMemory(pVideoInfo, sizeof(MPEG2VIDEOINFO));

		// ビデオヘッダ設定
		VIDEOINFOHEADER2 &VideoHeader = pVideoInfo->hdr;
		::SetRect(&VideoHeader.rcSource, 0, 0, 720, 480);
		VideoHeader.bmiHeader.biHeight = 480;
		VideoHeader.bmiHeader.biWidth = 720;
		
		// 映像出力ピン作成
		if(m_pMp2DemuxInterface->CreateOutputPin(&MediaType, L"Video", &m_pMp2DemuxVideoPin) != S_OK)throw 1UL;

		// 音声メディアフォーマット設定	
		MediaType.InitMediaType();
		MediaType.SetType(&MEDIATYPE_Audio);
		MediaType.SetSubtype(&MEDIASUBTYPE_NULL);
		MediaType.SetVariableSize();
		MediaType.SetTemporalCompression(TRUE);
		MediaType.SetSampleSize(0);
		MediaType.SetFormatType(&FORMAT_None);

		// 音声出力ピン作成
		if(m_pMp2DemuxInterface->CreateOutputPin(&MediaType, L"Audio", &m_pMp2DemuxAudioPin) != S_OK)throw 1UL;
		
		// 映像出力ピンのIMPEG2PIDMapインタフェースのクエリー
		if(m_pMp2DemuxVideoPin->QueryInterface(IID_IMPEG2PIDMap, reinterpret_cast<LPVOID *>(&m_pMp2DemuxVideoMap)) != S_OK)throw 1UL;

		// 音声出力ピンのIMPEG2PIDMapインタフェースのクエリー
		if(m_pMp2DemuxAudioPin->QueryInterface(IID_IMPEG2PIDMap, reinterpret_cast<LPVOID *>(&m_pMp2DemuxAudioMap)) != S_OK)throw 1UL;

		// CAacDecFilterインスタンス作成
		m_pAacDecFilter = static_cast<CAacDecFilter *>(CAacDecFilter::CreateInstance(NULL, &hReturn));
		if((!m_pAacDecFilter) || (hReturn != S_OK))throw 1UL;
		m_pAacDecFilter->AddRef();
		
		// CAacDecFilterをフィルタグラフに追加
		if(m_pFilterGraph->AddFilter(static_cast<IBaseFilter *>(m_pAacDecFilter), L"AacDecFilter") != S_OK)throw 1UL;

		// CAacDecFilterの入力ピンを取得
		if(!(m_pAacDecInputPin = m_pAacDecFilter->GetPin(0)))throw 1UL;

		// CAacDecFilterの出力ピンを取得
		if(!(m_pAacDecOutputPin = m_pAacDecFilter->GetPin(1)))throw 1UL;

		// ピンを接続する(MPEG-2 Demultiplexer [Audio] → [入力] CAacDecFilter)
		if(m_pFilterGraph->ConnectDirect(m_pMp2DemuxAudioPin, m_pAacDecInputPin, NULL) != S_OK)throw 1UL;

		// 映像レンダラー構築
		if(m_pFilterGraph->Render(m_pMp2DemuxVideoPin) != S_OK)throw 1UL;

		// 音声レンダラー構築
		if(m_pFilterGraph->Render(m_pAacDecOutputPin) != S_OK)throw 1UL;

		// IVideoWindowインタフェースのクエリー
		if(m_pFilterGraph->QueryInterface(IID_IVideoWindow, reinterpret_cast<LPVOID *>(&m_pVideoWindow)) != S_OK)throw 1UL;
		
		// オーナウィンドウ設定
		if(hOwnerHwnd){
			m_pVideoWindow->put_Owner((OAHWND)hOwnerHwnd);
			m_pVideoWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS);
			
			RECT Rect;
			::GetClientRect(hOwnerHwnd, &Rect);
			m_pVideoWindow->SetWindowPosition(0, 0, Rect.right - Rect.left, Rect.bottom - Rect.top);
			m_pVideoWindow->SetWindowForeground(OATRUE);
			m_pVideoWindow->put_Visible(OATRUE);
			}
		}
	catch(DWORD dwErrorCode){
		CloseViewer();
		TRACE(TEXT("フィルタグラフ構築失敗[dwErrorCode = %lu]\n"), dwErrorCode);
		return false;
		}

	TRACE(TEXT("フィルタグラフ構築成功\n"));
	
	return true;
}

void CMediaViewer::CloseViewer(void)
{
	// COMインスタンスを開放する
	if(m_pVideoWindow)m_pVideoWindow->put_Owner(NULL);

	SAFE_RELEASE(m_pMp2DemuxVideoMap);
	SAFE_RELEASE(m_pMp2DemuxAudioMap);
	SAFE_RELEASE(m_pMp2DemuxVideoPin);
	SAFE_RELEASE(m_pMp2DemuxAudioPin);
	SAFE_RELEASE(m_pMp2DemuxInputPin);
	SAFE_RELEASE(m_pMp2DemuxRefClock);
	SAFE_RELEASE(m_pMp2DemuxInterface);
	SAFE_RELEASE(m_pMp2DemuxFilter);

	SAFE_RELEASE(m_pAacDecFilter);
	SAFE_RELEASE(m_pBonSrcFilter);

	SAFE_RELEASE(m_pMediaControl);
	SAFE_RELEASE(m_pVideoWindow);
	SAFE_RELEASE(m_pFilterGraph);

	RemoveFromRot(m_dwRegister);
}

const bool CMediaViewer::Play(void)
{
	if(!m_pMediaControl)return false;

	// フィルタグラフを再生する
	m_pMediaControl->Pause();
		
	return (m_pMediaControl->Run() == S_OK)? true : false;
}

const bool CMediaViewer::Stop(void)
{
	if(!m_pMediaControl)return false;

	// フィルタグラフを停止する
	return (m_pMediaControl->Stop() == S_OK)? true : false;
}

const bool CMediaViewer::SetVideoPID(const WORD wPID)
{
	// 映像出力ピンにPIDをマッピングする
	if(!m_pMp2DemuxVideoMap)return false;

	if(wPID == m_wVideoEsPID)return true;

	DWORD dwTempPID;

	// 現在のPIDをアンマップ
	if(m_wVideoEsPID != 0xFFFFU){
		dwTempPID = (DWORD)m_wVideoEsPID;
		if(m_pMp2DemuxVideoMap->UnmapPID(1UL, &dwTempPID) != S_OK)return false;
		}

	// 新規にPIDをマップ
	if(wPID != 0xFFFFU){
		dwTempPID = wPID;
		if(m_pMp2DemuxVideoMap->MapPID(1UL, &dwTempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)return false;
		}

	m_wVideoEsPID = wPID;
	
	return true;
}

const bool CMediaViewer::SetAudioPID(const WORD wPID)
{
	// 音声出力ピンにPIDをマッピングする
	if(!m_pMp2DemuxAudioMap)return false;

	if(wPID == m_wAudioEsPID)return true;

	DWORD dwTempPID;

	// 現在のPIDをアンマップ
	if(m_wAudioEsPID != 0xFFFFU){
		dwTempPID = (DWORD)m_wAudioEsPID;
		if(m_pMp2DemuxAudioMap->UnmapPID(1UL, &dwTempPID) != S_OK)return false;
		}

	// 新規にPIDをマップ
	if(wPID != 0xFFFFU){
		dwTempPID = wPID;
		if(m_pMp2DemuxAudioMap->MapPID(1UL, &dwTempPID, MEDIA_ELEMENTARY_STREAM) != S_OK)return false;
		}

	m_wAudioEsPID = wPID;
	
	return true;
}

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

void CMediaViewer::RemoveFromRot(const DWORD pdwRegister) const
{
	// デバッグ用
	IRunningObjectTable *pROT;

	if(SUCCEEDED(::GetRunningObjectTable(0, &pROT))){
		pROT->Revoke(pdwRegister);
		pROT->Release();
		}
}
