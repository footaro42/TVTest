// TsDescrambler.cpp: CTsDescrambler クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsDescrambler.h"
#include "Multi2Decoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// ECM処理内部クラス
class CEcmProcessor : public CPsiSingleTable
					, public CDynamicReferenceable
{
public:
	CEcmProcessor(CTsDescrambler *pDescrambler);

// CTsPidMapTarget
	virtual void OnPidMapped(const WORD wPID, const PVOID pParam);
	virtual void OnPidUnmapped(const WORD wPID);

// CEcmProcessor
	const bool DescramblePacket(CTsPacket *pTsPacket);
	const bool SetScrambleKey(const BYTE *pEcmData, DWORD EcmSize);

protected:
// CPsiSingleTable
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

private:
	CTsDescrambler *m_pDescrambler;
	CMulti2Decoder m_Multi2Decoder;
#ifdef MULTI2_SSE2
	CMulti2Decoder::DecodeFunc m_pDecodeFunc;
#endif
	bool m_bInQueue;
	CLocalEvent m_SetScrambleKeyEvent;
	volatile bool m_bSetScrambleKey;
	CCriticalLock m_Multi2Lock;

	bool m_bLastEcmSucceed;
};


// ESスクランブル解除内部クラス
class CTsDescrambler::CEsProcessor : public CTsPidMapTarget
{
	CEcmProcessor *m_pEcmProcessor;

public:
	CEsProcessor(CEcmProcessor *pEcmProcessor);
	virtual ~CEsProcessor();
	const CEcmProcessor *GetEcmProcessor() const { return m_pEcmProcessor; }

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket);
	virtual void OnPidMapped(const WORD wPID, const PVOID pParam);
	virtual void OnPidUnmapped(const WORD wPID);
};


class CDescramblePmtTable : public CPmtTable
{
	CTsDescrambler *m_pDescrambler;
	CTsPidMapManager *m_pMapManager;
	CEcmProcessor *m_pEcmProcessor;
	WORD m_EcmPID;
	WORD m_ServiceID;
	std::vector<WORD> m_EsPIDList;
	void UnmapEcmTarget();

public:
	CDescramblePmtTable(CTsDescrambler *pDescrambler);
	void UpdateTarget();
	void SetTarget();
	void ResetTarget();
	// CTsPidMapTarget
	virtual void OnPidUnmapped(const WORD wPID);
};


//////////////////////////////////////////////////////////////////////
// CTsDescrambler 構築/消滅
//////////////////////////////////////////////////////////////////////

CTsDescrambler::CTsDescrambler(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_bDescramble(true)
	, m_InputPacketCount(0)
	, m_ScramblePacketCount(0)
	, m_CurTransportStreamID(0)
	, m_DescrambleServiceID(0)
	, m_Queue(&m_BcasCard)
{
	m_bEnableSSE2 = IsSSE2Available();
	Reset();
}

CTsDescrambler::~CTsDescrambler()
{
	CloseBcasCard();
}

void CTsDescrambler::Reset(void)
{
	CBlockLock Lock(&m_DecoderLock);

	m_Queue.Clear();

	// 内部状態を初期化する
	m_PidMapManager.UnmapAllTarget();

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0000U, new CPatTable, OnPatUpdated, this);

	// 統計データ初期化
	m_InputPacketCount = 0;
	m_ScramblePacketCount = 0;

	m_CurTransportStreamID = 0;

	// スクランブル解除ターゲット初期化
	m_DescrambleServiceID = 0;
	m_ServiceList.clear();
}

const bool CTsDescrambler::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if(dwInputIndex >= GetInputNum())return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;
	*/

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// 入力パケット数カウント
	m_InputPacketCount++;

	if (!pTsPacket->IsScrambled() || m_bDescramble) {
		// PIDルーティング
		m_PidMapManager.StorePacket(pTsPacket);
	} else {
		// 復号漏れパケット数カウント
		m_ScramblePacketCount++;
	}

	// パケットを下流デコーダにデータを渡す
	OutputMedia(pMediaData);

	return true;
}

const bool CTsDescrambler::EnableDescramble(bool bDescramble)
{
	CBlockLock Lock(&m_DecoderLock);

	m_bDescramble = bDescramble;
	return true;
}

