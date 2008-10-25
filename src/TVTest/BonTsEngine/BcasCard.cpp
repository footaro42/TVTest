// BcasCard.cpp: CBcasCard クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "BcasCard.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CBcasCard::CBcasCard()
	: m_pCardReader(NULL)
	, m_dwLastError(BCEC_NOERROR)
{
	// 内部状態初期化
	::ZeroMemory(&m_BcasCardInfo, sizeof(m_BcasCardInfo));
	::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));
}


CBcasCard::~CBcasCard()
{
	CloseCard();
}


const DWORD CBcasCard::GetCardReaderNum(void) const
{
	// カードリーダー数を返す
	if (m_pCardReader)
		return m_pCardReader->NumReaders();
	return 0;
}


LPCTSTR CBcasCard::EnumCardReader(const DWORD dwIndex) const
{
	if (m_pCardReader)
		return m_pCardReader->EnumReader(dwIndex);
	return NULL;
}


const bool CBcasCard::OpenCard(CCardReader::ReaderType ReaderType, LPCTSTR lpszReader)
{
	// 一旦クローズする
	CloseCard();

	m_pCardReader=CCardReader::CreateCardReader(ReaderType);
	if (m_pCardReader==NULL) {
		m_dwLastError = BCEC_CARDOPENERROR;
		return false;
	}
	if (!m_pCardReader->Open(lpszReader)) {
		CloseCard();
		m_dwLastError = BCEC_CARDOPENERROR;
		return false;
	}

	// カード初期化
	if (!InitialSetting())
		return false;

	m_dwLastError = BCEC_NOERROR;

	return true;
}


void CBcasCard::CloseCard(void)
{
	// カードをクローズする
	if (m_pCardReader) {
		m_pCardReader->Close();
		delete m_pCardReader;
		m_pCardReader=NULL;
	}
}


const bool CBcasCard::ReOpenCard()
{
	if (m_pCardReader==NULL)
		return false;
	return OpenCard(m_pCardReader->GetReaderType(),m_pCardReader->GetReaderName());
}


const bool CBcasCard::IsCardOpen() const
{
	return m_pCardReader!=NULL;
}


LPCTSTR CBcasCard::GetCardReaderName() const
{
	if (m_pCardReader)
		return m_pCardReader->GetReaderName();
	return NULL;
}


const DWORD CBcasCard::GetLastError(void) const
{
	// 最後に発生したエラーを返す
	return m_dwLastError;
}


const bool CBcasCard::InitialSetting(void)
{
	// 「Initial Setting Conditions Command」を処理する
	/*
	if (!m_pCardReader) {
		m_dwLastError = BCEC_CARDNOTOPEN;
		return false;
	}
	*/

	static const BYTE InitSettingCmd[] = {0x90U, 0x30U, 0x00U, 0x00U, 0x00U};

	// バッファ準備
	DWORD dwRecvSize;
	BYTE RecvData[1024];
	::ZeroMemory(RecvData, sizeof(RecvData));

	// コマンド送信
	dwRecvSize=sizeof(RecvData);
	if (!m_pCardReader->Transmit(InitSettingCmd, sizeof(InitSettingCmd), RecvData, &dwRecvSize)) {
		m_dwLastError = BCEC_TRANSMITERROR;
		return false;
	}

	if (dwRecvSize < 57UL) {
		m_dwLastError = BCEC_TRANSMITERROR;
		return false;
	}

	// レスポンス解析
	::CopyMemory(m_BcasCardInfo.BcasCardID, &RecvData[8], 6UL);		// +8	Card ID
	::CopyMemory(m_BcasCardInfo.SystemKey, &RecvData[16], 32UL);	// +16	Descrambling system key
	::CopyMemory(m_BcasCardInfo.InitialCbc, &RecvData[48], 8UL);	// +48	Descrambler CBC initial value

	// ECMステータス初期化
	::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));

	return true;
}


const BYTE * CBcasCard::GetBcasCardID(void)
{
	// Card ID を返す
	if( !m_pCardReader){
		m_dwLastError = BCEC_CARDNOTOPEN;
		return NULL;
	}

	m_dwLastError = BCEC_NOERROR;

	return m_BcasCardInfo.BcasCardID;
}


