// DtvEngine.cpp: CDtvEngine クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DtvEngine.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CDtvEngine 構築/消滅
//////////////////////////////////////////////////////////////////////

CDtvEngine::CDtvEngine(void)
	: m_pDtvEngineHandler(NULL)
	, m_wCurService(0U)
	, m_BonSrcDecoder(this)
	, m_TsPacketParser(this)
	, m_TsDescrambler(this)
	, m_ProgManager(this)
	, m_MediaViewer(this)
	, m_MediaTee(this)
	, m_FileWriter(this)
	, m_FileReader(this)
	, m_bIsFileMode(false)
{

}

CDtvEngine::~CDtvEngine(void)
{
	CloseEngine();
}

const bool CDtvEngine::OpenEngine(CDtvEngineHandler *pDtvEngineHandler, HWND hHostHwnd)
{
	// 完全に暫定
	CloseEngine();

	// グラフ構成図
	//
	// m_BonSrcDecoder or m_FileReader
	//		↓
	// m_TsPacketParser
	//		↓
	// m_TsDescrambler
	//		↓
	// m_MediaTee→→→→→→
	//		↓				↓
	// m_ProgManager	m_FileWriter
	//		↓
	// m_MediaViewer

	// デコーダグラフ構築
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);	// Output #0 : CMediaData
	m_TsPacketParser.SetOutputDecoder(&m_TsDescrambler);	// Output #0 : CTsPacket
	m_TsDescrambler.SetOutputDecoder(&m_MediaTee);			// Output #0 : CTsPacket
	m_MediaTee.SetOutputDecoder(&m_ProgManager, 0UL);		// Output #0 : CTsPacket
	m_MediaTee.SetOutputDecoder(&m_FileWriter, 1UL);		// Output #1 : CTsPacket
	m_ProgManager.SetOutputDecoder(&m_MediaViewer);			// Output #0 : CTsPacket

	// 一旦リセット
	ResetEngine();

	// イベントハンドラ設定
	m_pDtvEngineHandler = pDtvEngineHandler;

	try{
		// チューナを開く
		if(!m_BonSrcDecoder.OpenTuner())throw 1UL;
	
		// B-CASカードを開く
		if(!m_TsDescrambler.OpenBcasCard())throw 2UL;

		// ビューアを開く
		if(!m_MediaViewer.OpenViewer(hHostHwnd))throw 3UL;
		}
	catch(DWORD dwErrorNo){
		// エラー
		switch(dwErrorNo){
			case 1UL :	::AfxMessageBox(TEXT("チューナの初期化に失敗しました。"));		break;
			case 2UL :	::AfxMessageBox(TEXT("B-CASカードの初期化に失敗しました。"));	break;
			case 3UL :	::AfxMessageBox(TEXT("DirectShowの初期化に失敗しました。"));	break;
			}		
		
		CloseEngine();
		
		return false;
		}
	
	return true;
}

const bool CDtvEngine::CloseEngine(void)
{
	// デコーダクローズ
	m_BonSrcDecoder.CloseTuner();
	m_TsDescrambler.CloseBcasCard();
	m_MediaViewer.CloseViewer();

	// イベントハンドラ解除
	m_pDtvEngineHandler = NULL;

	return true;
}

const bool CDtvEngine::ResetEngine(void)
{
	// デコーダグラフリセット
	m_BonSrcDecoder.Reset();

	return true;
}

const bool CDtvEngine::EnablePreview(const bool bEnable)
{
	if(bEnable){
		// プレビュー有効
		if(!m_BonSrcDecoder.Play())return false;
		if(!m_MediaViewer.Play())return false;
		}
	else{
		// プレビュー無効
		if(!m_MediaViewer.Stop())return false;
		if(!m_BonSrcDecoder.Stop())return false;
		}

	return true;
}

const bool CDtvEngine::SetChannel(const BYTE byTuningSpace, const WORD wChannel)
{
	// チャンネル変更
	return  m_BonSrcDecoder.SetChannel((BYTE)wChannel);
}

const bool CDtvEngine::SetService(const WORD wService)
{
	// サービス変更
	if(wService < m_ProgManager.GetServiceNum()){
		WORD wVideoPID = 0xFFFF;
		WORD wAudioPID = 0xFFFF;
	
		m_wCurService = wService;
		m_ProgManager.GetServiceEsPID(&wVideoPID, &wAudioPID, m_wCurService);
		m_MediaViewer.SetVideoPID(wVideoPID);
		m_MediaViewer.SetAudioPID(wAudioPID);
		
		return true;
		}

	return false;
}

