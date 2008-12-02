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

CBonSrcDecoder::CBonSrcDecoder(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 0UL, 1UL)
	, m_pBonDriver(NULL)
	, m_pBonDriver2(NULL)
	, m_hStreamRecvThread(NULL)
	, m_bPauseSignal(false)
	, m_hResumeEvent(NULL)
	, m_bKillSignal(false)
	, m_TsStream(0x10000UL)
	, m_bIsPlaying(false)
	, m_dwLastError(ERR_NOERROR)
	, m_BitRate(0)
	, m_StreamRemain(0)
	/*
	, m_RequestSpace(-1)
	, m_RequestChannel(-1)
	*/
{
}

CBonSrcDecoder::~CBonSrcDecoder()
{
	CloseTuner();
}

void CBonSrcDecoder::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	PauseStreamRecieve();

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	ResumeStreamRecieve();
}

const bool CBonSrcDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	// ソースデコーダのため入力は処理しない
	return false;
}

const bool CBonSrcDecoder::OpenTuner(HMODULE hBonDrvDll)
{
	// オープンチェック
	if (m_pBonDriver) {
		m_dwLastError = ERR_ALREADYOPEN;
		return false;
	}

	// ドライバポインタの取得
	PFCREATEBONDRIVER *pf=(PFCREATEBONDRIVER*)GetProcAddress(hBonDrvDll,"CreateBonDriver");
	if (!pf || (m_pBonDriver = pf()) == NULL) {
		m_dwLastError = ERR_DRIVER;
		return false;
	}

	// チューナを開く
	if (!m_pBonDriver->OpenTuner()) {
		m_dwLastError = ERR_TUNEROPEN;
		goto OnError;
	}

	// IBonDriver2インタフェース取得
	m_pBonDriver2 = dynamic_cast<IBonDriver2 *>(m_pBonDriver);

	/*
	// 初期チャンネルをセットする
	if (m_pBonDriver2) {
		// IBonDriver2
		if (!m_pBonDriver2->SetChannel(0UL, 0UL)) {
			m_dwLastError = ERR_TUNER;
			goto OnError;
		}
	} else {
		// IBonDriver
		if (!m_pBonDriver->SetChannel(13U)) {
			m_dwLastError = ERR_TUNER;
			goto OnError;
		}
	}
	*/

	// ストリーム受信スレッド起動
	m_bPauseSignal = false;
	m_hResumeEvent = ::CreateEvent(NULL,FALSE,FALSE,NULL);
	m_bKillSignal = false;
	m_bIsPlaying = false;
	m_hStreamRecvThread = ::CreateThread(NULL, 0UL, CBonSrcDecoder::StreamRecvThread, this, 0UL, NULL);
	if (!m_hStreamRecvThread) {
		m_dwLastError = ERR_INTERNAL;
		goto OnError;
	}

	m_dwLastError = ERR_NOERROR;

	ResetBitRate();

	return true;

OnError:
	m_pBonDriver->CloseTuner();
	m_pBonDriver->Release();
	m_pBonDriver = NULL;
	m_pBonDriver2 = NULL;
	return false;
}

const bool CBonSrcDecoder::CloseTuner(void)
{
	// ストリーム停止
	m_bIsPlaying = false;

	if (m_hStreamRecvThread) {
		// ストリーム受信スレッド停止
		m_bKillSignal=true;
		m_bPauseSignal=true;
		if (::WaitForSingleObject(m_hStreamRecvThread, 1000UL) != WAIT_OBJECT_0) {
			// スレッド強制終了
			TRACE(TEXT("Terminate stream recieve thread.\n"));
			::TerminateThread(m_hStreamRecvThread, 0UL);
		}
		::CloseHandle(m_hStreamRecvThread);
		m_hStreamRecvThread = NULL;
	}
	if (m_hResumeEvent) {
		::CloseHandle(m_hResumeEvent);
		m_hResumeEvent=NULL;
	}

	if (m_pBonDriver) {
		// チューナを閉じる
		m_pBonDriver->CloseTuner();

		// ドライバインスタンス開放
		m_pBonDriver->Release();
		m_pBonDriver = NULL;
		m_pBonDriver2 = NULL;
	}

	m_dwLastError = ERR_NOERROR;

	return true;
}

const bool CBonSrcDecoder::IsOpen() const
{
	return m_pBonDriver!=NULL;
}