const bool CTsDescrambler::OpenBcasCard(CCardReader::ReaderType ReaderType)
{
	CloseBcasCard();

#if 0
	// カードリーダからB-CASカードを検索して開く
	const bool bReturn = m_BcasCard.OpenCard(ReaderType);

	// エラーコードセット
	if(pErrorCode)*pErrorCode = m_BcasCard.GetLastError();

	if (bReturn)
		m_Queue.BeginBcasThread();

	return bReturn;
#else
	// カードリーダにアクセスするスレッドでカードリーダを開く
	// HDUSのカードリーダがCOMを使うため、アクセスするスレッドでCoInitializeする
	bool bOK = m_Queue.BeginBcasThread(ReaderType);

	SetError(m_Queue.GetLastErrorException());

	return bOK;
#endif
}

void CTsDescrambler::CloseBcasCard(void)
{
	m_Queue.EndBcasThread();
	// B-CASカードを閉じる
	//m_BcasCard.CloseCard();
}

const bool CTsDescrambler::IsBcasCardOpen() const
{
	return m_BcasCard.IsCardOpen();
}

const bool CTsDescrambler::GetBcasCardID(BYTE *pCardID)
{
	if (pCardID == NULL)
		return false;

	// カードID取得
	const BYTE *pBuff = m_BcasCard.GetBcasCardID();
	if (pBuff == NULL)
		return false;

	// バッファにコピー
	::CopyMemory(pCardID, pBuff, 6UL);

	return true;
}

LPCTSTR CTsDescrambler::GetCardReaderName() const
{
	return m_BcasCard.GetCardReaderName();
}

int CTsDescrambler::FormatBcasCardID(LPTSTR pszText,int MaxLength) const
{
	return m_BcasCard.FormatCardID(pszText, MaxLength);
}

char CTsDescrambler::GetBcasCardManufacturerID() const
{
	return m_BcasCard.GetCardManufacturerID();
}

BYTE CTsDescrambler::GetBcasCardVersion() const
{
	return m_BcasCard.GetCardVersion();
}

const DWORD CTsDescrambler::GetInputPacketCount(void) const
{
	// 入力パケット数を返す
	return (DWORD)m_InputPacketCount;
}

const DWORD CTsDescrambler::GetScramblePacketCount(void) const
{
	// 復号漏れパケット数を返す
	return (DWORD)m_ScramblePacketCount;
}

void CTsDescrambler::ResetScramblePacketCount(void)
{
	m_ScramblePacketCount = 0;
}

int CTsDescrambler::GetServiceIndexByID(WORD ServiceID) const
{
	int Index;

	// プログラムIDからサービスインデックスを検索する
	for (Index = m_ServiceList.size() - 1 ; Index >= 0  ; Index--) {
		if (m_ServiceList[Index].ServiceID == ServiceID)
			break;
	}

	return Index;
}

bool CTsDescrambler::SetTargetServiceID(WORD ServiceID)
{
	if (m_DescrambleServiceID != ServiceID) {
		TRACE(TEXT("CTsDescrambler::SetTargetServiceID() SID = %d (%04x)\n"),
			  ServiceID, ServiceID);

		CBlockLock Lock(&m_DecoderLock);

		m_DescrambleServiceID = ServiceID;

		for (size_t i = 0 ; i < m_ServiceList.size() ; i++) {
			CDescramblePmtTable *pPmtTable = dynamic_cast<CDescramblePmtTable *>(m_PidMapManager.GetMapTarget(m_ServiceList[i].PmtPID));

			if (pPmtTable) {
				const bool bTarget = m_DescrambleServiceID == 0
						|| m_ServiceList[i].ServiceID == m_DescrambleServiceID;

				if (bTarget && !m_ServiceList[i].bTarget) {
					pPmtTable->SetTarget();
					m_ServiceList[i].bTarget = true;
				}
			}
		}

		for (size_t i = 0 ; i < m_ServiceList.size() ; i++) {
			CDescramblePmtTable *pPmtTable = dynamic_cast<CDescramblePmtTable *>(m_PidMapManager.GetMapTarget(m_ServiceList[i].PmtPID));

			if (pPmtTable) {
				const bool bTarget = m_DescrambleServiceID == 0
						|| m_ServiceList[i].ServiceID == m_DescrambleServiceID;

				if (!bTarget && m_ServiceList[i].bTarget) {
					pPmtTable->ResetTarget();
					m_ServiceList[i].bTarget = false;
				}
			}
		}

#ifdef _DEBUG
		PrintStatus();
#endif
	}
	return true;
}

