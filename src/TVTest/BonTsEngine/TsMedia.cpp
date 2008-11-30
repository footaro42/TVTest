// TsMedia.cpp: TSメディアラッパークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsMedia.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// CPesPacketクラスの構築/消滅
/////////////////////////////////////////////////////////////////////////////

CPesPacket::CPesPacket()
	: CMediaData()
{
	Reset();
}

CPesPacket::CPesPacket(const DWORD dwBuffSize)
	: CMediaData(dwBuffSize)
{
	Reset();
}

CPesPacket::CPesPacket(const CPesPacket &Operand)
	: CMediaData()
{
	Reset();

	*this = Operand;
}

CPesPacket & CPesPacket::operator = (const CPesPacket &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		CMediaData::operator = (Operand);
		m_Header = Operand.m_Header;
	}

	return *this;
}

const bool CPesPacket::ParseHeader(void)
{
	if(m_dwDataSize < 9UL)return false;														// PES_header_data_lengthまでは6バイト
	else if(m_pData[0] != 0x00U || m_pData[1] != 0x00U || m_pData[2] != 0x01U)return false;	// packet_start_code_prefix異常
	else if((m_pData[6] & 0xC0U) != 0x80U)return false;										// 固定ビット異常

	// ヘッダ解析
	m_Header.byStreamID					= m_pData[3];										// +3 bit7-0
	m_Header.wPacketLength				= ((WORD)m_pData[4] << 8) | (WORD)m_pData[5];		// +4, +5
	m_Header.byScramblingCtrl			= (m_pData[6] & 0x30U) >> 4;						// +6 bit5-4
	m_Header.bPriority					= (m_pData[6] & 0x08U)? true : false;				// +6 bit3
	m_Header.bDataAlignmentIndicator	= (m_pData[6] & 0x04U)? true : false;				// +6 bit2
	m_Header.bCopyright					= (m_pData[6] & 0x02U)? true : false;				// +6 bit1
	m_Header.bOriginalOrCopy			= (m_pData[6] & 0x01U)? true : false;				// +6 bit0
	m_Header.byPtsDtsFlags				= (m_pData[7] & 0xC0U) >> 6;						// +7 bit7-6
	m_Header.bEscrFlag					= (m_pData[7] & 0x20U)? true : false;				// +7 bit5
	m_Header.bEsRateFlag				= (m_pData[7] & 0x10U)? true : false;				// +7 bit4
	m_Header.bDsmTrickModeFlag			= (m_pData[7] & 0x08U)? true : false;				// +7 bit3
	m_Header.bAdditionalCopyInfoFlag	= (m_pData[7] & 0x04U)? true : false;				// +7 bit2
	m_Header.bCrcFlag					= (m_pData[7] & 0x02U)? true : false;				// +7 bit1
	m_Header.bExtensionFlag				= (m_pData[7] & 0x01U)? true : false;				// +7 bit0
	m_Header.byHeaderDataLength			= m_pData[8];										// +8 bit7-0

	// ヘッダのフォーマット適合性をチェックする
	if(m_Header.byScramblingCtrl != 0U)return false;	// Not scrambled のみ対応
	else if(m_Header.byPtsDtsFlags == 1U)return false;	// 未定義のフラグ

	return true;
}

