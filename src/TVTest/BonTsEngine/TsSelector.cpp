#include "stdafx.h"
#include "TsSelector.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CTsSelector::CTsSelector(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_InputPacketCount(0)
	, m_OutputPacketCount(0)
	, m_TargetServiceID(0)
	, m_TargetPmtPID(0)
	, m_LastTSID(0)
	, m_LastPmtPID(0)
	, m_LastVersion(0)
	, m_Version(0)
{
	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0000, new CPatTable, OnPatUpdated, this);
	// CATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0001, new CCatTable, OnCatUpdated, this);

	m_PatPacket.SetSize(TS_PACKETSIZE);
}


CTsSelector::~CTsSelector()
{
}


void CTsSelector::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// 内部状態を初期化する
	m_PidMapManager.UnmapAllTarget();

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0000U, new CPatTable, OnPatUpdated, this);
	// CATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0001U, new CCatTable, OnCatUpdated, this);

	/*
	// 統計データ初期化
	m_InputPacketCount = 0;
	m_OutputPacketCount = 0;
	*/

	// 対象サービス初期化
	m_TargetPIDList.clear();
	m_TargetServiceID = 0;
	m_TargetPmtPID = 0;

	m_LastTSID = 0;
	m_LastPmtPID = 0;
	m_LastVersion = 0;
	m_Version = 0;

	// 下流デコーダを初期化する
	ResetDownstreamDecoder();
}


const bool CTsSelector::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;
	*/

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// 入力パケット数カウント
	m_InputPacketCount++;

	// PIDルーティング
	m_PidMapManager.StorePacket(pTsPacket);

	WORD PID = pTsPacket->GetPID();
	if (PID<0x0030 || IsTargetPID(PID)) {
		m_OutputPacketCount++;
		// パケットを下流デコーダにデータを渡す
		if (PID == 0x0000 && m_TargetPmtPID != 0
				&& MakePat(pTsPacket, &m_PatPacket)) {
			OutputMedia(&m_PatPacket);
		} else {
			OutputMedia(pMediaData);
		}
	}

	return true;
}


ULONGLONG CTsSelector::GetInputPacketCount() const
{
	// 入力パケット数を返す
	return m_InputPacketCount;
}


ULONGLONG CTsSelector::GetOutputPacketCount() const
{
	// 出力パケット数を返す
	return m_OutputPacketCount;
}


bool CTsSelector::SetTargetServiceID(WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceID != m_TargetServiceID) {
		m_TargetServiceID = ServiceID;
		m_TargetPIDList.clear();
		m_TargetPmtPID = 0;

		CPatTable *pPatTable=dynamic_cast<CPatTable *>(m_PidMapManager.GetMapTarget(0x0000));
		if (pPatTable!=NULL) {
			for (WORD i=0;i<pPatTable->GetProgramNum();i++) {
				WORD PmtPID = pPatTable->GetPmtPID(i);

				if (m_TargetServiceID==0
						|| pPatTable->GetProgramID(i)==m_TargetServiceID) {

					if (m_TargetServiceID != 0)
						m_TargetPmtPID = PmtPID;
					AddTargetPID(PmtPID);
					m_PidMapManager.MapTarget(PmtPID,new CPmtTable,OnPmtUpdated,this);
				} else {
					m_PidMapManager.UnmapTarget(PmtPID);
				}
			}
		}
	}
	return true;
}


bool CTsSelector::IsTargetPID(WORD PID)
{
	if (m_TargetPIDList.size() == 0)
		return m_TargetServiceID == 0;
	for (size_t i = 0 ; i < m_TargetPIDList.size() ; i++) {
		if (m_TargetPIDList[i] == PID)
			return true;
	}
	return false;
}


bool CTsSelector::AddTargetPID(WORD PID)
{
	for (int i = m_TargetPIDList.size()-1 ; i >= 0  ; i--) {
		if (m_TargetPIDList[i] == PID)
			return true;
	}
	m_TargetPIDList.push_back(PID);
	return true;
}


void CALLBACK CTsSelector::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CTsSelector *pThis = static_cast<CTsSelector *>(pParam);
	CPatTable *pPatTable = static_cast<CPatTable *>(pMapTarget);

	// PMTテーブルPIDマップ追加
	for (WORD i = 0 ; i < pPatTable->GetProgramNum() ; i++) {
		if (pThis->m_TargetServiceID == 0
				|| pPatTable->GetProgramID(i) == pThis->m_TargetServiceID) {
			WORD PmtPID = pPatTable->GetPmtPID(i);

			if (pThis->m_TargetServiceID != 0)
				pThis->m_TargetPmtPID = PmtPID;
			pThis->AddTargetPID(PmtPID);
			pMapManager->MapTarget(PmtPID, new CPmtTable, OnPmtUpdated, pParam);
		}
	}
}


void CALLBACK CTsSelector::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CTsSelector *pThis = static_cast<CTsSelector *>(pParam);
	CPmtTable *pPmtTable = static_cast<CPmtTable *>(pMapTarget);

	// PCRのPID追加
	WORD PcrPID = pPmtTable->GetPcrPID();
	if (PcrPID < 0x1FFF)
		pThis->AddTargetPID(PcrPID);

	// ECMのPID追加
	WORD EcmPID = pPmtTable->GetEcmPID();
	if (EcmPID < 0x1FFF)
		pThis->AddTargetPID(EcmPID);

	// ESのPID追加
	for (WORD i = 0 ; i < pPmtTable->GetEsInfoNum() ; i++) {
		pThis->AddTargetPID(pPmtTable->GetEsPID(i));
	}
}


