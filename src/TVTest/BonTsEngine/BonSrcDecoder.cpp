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
	, m_bIsPlaying(false)
	, m_StreamRemain(0)
	, m_StreamThreadPriority(THREAD_PRIORITY_NORMAL)
	, m_bPurgeStreamOnChannelChange(true)
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

	if (!LockStream()) {
		Trace(TEXT("ストリーム受信スレッドが応答しません。"));
		return;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	UnlockStream();
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
		SetError(ERR_ALREADYOPEN, NULL);
		return false;
	}

	Trace(TEXT("チューナを開いています..."));

	// ドライバポインタの取得
	PFCREATEBONDRIVER *pfCreateBonDriver=(PFCREATEBONDRIVER*)::GetProcAddress(hBonDrvDll,"CreateBonDriver");
	if (pfCreateBonDriver == NULL) {
		SetError(ERR_DRIVER,TEXT("CreateBonDriver()のアドレスを取得できません。"),
							TEXT("指定されたDLLがBonDriverではありません。"));
		return false;
	}
	m_pBonDriver = pfCreateBonDriver();
	if (m_pBonDriver == NULL) {
		SetError(ERR_DRIVER,TEXT("IBonDriverを取得できません。"),
							TEXT("CreateBonDriver()の呼び出しでNULLが返されました。"));
		return false;
	}

	const HANDLE hThread = ::GetCurrentThread();
	const int ThreadPriority = ::GetThreadPriority(hThread);
	BOOL bTunerOpened;

	try {
		// チューナを開く
		bTunerOpened = m_pBonDriver->OpenTuner();

		// なぜかスレッドの優先度を変えるBonDriverがあるので元に戻す
		::SetThreadPriority(hThread, ThreadPriority);

		if (!bTunerOpened) {
			SetError(ERR_TUNEROPEN,TEXT("チューナを開けません。"),
								   TEXT("BonDriverにチューナを開くよう要求しましたがエラーが返されました。"));
			throw ERR_TUNEROPEN;
		}

		// IBonDriver2インタフェース取得
		m_pBonDriver2 = dynamic_cast<IBonDriver2 *>(m_pBonDriver);

		// ストリーム受信スレッド起動
		m_EndEvent.Create();
		m_bPauseSignal = false;
		m_bIsPlaying = false;
		m_hStreamRecvThread = (HANDLE)::_beginthreadex(NULL, 0, CBonSrcDecoder::StreamRecvThread, this, 0, NULL);
		if (!m_hStreamRecvThread) {
			SetError(ERR_INTERNAL, TEXT("ストリーム受信スレッドを作成できません。"));
			throw ERR_INTERNAL;
		}
	} catch (...) {
		if (bTunerOpened)
			m_pBonDriver->CloseTuner();
		m_pBonDriver->Release();
		m_pBonDriver = NULL;
		m_pBonDriver2 = NULL;
		return false;
	}

	ClearError();

	Trace(TEXT("チューナを開きました。"));

	return true;
}

const bool CBonSrcDecoder::CloseTuner(void)
{
	// ストリーム停止
	m_bIsPlaying = false;

	if (m_hStreamRecvThread) {
		// ストリーム受信スレッド停止
		Trace(TEXT("ストリーム受信スレッドを停止しています..."));
		m_EndEvent.Set();
		m_bPauseSignal = true;
		if (::WaitForSingleObject(m_hStreamRecvThread, 5000UL) != WAIT_OBJECT_0) {
			// スレッド強制終了
			Trace(TEXT("ストリーム受信スレッドが応答しないため強制終了します。"));
			::TerminateThread(m_hStreamRecvThread, -1);
		}
		::CloseHandle(m_hStreamRecvThread);
		m_hStreamRecvThread = NULL;
	}

	m_EndEvent.Close();

	if (m_pBonDriver) {
		// チューナを閉じる
		Trace(TEXT("チューナを閉じています..."));
		m_pBonDriver->CloseTuner();

		// ドライバインスタンス開放
		Trace(TEXT("BonDriverインターフェースを解放しています..."));
		m_pBonDriver->Release();
		m_pBonDriver = NULL;
		m_pBonDriver2 = NULL;
		Trace(TEXT("BonDriverインターフェースを解放しました。"));
	}

	m_BitRateCalculator.Reset();
	m_StreamRemain = 0;

	ClearError();

	return true;
}