void CPesPacket::Reset(void)
{
	// データをクリアする
	ClearSize();	
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

const BYTE CPesPacket::GetStreamID(void) const
{
	// Stream IDを返す
	return m_Header.byStreamID;
}

const WORD CPesPacket::GetPacketLength(void) const
{
	// PES Packet Lengthを返す
	return m_Header.wPacketLength;
}

const BYTE CPesPacket::GetScramblingCtrl(void) const
{	// PES Scrambling Controlを返す
	return m_Header.byScramblingCtrl;
}

const bool CPesPacket::IsPriority(void) const
{	// PES Priorityを返す
	return m_Header.bPriority;
}

const bool CPesPacket::IsDataAlignmentIndicator(void) const
{
	// Data Alignment Indicatorを返す
	return m_Header.bDataAlignmentIndicator;
}

const bool CPesPacket::IsCopyright(void) const
{
	// Copyrightを返す
	return m_Header.bCopyright;
}

const bool CPesPacket::IsOriginalOrCopy(void) const
{
	// Original or Copyを返す
	return m_Header.bOriginalOrCopy;
}

const BYTE CPesPacket::GetPtsDtsFlags(void) const
{
	// PTS DTS Flagsを返す
	return m_Header.byPtsDtsFlags;
}

const bool CPesPacket::IsEscrFlag(void) const
{
	// ESCR Flagを返す
	return m_Header.bEscrFlag;
}

const bool CPesPacket::IsEsRateFlag(void) const
{
	// ES Rate Flagを返す
	return m_Header.bEsRateFlag;
}

const bool CPesPacket::IsDsmTrickModeFlag(void) const
{
	// DSM Trick Mode Flagを返す
	return m_Header.bDsmTrickModeFlag;
}

const bool CPesPacket::IsAdditionalCopyInfoFlag(void) const
{
	// Additional Copy Info Flagを返す
	return m_Header.bAdditionalCopyInfoFlag;
}

const bool CPesPacket::IsCrcFlag(void) const
{
	// PES CRC Flagを返す
	return m_Header.bCrcFlag;
}

const bool CPesPacket::IsExtensionFlag(void) const
{
	// PES Extension Flagを返す
	return m_Header.bExtensionFlag;
}

const BYTE CPesPacket::GetHeaderDataLength(void) const
{
	// PES Header Data Lengthを返す
	return m_Header.byHeaderDataLength;
}

const LONGLONG CPesPacket::GetPtsCount(void)const
{
	// PTS(Presentation Time Stamp)を返す
	if(m_Header.byPtsDtsFlags){
		return HexToTimeStamp(&m_pData[9]);
		}

	// エラー(PTSがない)
	return -1LL;
}

const WORD CPesPacket::GetPacketCrc(void) const
{
	// PES Packet CRCを返す
	DWORD dwCrcPos = 9UL;

	// 位置を計算
	if(m_Header.byPtsDtsFlags == 2U)dwCrcPos += 5UL;
	if(m_Header.byPtsDtsFlags == 3U)dwCrcPos += 10UL;
	if(m_Header.bEscrFlag)dwCrcPos += 6UL;
	if(m_Header.bEsRateFlag)dwCrcPos += 3UL;
	if(m_Header.bDsmTrickModeFlag)dwCrcPos += 1UL;
	if(m_Header.bAdditionalCopyInfoFlag)dwCrcPos += 1UL;

	if(m_dwDataSize < (dwCrcPos + 2UL))return 0x0000U;

	return ((WORD)m_pData[dwCrcPos] << 8) | (WORD)m_pData[dwCrcPos + 1];
}

BYTE * CPesPacket::GetPayloadData(void) const
{
	// ペイロードポインタを返す
	const DWORD dwPayloadPos = m_Header.byHeaderDataLength + 9UL;

	return (m_dwDataSize >= (dwPayloadPos + 1UL))? &m_pData[dwPayloadPos] : NULL;
}

const DWORD CPesPacket::GetPayloadSize(void) const
{
	// ペイロードサイズを返す(実際の保持してる　※パケット長より少なくなることもある)
	const DWORD dwHeaderSize = m_Header.byHeaderDataLength + 9UL;

	return (m_dwDataSize > dwHeaderSize)? (m_dwDataSize - dwHeaderSize) : 0UL;
}

inline const LONGLONG CPesPacket::HexToTimeStamp(const BYTE *pHexData)
{
	// 33bit 90KHz タイムスタンプを解析する
	LONGLONG llCurPtsCount = 0LL;
	llCurPtsCount |= (LONGLONG)(pHexData[0] & 0x0EU) << 29;
	llCurPtsCount |= (LONGLONG)pHexData[1] << 22;
	llCurPtsCount |= (LONGLONG)(pHexData[2] & 0xFEU) << 14;
	llCurPtsCount |= (LONGLONG)pHexData[3] << 7;
	llCurPtsCount |= (LONGLONG)pHexData[4] >> 1;

	return llCurPtsCount;
}


//////////////////////////////////////////////////////////////////////
// CPesParserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CPesParser::CPesParser(IPacketHandler *pPacketHandler)
	: m_pPacketHandler(pPacketHandler)
	, m_PesPacket(0x10005UL)
	, m_bIsStoring(false)
	, m_wStoreCrc(0x0000U)
	, m_dwStoreSize(0UL)
{

}

CPesParser::CPesParser(const CPesParser &Operand)
{
	*this = Operand;
}

CPesParser & CPesParser::operator = (const CPesParser &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		m_pPacketHandler = Operand.m_pPacketHandler;
		m_PesPacket = Operand.m_PesPacket;
		m_bIsStoring = Operand.m_bIsStoring;
		m_wStoreCrc = Operand.m_wStoreCrc;
	}

	return *this;
}

