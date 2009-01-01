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
	if (m_pBonDriver == NULL)
		return;

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
		SetError(ERR_ALREADYOPEN,NULL);
		return false;
	}

	Trace(TEXT("チューナを開いています..."));

	// ドライバポインタの取得
	PFCREATEBONDRIVER *pf=(PFCREATEBONDRIVER*)GetProcAddress(hBonDrvDll,"CreateBonDriver");
	if (pf == NULL) {
		SetError(ERR_DRIVER,TEXT("CreateBonDriver()のアドレスを取得できません。"),
							TEXT("指定されたDLLがBonDriverではありません。"));
		return false;
	}
	if ((m_pBonDriver = pf()) == NULL) {
		SetError(ERR_DRIVER,TEXT("IBonDriverを取得できません。"),
							TEXT("CreateBonDriver()の呼び出しでNULLが返されました。"));
		return false;
	}

	// チューナを開く
	if (!m_pBonDriver->OpenTuner()) {
		SetError(ERR_TUNEROPEN,TEXT("チューナを開けません。"),
							   TEXT("IBonDriver::OpenTuner()の呼び出しでエラーが返されました。"));
		goto OnError;
	}

	// BonDriver_HDUSが、なぜか勝手にスレッドの優先度をHIGHESTにするので元に戻す
	::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_NORMAL);

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
		SetError(ERR_INTERNAL,TEXT("ストリーム受信スレッドを作成できません。"));
		goto OnError;
	}

	ClearError();

	Trace(TEXT("チューナを開きました。"));

	return true;

OnError:
	m_pBonDriver->CloseTuner();
	m_pBonDriver->Release();
	m_pBonDriver = NULL;
	m_pBonDriver2 = NULL;
	if (m_hResumeEvent) {
		::CloseHandle(m_hResumeEvent);
		m_hResumeEvent=NULL;
	}
	return false;
}

