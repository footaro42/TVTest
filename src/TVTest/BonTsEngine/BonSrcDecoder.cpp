// BonSrcDecoder.cpp: CBonSrcDecoder クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <TypeInfo.h>
#include "BonSrcDecoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


typedef IBonDriver* (PFCREATEBONDRIVER)(void);


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

using namespace std;

CBonSrcDecoder::CBonSrcDecoder(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 0UL, 1UL)
	, m_pBonDriver(NULL)
	, m_pBonDriver2(NULL)
	, m_hStreamRecvThread(NULL)
	, m_bPauseSignal(false)
	, m_bResumeSignal(false)
	, m_bKillSignal(false)
	, m_TsStream(0x10000UL)
	, m_bIsPlaying(false)
	, m_dwLastError(BSDEC_NOERROR)
	, m_BitRate(0)
	, m_StreamRemain(0)
{

}

CBonSrcDecoder::~CBonSrcDecoder()
{
	CloseTuner();
}

void CBonSrcDecoder::Reset(void)
{
	CBlockLock Lock(&m_CriticalLock);

	PauseStreamRecieve();

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();

	ResumeStreamRecieve();
}

const bool CBonSrcDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	// ソースデコーダのため入力は処理しない
	return false;
}

const bool CBonSrcDecoder::OpenTuner(HMODULE hBonDrvDll)
{
	CBlockLock Lock(&m_CriticalLock);
	
	// オープンチェック
	if(m_pBonDriver){
		m_dwLastError = BSDEC_ALREADYOPEN;
		return false;
		}

	// ドライバポインタの取得
	PFCREATEBONDRIVER *pf=(PFCREATEBONDRIVER*)GetProcAddress(hBonDrvDll,"CreateBonDriver");
	if(!pf){
		m_dwLastError = BSDEC_DRIVERERROR;
		return false;
		}

	// ドライバインスタンス生成
	if(!(m_pBonDriver = pf())){
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
		}

	// IBonDriver2インタフェース取得
	try{
		m_pBonDriver2 = dynamic_cast<IBonDriver2 *>(m_pBonDriver);
		}
	catch(std::__non_rtti_object){
		m_pBonDriver2 = NULL;
		}

	// チューナを開く
	if(!m_pBonDriver->OpenTuner()){
		CloseTuner();
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
		}
	
	// 初期チャンネルをセットする
	/*
	if(m_pBonDriver2){
		// IBonDriver2
		if(!m_pBonDriver2->SetChannel(0UL, 0UL)){
			CloseTuner();
			m_dwLastError = BSDEC_TUNERERROR;
			return false;
			}
		}
	else{
		// IBonDriver
		if(!m_pBonDriver->SetChannel(13U)){
			CloseTuner();
			m_dwLastError = BSDEC_TUNERERROR;
			return false;
			}
		}
	*/

	// ストリーム受信スレッド起動
	DWORD dwThreadID = 0UL;

	m_bPauseSignal=false;
	m_bKillSignal=false;
	m_bIsPlaying = false;

	if(!(m_hStreamRecvThread = ::CreateThread(NULL, 0UL, CBonSrcDecoder::StreamRecvThread, (LPVOID)this, 0UL, &dwThreadID))){
		CloseTuner();
		m_dwLastError = BSDEC_INTERNALERROR;
		return false;
		}

	m_dwLastError = BSDEC_NOERROR;

	ResetBitRate();
	return true;
}

const bool CBonSrcDecoder::CloseTuner(void)
{
	CTryBlockLock Lock(&m_CriticalLock);
	// しばらく待っても入れない場合は恐らくデッドロックしているので、強制的に終了させる
	Lock.TryLock(2000);

	// ストリーム停止
	m_bIsPlaying = false;

	if (m_hStreamRecvThread) {
		// ストリーム受信スレッド停止
		m_bKillSignal=true;
		m_bPauseSignal=true;
		if (::WaitForSingleObject(m_hStreamRecvThread,1000UL)!=WAIT_OBJECT_0) {
			// スレッド強制終了
			TRACE(TEXT("Terminate stream recieve thread.\n"));
			::TerminateThread(m_hStreamRecvThread, 0UL);
		}
		::CloseHandle(m_hStreamRecvThread);
		m_hStreamRecvThread = NULL;
	}

	if (m_pBonDriver) {
		// チューナを閉じる
		m_pBonDriver->CloseTuner();

		// ドライバインスタンス開放
		m_pBonDriver->Release();
		m_pBonDriver = NULL;
		m_pBonDriver2 = NULL;
	}

	m_dwLastError = BSDEC_NOERROR;

	return true;
}

const bool CBonSrcDecoder::IsOpen() const
{
	return m_pBonDriver!=NULL;
}

const bool CBonSrcDecoder::Play(void)
{
	CTryBlockLock Lock(&m_CriticalLock);
	if (!Lock.TryLock(1000))
		return false;

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

	PauseStreamRecieve();

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();

	ResumeStreamRecieve();

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
	CTryBlockLock Lock(&m_CriticalLock);
	if (!Lock.TryLock(1000))
		return false;

	if(!m_pBonDriver){
		// チューナが開かれていない
		m_dwLastError = BSDEC_TUNERNOTOPEN;
		return false;
		}

	PauseStreamRecieve();

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// チャンネルを変更する
	if(!m_pBonDriver->SetChannel(byChannel)){
		ResumeStreamRecieve();
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
		}

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();

	ResumeStreamRecieve();

	m_dwLastError = BSDEC_NOERROR;

	return true;
}