const bool CPesParser::StorePacket(const CTsPacket *pPacket)
{
	const BYTE *pData = pPacket->GetPayloadData();
	const BYTE bySize = pPacket->GetPayloadSize();
	if(!bySize || !pData)return false;

	bool bTrigger = false;
	BYTE byPos = 0U;

	if(pPacket->m_Header.bPayloadUnitStartIndicator){
		// ヘッダ先頭 + [ペイロード断片]

		// PESパケット境界なしのストアを完了する
		if(m_bIsStoring && !m_PesPacket.GetPacketLength()){
			OnPesPacket(&m_PesPacket);
			}

		m_bIsStoring = false;
		bTrigger = true;
		m_PesPacket.ClearSize();

		byPos += StoreHeader(&pData[byPos], bySize - byPos);
		byPos += StorePayload(&pData[byPos], bySize - byPos);
		}
	else{
		// [ヘッダ断片] + ペイロード + [スタッフィングバイト]
		byPos += StoreHeader(&pData[byPos], bySize - byPos);
		byPos += StorePayload(&pData[byPos], bySize - byPos);
		}

	return bTrigger;
}

void CPesParser::Reset(void)
{
	// 状態を初期化する
	m_PesPacket.Reset();
	m_bIsStoring = false;
	m_dwStoreSize = 0UL;
}

void CPesParser::OnPesPacket(const CPesPacket *pPacket) const
{
	// ハンドラ呼び出し
	if(m_pPacketHandler)m_pPacketHandler->OnPesPacket(this, pPacket);
}

const BYTE CPesParser::StoreHeader(const BYTE *pPayload, const BYTE byRemain)
{
	// ヘッダを解析してセクションのストアを開始する
	if(m_bIsStoring)return 0U;

	const BYTE byHeaderRemain = 9U - (BYTE)m_PesPacket.GetSize();

	if(byRemain >= byHeaderRemain){
		// ヘッダストア完了、ヘッダを解析してペイロードのストアを開始する
		m_PesPacket.AddData(pPayload, byHeaderRemain);
		if(m_PesPacket.ParseHeader()){
			// ヘッダフォーマットOK
			m_dwStoreSize = m_PesPacket.GetPacketLength();
			if(m_dwStoreSize)m_dwStoreSize += 6UL;
			m_bIsStoring = true;
			return byHeaderRemain;
			}
		else{
			// ヘッダエラー
			m_PesPacket.Reset();
			return byRemain;
			}
		}
	else{
		// ヘッダストア未完了、次のデータを待つ
		m_PesPacket.AddData(pPayload, byRemain);
		return byRemain;
		}
}

const BYTE CPesParser::StorePayload(const BYTE *pPayload, const BYTE byRemain)
{
	// セクションのストアを完了する
	if(!m_bIsStoring)return 0U;
	
	const DWORD dwStoreRemain = m_dwStoreSize - m_PesPacket.GetSize();

	if(m_dwStoreSize && (dwStoreRemain <= (DWORD)byRemain)){
		// ストア完了
		m_PesPacket.AddData(pPayload, dwStoreRemain);

		// CRC正常、コールバックにセクションを渡す
		OnPesPacket(&m_PesPacket);

		// 状態を初期化し、次のセクション受信に備える
		m_PesPacket.Reset();
		m_bIsStoring = false;

		return (BYTE)dwStoreRemain;
		}
	else{
		// ストア未完了、次のペイロードを待つ
		m_PesPacket.AddData(pPayload, byRemain);
		return byRemain;
		}
}


//////////////////////////////////////////////////////////////////////
// CAdtsFrameクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CAdtsFrame::CAdtsFrame()
	: CMediaData()
{
	Reset();
}

CAdtsFrame::CAdtsFrame(const CAdtsFrame &Operand)
{
	Reset();

	*this = Operand;
}

CAdtsFrame & CAdtsFrame::operator = (const CAdtsFrame &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		CMediaData::operator = (Operand);
		m_Header = Operand.m_Header;
	}

	return *this;
}

