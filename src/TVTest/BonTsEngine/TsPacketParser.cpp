// TsPacketParser.cpp: CTsPacketParser クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsPacketParser.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define TS_HEADSYNCBYTE		(0x47U)


//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////


CTsPacketParser::CTsPacketParser(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_bOutputNullPacket(false)
	, m_InputPacketCount(0)
	, m_OutputPacketCount(0)
	, m_ErrorPacketCount(0)
	, m_ContinuityErrorPacketCount(0)
	, m_bLockEpgDataCap(false)
{
	// パケット連続性カウンタを初期化する
	::FillMemory(m_abyContCounter, sizeof(m_abyContCounter), 0x10UL);
}

CTsPacketParser::~CTsPacketParser()
{
	m_EpgCap.UnInitialize();
}

void CTsPacketParser::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// パケットカウンタをクリアする
	m_InputPacketCount = 0;
	m_OutputPacketCount = 0;
	m_ErrorPacketCount = 0;
	m_ContinuityErrorPacketCount = 0;

	// パケット連続性カウンタを初期化する
	::FillMemory(m_abyContCounter, sizeof(m_abyContCounter), 0x10UL);

	// 状態をリセットする
	m_TsPacket.ClearSize();

#ifdef TVH264
	m_PATGenerator.Reset();
#endif
}

const bool CTsPacketParser::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;
	*/

	// TSパケットを処理する
	SyncPacket(pMediaData->GetData(), pMediaData->GetSize());

	return true;
}

void CTsPacketParser::SetOutputNullPacket(const bool bEnable)
{
	// NULLパケットの出力有無を設定する
	m_bOutputNullPacket = bEnable;
}

const DWORD CTsPacketParser::GetInputPacketCount(void) const
{
	// 入力パケット数を返す
	return (DWORD)m_InputPacketCount;
}

const DWORD CTsPacketParser::GetOutputPacketCount(void) const
{
	// 出力パケット数を返す
	return (DWORD)m_OutputPacketCount;
}

const DWORD CTsPacketParser::GetErrorPacketCount(void) const
{
	// エラーパケット数を返す
	return (DWORD)m_ErrorPacketCount;
}

const DWORD CTsPacketParser::GetContinuityErrorPacketCount(void) const
{
	// 連続性エラーパケット数を返す
	return (DWORD)m_ContinuityErrorPacketCount;
}

void CTsPacketParser::ResetErrorPacketCount(void)
{
	m_ErrorPacketCount=0;
	m_ContinuityErrorPacketCount=0;
}

void inline CTsPacketParser::SyncPacket(const BYTE *pData, const DWORD dwSize)
{
	// ※この方法は完全ではない、同期が乱れた場合に前回呼び出し時のデータまでさかのぼっては再同期はできない
	DWORD dwCurSize;
	DWORD dwCurPos = 0UL;

	while (dwCurPos < dwSize) {
		dwCurSize = m_TsPacket.GetSize();

		if (dwCurSize==0) {
			// 同期バイト待ち中
			do {
				if (pData[dwCurPos++] == TS_HEADSYNCBYTE) {
					// 同期バイト発見
					m_TsPacket.AddByte(TS_HEADSYNCBYTE);
					break;
				}
			} while (dwCurPos < dwSize);
		} else if (dwCurSize == TS_PACKETSIZE) {
			// パケットサイズ分データがそろった

			if (pData[dwCurPos] == TS_HEADSYNCBYTE) {
				// 次のデータは同期バイト
				ParsePacket();
			} else {
				// 同期エラー
				m_TsPacket.ClearSize();

				// 位置を元に戻す
				if (dwCurPos >= (TS_PACKETSIZE - 1UL))
					dwCurPos -= (TS_PACKETSIZE - 1UL);
				else
					dwCurPos = 0UL;
			}
		} else {
			// データ待ち
			DWORD dwRemain = (TS_PACKETSIZE - dwCurSize);
			if ((dwSize - dwCurPos) >= dwRemain) {
				m_TsPacket.AddData(&pData[dwCurPos], dwRemain);
				dwCurPos += dwRemain;
			} else {
				m_TsPacket.AddData(&pData[dwCurPos], dwSize - dwCurPos);
				break;
			}
		}
	}
}

bool inline CTsPacketParser::ParsePacket(void)
{
	bool bOK;

	// 入力カウントインクリメント
	m_InputPacketCount++;

	// パケットを解析/チェックする
	switch (m_TsPacket.ParsePacket(m_abyContCounter)) {
	case CTsPacket::EC_CONTINUITY:
		m_ContinuityErrorPacketCount++;
	case CTsPacket::EC_VALID:
		{
#ifdef TVH264
			/*
			// PAT の無い状態をシミュレート
			if (m_TsPacket.GetPID() == 0) {
				bOK = true;
				break;
			}
			*/
			if (m_PATGenerator.StorePacket(&m_TsPacket)) {
				if (m_PATGenerator.GetPAT(&m_PATPacket)) {
					OutputMedia(&m_PATPacket);
				}
			}
#endif

			// 次のデコーダにデータを渡す
			WORD PID;
			if (m_bOutputNullPacket || ((PID=m_TsPacket.GetPID()) != 0x1FFFU)) {
				if (!m_bLockEpgDataCap
						&& (PID == 0x0000 || PID == 0x0010 || PID == 0x0011 || PID == 0x0012
						|| PID == 0x0014 || PID==0x0026 || PID==0x0027)) {
					m_EpgCap.AddTSPacket(m_TsPacket.GetData(), m_TsPacket.GetSize());
				}
				// 出力カウントインクリメント
				m_OutputPacketCount++;

				OutputMedia(&m_TsPacket);
			}
		}
		bOK=true;
		break;
	case CTsPacket::EC_FORMAT:
	case CTsPacket::EC_TRANSPORT:
		// エラーカウントインクリメント
		m_ErrorPacketCount++;
		bOK=false;
		break;
	}

	// サイズをクリアし次のストアに備える
	m_TsPacket.ClearSize();

	return bOK;
}


bool CTsPacketParser::InitializeEpgDataCap(LPCTSTR pszDllFileName)
{
	return m_EpgCap.Initialize(pszDllFileName,FALSE)==NO_ERR;
}


bool CTsPacketParser::UnInitializeEpgDataCap()
{
	m_EpgCap.UnInitialize();
	return true;
}


bool CTsPacketParser::IsEpgDataCapLoaded() const
{
	return m_EpgCap.IsLoaded();
}


/*
CEpgDataInfo *CTsPacketParser::GetEpgDataInfo(WORD wSID,bool bNext)
{
	EPG_DATA_INFO *pData;
	CEpgDataInfo *pInfo=NULL;

	if (m_EpgCap.GetPFData(wSID,&pData,bNext)==NO_ERR)
		pInfo=new CEpgDataInfo(pData);
	return pInfo;
}
*/


bool CTsPacketParser::LockEpgDataCap()
{
	CBlockLock Lock(&m_DecoderLock);

	m_bLockEpgDataCap=true;
	return true;
}


bool CTsPacketParser::UnlockEpgDataCap()
{
	CBlockLock Lock(&m_DecoderLock);

	m_bLockEpgDataCap=false;
	return true;
}


#ifdef TVH264
bool CTsPacketParser::SetTransportStreamID(WORD TransportStreamID)
{
	return m_PATGenerator.SetTransportStreamID(TransportStreamID);
}
#endif