bool CTsDescrambler::IsSSE2Available()
{
#ifdef MULTI2_SSE2
	return CMulti2Decoder::IsSSE2Available();
#else
	return false;
#endif
}

bool CTsDescrambler::EnableSSE2(bool bEnable)
{
	if (bEnable && !IsSSE2Available())
		return false;
	m_bEnableSSE2 = bEnable;
	return true;
}

void CALLBACK CTsDescrambler::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CTsDescrambler *pThis = static_cast<CTsDescrambler *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);

	TRACE(TEXT("CTsDescrambler::OnPatUpdated()\n"));

	const WORD TsID = pPatTable->GetTransportStreamID();
	if (TsID != pThis->m_CurTransportStreamID) {
		// TSIDが変化したらリセットする
		pThis->m_Queue.Clear();
		for (WORD PID = 0x0001 ; PID < 0x2000 ; PID++)
			pThis->m_PidMapManager.UnmapTarget(PID);
		pThis->m_ServiceList.clear();
		pThis->m_CurTransportStreamID = TsID;
	} else {
		// 無くなったPMTをスクランブル解除対象から除外する
		for (size_t i = 0 ; i < pThis->m_ServiceList.size() ; i++) {
			const WORD PmtPID = pThis->m_ServiceList[i].PmtPID;
			WORD j;

			for (j = 0 ; j < pPatTable->GetProgramNum() ; j++) {
				if (pPatTable->GetPmtPID(j) == PmtPID)
					break;
			}
			if (j == pPatTable->GetProgramNum()) {
				pThis->m_PidMapManager.UnmapTarget(PmtPID);
				pThis->m_ServiceList[i].bTarget = false;
			}
		}
	}

	std::vector<TAG_SERVICEINFO> ServiceList;
	ServiceList.resize(pPatTable->GetProgramNum());
	for (WORD i = 0 ; i < pPatTable->GetProgramNum() ; i++) {
		const WORD PmtPID = pPatTable->GetPmtPID(i);
		const WORD ServiceID = pPatTable->GetProgramID(i);

		ServiceList[i].bTarget = pThis->m_DescrambleServiceID == 0
								|| ServiceID == pThis->m_DescrambleServiceID;
		ServiceList[i].ServiceID = ServiceID;
		ServiceList[i].PmtPID = PmtPID;
		size_t j;
		for (j = 0 ; j < pThis->m_ServiceList.size() ; j++) {
			if (pThis->m_ServiceList[j].PmtPID == PmtPID)
				break;
		}
		if (j < pThis->m_ServiceList.size()) {
			ServiceList[i].EcmPID = pThis->m_ServiceList[j].EcmPID;
			ServiceList[i].EsPIDList = pThis->m_ServiceList[j].EsPIDList;
		} else {
			ServiceList[i].EcmPID = 0xFFFF;
			ServiceList[i].EsPIDList.clear();
		}

		CDescramblePmtTable *pPmtTable = dynamic_cast<CDescramblePmtTable *>(pThis->m_PidMapManager.GetMapTarget(PmtPID));
		if (pPmtTable == NULL)
			pThis->m_PidMapManager.MapTarget(PmtPID, new CDescramblePmtTable(pThis), OnPmtUpdated, pThis);
	}
	pThis->m_ServiceList = ServiceList;
}