const bool CAdtsFrame::ParseHeader(void)
{
	// adts_fixed_header()
	if(m_dwDataSize < 7UL)return false;									// ADTSヘッダは7バイト
	else if(m_pData[0] != 0xFFU || m_pData[1] != 0xF8U)return false;	// Syncword、ID、layer、protection_absent異常　※CRCなしは非対応
	
	m_Header.byProfile				= (m_pData[2] & 0xC0U) >> 6;									// +2 bit7-6
	m_Header.bySamplingFreqIndex	= (m_pData[2] & 0x3CU) >> 2;									// +2 bit5-2
	m_Header.bPrivateBit			= (m_pData[2] & 0x02U)? true : false;							// +2 bit1
	m_Header.byChannelConfig		= ((m_pData[2] & 0x01U) << 2) | ((m_pData[3] & 0xC0U) >> 6);	// +3 bit0, +4 bit7-6
	m_Header.bOriginalCopy			= (m_pData[3] & 0x20U)? true : false;							// +3 bit5
	m_Header.bHome					= (m_pData[3] & 0x10U)? true : false;							// +3 bit4

	// adts_variable_header()
	m_Header.bCopyrightIdBit		= (m_pData[3] & 0x08U)? true : false;							// +3 bit3
	m_Header.bCopyrightIdStart		= (m_pData[3] & 0x04U)? true : false;							// +3 bit2
	m_Header.wFrameLength			= ((WORD)(m_pData[3] & 0x03U) << 11) | ((WORD)m_pData[4] << 3) | ((WORD)(m_pData[5] & 0xE0U) >> 5);
	m_Header.wBufferFullness		= ((WORD)(m_pData[5] & 0x1FU) << 6) | ((WORD)(m_pData[6] & 0xFCU) >> 2);
	m_Header.byRawDataBlockNum		= m_pData[6] & 0x03U;

	// フォーマット適合性チェック
	if(m_Header.byProfile == 3U)return false;							// 未定義のプロファイル
	else if(m_Header.bySamplingFreqIndex > 0x0BU)return false;			// 未定義のサンプリング周波数
	else if(m_Header.wFrameLength < 2U)return false;					// データなしの場合も最低CRCのサイズが必要
	else if(m_Header.byRawDataBlockNum)return false;					// 本クラスは単一のRaw Data Blockにしか対応しない

	return true;
}

void CAdtsFrame::Reset(void)
{
	// データをクリアする
	ClearSize();	
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

const BYTE CAdtsFrame::GetProfile(void) const
{
	// Profile を返す
	return m_Header.byProfile;
}

const BYTE CAdtsFrame::GetSamplingFreqIndex(void) const
{
	// Sampling Frequency Index を返す
	return m_Header.bySamplingFreqIndex;
}

const bool CAdtsFrame::IsPrivateBit(void) const
{
	// Private Bit を返す
	return m_Header.bPrivateBit;
}

const BYTE CAdtsFrame::GetChannelConfig(void) const
{
	// Channel Configuration を返す
	return m_Header.byChannelConfig;
}

const bool CAdtsFrame::IsOriginalCopy(void) const
{
	// Original/Copy を返す
	return m_Header.bOriginalCopy;
}

const bool CAdtsFrame::IsHome(void) const
{
	// Home を返す
	return m_Header.bHome;
}

const bool CAdtsFrame::IsCopyrightIdBit(void) const
{
	// Copyright Identification Bit を返す
	return m_Header.bCopyrightIdBit;
}

const bool CAdtsFrame::IsCopyrightIdStart(void) const
{
	// Copyright Identification Start を返す
	return m_Header.bCopyrightIdStart;
}

const WORD CAdtsFrame::GetFrameLength(void) const
{
	// Frame Length を返す
	return m_Header.wFrameLength;
}

const WORD CAdtsFrame::GetBufferFullness(void) const
{
	// ADTS Buffer Fullness を返す
	return m_Header.wBufferFullness;
}

const BYTE CAdtsFrame::GetRawDataBlockNum(void) const
{
	// Number of Raw Data Blocks in Frame を返す
	return m_Header.byRawDataBlockNum;
}


//////////////////////////////////////////////////////////////////////
// CAdtsParserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CAdtsParser::CAdtsParser(IFrameHandler *pFrameHandler)
	: m_pFrameHandler(pFrameHandler)
{
	// ADTSフレーム最大長のバッファ確保
	m_AdtsFrame.GetBuffer(0x2000UL);

	Reset();
}

CAdtsParser::CAdtsParser(const CAdtsParser &Operand)
{
	*this = Operand;
}

CAdtsParser & CAdtsParser::operator = (const CAdtsParser &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		m_pFrameHandler = Operand.m_pFrameHandler;
		m_AdtsFrame = Operand.m_AdtsFrame;
		m_bIsStoring = Operand.m_bIsStoring;
		m_wStoreCrc = Operand.m_wStoreCrc;
	}

	return *this;
}