const bool CBonSrcDecoder::CloseTuner(void)
{
	// ストリーム停止
	m_bIsPlaying = false;

	if (m_hStreamRecvThread) {
		// ストリーム受信スレッド停止
		Trace(TEXT("ストリーム受信スレッドを停止しています..."));
		m_bKillSignal=true;
		m_bPauseSignal=true;
		if (::WaitForSingleObject(m_hStreamRecvThread, 1000UL) != WAIT_OBJECT_0) {
			// スレッド強制終了
			::TerminateThread(m_hStreamRecvThread, 0UL);
			Trace(TEXT("ストリーム受信スレッドを強制終了しました。"));
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
		Trace(TEXT("チューナを閉じています..."));
		m_pBonDriver->CloseTuner();

		// ドライバインスタンス開放
		Trace(TEXT("ドライバを解放しています..."));
		m_pBonDriver->Release();
		m_pBonDriver = NULL;
		m_pBonDriver2 = NULL;
		Trace(TEXT("チューナを閉じました。"));
	}

	ClearError();

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
		SetError(ERR_NOTOPEN,NULL);
		return false;
	}

	if (m_bIsPlaying) {
		// 既に再生中
		/*
		SetError(ERR_ALREADYPLAYING,NULL);
		return false;
		*/
		return true;
	}

	if (!PauseStreamRecieve()) {
		SetError(ERR_TIMEOUT,TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	// ストリームを再生状態にする
	m_bIsPlaying = true;

	ResumeStreamRecieve();

	ClearError();

	return true;
}

const bool CBonSrcDecoder::Stop(void)
{
	if (!m_bIsPlaying) {
		// ストリームは再生中でない
		/*
		SetError(ERR_NOTPLAYING,NULL);
		return false;
		*/
		return true;
	}

	if (!PauseStreamRecieve()) {
		SetError(ERR_TIMEOUT,TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	m_bIsPlaying = false;

	ResumeStreamRecieve();

	ClearError();

	return true;
}

const bool CBonSrcDecoder::SetChannel(const BYTE byChannel)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN,NULL);
		return false;
	}

	if (!PauseStreamRecieve()) {
		SetError(ERR_TIMEOUT,TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// チャンネルを変更する
	if (!m_pBonDriver->SetChannel(byChannel)) {
		ResumeStreamRecieve();
		SetError(ERR_TUNER,TEXT("チャンネルの変更ができません。"),
						   TEXT("IBonDriver::SetChannel()の呼び出しでエラーが返されました。"));
		return false;
	}

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	ResumeStreamRecieve();

	ClearError();

	return true;
}

const bool CBonSrcDecoder::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	if (m_pBonDriver2 == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN,NULL);
		return false;
	}

	if (!PauseStreamRecieve()) {
		SetError(ERR_TIMEOUT,TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver2->PurgeTsStream();

	// チャンネルを変更する
	if (!m_pBonDriver2->SetChannel(dwSpace, dwChannel)) {
		ResumeStreamRecieve();
		SetError(ERR_TUNER,TEXT("チャンネルの変更ができません。"),
						   TEXT("IBonDriver2::SetChannel()の呼び出しでエラーが返されました。"));
		return false;
	}

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	ResumeStreamRecieve();

	ClearError();

	return true;
}

const float CBonSrcDecoder::GetSignalLevel(void)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN,NULL);
		return 0.0f;
	}

	ClearError();

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

const bool CBonSrcDecoder::PurgeStream(void)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN,NULL);
		return false;
	}

	if (!PauseStreamRecieve()) {
		SetError(ERR_TIMEOUT,TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	ResumeStreamRecieve();

	ClearError();

	return true;
}


void CBonSrcDecoder::OnTsStream(BYTE *pStreamData, DWORD dwStreamSize)
{
	if (m_bIsPlaying) {
		// 最上位デコーダに入力する
		m_TsStream.SetData(pStreamData, dwStreamSize);
		OutputMedia(&m_TsStream);
	}
}


DWORD WINAPI CBonSrcDecoder::StreamRecvThread(LPVOID pParam)
{
	CBonSrcDecoder *pThis = static_cast<CBonSrcDecoder *>(pParam);

	::CoInitialize(NULL);

	BYTE *pStreamData = NULL;
	DWORD dwStreamSize = 0UL;
	DWORD dwStreamRemain = 0UL;

	// チューナからTSデータを取り出すスレッド
	while (true) {
		DWORD BitRateTime=::GetTickCount();
		DWORD TotalSize=0;
		pThis->m_BitRate=0;

		while (!pThis->m_bPauseSignal) {
			// 処理簡略化のためポーリング方式を採用する
			do {
				if (pThis->m_pBonDriver->GetTsStream(&pStreamData,&dwStreamSize,&dwStreamRemain)
						&& pStreamData && dwStreamSize) {
#if 1
					pThis->OnTsStream(pStreamData, dwStreamSize);
#else
					// 一度に送るサイズを小さくする程CPU使用率の変動が少なくなる
					DWORD Remain,Size;

					for (Remain=dwStreamSize;Remain>0;Remain-=Size) {
						Size=min(Remain,188*64);
						pThis->OnTsStream(pStreamData+(dwStreamSize-Remain),Size);
					}
#endif
					TotalSize+=dwStreamSize;
				}

				// ビットレート計算
				DWORD Now=::GetTickCount();
				if (Now>=BitRateTime) {
					if (Now-BitRateTime>=1000) {
						pThis->m_BitRate=(DWORD)(((ULONGLONG)TotalSize*8*1000)/(ULONGLONG)(Now-BitRateTime));
						BitRateTime=Now;
						TotalSize=0;
					}
				} else {
					BitRateTime=Now;
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
		::SetEvent(pThis->m_hResumeEvent);
		while (pThis->m_bPauseSignal)
			Sleep(1);
		::SetEvent(pThis->m_hResumeEvent);
	}

	pThis->m_BitRate=0;

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
	return true;
}


bool CBonSrcDecoder::ResumeStreamRecieve(DWORD TimeOut)
{
	m_bPauseSignal = false;
	if (::WaitForSingleObject(m_hResumeEvent, TimeOut) == WAIT_TIMEOUT)
		return false;
	return true;
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


DWORD CBonSrcDecoder::GetStreamRemain() const
{
	return m_StreamRemain;
}