void CALLBACK CTsSelector::OnCatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// CATが更新された
	CTsSelector *pThis = static_cast<CTsSelector *>(pParam);
	CCatTable *pCatTable = static_cast<CCatTable *>(pMapTarget);
	const CCaMethodDesc *pCaMethodDesc = dynamic_cast<const CCaMethodDesc*>(pCatTable->GetCatDesc()->GetDescByTag(CCaMethodDesc::DESC_TAG));

	if (pCaMethodDesc == NULL)
		return;

	// EMMのPID追加
	WORD EmmPID = pCaMethodDesc->GetCaPID();
	if (EmmPID < 0x1FFF)
		pThis->AddTargetPID(EmmPID);
}


bool CTsSelector::MakePat(const CTsPacket *pSrcPacket, CTsPacket *pDstPacket)
{
	const BYTE *pPayloadData = pSrcPacket->GetPayloadData();
	if (pPayloadData == NULL)
		return false;
	const BYTE *pSrcData = pSrcPacket->GetData();
	BYTE *pDstData = pDstPacket->GetData();
	SIZE_T HeaderSize = pPayloadData-pSrcData;

	if (!pSrcPacket->m_Header.bPayloadUnitStartIndicator)
		return false;
	SIZE_T UnitStartPos = pPayloadData[0] + 1;
	pPayloadData += UnitStartPos;
	HeaderSize += UnitStartPos;
	if (HeaderSize >= TS_PACKETSIZE)
		return false;

	::FillMemory(pDstData, TS_PACKETSIZE, 0xFF);
	::CopyMemory(pDstData, pSrcData, HeaderSize);
	pDstData += HeaderSize;

	if (pPayloadData[0] != 0)	// table_id 不正
		return false;

	WORD SectionLength = ((WORD)(pPayloadData[1]&0x0F)<<8) | pPayloadData[2];
	if (SectionLength > TS_PACKETSIZE-HeaderSize-3-4 )
		return false;

	DWORD CRC = ((DWORD)pPayloadData[3+SectionLength-4+0]<<24) |
				((DWORD)pPayloadData[3+SectionLength-4+1]<<16) |
				((DWORD)pPayloadData[3+SectionLength-4+2]<<8) |
				((DWORD)pPayloadData[3+SectionLength-4+3]);
	if (CCrcCalculator::CalcCrc32(pPayloadData, 3+SectionLength-4) != CRC)
		return false;

	WORD TSID = ((WORD)pPayloadData[3]<<8) | pPayloadData[4];
	BYTE Version = (pPayloadData[5]&0x3E)>>1;
	if (TSID != m_LastTSID) {
		m_Version = 0;
	} else if (m_TargetPmtPID != m_LastPmtPID || Version != m_LastVersion) {
		m_Version = (m_Version+1)&0x1F;
	}
	m_LastTSID = TSID;
	m_LastPmtPID = m_TargetPmtPID;
	m_LastVersion = Version;

	const BYTE *pProgramData = pPayloadData+8;
	SIZE_T Pos = 0;
	DWORD NewProgramListSize = 0;
	bool bHasPmtPID = false;
	while (Pos < (SIZE_T)SectionLength-(5+4)) {
		//WORD ProgramNumber = ((WORD)pProgramData[Pos]<<8) | pProgramData[Pos+1];
		WORD PID = ((WORD)(pProgramData[Pos+2]&0x1F)<<8) | pProgramData[Pos+3];

		if (PID == 0x0010 || PID == m_TargetPmtPID) {
			::CopyMemory(pDstData+8+NewProgramListSize, pProgramData+Pos, 4);
			NewProgramListSize += 4;
			if (PID == m_TargetPmtPID)
				bHasPmtPID = true;
		}
		Pos += 4;
	}
	if (!bHasPmtPID)
		return false;

	pDstData[0] = 0;
	pDstData[1] = (pPayloadData[1]&0xF0) | (BYTE)((NewProgramListSize+(5+4))>>8);
	pDstData[2] = (BYTE)((NewProgramListSize+(5+4))&0xFF);
	pDstData[3] = (BYTE)(TSID>>8);
	pDstData[4] = (BYTE)(TSID&0xFF);
	pDstData[5] = (pPayloadData[5]&0xC1) | (m_Version<<1);
	pDstData[6] = pPayloadData[6];
	pDstData[7] = pPayloadData[7];
	CRC = CCrcCalculator::CalcCrc32(pDstData, 8+NewProgramListSize);
	pDstData[8+NewProgramListSize+0] = (BYTE)(CRC>>24);
	pDstData[8+NewProgramListSize+1] = (BYTE)((CRC>>16)&0xFF);
	pDstData[8+NewProgramListSize+2] = (BYTE)((CRC>>8)&0xFF);
	pDstData[8+NewProgramListSize+3] = (BYTE)(CRC&0xFF);

	pDstPacket->ParsePacket();

	return true;
}