const WORD CDtvEngine::GetService(void) const
{
	// サービス取得
	return m_wCurService;
}

const bool CDtvEngine::PlayFile(LPCTSTR lpszFileName)
{
	// ※グラフ全体の排他制御を実装しないとまともに動かない！！

	// 再生中の場合は閉じる
	m_FileReader.StopReadAnsync();

	// スレッド終了を待つ
	while(m_FileReader.IsAnsyncReadBusy()){
		::Sleep(1UL);
		}

	m_FileReader.CloseFile();

	// デバイスから再生中の場合はグラフを再構築する
	if(!m_bIsFileMode){
		m_BonSrcDecoder.Stop();
		m_BonSrcDecoder.SetOutputDecoder(NULL);
		m_FileReader.SetOutputDecoder(&m_TsPacketParser);
		}

	try{
		// グラフリセット
		m_FileReader.Reset();

		// ファイルオープン
		if(!m_FileReader.OpenFile(lpszFileName))throw 0UL;
		
		// 非同期再生開始
		if(!m_FileReader.StartReadAnsync())throw 1UL;
		}
	catch(const DWORD dwErrorStep){
		// エラー発生
		StopFile();
		return false;
		}

	m_bIsFileMode = true;

	return true;
}

void CDtvEngine::StopFile(void)
{
	// ※グラフ全体の排他制御を実装しないとまともに動かない！！

	m_bIsFileMode = false;

	// 再生中の場合は閉じる
	m_FileReader.StopReadAnsync();
	
	// スレッド終了を待つ
	while(m_FileReader.IsAnsyncReadBusy()){
		::Sleep(1UL);
		}

	m_FileReader.CloseFile();
	
	// グラフを再構築する
	m_FileReader.SetOutputDecoder(NULL);
	m_BonSrcDecoder.SetOutputDecoder(&m_TsPacketParser);
	m_BonSrcDecoder.Play();
}

const DWORD CDtvEngine::GetLastError(void) const
{
	return 0UL;
}

const DWORD CDtvEngine::SendDtvEngineEvent(const DWORD dwEventID, PVOID pParam)
{
	// イベントハンドラにイベントを送信する
	if(m_pDtvEngineHandler){
		return m_pDtvEngineHandler->OnDtvEngineEvent(this, dwEventID, pParam);
		}

	return 0UL;
}

const DWORD CDtvEngine::OnDecoderEvent(CMediaDecoder *pDecoder, const DWORD dwEventID, PVOID pParam)
{
	// デコーダからのイベントを受け取る(暫定)
	if(pDecoder == &m_ProgManager){
		
		// プログラムマネージャからのイベント
		switch(dwEventID){
			case CProgManager::EID_SERVICE_LIST_UPDATED :
				// サービスの構成が変化した
				SetService(0U);
				SendDtvEngineEvent(0UL, static_cast<PVOID>(&m_ProgManager));

				return 0UL;
				
			case CProgManager::EID_SERVICE_INFO_UPDATED :
				// サービス名が更新された
				SendDtvEngineEvent(0UL, static_cast<PVOID>(&m_ProgManager));
				
				return 0UL;
			}
		}
	else if(pDecoder == &m_FileReader){
		
		CFileReader *pFileReader = dynamic_cast<CFileReader *>(pDecoder);
		
		// ファイルリーダからのイベント
		switch(dwEventID){
			case CFileReader::EID_READ_ASYNC_START :
				// 非同期リード開始
				return 0UL;
			
			case CFileReader::EID_READ_ASYNC_END :
				// 非同期リード終了
				return 0UL;
			
			case CFileReader::EID_READ_ASYNC_POSTREAD :
				// 非同期リード後
				
				if(pFileReader->GetReadPos() >= pFileReader->GetFileSize()){
					// 最初に巻き戻す(ループ再生)
					pFileReader->SetReadPos(0ULL);
					pFileReader->Reset();
					}
				
				return 0UL;
			}		
		}

	return 0UL;
}


//////////////////////////////////////////////////////////////////////
// CDtvEngineHandler 構築/消滅
//////////////////////////////////////////////////////////////////////

const DWORD CDtvEngineHandler::OnDtvEngineEvent(CDtvEngine *pEngine, const DWORD dwEventID, PVOID pParam)
{
	// デフォルトの処理
	return 0UL;
}