void CALLBACK CTsDescrambler::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CTsDescrambler *pThis = static_cast<CTsDescrambler *>(pParam);
	CDescramblePmtTable *pPmtTable = dynamic_cast<CDescramblePmtTable *>(pMapTarget);

	const WORD ServiceID = pPmtTable->GetProgramNumberID();
	const int ServiceIndex = pThis->GetServiceIndexByID(ServiceID);
	if (ServiceIndex < 0)
		return;

	TRACE(TEXT("CTsDescrambler::OnPmtUpdated() SID = %04d\n"), ServiceID);

	pThis->m_ServiceList[ServiceIndex].EcmPID = pPmtTable->GetEcmPID();

	pThis->m_ServiceList[ServiceIndex].EsPIDList.resize(pPmtTable->GetEsInfoNum());
	for (WORD i = 0 ; i < pPmtTable->GetEsInfoNum() ; i++)
		pThis->m_ServiceList[ServiceIndex].EsPIDList[i] = pPmtTable->GetEsPID(i);

	pThis->m_ServiceList[ServiceIndex].bTarget = pThis->m_DescrambleServiceID == 0
								|| ServiceID == pThis->m_DescrambleServiceID;
	if (pThis->m_ServiceList[ServiceIndex].bTarget)
		pPmtTable->UpdateTarget();
	else
		pPmtTable->ResetTarget();

#ifdef _DEBUG
	pThis->PrintStatus();
#endif
}

#ifdef _DEBUG
void CTsDescrambler::PrintStatus(void) const
{
	TRACE(TEXT("****** Descramble ES PIDs ******\n"));
	for (WORD PID = 0x0001 ; PID < 0x2000 ; PID++) {
		CEsProcessor *pEsProcessor = dynamic_cast<CEsProcessor*>(m_PidMapManager.GetMapTarget(PID));

		if (pEsProcessor)
			TRACE(TEXT("ES PID = %04x (%d)\n"), PID, PID);
	}
	TRACE(TEXT("****** Descramble ECM PIDs ******\n"));
	for (WORD PID = 0x0001 ; PID < 0x2000 ; PID++) {
		CEcmProcessor *pEcmProcessor = dynamic_cast<CEcmProcessor*>(m_PidMapManager.GetMapTarget(PID));

		if (pEcmProcessor)
			TRACE(TEXT("ECM PID = %04x (%d)\n"), PID, PID);
	}
}
#endif


CDescramblePmtTable::CDescramblePmtTable(CTsDescrambler *pDescrambler)
	: m_pDescrambler(pDescrambler)
	, m_pMapManager(&pDescrambler->m_PidMapManager)
	, m_pEcmProcessor(NULL)
	, m_EcmPID(0)
	, m_ServiceID(0)
{
}

void CDescramblePmtTable::UnmapEcmTarget()
{
	// ECMが他のスクランブル解除対象サービスと異なる場合はアンマップ
	bool bFinded = false;
	for (size_t i = 0 ; i < m_pDescrambler->m_ServiceList.size() ; i++) {
		if (m_pDescrambler->m_ServiceList[i].ServiceID != m_ServiceID
				&& m_pDescrambler->m_ServiceList[i].bTarget
				&& m_pDescrambler->m_ServiceList[i].EcmPID == m_EcmPID) {
			bFinded = true;
			break;
		}
	}
	if (!bFinded)
		m_pMapManager->UnmapTarget(m_EcmPID);

	// ESのPIDマップ削除
	for (size_t i = 0 ; i < m_EsPIDList.size() ; i++) {
		const WORD EsPID = m_EsPIDList[i];

		bFinded = false;
		for (size_t j = 0 ; j < m_pDescrambler->m_ServiceList.size() ; j++) {
			if (m_pDescrambler->m_ServiceList[j].ServiceID != m_ServiceID
					&& m_pDescrambler->m_ServiceList[j].bTarget) {
				for (size_t k = 0 ; k < m_pDescrambler->m_ServiceList[j].EsPIDList.size() ; k++) {
					if (m_pDescrambler->m_ServiceList[j].EsPIDList[k] == EsPID) {
						bFinded = true;
						break;
					}
				}
				if (bFinded)
					break;
			}
		}
		if (!bFinded)
			m_pMapManager->UnmapTarget(EsPID);
	}
}