const bool CBonSrcDecoder::Play(void)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		m_dwLastError = ERR_NOTOPEN;
		return false;
	}

	if (m_bIsPlaying) {
		// 既に再生中
		m_dwLastError = ERR_ALREADYPLAYING;
		return false;
	}

	if (!PauseStreamRecieve()) {
		m_dwLastError = ERR_TIMEOUT;
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	ResumeStreamRecieve();

	// ストリームを再生状態にする
	m_bIsPlaying = true;
	m_dwLastError = ERR_NOERROR;

	return true;
}

const bool CBonSrcDecoder::Stop(void)
{
	if (!m_bIsPlaying) {
		// ストリームは再生中でない
		m_dwLastError = ERR_NOTPLAYING;
		return false;
	}

	if (!PauseStreamRecieve()) {
		m_dwLastError = ERR_TIMEOUT;
		return false;
	}

	m_bIsPlaying = false;

	ResumeStreamRecieve();

	m_dwLastError = ERR_NOERROR;

	return true;
}

const bool CBonSrcDecoder::SetChannel(const BYTE byChannel)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		m_dwLastError = ERR_NOTOPEN;
		return false;
	}

	/*
	m_RequestChannel = byChannel;
	if (!PauseStreamRecieve()) {
		m_RequestChannel = -1;
		m_dwLastError = ERR_TIMEOUT;
		return false;
	}

	if (!m_bSetChannelResult) {
		ResumeStreamRecieve();
		m_dwLastError = ERR_TUNER;
		return false;
	}
	*/

	if (!PauseStreamRecieve()) {
		m_dwLastError = ERR_TIMEOUT;
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// チャンネルを変更する
	if (!m_pBonDriver->SetChannel(byChannel)) {
		ResumeStreamRecieve();
		m_dwLastError = ERR_TUNER;
		return false;
	}

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	ResumeStreamRecieve();

	m_dwLastError = ERR_NOERROR;

	return true;
}

const bool CBonSrcDecoder::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	if (m_pBonDriver2 == NULL) {
		// チューナが開かれていない
		m_dwLastError = ERR_NOTOPEN;
		return false;
	}

	/*
	m_RequestSpace = dwSpace;
	m_RequestChannel = dwChannel;
	if (!PauseStreamRecieve()) {
		m_RequestSpace = -1;
		m_RequestChannel = -1;
		m_dwLastError = ERR_TIMEOUT;
		return false;
	}

	if (!m_bSetChannelResult) {
		ResumeStreamRecieve();
		m_dwLastError = ERR_TUNER;
		return false;
	}
	*/

	if (!PauseStreamRecieve()) {
		m_dwLastError = ERR_TIMEOUT;
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver2->PurgeTsStream();

	// チャンネルを変更する
	if (!m_pBonDriver2->SetChannel(dwSpace, dwChannel)) {
		ResumeStreamRecieve();
		m_dwLastError = ERR_TUNER;
		return false;
	}

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	ResumeStreamRecieve();

	m_dwLastError = ERR_NOERROR;

	return true;
}

const float CBonSrcDecoder::GetSignalLevel(void)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		m_dwLastError = ERR_NOTOPEN;
		return 0.0f;
	}

	m_dwLastError = ERR_NOERROR;

	// 信号レベルを返す
	return m_pBonDriver->GetSignalLevel();
}

const bool CBonSrcDecoder::IsBonDriver2(void) const
{
	// IBonDriver2インタフェースの使用可否を返す
	return m_pBonDriver2 != NULL;
}

LPCTSTR CBonSrcDecoder::GetSpaceName(const DWORD dwSpace) const
{
	// チューニング空間名を返す
	if (m_pBonDriver2 == NULL) {
		//m_dwLastError = ERR_NOTOPEN;
		return NULL;
	}
	return m_pBonDriver2->EnumTuningSpace(dwSpace);
}

LPCTSTR CBonSrcDecoder::GetChannelName(const DWORD dwSpace, const DWORD dwChannel) const
{
	// チャンネル名を返す
	if (m_pBonDriver2 == NULL) {
		//m_dwLastError = ERR_NOTOPEN;
		return NULL;
	}
	return m_pBonDriver2->EnumChannelName(dwSpace, dwChannel);
}

const DWORD CBonSrcDecoder::GetLastError(void) const
{
	// 最後に発生したエラーを返す
	return m_dwLastError;
}

