// TsDescrambler.cpp: CTsDescrambler クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsDescrambler.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CTsDescrambler 構築/消滅
//////////////////////////////////////////////////////////////////////

CTsDescrambler::CTsDescrambler(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_InputPacketCount(0)
	, m_ScramblePacketCount(0)
	, m_DescrambleServiceID(0)
	, m_Queue(&m_BcasCard)
{
	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0000U, new CPatTable, OnPatUpdated, this);
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

	// スクランブル解除ターゲット初期化
	m_DescramblePIDList.clear();
	m_DescrambleServiceID=0;

	// 下流デコーダを初期化する
	ResetDownstreamDecoder();
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

	if (!pTsPacket->IsScrambled() || IsTargetPID(pTsPacket->GetPID())) {
		// PIDルーティング
		m_PidMapManager.StorePacket(pTsPacket);

		/*
		if (pTsPacket->IsScrambled()) {
			// 復号漏れパケット数カウント
			m_ScramblePacketCount++;
		}
		*/
	}

	// パケットを下流デコーダにデータを渡す
	OutputMedia(pMediaData);

	return true;
}

const bool CTsDescrambler::OpenBcasCard(CCardReader::ReaderType ReaderType,DWORD *pErrorCode)
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
	bool bOK=m_Queue.BeginBcasThread(ReaderType);

	// エラーコードセット
	if(pErrorCode)*pErrorCode = m_BcasCard.GetLastError();

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
	// カードID取得
	const BYTE *pBuff = m_BcasCard.GetBcasCardID();

	// バッファにコピー
	if(pCardID && pBuff)::CopyMemory(pCardID, pBuff, 6UL);

	return (pBuff)? true : false;
}

LPCTSTR CTsDescrambler::GetCardReaderName() const
{
	return m_BcasCard.GetCardReaderName();
}

int CTsDescrambler::FormatBcasCardID(LPTSTR pszText,int MaxLength) const
{
	return m_BcasCard.FormatCardID(pszText,MaxLength);
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
	m_ScramblePacketCount=0;
}

bool CTsDescrambler::SetTargetPID(const WORD *pPIDList,int NumPIDs)
{
	CBlockLock Lock(&m_DecoderLock);

	TRACE(TEXT("CTsDescrambler::SetTargetPID()\n"));
	m_DescrambleListLock.Lock();
	m_DescramblePIDList.clear();
	if (pPIDList!=NULL) {
		for (int i=0;i<NumPIDs;i++) {
			TRACE(TEXT("Descramble target PID %d = %04x\n"),i,pPIDList[i]);
			m_DescramblePIDList.push_back(pPIDList[i]);
		}
	}
	m_DescrambleServiceID=0;
	m_DescrambleListLock.Unlock();
	return true;
}

bool CTsDescrambler::IsTargetPID(WORD PID)
{
	CBlockLock Lock(&m_DescrambleListLock);

	if (m_DescramblePIDList.size()==0)
		return true;
	for (size_t i=0;i<m_DescramblePIDList.size();i++) {
		if (m_DescramblePIDList[i]==PID)
			return true;
	}
	return false;
}

bool CTsDescrambler::HasTargetPID(const std::vector<WORD> *pList)
{
	CBlockLock Lock(&m_DescrambleListLock);

	if (m_DescramblePIDList.size()==0)
		return true;
	for (size_t i=0;i<pList->size();i++) {
		for (size_t j=0;j<m_DescramblePIDList.size();j++) {
			if (m_DescramblePIDList[j]==(*pList)[i])
				return true;
		}
	}
	return false;
}

bool CTsDescrambler::SetTargetServiceID(WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	m_DescrambleListLock.Lock();

	m_DescramblePIDList.clear();
	m_DescrambleServiceID=ServiceID;

	CPatTable *pPatTable=dynamic_cast<CPatTable *>(m_PidMapManager.GetMapTarget(0x0000));
	if (pPatTable!=NULL) {
		for (WORD i=0;i<pPatTable->GetProgramNum();i++) {
			WORD PmtPID=pPatTable->GetPmtPID(i);

			if (m_DescrambleServiceID==0
					|| pPatTable->GetProgramID(i)==m_DescrambleServiceID) {
				if (m_PidMapManager.GetMapTarget(PmtPID)==NULL)
					m_PidMapManager.MapTarget(PmtPID,new CPmtTable,OnPmtUpdated,this);
			} else {
				m_PidMapManager.UnmapTarget(PmtPID);
			}
		}
	}
	m_DescrambleListLock.Unlock();
	return true;
}