void CDescramblePmtTable::UpdateTarget()
{
	// PMTが更新された
	WORD EcmPID = GetEcmPID();
	if (EcmPID >= 0x1FFF)
		EcmPID = 0;

	if (m_EcmPID != EcmPID) {
		// ECMのPIDが変わった
		if (m_EcmPID != 0) {
			ResetTarget();
		}
		if (EcmPID != 0) {
			m_pEcmProcessor = dynamic_cast<CEcmProcessor*>(m_pMapManager->GetMapTarget(EcmPID));

			if (m_pEcmProcessor == NULL) {
				// ECM処理内部クラス新規マップ
				m_pEcmProcessor = new CEcmProcessor(m_pDescrambler);
				m_pMapManager->MapTarget(EcmPID, m_pEcmProcessor);
			}
		}
		m_EcmPID = EcmPID;
	}

	if (m_pEcmProcessor) {
		// ESのPIDマップ追加
		m_EsPIDList.resize(GetEsInfoNum());
		for (WORD i = 0 ; i < GetEsInfoNum() ; i++) {
			const WORD EsPID = GetEsPID(i);
			const CTsDescrambler::CEsProcessor *pEsProcessor = dynamic_cast<CTsDescrambler::CEsProcessor*>(m_pMapManager->GetMapTarget(EsPID));

			if (pEsProcessor == NULL
					|| pEsProcessor->GetEcmProcessor() != m_pEcmProcessor)
				m_pMapManager->MapTarget(EsPID, new CTsDescrambler::CEsProcessor(m_pEcmProcessor));
			m_EsPIDList[i] = EsPID;
		}
	}

	m_ServiceID = GetProgramNumberID();
}

void CDescramblePmtTable::SetTarget()
{
	// スクランブル解除対象に設定
	if (m_EcmPID == 0)
		UpdateTarget();
}

void CDescramblePmtTable::ResetTarget()
{
	// スクランブル解除対象から除外
	if (m_EcmPID != 0) {
		// ECMとESのPIDをアンマップ
		UnmapEcmTarget();

		m_pEcmProcessor = NULL;
		m_EcmPID = 0;
		m_EsPIDList.clear();
	}
}

void CDescramblePmtTable::OnPidUnmapped(const WORD wPID)
{
	ResetTarget();

	CPmtTable::OnPidUnmapped(wPID);
}


//////////////////////////////////////////////////////////////////////
// CEcmProcessor 構築/消滅
//////////////////////////////////////////////////////////////////////

CEcmProcessor::CEcmProcessor(CTsDescrambler *pDescrambler)
	: CPsiSingleTable(true)
	, m_pDescrambler(pDescrambler)
	, m_bInQueue(false)
	, m_bLastEcmSucceed(true)
{
#ifdef MULTI2_SSE2
	if (pDescrambler->m_bEnableSSE2)
		m_pDecodeFunc = &CMulti2Decoder::DecodeSSE2;
	else
		m_pDecodeFunc = &CMulti2Decoder::Decode;
#endif

	// MULTI2デコーダにシステムキーと初期CBCをセット
	if (m_pDescrambler->m_BcasCard.IsCardOpen())
		m_Multi2Decoder.Initialize(m_pDescrambler->m_BcasCard.GetSystemKey(),
								   m_pDescrambler->m_BcasCard.GetInitialCbc());

	m_SetScrambleKeyEvent.Create(true);
}

void CEcmProcessor::OnPidMapped(const WORD wPID, const PVOID pParam)
{
	TRACE(TEXT("CEcmProcessor::OnPidMapped() PID = %d (0x%04x)\n"), wPID, wPID);
	AddRef();
}

void CEcmProcessor::OnPidUnmapped(const WORD wPID)
{
	TRACE(TEXT("CEcmProcessor::OnPidUnmapped() PID = %d (0x%04x)\n"), wPID, wPID);

	//CPsiSingleTable::OnPidUnmapped(wPID);
	ReleaseRef();
}