const bool CBonSrcDecoder::IsOpen() const
{
	return m_pBonDriver != NULL;
}

const bool CBonSrcDecoder::Play(void)
{
	TRACE(TEXT("CBonSrcDecoder::Play()\n"));

	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
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

	if (!LockStream()) {
		SetError(ERR_TIMEOUT, TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	// ストリームを再生状態にする
	m_bIsPlaying = true;

	UnlockStream();

	ClearError();

	return true;
}

const bool CBonSrcDecoder::Stop(void)
{
	TRACE(TEXT("CBonSrcDecoder::Stop()\n"));

	if (!m_bIsPlaying) {
		// ストリームは再生中でない
		/*
		SetError(ERR_NOTPLAYING, NULL);
		return false;
		*/
		return true;
	}

	if (!LockStream()) {
		SetError(ERR_TIMEOUT, TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	m_bIsPlaying = false;

	UnlockStream();

	ClearError();

	return true;
}

const bool CBonSrcDecoder::SetChannel(const BYTE byChannel)
{
	TRACE(TEXT("CBonSrcDecoder::SetChannel(%d)\n"), byChannel);

	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (!LockStream()) {
		SetError(ERR_TIMEOUT, TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	if (m_bPurgeStreamOnChannelChange)
		m_pBonDriver->PurgeTsStream();

	// チャンネルを変更する
	if (!m_pBonDriver->SetChannel(byChannel)) {
		UnlockStream();
		SetError(ERR_TUNER,TEXT("チャンネルの変更がBonDriverに受け付けられません。"),
						   TEXT("IBonDriver::SetChannel()の呼び出しでエラーが返されました。"));
		return false;
	}

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	UnlockStream();

	ClearError();

	return true;
}

const bool CBonSrcDecoder::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	TRACE(TEXT("CBonSrcDecoder::SetChannel(%lu, %lu)\n"), dwSpace, dwChannel);

	if (m_pBonDriver2 == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (!LockStream()) {
		SetError(ERR_TIMEOUT, TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	if (m_bPurgeStreamOnChannelChange)
		m_pBonDriver2->PurgeTsStream();

	// チャンネルを変更する
	if (!m_pBonDriver2->SetChannel(dwSpace, dwChannel)) {
		UnlockStream();
		SetError(ERR_TUNER,TEXT("チャンネルの変更がBonDriverに受け付けられません。"),
						   TEXT("IBonDriver2::SetChannel()の呼び出しでエラーが返されました。"));
		return false;
	}

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	UnlockStream();

	ClearError();

	return true;
}

const bool CBonSrcDecoder::SetChannelAndPlay(const DWORD dwSpace, const DWORD dwChannel)
{
	TRACE(TEXT("CBonSrcDecoder::SetChannelAndPlay(%lu, %lu)\n"), dwSpace, dwChannel);

	if (m_pBonDriver2 == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (!LockStream()) {
		SetError(ERR_TIMEOUT, TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	if (m_bPurgeStreamOnChannelChange)
		m_pBonDriver2->PurgeTsStream();

	// チャンネルを変更する
	if (!m_pBonDriver2->SetChannel(dwSpace, dwChannel)) {
		UnlockStream();
		SetError(ERR_TUNER,TEXT("チャンネルの変更がBonDriverに受け付けられません。"),
						   TEXT("IBonDriver2::SetChannel()の呼び出しでエラーが返されました。"));
		return false;
	}

	// 下位デコーダをリセットする
	ResetDownstreamDecoder();

	m_bIsPlaying = true;

	UnlockStream();

	ClearError();

	return true;
}

const float CBonSrcDecoder::GetSignalLevel(void)
{
	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
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
		//SetError(ERR_NOTOPEN, NULL);
		return NULL;
	}
	return m_pBonDriver2->EnumTuningSpace(dwSpace);
}

LPCTSTR CBonSrcDecoder::GetChannelName(const DWORD dwSpace, const DWORD dwChannel) const
{
	// チャンネル名を返す
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return NULL;
	}
	return m_pBonDriver2->EnumChannelName(dwSpace, dwChannel);
}

const bool CBonSrcDecoder::PurgeStream(void)
{
	TRACE(TEXT("CBonSrcDecoder::PurgeStream()\n"));

	if (m_pBonDriver == NULL) {
		// チューナが開かれていない
		SetError(ERR_NOTOPEN, NULL);
		return false;
	}

	if (!LockStream()) {
		SetError(ERR_TIMEOUT, TEXT("ストリーム受信スレッドが応答しません。"));
		return false;
	}

	// 未処理のストリームを破棄する
	m_pBonDriver->PurgeTsStream();

	UnlockStream();

	ClearError();

	return true;
}

int CBonSrcDecoder::NumSpaces() const
{
	int i;

	for (i = 0; GetSpaceName(i) != NULL; i++);
	return i;
}

LPCTSTR CBonSrcDecoder::GetTunerName() const
{
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return NULL;
	}
	return m_pBonDriver2->GetTunerName();
}

int CBonSrcDecoder::GetCurSpace() const
{
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return -1;
	}
	return m_pBonDriver2->GetCurSpace();
}

int CBonSrcDecoder::GetCurChannel() const
{
	if (m_pBonDriver2 == NULL) {
		//SetError(ERR_NOTOPEN, NULL);
		return -1;
	}
	return m_pBonDriver2->GetCurChannel();
}

DWORD CBonSrcDecoder::GetBitRate() const
{
	return m_BitRateCalculator.GetBitRate();
}

DWORD CBonSrcDecoder::GetStreamRemain() const
{
	return m_StreamRemain;
}

bool CBonSrcDecoder::SetStreamThreadPriority(int Priority)
{
	if (m_StreamThreadPriority != Priority) {
		TRACE(TEXT("CBonSrcDecoder::SetStreamThreadPriority(%d)\n"), Priority);
		if (m_hStreamRecvThread) {
			if (!::SetThreadPriority(m_hStreamRecvThread, Priority))
				return false;
		}
		m_StreamThreadPriority = Priority;
	}
	return true;
}

void CBonSrcDecoder::SetPurgeStreamOnChannelChange(bool bPurge)
{
	TRACE(TEXT("CBonSrcDecoder::SetPurgeStreamOnChannelChange(%s)\n"),
		  bPurge ? TEXT("true") : TEXT("false"));
	m_bPurgeStreamOnChannelChange = bPurge;
}

unsigned int __stdcall CBonSrcDecoder::StreamRecvThread(LPVOID pParam)
{
	// チューナからTSデータを取り出すスレッド
	CBonSrcDecoder *pThis = static_cast<CBonSrcDecoder *>(pParam);

	::CoInitialize(NULL);

	::SetThreadPriority(::GetCurrentThread(),pThis->m_StreamThreadPriority);

	pThis->m_BitRateCalculator.Initialize();

	CMediaData TsStream(0x10000UL);

	DWORD Wait;
	do {
		if (pThis->m_bPauseSignal) {
			Wait = 100;
		} else {
			BYTE *pStreamData = NULL;
			DWORD dwStreamSize = 0;
			DWORD dwStreamRemain = 0;

			pThis->m_StreamLock.Lock();
			if (pThis->m_pBonDriver->GetTsStream(&pStreamData, &dwStreamSize, &dwStreamRemain)
					&& pStreamData && dwStreamSize) {
				if (pThis->m_bIsPlaying) {
					// 最上位デコーダに入力する
					TsStream.SetData(pStreamData, dwStreamSize);
					pThis->OutputMedia(&TsStream);
				}
			} else {
				dwStreamSize = 0;
				dwStreamRemain = 0;
			}
			pThis->m_StreamLock.Unlock();

			pThis->m_BitRateCalculator.Update(dwStreamSize);
			pThis->m_StreamRemain = dwStreamRemain;

			if (dwStreamRemain != 0)
				Wait = 0;
			else
				Wait = 10;
		}
	} while (pThis->m_EndEvent.Wait(Wait) == WAIT_TIMEOUT);

	pThis->m_BitRateCalculator.Reset();

	::CoUninitialize();

	TRACE(TEXT("CBonSrcDecoder::StreamRecvThread() return\n"));

	return 0;
}

bool CBonSrcDecoder::LockStream()
{
	m_bPauseSignal = true;
	bool bOK = m_StreamLock.TryLock(5000);
	m_bPauseSignal = false;
	return bOK;
}

void CBonSrcDecoder::UnlockStream()
{
	m_StreamLock.Unlock();
}
