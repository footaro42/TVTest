// BonSrcDecoder.h: CBonSrcDecoder クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "IBonDriver.h"
#include "IBonDriver2.h"


/////////////////////////////////////////////////////////////////////////////
// Bonソースデコーダ(チューナからTS平分ストリームを受信する)
/////////////////////////////////////////////////////////////////////////////
// Output	#0	: CMediaData		平分TSストリーム
/////////////////////////////////////////////////////////////////////////////

class CBonSrcDecoder : public CMediaDecoder
{
public:
	// エラーコード
	enum {
		ERR_NOERROR,		// エラーなし
		ERR_DRIVER,			// ドライバエラー
		ERR_TUNEROPEN,		// チューナオープンエラー
		ERR_TUNER,			// チューナエラー
		ERR_NOTOPEN,		// チューナが開かれていない
		ERR_ALREADYOPEN,	// チューナが既に開かれている
		ERR_NOTPLAYING,		// 再生されていない
		ERR_ALREADYPLAYING,	// 既に再生されている
		ERR_TIMEOUT,		// タイムアウト
		ERR_INTERNAL		// 内部エラー
	};

	CBonSrcDecoder(IEventHandler *pEventHandler = NULL);
	virtual ~CBonSrcDecoder();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CBonSrcDecoder
	const bool OpenTuner(HMODULE hBonDrvDll);
	const bool CloseTuner(void);
	const bool IsOpen() const;

	const bool Play(void);
	const bool Stop(void);

	const bool SetChannel(const BYTE byChannel);
	const bool SetChannel(const DWORD dwSpace, const DWORD dwChannel);
	const bool SetChannelAndPlay(const DWORD dwSpace, const DWORD dwChannel);
	const float GetSignalLevel(void);

	const bool IsBonDriver2(void) const;
	LPCTSTR GetSpaceName(const DWORD dwSpace) const;
	LPCTSTR GetChannelName(const DWORD dwSpace, const DWORD dwChannel) const;

	const bool PurgeStream(void);

	// Append by HDUSTestの中の人
	int NumSpaces() const;
	LPCTSTR GetTunerName() const;
	int GetCurSpace() const;
	int GetCurChannel() const;
	DWORD GetBitRate() const;
	DWORD GetStreamRemain() const;
	bool SetStreamThreadPriority(int Priority);
	void SetPurgeStreamOnChannelChange(bool bPurge);

private:
	static DWORD WINAPI StreamRecvThread(LPVOID pParam);
	bool LockStream();
	void UnlockStream();

	IBonDriver *m_pBonDriver;
	IBonDriver2 *m_pBonDriver2;	

	HANDLE m_hStreamRecvThread;
	CCriticalLock m_StreamLock;
	volatile bool m_bKillSignal;
	volatile bool m_bPauseSignal;

	volatile bool m_bIsPlaying;
	DWORD m_dwLastError;

	CBitRateCalculator m_BitRateCalculator;
	DWORD m_StreamRemain;

	int m_StreamThreadPriority;
	bool m_bPurgeStreamOnChannelChange;
};
