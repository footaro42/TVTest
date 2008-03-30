// BonSrcDecoder.cpp: CBonSrcDecoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BonSrcDecoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma comment(lib, "BonDriver.lib")


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CBonSrcDecoder::CBonSrcDecoder(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler)
	, m_pBonDriver(NULL)
	, m_hStreamRecvThread(NULL)
	, m_TsStream(0x10000UL)
	, m_bKillSignal(true)
	, m_bIsPlaying(false)
	, m_dwLastError(BSDEC_NOERROR)
{

}

CBonSrcDecoder::~CBonSrcDecoder()
{
	CloseTuner();
}

void CBonSrcDecoder::Reset(void)
{
	CBlockLock Lock(&m_CriticalLock);

	// 未処理のストリームを破棄する
	if(m_bIsPlaying){
		// 未処理のストリームを破棄する
		m_pBonDriver->PurgeTsStream();
		}

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();
}

const DWORD CBonSrcDecoder::GetInputNum(void) const
{
	return 0UL;
}

const DWORD CBonSrcDecoder::GetOutputNum(void) const
{
	return 1UL;
}

const bool CBonSrcDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	// ソースデコーダのため入力は処理しない
	return false;
}

const bool CBonSrcDecoder::OpenTuner(void)
{
	CBlockLock Lock(&m_CriticalLock);
	
	// オープンチェック
	if(m_pBonDriver){
		m_dwLastError = BSDEC_ALREADYOPEN;
		return false;
		}

	// ドライバインスタンス生成
	if(!(m_pBonDriver = ::CreateBonDriver())){
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
		}

	// チューナを開く
	if(!m_pBonDriver->OpenTuner()){
		CloseTuner();
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
		}

	// 初期チャンネルをセットする
	if(!m_pBonDriver->SetChannel(13U)){
		CloseTuner();
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
		}

	// ストリーム受信スレッド起動
	DWORD dwThreadID = 0UL;
	m_bKillSignal = false;
	m_bIsPlaying = false;

	if(!(m_hStreamRecvThread = ::CreateThread(NULL, 0UL, CBonSrcDecoder::StreamRecvThread, (LPVOID)this, 0UL, &dwThreadID))){
		CloseTuner();
		m_dwLastError = BSDEC_INTERNALERROR;
		return false;
		}

	m_dwLastError = BSDEC_NOERROR;

	return true;
}

const bool CBonSrcDecoder::CloseTuner(void)
{
	CBlockLock Lock(&m_CriticalLock);

	// ストリーム停止
	m_bIsPlaying = false;
	m_bKillSignal = true;

	if(m_hStreamRecvThread){
		// ストリーム受信スレッド停止
		if(::WaitForSingleObject(m_hStreamRecvThread, 1000UL) != WAIT_OBJECT_0){
			// スレッド強制終了
			::TerminateThread(m_hStreamRecvThread, 0UL);
			}

		::CloseHandle(m_hStreamRecvThread);
		m_hStreamRecvThread = NULL;
		}

	if(m_pBonDriver){
		// チューナを閉じる
		m_pBonDriver->CloseTuner();

		// ドライバインスタンス開放
		m_pBonDriver->Release();
		m_pBonDriver = NULL;		
		}

	m_dwLastError = BSDEC_NOERROR;

	return true;
}

const bool CBonSrcDecoder::Play(void)
{
	CBlockLock Lock(&m_CriticalLock);
	
	if(!m_pBonDriver){
		// チューナが開かれていない
		m_dwLastError = BSDEC_TUNERNOTOPEN;
		return false;
		}

	if(m_bIsPlaying){
		// 既に再生中
		m_dwLastError = BSDEC_ALREADYPLAYING;
		return false;
		}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();

	// ストリームを再生状態にする
	m_bIsPlaying = true;
	m_dwLastError = BSDEC_NOERROR;

	return true;
}

const bool CBonSrcDecoder::Stop(void)
{
	if(!m_bIsPlaying){
		// ストリームは再生中でない
		m_dwLastError = BSDEC_NOTPLAYING;
		return false;
		}

	m_bIsPlaying = false;
	m_dwLastError = BSDEC_NOERROR;

	return true;
}

const bool CBonSrcDecoder::SetChannel(const BYTE byChannel)
{
	CBlockLock Lock(&m_CriticalLock);

	if(!m_pBonDriver){
		// チューナが開かれていない
		m_dwLastError = BSDEC_TUNERNOTOPEN;
		return false;
		}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// チャンネルを変更する
	if(!m_pBonDriver->SetChannel(byChannel)){
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
		}

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();

	m_dwLastError = BSDEC_NOERROR;
	
	return true;
}

const float CBonSrcDecoder::GetSignalLevel(void)
{
	if(!m_pBonDriver){
		// チューナが開かれていない
		m_dwLastError = BSDEC_TUNERNOTOPEN;
		return 0.0f;
		}

	m_dwLastError = BSDEC_NOERROR;

	// 信号レベルを返す
	return m_pBonDriver->GetSignalLevel();
}

const DWORD CBonSrcDecoder::GetLastError(void) const
{
	// 最後に発生したエラーを返す
	return m_dwLastError;
}

const bool CBonSrcDecoder::PurgeStream(void)
{
	CBlockLock Lock(&m_CriticalLock);

	if(!m_pBonDriver){
		// チューナが開かれていない
		m_dwLastError = BSDEC_TUNERNOTOPEN;
		return false;
		}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();
	
	m_dwLastError = BSDEC_NOERROR;

	return true;
}

void CBonSrcDecoder::OnTsStream(BYTE *pStreamData, DWORD dwStreamSize)
{
	CBlockLock Lock(&m_CriticalLock);

	if(!m_bIsPlaying)return;

	// 最上位デコーダに入力する
	m_TsStream.SetData(pStreamData, dwStreamSize);
	OutputMedia(&m_TsStream);
}

DWORD WINAPI CBonSrcDecoder::StreamRecvThread(LPVOID pParam)
{
	CBonSrcDecoder *pThis = static_cast<CBonSrcDecoder *>(pParam);

	BYTE *pStreamData = NULL;
	DWORD dwStreamSize = 0UL;
	DWORD dwStreamRemain = 0UL;

	// チューナからTSデータを取り出すスレッド
	while(!pThis->m_bKillSignal){
		
		// 処理簡略化のためポーリング方式を採用する
		do{
			if(pThis->m_pBonDriver->GetTsStream(&pStreamData, &dwStreamSize, &dwStreamRemain)){
				if(pStreamData && dwStreamSize){
					pThis->OnTsStream(pStreamData, dwStreamSize);
					}
				}
			}
		while(dwStreamRemain);

		// ウェイト(24Mbpsとして次のデータ到着まで約15msかかる)
		::Sleep(1UL);
		}

	return 0UL;
}

