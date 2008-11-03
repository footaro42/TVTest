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
	, m_dwInputPacketCount(0UL)
	, m_dwOutputPacketCount(0UL)
	, m_dwErrorPacketCount(0UL)
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
	// パケットカウンタをクリアする
	m_dwInputPacketCount =	0UL;
	m_dwOutputPacketCount = 0UL;
	m_dwErrorPacketCount =	0UL;

	// パケット連続性カウンタを初期化する
	::FillMemory(m_abyContCounter, sizeof(m_abyContCounter), 0x10UL);

	// 状態をリセットする
	m_TsPacket.ClearSize();

	CMediaDecoder::Reset();
}

const bool CTsPacketParser::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if(dwInputIndex >= GetInputNum())return false;
	if(!pMediaData)return false;

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
	return m_dwInputPacketCount;
}

const DWORD CTsPacketParser::GetOutputPacketCount(void) const
{
	// 出力パケット数を返す
	return m_dwOutputPacketCount;
}

const DWORD CTsPacketParser::GetErrorPacketCount(void) const
{
	// エラーパケット数を返す
	return m_dwErrorPacketCount;
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
			for ( ; dwCurPos < dwSize ; dwCurPos++) {
				if (pData[dwCurPos] == TS_HEADSYNCBYTE) {
					// 同期バイト発見
					m_TsPacket.AddByte(TS_HEADSYNCBYTE);
					dwCurPos++;
					break;
				}
			}
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

void inline CTsPacketParser::ParsePacket(void)
{
	// パケットを解析する
	m_TsPacket.ParseHeader();

	// パケットをチェックする
	if (m_TsPacket.CheckPacket(&m_abyContCounter[m_TsPacket.GetPID()])) {
		// 入力カウントインクリメント
		//if(m_dwInputPacketCount < 0xFFFFFFFFUL)m_dwInputPacketCount++;
		m_dwInputPacketCount++;

		// 次のデコーダにデータを渡す
		WORD PID;
		if (m_bOutputNullPacket || ((PID=m_TsPacket.GetPID()) != 0x1FFFU)) {
			if (PID == 0x0000 || PID == 0x0010 || PID == 0x0011
					|| PID == 0x0012 || PID == 0x0014) {
				m_EpgCap.AddTSPacket(m_TsPacket.GetData(),m_TsPacket.GetSize());
			}
			// 出力カウントインクリメント
			//if(m_dwOutputPacketCount < 0xFFFFFFFFUL)m_dwOutputPacketCount++;
			m_dwOutputPacketCount++;

			OutputMedia(&m_TsPacket);
		}
	} else {
		// エラーカウントインクリメント
		//if(m_dwErrorPacketCount < 0xFFFFFFFFUL)m_dwErrorPacketCount++;
		m_dwErrorPacketCount++;
	}

	// サイズをクリアし次のストアに備える
	m_TsPacket.ClearSize();
}


bool CTsPacketParser::InitializeEpgDataCap(LPCTSTR pszDllFileName)
{
	return m_EpgCap.Initialize(pszDllFileName,TRUE)==NO_ERR;
}


bool CTsPacketParser::UnInitializeEpgDataCap()
{
	return m_EpgCap.UnInitialize()==NO_ERR;
}


CEpgDataInfo *CTsPacketParser::GetEpgDataInfo(WORD wSID,bool bNext)
{
	EPG_DATA_INFO Item;
	CEpgDataInfo *pInfo=NULL;

	ZeroMemory(&Item,sizeof(EPG_DATA_INFO));
	if (m_EpgCap.GetPFData(wSID,&Item,bNext)==NO_ERR)
		pInfo=new CEpgDataInfo(&Item);
	m_EpgCap.ReleasePFData(&Item);
	return pInfo;
}