const bool CEcmProcessor::DescramblePacket(CTsPacket *pTsPacket)
{
	if (!m_bInQueue) {
		// まだECMが来ていない
		m_pDescrambler->m_ScramblePacketCount++;
		return false;
	}

	// キー取得中だったら待つ
	if (m_SetScrambleKeyEvent.Wait(500) == WAIT_TIMEOUT) {
		m_pDescrambler->m_ScramblePacketCount++;
		return false;
	}

	// スクランブル解除
	if (m_bLastEcmSucceed) {
		CBlockLock Lock(&m_Multi2Lock);

#ifdef MULTI2_SSE2
		if ((m_Multi2Decoder.*m_pDecodeFunc)
#else
		if (m_Multi2Decoder.Decode
#endif
				(pTsPacket->GetPayloadData(),
				(DWORD)pTsPacket->GetPayloadSize(),
				pTsPacket->m_Header.byTransportScramblingCtrl)) {
			// トランスポートスクランブル制御再設定
			pTsPacket->SetAt(3UL, pTsPacket->GetAt(3UL) & 0x3FU);
			pTsPacket->m_Header.byTransportScramblingCtrl = 0U;
			return true;
		}
	}

	m_pDescrambler->m_ScramblePacketCount++;

	return false;
}

const bool CEcmProcessor::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
	if (pCurSection->GetTableID() != 0x82)
		return false;

#if 0
	return SetScrambleKey(pCurSection->GetPayloadData(), pCurSection->GetPayloadSize());
#else
	// B-CASアクセスキューに追加
	if (m_pDescrambler->m_Queue.Enqueue(this, pCurSection->GetPayloadData(), pCurSection->GetPayloadSize()))
		m_bInQueue = true;
#endif
	return true;
}

const bool CEcmProcessor::SetScrambleKey(const BYTE *pEcmData, DWORD EcmSize)
{
	// ECMをB-CASカードに渡してキー取得
	const BYTE *pKsData = m_pDescrambler->m_BcasCard.GetKsFromEcm(pEcmData, EcmSize);

	// ECM処理失敗時は一度だけB-CASカードを再初期化する
	if (!pKsData && m_bLastEcmSucceed
			&& (m_pDescrambler->m_BcasCard.GetLastErrorCode() != BCEC_ECMREFUSED)) {
		if (m_pDescrambler->m_BcasCard.ReOpenCard()) {
			TRACE(TEXT("CEcmProcessor::SetScrambleKey() Re open card.\n"));
			m_Multi2Decoder.Initialize(m_pDescrambler->m_BcasCard.GetSystemKey(),
									   m_pDescrambler->m_BcasCard.GetInitialCbc());
			pKsData = m_pDescrambler->m_BcasCard.GetKsFromEcm(pEcmData, EcmSize);
		}
	}

	// スクランブルキー更新
	m_Multi2Lock.Lock();
	m_Multi2Decoder.SetScrambleKey(pKsData);
	m_Multi2Lock.Unlock();

	// ECM処理成功状態更新
	m_bLastEcmSucceed = pKsData != NULL;

	m_SetScrambleKeyEvent.Set();

	return true;
}


//////////////////////////////////////////////////////////////////////
// CTsDescrambler::CEsProcessor 構築/消滅
//////////////////////////////////////////////////////////////////////

CTsDescrambler::CEsProcessor::CEsProcessor(CEcmProcessor *pEcmProcessor)
	: CTsPidMapTarget()
	, m_pEcmProcessor(pEcmProcessor)
{
}

CTsDescrambler::CEsProcessor::~CEsProcessor()
{
}

const bool CTsDescrambler::CEsProcessor::StorePacket(const CTsPacket *pPacket)
{
	// スクランブル解除
	if (pPacket->IsScrambled()
			&& !m_pEcmProcessor->DescramblePacket(const_cast<CTsPacket *>(pPacket)))
		return false;

	return true;
}

void CTsDescrambler::CEsProcessor::OnPidMapped(const WORD wPID, const PVOID pParam)
{
	TRACE(TEXT("CEsProcessor::OnPidMapped() PID = %d (0x%04x)\n"), wPID, wPID);
}

void CTsDescrambler::CEsProcessor::OnPidUnmapped(const WORD wPID)
{
	TRACE(TEXT("CEsProcessor::OnPidUnmapped() PID = %d (0x%04x)\n"), wPID, wPID);
	delete this;
}




CBcasAccess::CBcasAccess(CEcmProcessor *pEcmProcessor, const BYTE *pData, DWORD Size)
{
	m_pEcmProcessor = pEcmProcessor;
	m_pEcmProcessor->AddRef();
	::CopyMemory(m_EcmData, pData, Size);
	m_EcmSize = Size;
}