const bool CAdtsParser::StorePacket(const CPesPacket *pPacket)
{
	return StoreEs(pPacket->GetPayloadData(), pPacket->GetPayloadSize());
}

const bool CAdtsParser::StoreEs(const BYTE *pData, const DWORD dwSize)
{
	bool bTrigger = false;
	DWORD dwPos = 0UL;

	if(!dwSize || !dwSize)return bTrigger;

	while (dwPos < dwSize) {
		if (!m_bIsStoring) {
			// ヘッダを検索する
			m_bIsStoring = SyncFrame(pData[dwPos++]);
			if(m_bIsStoring)bTrigger = true;
		} else {
			// データをストアする
			const DWORD dwStoreRemain = m_AdtsFrame.GetFrameLength() - (WORD)m_AdtsFrame.GetSize();
			const DWORD dwDataRemain = dwSize - dwPos;

			if (dwStoreRemain <= dwDataRemain) {
				// ストア完了
				m_AdtsFrame.AddData(&pData[dwPos], dwStoreRemain);
				dwPos += dwStoreRemain;
				m_bIsStoring = false;

				// 本来ならここでCRCチェックをすべき
				// チェック対象領域が可変で複雑なので保留、誰か実装しませんか...

				// フレーム出力
				OnAdtsFrame(&m_AdtsFrame);

				// 次のフレームを処理するためリセット
				m_AdtsFrame.ClearSize();
			} else {
				// ストア未完了、次のペイロードを待つ
				m_AdtsFrame.AddData(&pData[dwPos], dwDataRemain);
				dwPos += dwDataRemain;
			}
		}
	}

	return bTrigger;
}

void CAdtsParser::Reset(void)
{
	// 状態を初期化する
	m_bIsStoring = false;
	m_AdtsFrame.Reset();
}

void CAdtsParser::OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket)
{
	// CPesParser::IPacketHandlerインタフェースの実装
	StorePacket(pPacket);
}

void CAdtsParser::OnAdtsFrame(const CAdtsFrame *pFrame) const
{
	// ハンドラ呼び出し
	if(m_pFrameHandler)m_pFrameHandler->OnAdtsFrame(this, pFrame);
}

inline const bool CAdtsParser::SyncFrame(const BYTE byData)
{
	switch(m_AdtsFrame.GetSize()){
		case 0UL :
			// syncword(8bit)
			if(byData == 0xFFU)m_AdtsFrame.AddByte(byData);
			break;

		case 1UL :
			// syncword(4bit), ID, layer, protection_absent	※CRC付きのフレームのみ対応
			if(byData == 0xF8U)m_AdtsFrame.AddByte(byData);
			else m_AdtsFrame.ClearSize();
			break;

		case 2UL :
		case 3UL :
		case 4UL :
		case 5UL :
			// adts_fixed_header() - adts_variable_header()
			m_AdtsFrame.AddByte(byData);
			break;

		case 6UL :
			// ヘッダが全てそろった
			m_AdtsFrame.AddByte(byData);

			// ヘッダを解析する
			if(m_AdtsFrame.ParseHeader())return true;
			else m_AdtsFrame.ClearSize();
			break;

		default:
			// 例外
			m_AdtsFrame.ClearSize();
			break;
		}

	return false;
}


//////////////////////////////////////////////////////////////////////
// CMpeg2Sequenceクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CMpeg2Sequence::CMpeg2Sequence()
	: CMediaData()
{
	Reset();
}

CMpeg2Sequence::CMpeg2Sequence(const CMpeg2Sequence &Operand)
{
	Reset();

	*this = Operand;
}

CMpeg2Sequence & CMpeg2Sequence::operator = (const CMpeg2Sequence &Operand)
{
	if (&Operand != this) {
		// インスタンスのコピー
		CMediaData::operator = (Operand);
		m_Header = Operand.m_Header;
	}

	return *this;
}