void CTsDescrambler::IncrementScramblePacketCount()
{
	m_ScramblePacketCount++;
}

void CALLBACK CTsDescrambler::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CTsDescrambler *pThis = static_cast<CTsDescrambler *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);

#ifdef _DEBUG
	if(!pPatTable)::DebugBreak();
#endif

	// PMTテーブルPIDマップ追加
	for(WORD wIndex = 0U ; wIndex < pPatTable->GetProgramNum() ; wIndex++){
		if (pThis->m_DescrambleServiceID==0
			|| pPatTable->GetProgramID(wIndex)==pThis->m_DescrambleServiceID)
			pMapManager->MapTarget(pPatTable->GetPmtPID(wIndex), new CPmtTable, OnPmtUpdated, pParam);
	}
}

void CALLBACK CTsDescrambler::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CTsDescrambler *pThis = static_cast<CTsDescrambler *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);

#ifdef _DEBUG
	if(!pPmtTable)::DebugBreak();
#endif

	// ECMのPIDマップ追加
	const WORD wEcmPID = pPmtTable->GetEcmPID();
	if(wEcmPID >= 0x1FFFU)return;

	// 既存のECM処理ターゲットを確認
	CEcmProcessor *pEcmProcessor = dynamic_cast<CEcmProcessor *>(pMapManager->GetMapTarget(wEcmPID));

	if (!pEcmProcessor) {
		// ECM処理内部クラス新規マップ
		pEcmProcessor = new CEcmProcessor(pThis, &pThis->m_BcasCard, &pThis->m_Queue);
		pMapManager->MapTarget(wEcmPID, pEcmProcessor);
	}

	// ESのPIDマップ追加
	for (WORD wIndex = 0U ; wIndex < pPmtTable->GetEsInfoNum() ; wIndex++) {
		pEcmProcessor->AddEsPID(pPmtTable->GetEsPID(wIndex));
		pMapManager->MapTarget(pPmtTable->GetEsPID(wIndex), new CEsProcessor(pEcmProcessor), NULL, pParam);
	}
}


//////////////////////////////////////////////////////////////////////
// CEcmProcessor 構築/消滅
//////////////////////////////////////////////////////////////////////

CEcmProcessor::CEcmProcessor(CTsDescrambler *pDescrambler, CBcasCard *pBcasCard, CBcasAccessQueue *pQueue)
	: CDynamicReferenceable()
	, CPsiSingleTable(true)
	, m_pDescrambler(pDescrambler)
	, m_pBcasCard(pBcasCard)
	, m_pQueue(pQueue)
	, m_bInQueue(false)
	, m_bSetScrambleKey(false)
	, m_bLastEcmSucceed(true)
{
	// MULTI2デコーダにシステムキーと初期CBCをセット
	if (m_pBcasCard->IsCardOpen())
		m_Multi2Decoder.Initialize(m_pBcasCard->GetSystemKey(),
								   m_pBcasCard->GetInitialCbc());
}

void CEcmProcessor::OnPidMapped(const WORD wPID, const PVOID pParam)
{
	// 参照カウント追加
	AddRef();
}

void CEcmProcessor::OnPidUnmapped(const WORD wPID)
{
	// 参照カウント開放
	ReleaseRef();
}

const bool CEcmProcessor::DescramblePacket(CTsPacket *pTsPacket)
{
	if (!m_bInQueue) {	// まだECMが来ていない
		m_pDescrambler->IncrementScramblePacketCount();
		return false;
	}
	for (int i=0;!m_bSetScrambleKey;i++) {
		if (i==500)	// デッドロック回避
			return false;
		Sleep(1);
	}
	// スクランブル解除
	m_Multi2Lock.Lock();
	bool bOK=m_Multi2Decoder.Decode(pTsPacket->GetPayloadData(),
									(DWORD)pTsPacket->GetPayloadSize(),
									pTsPacket->m_Header.byTransportScramblingCtrl);
	m_Multi2Lock.Unlock();
	if (bOK) {
		// トランスポートスクランブル制御再設定
		pTsPacket->SetAt(3UL, pTsPacket->GetAt(3UL) & 0x3FU);
		pTsPacket->m_Header.byTransportScramblingCtrl = 0U;
	} else {
		m_pDescrambler->IncrementScramblePacketCount();
	}
	return bOK;
}

