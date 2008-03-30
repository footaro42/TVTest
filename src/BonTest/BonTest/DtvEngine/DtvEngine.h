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

// ※この辺は全くの暫定です


//////////////////////////////////////////////////////////////////////
// デジタルTVエンジンクラス
//////////////////////////////////////////////////////////////////////

class CDtvEngineHandler;

class CDtvEngine : protected CMediaDecoder::IEventHandler
{
public:
	CDtvEngine(void);
	~CDtvEngine(void);

	const bool OpenEngine(CDtvEngineHandler *pDtvEngineHandler, HWND hHostHwnd);
	const bool CloseEngine(void);
	const bool ResetEngine(void);
	
	const bool EnablePreview(const bool bEnable = true);

	const bool SetChannel(const BYTE byTuningSpace, const WORD wChannel);
	const bool SetService(const WORD wService);
	const WORD GetService(void) const;
	
	const bool PlayFile(LPCTSTR lpszFileName);
	void StopFile(void);
	
	const DWORD GetLastError(void) const;

//protected:
	const DWORD SendDtvEngineEvent(const DWORD dwEventID, PVOID pParam = NULL);
	virtual const DWORD OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam);

	// IMediaDecoder から派生したメディアデコーダクラス
	CBonSrcDecoder m_BonSrcDecoder;			// TSソースチューナー(HAL化すべき)
	CTsPacketParser m_TsPacketParser;		// TSパケッタイザー
	CTsDescrambler m_TsDescrambler;			// TSデスクランブラー
	CProgManager m_ProgManager;				// TSプログラムマネージャー
	CMediaViewer m_MediaViewer;				// メディアビューアー
	CMediaTee m_MediaTee;					// メディアティー
	CFileWriter m_FileWriter;				// ファイルライター
	CFileReader m_FileReader;				// ファイルリーダー

	CDtvEngineHandler *m_pDtvEngineHandler;
	WORD m_wCurService;

	bool m_bIsFileMode;
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
