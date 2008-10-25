// DtvEngine.h: CDtvEngine クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "BonSrcDecoder.h"
#include "TsPacketParser.h"
#include "TsDescrambler.h"
#include "ProgManager.h"
#include "MediaViewer.h"
#include "MediaTee.h"
#include "FileWriter.h"
#include "FileReader.h"
#include "MediaBuffer.h"
#include "Exception.h"
#include "../Tracer.h"

// ※この辺は全くの暫定です


//////////////////////////////////////////////////////////////////////
// デジタルTVエンジンクラス
//////////////////////////////////////////////////////////////////////

class CDtvEngineHandler;

class CDtvEngine : protected CMediaDecoder::IEventHandler, public CBonErrorHandler
{
public:
	enum EVENTID
	{
		EID_SERVICE_LIST_UPDATED,	// サービスリスト更新
		EID_SERVICE_INFO_UPDATED,	// サービス情報更新
		EID_PCR_TIMESTAMP_UPDATED,	// PCRタイムスタンプ更新(かなり頻繁)
		EID_FILE_WRITE_ERROR
	};

	CDtvEngine(void);
	~CDtvEngine(void);

	const bool BuildEngine(CDtvEngineHandler *pDtvEngineHandler,
								bool bDescramble=true,bool bBuffering=false);
	const bool IsEngineBuild() const { return m_bBuiled; };
	const bool IsBuildComplete() const { return m_bBuildComplete; }
	const bool CloseEngine(void);
	const bool ResetEngine(void);

	const bool OpenSrcFilter_BonDriver(HMODULE hBonDriverDll);
	const bool OpenSrcFilter_File(LPCTSTR lpszFileName);
	const bool ReleaseSrcFilter();
	const bool IsSrcFilterOpen() const;

	const bool EnablePreview(const bool bEnable = true);
	const bool SetViewSize(const int x,const int y);
	const bool SetVolume(const float fVolume);
	const bool GetVideoSize(WORD *pwWidth,WORD *pwHeight);
	const bool GetVideoAspectRatio(BYTE *pbyAspectRateX,BYTE *pbyAspectRateY);
	const BYTE GetAudioChannelNum();
	const bool SetStereoMode(int iMode);
	const bool GetVideoDecoderName(LPWSTR lpName,int iBufLen);
	const bool DisplayVideoDecoderProperty(HWND hWndParent);

	const bool SetChannel(const BYTE byTuningSpace, const WORD wChannel);
	const bool SetService(const WORD wService);
	const WORD GetService(void) const;
	const unsigned __int64 GetPcrTimeStamp() const;

	/*
	const bool PlayFile(LPCTSTR lpszFileName);
	void StopFile(void);
	*/

	// Append by HDUSTestの中の人
	bool BuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer=CVideoRenderer::RENDERER_DEFAULT,
		LPCWSTR pszMpeg2Decoder=NULL);
	bool RebuildMediaViewer(HWND hwndHost,HWND hwndMessage,
		CVideoRenderer::RendererType VideoRenderer=CVideoRenderer::RENDERER_DEFAULT,
		LPCWSTR pszMpeg2Decoder=NULL);
	bool OpenBcasCard(CCardReader::ReaderType CardReaderType);
	bool SetDescramble(bool bDescramble);
	bool ResetBuffer();
	bool GetOriginalVideoSize(WORD *pWidth,WORD *pHeight) const;
	bool SetDescrambleService(WORD Service);
	bool SetDescrambleCurServiceOnly(bool bOnly);
	bool GetDescrambleCurServiceOnly() const { return m_bDescrambleCurServiceOnly; }
	CEpgDataInfo *GetEpgDataInfo(WORD wSID, bool bNext);
	bool SetTracer(CTracer *pTracer);

//protected:
	// IMediaDecoder から派生したメディアデコーダクラス
	CBonSrcDecoder m_BonSrcDecoder;			// TSソースチューナー(HAL化すべき)
	CTsPacketParser m_TsPacketParser;		// TSパケッタイザー
	CTsDescrambler m_TsDescrambler;			// TSデスクランブラー
	CProgManager m_ProgManager;				// TSプログラムマネージャー
	CMediaViewer m_MediaViewer;				// メディアビューアー
	CMediaTee m_MediaTee;					// メディアティー
	CFileWriter m_FileWriter;				// ファイルライター
	CFileReader m_FileReader;				// ファイルリーダー
	CMediaBuffer m_MediaBuffer;

protected:
	const DWORD SendDtvEngineEvent(const DWORD dwEventID, PVOID pParam = NULL);
	virtual const DWORD OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam);
	bool CheckBuildComplete() const;

	CDtvEngineHandler *m_pDtvEngineHandler;
	WORD m_wCurTransportStream;
	WORD m_wCurService;
	WORD m_wCurVideoPID;
	WORD m_wCurAudioPID;
	unsigned __int64 m_u64CurPcrTimeStamp;

	bool m_bBuiled;
	bool m_bBuildComplete;
	bool m_bIsFileMode;
	bool m_bDescramble;

	bool m_bDescrambleCurServiceOnly;

	CTracer *m_pTracer;
	void Trace(LPCTSTR pszOutput, ...);
	void ResetParams();
};


//////////////////////////////////////////////////////////////////////
// デジタルTVイベントハンドラインタフェース
//////////////////////////////////////////////////////////////////////

// これは純粋仮想関数とすべき
class CDtvEngineHandler
{
friend CDtvEngine;

protected:
	virtual const DWORD OnDtvEngineEvent(CDtvEngine *pEngine, const DWORD dwEventID, PVOID pParam);
};