const bool CBonSrcDecoder::PurgeStream(void)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		m_dwLastError = ERR_NOTOPEN;
		return false;
	}

	if (!PauseStreamRecieve()) {
		m_dwLastError = ERR_TIMEOUT;
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	ResumeStreamRecieve();

	m_dwLastError = ERR_NOERROR;

	return true;
}


void CBonSrcDecoder::OnTsStream(BYTE *pStreamData, DWORD dwStreamSize)
{
	//CBlockLock Lock(&m_DecoderLock);

	if (!m_bIsPlaying)
		return;

	// 最上位デコーダに入力する
	m_TsStream.SetData(pStreamData, dwStreamSize);
	OutputMedia(&m_TsStream);
}


DWORD WINAPI CBonSrcDecoder::StreamRecvThread(LPVOID pParam)
{
	CBonSrcDecoder *pThis = static_cast<CBonSrcDecoder *>(pParam);

	::CoInitialize(NULL);

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
						// 一度に送るサイズを小さくする程CPU使用率の変動が少なくなる
						Size=min(Remain,188*64);
						pThis->OnTsStream(pStreamData+(dwStreamSize-Remain),Size);
					}
				}
				TotalSize+=dwStreamSize;

				// ビットレート計算
				DWORD Now=::GetTickCount();
				if (Now>=pThis->m_BitRateTime) {
					if (Now-pThis->m_BitRateTime>=1000) {
						pThis->m_BitRate=(DWORD)(((ULONGLONG)TotalSize*8*1000)/(ULONGLONG)(Now-pThis->m_BitRateTime));
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
			::Sleep(5UL);
			//pThis->m_pBonDriver->WaitTsStream(20);
		}
		if (pThis->m_bKillSignal)
			break;
		/*
		if (pThis->m_RequestChannel>=0) {
			pThis->m_pBonDriver->PurgeTsStream();
			if (pThis->m_RequestSpace>=0) {
				pThis->m_bSetChannelResult=pThis->m_pBonDriver2->SetChannel(
						pThis->m_RequestSpace,pThis->m_RequestChannel)!=FALSE;
				pThis->m_RequestSpace=-1;
			} else {
				pThis->m_bSetChannelResult=
					pThis->m_pBonDriver->SetChannel(pThis->m_RequestChannel)!=FALSE;
			}
			pThis->m_RequestChannel=-1;
		}
		*/
		::SetEvent(pThis->m_hResumeEvent);
		while (pThis->m_bPauseSignal)
			Sleep(1);
		::SetEvent(pThis->m_hResumeEvent);
	}

	::CoUninitialize();

	TRACE(TEXT("CBonSrcDecoder::StreamRecvThread() return\n"));

	return 0UL;
}


/*
	ストリームの受信を一時停止させる
	Purgeしたストリームを読みに行って落ちることがあるので、その対策
*/
bool CBonSrcDecoder::PauseStreamRecieve(DWORD TimeOut)
{
	::ResetEvent(m_hResumeEvent);
	m_bPauseSignal = true;
	if (::WaitForSingleObject(m_hResumeEvent, TimeOut) == WAIT_TIMEOUT)
		return false;
	ResetBitRate();
	return true;
}


void CBonSrcDecoder::ResumeStreamRecieve()
{
	m_bPauseSignal = false;
	::WaitForSingleObject(m_hResumeEvent, INFINITE);
}


int CBonSrcDecoder::NumSpaces() const
{
	int i;

	for (i=0; GetSpaceName(i)!=NULL; i++);
	return i;
}


LPCTSTR CBonSrcDecoder::GetTunerName() const
{
	if (m_pBonDriver2 == NULL) {
		//m_dwLastError = ERR_NOTOPEN;
		return NULL;
	}
	return m_pBonDriver2->GetTunerName();
}


int CBonSrcDecoder::GetCurSpace() const
{
	if (m_pBonDriver2 == NULL) {
		//m_dwLastError = ERR_NOTOPEN;
		return -1;
	}
	return m_pBonDriver2->GetCurSpace();
}


int CBonSrcDecoder::GetCurChannel() const
{
	if (m_pBonDriver2 == NULL) {
		//m_dwLastError = ERR_NOTOPEN;
		return -1;
	}
	return m_pBonDriver2->GetCurChannel();
}


DWORD CBonSrcDecoder::GetBitRate() const
{
	return m_BitRate;
}


void CBonSrcDecoder::ResetBitRate()
{
	m_BitRateTime = GetTickCount();
	m_BitRate=0;

}


DWORD CBonSrcDecoder::GetStreamRemain() const
{
	return m_StreamRemain;
}