const bool CMpeg2Sequence::ParseHeader(void)
{
	// ここではStart Code PrifixとStart Codeしかチェックしない。(シーケンスの同期のみを目的とする)

	// next_start_code()
	DWORD dwHeaderSize = 12UL;
	if(m_dwDataSize < dwHeaderSize)return false;
	else if(m_pData[0] || m_pData[1] || m_pData[2] != 0x01U || m_pData[3] != 0xB3U)return false;					// +0,+1,+2,+3	

	ZeroMemory(&m_Header,sizeof(m_Header));
	m_Header.wHorizontalSize			= ((WORD)m_pData[4] << 4) | ((WORD)(m_pData[5] & 0xF0U) >> 4);				// +4,+5 bit7-4
	m_Header.wVerticalSize				= ((WORD)(m_pData[5] & 0x0FU) << 8) | (WORD)m_pData[6];						// +5 bit3-0, +6
	m_Header.byAspectRatioInfo			= (m_pData[7] & 0xF0U) >> 4;												// +7 bit7-4
	m_Header.byFrameRateCode			= m_pData[7] & 0x0FU;														// +7 bit3-0
	m_Header.dwBitRate					= ((DWORD)m_pData[8] << 10) | ((DWORD)m_pData[9] << 2) | ((DWORD)(m_pData[10] & 0xC0U) >> 6);	// +8, +9, +10 bit7-6
	m_Header.bMarkerBit					= (m_pData[10] & 0x20U)? true : false;										// +10 bit5
	m_Header.wVbvBufferSize				= ((WORD)(m_pData[10] & 0x1FU) << 5) | ((WORD)(m_pData[11] & 0xF8U) >> 3);	// +10 bit4-0, +11 bit7-3
	m_Header.bConstrainedParamFlag		= (m_pData[11] & 0x04U)? true : false;										// +11 bit2
	m_Header.bLoadIntraQuantiserMatrix	= (m_pData[11] & 0x02U)? true : false;										// +11 bit1
	m_Header.bLoadNonIntraQuantiserMatrix=(m_pData[11] & 0x01U)? true : false;										// +11 bit0
	if(m_Header.bLoadIntraQuantiserMatrix){
		dwHeaderSize+=64;
		if(m_dwDataSize < dwHeaderSize)return false;

		}
	if(m_Header.bLoadNonIntraQuantiserMatrix){
		dwHeaderSize+=64;
		if(m_dwDataSize < dwHeaderSize)return false;

		}

	// 拡張ヘッダ検索する
	DWORD dwMaxSearchSize = 1024;
	for(DWORD i=dwHeaderSize;(unsigned)i<min(m_dwDataSize-3,dwMaxSearchSize);i++){
		if(m_pData[i]==0x00 && m_pData[i+1]==0x00 && m_pData[i+2]==0x01 && m_pData[i+3]==0xB5) {
			// 拡張ヘッダ発見
			int iExtPos=i+4;
			int iCode = (m_pData[iExtPos+0] & 0xF0U) >> 4;
			int iPosTmp;
			switch(iCode) {
			case 1:
				// シーケンス拡張(40bit)
				m_Header.Extention.Sequence.bHave = true;
				m_Header.Extention.Sequence.byProfileAndLevel = (m_pData[iExtPos+0] & 0x0FU) << 4 | (m_pData[iExtPos+1] & 0xF0U)>>4;
				m_Header.Extention.Sequence.bProgressive = m_pData[iExtPos+1] & 0x08 ? true : false;
				m_Header.Extention.Sequence.byChromaFormat = (m_pData[iExtPos+1] & 0x06U)>>1;
//				m_Header.wHorizontalSize += ((m_pData[iExtPos+1] & 0x01U) | (m_pData[iExtPos+2] & 0x80U) >> 7) << 12
//				m_Header.wVerticalSize   += ((m_pData[iExtPos+2] & 0x60U) >> 5) << 12
//				m_Header.dwBitRate += (((DWORD)m_pData[iExtPos+2] & 0x1FU) << 7 | (m_pData[iExtPos+3] & 0xFEU) >> 1);
				// Marker Bit
				if(!m_pData[iExtPos+3]&0x01U) return false;
				m_Header.Extention.Sequence.bLowDelay = m_pData[iExtPos+4] & 0x80 ? true : false;
				m_Header.Extention.Sequence.byFrameRateExtN = (m_pData[iExtPos+4] & 0x60U)>>5;
				m_Header.Extention.Sequence.byFrameRateExtD = (m_pData[iExtPos+4] & 0x18U)>>3;
				i+=5;
				break;
			case 2:
				// ディスプレイ拡張(32bit(+24bit))
				m_Header.Extention.Display.bHave = true;
				m_Header.Extention.Display.byVideoFormat = (m_pData[iExtPos+0] & 0x0EU) >> 1;
				m_Header.Extention.Display.bColorDescrption = m_pData[iExtPos+0] & 0x01U;
				if(m_Header.Extention.Display.bColorDescrption){
					m_Header.Extention.Display.Color.byColorPrimaries = m_pData[iExtPos+1];
					m_Header.Extention.Display.Color.byTransferCharacteristics = m_pData[iExtPos+2];
					m_Header.Extention.Display.Color.byMatrixCoefficients = m_pData[iExtPos+3];
					iPosTmp = 3;
					} else {
					iPosTmp = 0;
					}
				m_Header.Extention.Display.wDisplayHorizontalSize = ((WORD)m_pData[iExtPos+iPosTmp+1] << 6) | ((WORD)(m_pData[iExtPos+iPosTmp+2] & 0xFCU) >> 2);
				m_Header.Extention.Display.wDisplayVerticalSize   = ((WORD)(m_pData[iExtPos+iPosTmp+2] & 0x01U) << 13) | (WORD)m_pData[iExtPos+iPosTmp+3] << 5 | ((WORD)(m_pData[iExtPos+iPosTmp+4] & 0xF1U) >> 3);
				break;
				}
			}
		}

	// フォーマット適合性チェック
	if(!m_Header.byAspectRatioInfo || m_Header.byAspectRatioInfo > 4U)return false;		// アスペクト比が異常
	else if(!m_Header.byFrameRateCode || m_Header.byFrameRateCode > 8U)return false;	// フレームレートが異常
	else if(!m_Header.bMarkerBit)return false;											// マーカービットが異常
	else if(m_Header.bConstrainedParamFlag)return false;								// Constrained Parameters Flag が異常

	return true;
}