const bool CBonSrcDecoder::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	if(!IsBonDriver2())return SetChannel((BYTE)dwChannel + 13U);

	CTryBlockLock Lock(&m_CriticalLock);
	if (!Lock.TryLock(1000))
		return false;

	if (!m_pBonDriver2) {
		// チューナが開かれていない
		m_dwLastError = BSDEC_TUNERNOTOPEN;
		return false;
	}

	PauseStreamRecieve();

	// 未処理のストリームを破棄する
	m_pBonDriver2->PurgeTsStream();
	// チャンネルを変更する
	if (!m_pBonDriver2->SetChannel(dwSpace, dwChannel)) {
		ResumeStreamRecieve();
		m_dwLastError = BSDEC_TUNERERROR;
		return false;
	}

	// 下位デコーダをリセットする
	CMediaDecoder::Reset();

	ResumeStreamRecieve();

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

const bool CBonSrcDecoder::IsBonDriver2(void) const
{
	// IBonDriver2インタフェースの使用可否を返す
	return (m_pBonDriver2)? true : false;
}

LPCTSTR CBonSrcDecoder::GetSpaceName(const DWORD dwSpace) const
{
	// チューニング空間名を返す
	return (IsBonDriver2())? m_pBonDriver2->EnumTuningSpace(dwSpace) : NULL;
}

LPCTSTR CBonSrcDecoder::GetChannelName(const DWORD dwSpace, const DWORD dwChannel) const
{
	// チャンネル名を返す
	return (IsBonDriver2())? m_pBonDriver2->EnumChannelName(dwSpace, dwChannel) : NULL;
}

const DWORD CBonSrcDecoder::GetLastError(void) const
{
	// 最後に発生したエラーを返す
	return m_dwLastError;
}

const bool CBonSrcDecoder::PurgeStream(void)
{
	CTryBlockLock Lock(&m_CriticalLock);
	if (!Lock.TryLock(1000))
		return false;

	if(!m_pBonDriver){
		// チューナが開かれていない
		m_dwLastError = BSDEC_TUNERNOTOPEN;
		return false;
		}

	PauseStreamRecieve();

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	ResumeStreamRecieve();

	m_dwLastError = BSDEC_NOERROR;

	return true;
}


void CBonSrcDecoder::OnTsStream(BYTE *pStreamData, DWORD dwStreamSize)
{
	//CBlockLock Lock(&m_CriticalLock);

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
	DWORD TotalSize=0;

	// チューナからTSデータを取り出すスレッド
	while (true) {
		while (!pThis->m_bPauseSignal) {
			// 処理簡略化のためポーリング方式を採用する
			do {
				if (pThis->m_pBonDriver->GetTsStream(&pStreamData,&dwStreamSize,&dwStreamRemain)
						&& pStreamData && dwStreamSize) {
					//pThis->OnTsStream(pStreamData, dwStreamSize);
					DWORD Remain,Size;

					for (Remain=dwStreamSize;Remain>0;Remain-=Size) {
						Size=min(Remain,188*64);
						pThis->OnTsStream(pStreamData+(dwStreamSize-Remain),Size);
					}
				}
				TotalSize+=dwStreamSize;

				// ビットレート計算
				DWORD Now=::GetTickCount();
				if (Now>=pThis->m_BitRateTime) {
					if (Now-pThis->m_BitRateTime>=1000) {
						pThis->m_BitRate=(TotalSize)*1000/(Now-pThis->m_BitRateTime);
						pThis->m_BitRateTime=Now;
						TotalSize=0;
					}
				} else {
					pThis->m_BitRateTime=Now;
					TotalSize=0;
				}
				pThis->m_StreamRemain=dwStreamRemain;
			} while (dwStreamRemain>0 && !pThis->m_bPauseSignal);

			if (pThis->m_bPauseSignal)
				break;

			// ウェイト(24Mbpsとして次のデータ到着まで約15msかかる)
			::Sleep(1UL);
			//pThis->m_pBonDriver->WaitTsStream(20);
		}
		if (pThis->m_bKillSignal)
			break;
		pThis->m_bResumeSignal=true;
		while (pThis->m_bPauseSignal)
			Sleep(1);
	}

	TRACE(TEXT("CBonSrcDecoder::StreamRecvThread() return\n"));

	return 0UL;
}


/*
	ストリームの受信を一時停止させる
	Purgeしたストリームを読みに行って落ちることがあるので、その対策
*/
void CBonSrcDecoder::PauseStreamRecieve()
{
	m_bResumeSignal=false;
	m_bPauseSignal=true;
	while (!m_bResumeSignal)
		Sleep(1);
	ResetBitRate();
}


void CBonSrcDecoder::ResumeStreamRecieve()
{
	m_bPauseSignal=false;
}


int CBonSrcDecoder::NumSpaces() const
{
	int i;

	for (i=0;GetSpaceName(i)!=NULL;i++);
	return i;
}


LPCTSTR CBonSrcDecoder::GetTunerName() const
{
	if (m_pBonDriver2)
		return m_pBonDriver2->GetTunerName();
	return NULL;
}


DWORD CBonSrcDecoder::GetBitRate() const
{
	return m_BitRate;
}


void CBonSrcDecoder::ResetBitRate()
{
	m_BitRateTime=GetTickCount();
	m_BitRate=0;

}


DWORD CBonSrcDecoder::GetStreamRemain() const
{
	return m_StreamRemain;
}