CBcasAccess::CBcasAccess(const CBcasAccess &BcasAccess)
{
	m_pEcmProcessor = BcasAccess.m_pEcmProcessor;
	m_pEcmProcessor->AddRef();
	::CopyMemory(m_EcmData, BcasAccess.m_EcmData, BcasAccess.m_EcmSize);
	m_EcmSize = BcasAccess.m_EcmSize;
}


CBcasAccess::~CBcasAccess()
{
	m_pEcmProcessor->ReleaseRef();
}


CBcasAccess &CBcasAccess::operator=(const CBcasAccess &BcasAccess)
{
	m_pEcmProcessor->ReleaseRef();
	m_pEcmProcessor = BcasAccess.m_pEcmProcessor;
	m_pEcmProcessor->AddRef();
	::CopyMemory(m_EcmData, BcasAccess.m_EcmData, BcasAccess.m_EcmSize);
	m_EcmSize = BcasAccess.m_EcmSize;
	return *this;
}


bool CBcasAccess::SetScrambleKey()
{
	return m_pEcmProcessor->SetScrambleKey(m_EcmData, m_EcmSize);
}




CBcasAccessQueue::CBcasAccessQueue(CBcasCard *pBcasCard)
	: m_pBcasCard(pBcasCard)
	, m_hThread(NULL)
{
}


CBcasAccessQueue::~CBcasAccessQueue()
{
	EndBcasThread();
	//Clear();
}


void CBcasAccessQueue::Clear()
{
	CBlockLock Lock(&m_Lock);

	m_Queue.clear();
}


bool CBcasAccessQueue::Enqueue(CEcmProcessor *pEcmProcessor, const BYTE *pData, DWORD Size)
{
	if (m_hThread == NULL || Size > 256)
		return false;

	CBlockLock Lock(&m_Lock);

	CBcasAccess BcasAccess(pEcmProcessor, pData, Size);
	m_Queue.push_back(BcasAccess);
	m_Event.Set();
	return true;
}


bool CBcasAccessQueue::BeginBcasThread(CCardReader::ReaderType ReaderType)
{
	if (m_hThread)
		return false;
	m_ReaderType = ReaderType;
	if (m_Event.IsCreated())
		m_Event.Reset();
	else
		m_Event.Create();
	m_bKillEvent = false;
	m_bStartEvent = false;
	m_hThread = ::CreateThread(NULL, 0, BcasAccessThread, this, 0, NULL);
	if (m_hThread == NULL)
		return false;
	m_Event.Wait();
	if (!m_pBcasCard->IsCardOpen()) {
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		return false;
	}
	m_bStartEvent = true;
	ClearError();
	return true;
}


bool CBcasAccessQueue::EndBcasThread()
{
	if (m_hThread) {
		m_bKillEvent = true;
		m_Event.Set();
		Clear();
		if (::WaitForSingleObject(m_hThread, 1000) == WAIT_TIMEOUT) {
			TRACE(TEXT("Terminate BcasAccessThread\n"));
			::TerminateThread(m_hThread, 1);
		}
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}
	return true;
}


DWORD CALLBACK CBcasAccessQueue::BcasAccessThread(LPVOID lpParameter)
{
	CBcasAccessQueue *pThis=static_cast<CBcasAccessQueue*>(lpParameter);

	// カードリーダからB-CASカードを検索して開く
	if (!pThis->m_pBcasCard->OpenCard(pThis->m_ReaderType)) {
		pThis->SetError(pThis->m_pBcasCard->GetLastErrorException());
		pThis->m_Event.Set();
		return 1;
	}
	pThis->m_Event.Set();
	while (!pThis->m_bStartEvent)
		::Sleep(0);

	while (true) {
		pThis->m_Event.Wait();
		if (pThis->m_bKillEvent)
			break;
		while (true) {
			pThis->m_Lock.Lock();
			if (pThis->m_Queue.empty()) {
				pThis->m_Lock.Unlock();
				break;
			}
			CBcasAccess BcasAccess = pThis->m_Queue.front();
			pThis->m_Queue.pop_front();
			pThis->m_Lock.Unlock();
			BcasAccess.SetScrambleKey();
		}
	}

	// B-CASカードを閉じる
	pThis->m_pBcasCard->CloseCard();

	return 0;
}