void CMpeg2Sequence::Reset(void)
{
	// データをクリアする
	ClearSize();
	::ZeroMemory(&m_Header, sizeof(m_Header));
}

const WORD CMpeg2Sequence::GetHorizontalSize(void) const
{
	// Horizontal Size Value を返す
	return m_Header.wHorizontalSize;
}

const WORD CMpeg2Sequence::GetVerticalSize(void) const
{
	// Vertical Size Value を返す
	return m_Header.wVerticalSize;
}

const BYTE CMpeg2Sequence::GetAspectRatioInfo(void) const
{
	// Aspect Ratio Information を返す
	return m_Header.byAspectRatioInfo;
}

const BYTE CMpeg2Sequence::GetFrameRateCode(void) const
{
	// Frame Rate Code を返す
	return m_Header.byFrameRateCode;
}

const DWORD CMpeg2Sequence::GetBitRate(void) const
{
	// Bit Rate Value を返す
	return m_Header.dwBitRate;
}

const bool CMpeg2Sequence::IsMarkerBit(void) const
{
	// Marker Bit を返す
	return m_Header.bMarkerBit;
}

const WORD CMpeg2Sequence::GetVbvBufferSize(void) const
{
	// VBV Buffer Size Value を返す
	return m_Header.wVbvBufferSize;
}

const bool CMpeg2Sequence::IsConstrainedParamFlag(void) const
{
	// Constrained Parameters Flag を返す
	return m_Header.bConstrainedParamFlag;
}

const bool CMpeg2Sequence::IsLoadIntraQuantiserMatrix(void) const
{
	// Load Intra Quantiser Matrix を返す
	return m_Header.bLoadIntraQuantiserMatrix;
}

const bool CMpeg2Sequence::GetExtendDisplayInfo() const
{
	// Extention Sequence Display があるかを返す
	return m_Header.Extention.Display.bHave;
}

const WORD CMpeg2Sequence::GetExtendDisplayHorizontalSize(void) const
{
	// Horizontal Size Value を返す
	return m_Header.Extention.Display.wDisplayHorizontalSize;
}

const WORD CMpeg2Sequence::GetExtendDisplayVerticalSize(void) const
{
	// Vertical Size Value を返す
	return m_Header.Extention.Display.wDisplayVerticalSize;
}