const BYTE * CBcasCard::GetInitialCbc(void)
{
	// Descrambler CBC Initial Value を返す
	/*
	if(!m_pCardReader){
		m_dwLastError = BCEC_CARDNOTOPEN;
		return NULL;
	}
	*/

	m_dwLastError = BCEC_NOERROR;

	return m_BcasCardInfo.InitialCbc;
}


const BYTE * CBcasCard::GetSystemKey(void)
{
	// Descrambling System Key を返す
	/*
	if (!m_pCardReader) {
		m_dwLastError = BCEC_CARDNOTOPEN;
		return NULL;
	}
	*/

	m_dwLastError = BCEC_NOERROR;

	return m_BcasCardInfo.SystemKey;
}


const BYTE * CBcasCard::GetKsFromEcm(const BYTE *pEcmData, const DWORD dwEcmSize)
{
	static const BYTE EcmReceiveCmd[] = {0x90U, 0x34U, 0x00U, 0x00U};

	// 「ECM Receive Command」を処理する
	if (!m_pCardReader) {
		m_dwLastError = BCEC_CARDNOTOPEN;
		return NULL;
	}

	// ECMサイズをチェック
	if (!pEcmData || (dwEcmSize < 30UL) || (dwEcmSize > 256UL)) {
		m_dwLastError = BCEC_BADARGUMENT;
		return NULL;
	}

	// キャッシュをチェックする
	if (!StoreEcmData(pEcmData, dwEcmSize)) {
		// ECMが同一の場合はキャッシュ済みKsを返す
		m_dwLastError = BCEC_NOERROR;
		return m_EcmStatus.KsData;
	}

	// バッファ準備
	DWORD dwRecvSize = 0UL;
	BYTE SendData[1024];
	BYTE RecvData[1024];
	::ZeroMemory(RecvData, sizeof(RecvData));

	// コマンド構築
	::CopyMemory(SendData, EcmReceiveCmd, sizeof(EcmReceiveCmd));				// CLA, INS, P1, P2
	SendData[sizeof(EcmReceiveCmd)] = (BYTE)dwEcmSize;							// COMMAND DATA LENGTH
	::CopyMemory(&SendData[sizeof(EcmReceiveCmd) + 1], pEcmData, dwEcmSize);	// ECM
	SendData[sizeof(EcmReceiveCmd) + dwEcmSize + 1] = 0x00U;					// RESPONSE DATA LENGTH

	// コマンド送信
	dwRecvSize=sizeof(RecvData);
	if (!m_pCardReader->Transmit(SendData, sizeof(EcmReceiveCmd) + dwEcmSize + 2UL, RecvData, &dwRecvSize)){
		::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));
		m_dwLastError = BCEC_TRANSMITERROR;
		return NULL;
	}

	// サイズチェック
	if (dwRecvSize != 25UL) {
		::ZeroMemory(&m_EcmStatus, sizeof(m_EcmStatus));
		m_dwLastError = BCEC_TRANSMITERROR;
		return NULL;
	}	

	// レスポンス解析
	::CopyMemory(m_EcmStatus.KsData, &RecvData[6], sizeof(m_EcmStatus.KsData));

	// リターンコード解析
	switch (((WORD)RecvData[4] << 8) | (WORD)RecvData[5]) {
	// Purchased: Viewing
	case 0x0200U :	// Payment-deferred PPV
	case 0x0400U :	// Prepaid PPV
	case 0x0800U :	// Tier
		m_dwLastError = BCEC_NOERROR;
		return m_EcmStatus.KsData;
	
	// 上記以外(視聴不可)
	default :
		m_dwLastError = BCEC_ECMREFUSED;
		return NULL;
	}
}


const bool CBcasCard::StoreEcmData(const BYTE *pEcmData, const DWORD dwEcmSize)
{
	bool bUpdate = false;

	// ECMデータ比較
	if (m_EcmStatus.dwLastEcmSize != dwEcmSize) {
		// サイズが変化した
		bUpdate = true;
	} else {
		// サイズが同じ場合はデータをチェックする
		for (DWORD dwPos = 0UL ; dwPos < dwEcmSize ; dwPos++) {
			if (pEcmData[dwPos] != m_EcmStatus.LastEcmData[dwPos]) {
				// データが不一致
				bUpdate = true;
				break;
			}
		}
	}

	// ECMデータを保存する
	if (bUpdate) {
		m_EcmStatus.dwLastEcmSize = dwEcmSize;
		::CopyMemory(m_EcmStatus.LastEcmData, pEcmData, dwEcmSize);
	}

	return bUpdate;
}