const bool CEcmProcessor::OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection)
{
#if 0
	return SetScrambleKey(pCurSection->GetPayloadData(), pCurSection->GetPayloadSize());
#else
	// B-CASアクセスキューに追加
	if (m_pQueue->Enqueue(this, pCurSection->GetPayloadData(), pCurSection->GetPayloadSize()))
		m_bInQueue=true;
#endif
	return true;
}

const bool CEcmProcessor::SetScrambleKey(const BYTE *pEcmData, DWORD EcmSize)
{
	if (!m_pDescrambler->HasTargetPID(&m_EsPIDList)) {
		m_bSetScrambleKey=true;
		return false;
	}

	// ECMをB-CASカードに渡してキー取得
	const BYTE *pKsData = m_pBcasCard->GetKsFromEcm(pEcmData, EcmSize);

	// ECM処理失敗時は一度だけB-CASカードを再初期化する
	if (!pKsData && m_bLastEcmSucceed && (m_pBcasCard->GetLastError() != BCEC_ECMREFUSED)){
		if (m_pBcasCard->ReOpenCard()) {
			TRACE(TEXT("CTsDescrambler::CEcmProcessor::SetScrambleKey() Re open card.\n"));
			pKsData = m_pBcasCard->GetKsFromEcm(pEcmData, EcmSize);
		}
	}

	// スクランブルキー更新
	m_Multi2Lock.Lock();
	m_Multi2Decoder.SetScrambleKey(pKsData);
	m_Multi2Lock.Unlock();

	// ECM処理成功状態更新
	m_bLastEcmSucceed = pKsData!=NULL;

	m_bSetScrambleKey=true;

	return true;
}

const bool CEcmProcessor::AddEsPID(WORD PID)
{
	m_EsPIDList.push_back(PID);
	return true;
}


//////////////////////////////////////////////////////////////////////
// CTsDescrambler::CEsProcessor 構築/消滅
//////////////////////////////////////////////////////////////////////

CTsDescrambler::CEsProcessor::CEsProcessor(CEcmProcessor *pEcmProcessor)
	: CTsPidMapTarget()
	, m_pEcmProcessor(pEcmProcessor)
{
	// 参照カウント追加
	m_pEcmProcessor->AddRef();
}

CTsDescrambler::CEsProcessor::~CEsProcessor()
{
	// 参照カウント削除
	m_pEcmProcessor->ReleaseRef();
}

const bool CTsDescrambler::CEsProcessor::StorePacket(const CTsPacket *pPacket)
{
	// スクランブル解除
	m_pEcmProcessor->DescramblePacket(const_cast<CTsPacket *>(pPacket));

	return false;
}

void CTsDescrambler::CEsProcessor::OnPidUnmapped(const WORD wPID)
{
	// インスタンス開放
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
	, m_hEvent(NULL)
{
}


CBcasAccessQueue::~CBcasAccessQueue()
{
	EndBcasThread();
	//Clear();
	if (m_hEvent)
		::CloseHandle(m_hEvent);
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
	::SetEvent(m_hEvent);
	return true;
}


bool CBcasAccessQueue::BeginBcasThread(CCardReader::ReaderType ReaderType)
{
	if (m_hThread)
		return false;
	m_ReaderType = ReaderType;
	if (m_hEvent == NULL)
		m_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	else
		::ResetEvent(m_hEvent);
	m_bKillEvent = false;
	m_bStartEvent = false;
	m_hThread = ::CreateThread(NULL, 0, BcasAccessThread, this, 0, NULL);
	if (m_hThread == NULL)
		return false;
	::WaitForSingleObject(m_hEvent, INFINITE);
	if (!m_pBcasCard->IsCardOpen()) {
		::WaitForSingleObject(m_hThread, INFINITE);
		::CloseHandle(m_hThread);
		m_hThread = NULL;
		return false;
	}
	m_bStartEvent = true;
	return true;
}


bool CBcasAccessQueue::EndBcasThread()
{
	if (m_hThread) {
		m_bKillEvent = true;
		::SetEvent(m_hEvent);
		Clear();
		if (::WaitForSingleObject(m_hThread, 1000) == WAIT_TIMEOUT) {
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
	bool bOK = pThis->m_pBcasCard->OpenCard(pThis->m_ReaderType);
	::SetEvent(pThis->m_hEvent);
	if (!bOK)
		return 1;
	while (!pThis->m_bStartEvent)
		::Sleep(0);

	while (true) {
		::WaitForSingleObject(pThis->m_hEvent, INFINITE);
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