//////////////////////////////////////////////////////////////////////
// CMpeg2Parserクラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CMpeg2Parser::CMpeg2Parser(ISequenceHandler *pSequenceHandler)
	: m_pSequenceHandler(pSequenceHandler)
{
	Reset();
}

CMpeg2Parser::CMpeg2Parser(const CMpeg2Parser &Operand)
{
	*this = Operand;
}

CMpeg2Parser & CMpeg2Parser::operator = (const CMpeg2Parser &Operand)
{
	// インスタンスのコピー
	m_pSequenceHandler = Operand.m_pSequenceHandler;
	m_Mpeg2Sequence = Operand.m_Mpeg2Sequence;
	//m_bIsStoring = Operand.m_bIsStoring;
	m_dwSyncState = Operand.m_dwSyncState;

	return *this;
}

const bool CMpeg2Parser::StorePacket(const CPesPacket *pPacket)
{
	return StoreEs(pPacket->GetPayloadData(),pPacket->GetPayloadSize());
}

const bool CMpeg2Parser::StoreEs(const BYTE *pData, const DWORD dwSize)
{
	static const BYTE StartCode[] = {0x00U, 0x00U, 0x01U, 0xB3U};
	bool bTrigger=false;
	DWORD dwPos,dwStart;

	for (dwPos=0UL;dwPos<dwSize;dwPos+=dwStart) {
		// スタートコードを検索する
		//dwStart = FindStartCode(&pData[dwPos], dwSize - dwPos);
		DWORD Remain=dwSize-dwPos;
		DWORD SyncState=m_dwSyncState;
		for (dwStart=0UL;dwStart<Remain;dwStart++) {
			SyncState=(SyncState<<8)|(DWORD)pData[dwStart+dwPos];
			if (SyncState==0x000001B3UL) {
				// スタートコード発見、シフトレジスタを初期化する
				SyncState=0xFFFFFFFFUL;
				break;
			}
		}
		m_dwSyncState=SyncState;

		if (dwStart<Remain) {
			dwStart++;
			if (m_Mpeg2Sequence.GetSize()>=4UL) {
				// スタートコードの断片を取り除く
				if (dwStart<4UL)
					m_Mpeg2Sequence.TrimTail(4UL-dwStart);

				// シーケンスを出力する
				if (m_Mpeg2Sequence.ParseHeader())
					OnMpeg2Sequence(&m_Mpeg2Sequence);
			}

			// スタートコードをセットする
			m_Mpeg2Sequence.SetData(StartCode,4UL);
			bTrigger=true;
		} else if (m_Mpeg2Sequence.GetSize()>=4UL) {
			// シーケンスストア
			if (m_Mpeg2Sequence.AddData(&pData[dwPos],Remain)>=0x1000000UL) {
				// 例外(シーケンスが16MBを超える)
				m_Mpeg2Sequence.ClearSize();
			}
		}
	}

	return bTrigger;
}

void CMpeg2Parser::Reset(void)
{
	// 状態を初期化する
	//m_bIsStoring = false;
	m_dwSyncState = 0xFFFFFFFFUL;

	m_Mpeg2Sequence.Reset();
}

void CMpeg2Parser::OnPesPacket(const CPesParser *pPesParser, const CPesPacket *pPacket)
{
	// CPesParser::IPacketHandlerインタフェースの実装
	StorePacket(pPacket);
}

void CMpeg2Parser::OnMpeg2Sequence(const CMpeg2Sequence *pSequence) const
{
	// ハンドラ呼び出し
	if(m_pSequenceHandler)m_pSequenceHandler->OnMpeg2Sequence(this, pSequence);
}

/*
inline const DWORD CMpeg2Parser::FindStartCode(const BYTE *pData, const DWORD dwDataSize)
{
	// Sequence Header Code (0x000001B3) を検索する
	DWORD dwPos;

	for(dwPos = 0UL ; dwPos < dwDataSize ; dwPos++){
		m_dwSyncState <<= 8;
		m_dwSyncState |= (DWORD)pData[dwPos];

		if(m_dwSyncState == 0x000001B3UL){
			// スタートコード発見、シフトレジスタを初期化する
			m_dwSyncState = 0xFFFFFFFFUL;
			break;
			}
		}

	return dwPos;
}
*/
