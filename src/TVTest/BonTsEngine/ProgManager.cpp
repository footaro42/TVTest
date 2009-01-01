// ProgManager.cpp: CProgManager クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsTable.h"
//#include "TsProgGuide.h"
#include "ProgManager.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CProgManager 構築/消滅
//////////////////////////////////////////////////////////////////////

CProgManager::CProgManager(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1UL, 1UL)
	, m_pProgDatabase(new CProgDatabase(*this))
{
//	m_PidMapManager.MapTarget(0x0012U, new CEitParser(NULL));
}


CProgManager::~CProgManager()
{
	// プログラムデータベースインスタンス開放
	delete m_pProgDatabase;
}


void CProgManager::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	// サービスリストをクリア
	m_ServiceList.clear();

	// プログラムデータベースリセット
	m_pProgDatabase->Reset();

	// 下位デコーダをリセット
	ResetDownstreamDecoder();
}


const bool CProgManager::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if(dwInputIndex >= GetInputNum())return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;
	*/

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// PIDルーティング
	m_PidMapManager.StorePacket(pTsPacket);

	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}


const WORD CProgManager::GetServiceNum(void)
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス数を返す
	return m_ServiceList.size();
}


const bool CProgManager::GetServiceID(WORD *pwServiceID, const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pwServiceID == NULL)
		return false;

	// サービスIDを取得する
	if (wIndex == 0xFFFF) {
		if (m_pProgDatabase->m_ServiceList.size() == 0
				|| !m_pProgDatabase->m_ServiceList[0].bIsUpdated)
			return false;
		*pwServiceID = m_pProgDatabase->m_ServiceList[0].wServiceID;
	} else if ((size_t)wIndex < m_ServiceList.size()) {
		*pwServiceID = m_ServiceList[wIndex].wServiceID;
	} else {
		return false;
	}
	return true;
}


const bool CProgManager::GetVideoEsPID(WORD *pwVideoPID, const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pwVideoPID && (size_t)wIndex < m_ServiceList.size()) {
		*pwVideoPID = m_ServiceList[wIndex].wVideoEsPID;
		return true;
	}
	return false;
}


const bool CProgManager::GetAudioEsPID(WORD *pwAudioPID, const WORD wAudioIndex, const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pwAudioPID && (size_t)wIndex < m_ServiceList.size()
			&& (size_t)wAudioIndex < m_ServiceList[wIndex].AudioEsList.size()) {
		*pwAudioPID = m_ServiceList[wIndex].AudioEsList[wAudioIndex].PID;
		return true;
	}
	return false;
}


const BYTE CProgManager::GetAudioComponentTag(const WORD wAudioIndex,const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)wIndex < m_ServiceList.size()
			&& (size_t)wAudioIndex < m_ServiceList[wIndex].AudioEsList.size()) {
		return m_ServiceList[wIndex].AudioEsList[wAudioIndex].ComponentTag;
	}
	return 0;
}


const BYTE CProgManager::GetAudioComponentType(const WORD wAudioIndex,const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)wIndex < m_ServiceList.size()
			&& (size_t)wAudioIndex < m_ServiceList[wIndex].AudioEsList.size()) {
		const CDescBlock *pDescBlock=GetHEitItemDesc(wIndex);

		if (pDescBlock) {
			for (WORD i=0;i<pDescBlock->GetDescNum();i++) {
				const CBaseDesc *pDesc=pDescBlock->GetDescByIndex(i);

				if (pDesc->GetTag()==CAudioComponentDesc::DESC_TAG) {
					const CAudioComponentDesc *pAudioDesc=dynamic_cast<const CAudioComponentDesc*>(pDesc);

					if (pAudioDesc->GetComponentTag()==m_ServiceList[wIndex].AudioEsList[wAudioIndex].ComponentTag)
						return pAudioDesc->GetComponentType();
				}
			}
		}
	}
	return 0;
}


const WORD CProgManager::GetAudioEsNum(const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)wIndex < m_ServiceList.size())
		return m_ServiceList[wIndex].AudioEsList.size();
	return 0;
}


const bool CProgManager::GetSubtitleEsPID(WORD *pwSubtitlePID, const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pwSubtitlePID && (size_t)wIndex < m_ServiceList.size()
			&& m_ServiceList[wIndex].wSubtitleEsPID != 0xFFFF) {
		*pwSubtitlePID = m_ServiceList[wIndex].wSubtitleEsPID;
		return true;
	}
	return false;
}


const bool CProgManager::GetPcrTimeStamp(unsigned __int64 *pu64PcrTimeStamp, const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	// PCRを取得する
	if ((size_t)wIndex < m_ServiceList.size() && pu64PcrTimeStamp) {
		*pu64PcrTimeStamp = m_ServiceList[wIndex].u64TimeStamp;
		return true;
	}
	return false;
}


const DWORD CProgManager::GetServiceName(LPTSTR lpszDst, const WORD wIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス名を取得する
	if ((size_t)wIndex < m_ServiceList.size()) {
		if (lpszDst)
			::lstrcpy(lpszDst, m_ServiceList[wIndex].szServiceName);
		return ::lstrlen(m_ServiceList[wIndex].szServiceName);
	}
	return 0U;
}


const WORD CProgManager::GetTransportStreamID() const
{
	return m_pProgDatabase->m_wTransportStreamID;
}


WORD CProgManager::GetNetworkID(void) const
{
	return m_pProgDatabase->m_NitInfo.wNetworkID;
}


BYTE CProgManager::GetBroadcastingID(void) const
{
	return m_pProgDatabase->m_NitInfo.byBroadcastingID;
}


DWORD CProgManager::GetNetworkName(LPTSTR pszName,int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName)
		::lstrcpyn(pszName,m_pProgDatabase->m_NitInfo.szNetworkName,MaxLength);
	return ::lstrlen(m_pProgDatabase->m_NitInfo.szNetworkName);
}


BYTE CProgManager::GetRemoteControlKeyID(void) const
{
	return m_pProgDatabase->m_NitInfo.byRemoteControlKeyID;
}


DWORD CProgManager::GetTSName(LPTSTR pszName,int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName)
		::lstrcpyn(pszName,m_pProgDatabase->m_NitInfo.szTSName,MaxLength);
	return ::lstrlen(m_pProgDatabase->m_NitInfo.szTSName);
}


const WORD CProgManager::GetEventID(const WORD ServiceIndex, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable=dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index=pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].wServiceID);

			if (Index>=0)
				return pEitTable->GetEventID(Index,fNext?1:0);
		}
	}
	return 0;
}


const bool CProgManager::GetStartTime(const WORD ServiceIndex, SYSTEMTIME *pSystemTime, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)ServiceIndex < m_ServiceList.size() && pSystemTime) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].wServiceID);

			if (Index >= 0) {
				const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Index, fNext ? 1 : 0);
				if (pStartTime)
					*pSystemTime=*pStartTime;
				return true;
			}
		}
	}
	return false;
}


const DWORD CProgManager::GetDuration(const WORD ServiceIndex, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].wServiceID);

			if (Index >= 0)
				return pEitTable->GetDuration(Index, fNext ? 1 : 0);
		}
	}
	return 0;
}


const int CProgManager::GetEventName(const WORD ServiceIndex, LPTSTR pszName, int MaxLength, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, fNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}
	return 0;
}


const int CProgManager::GetEventText(const WORD ServiceIndex, LPTSTR pszText, int MaxLength, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, fNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}
	return 0;
}


const CDescBlock *CProgManager::GetHEitItemDesc(const WORD ServiceIndex, const bool fNext) const
{
	if ((size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].wServiceID);

			if (Index>=0)
				return pEitTable->GetItemDesc(Index, fNext ? 1 : 0);
		}
	}
	return NULL;
}


void CProgManager::OnServiceListUpdated(void)
{
	// サービスリストクリア、リサイズ
	m_ServiceList.clear();

	// サービスリスト構築
	for (size_t Index = 0, ServiceNum = 0 ; Index < m_pProgDatabase->m_ServiceList.size() ; Index++) {
		if (m_pProgDatabase->m_ServiceList[Index].wVideoEsPID != 0xFFFFU) {
			// MPEG2映像のみ(ワンセグ、データ放送以外)
			m_ServiceList.resize(ServiceNum + 1);
			m_ServiceList[ServiceNum].wServiceID = m_pProgDatabase->m_ServiceList[Index].wServiceID;
			m_ServiceList[ServiceNum].wVideoEsPID = m_pProgDatabase->m_ServiceList[Index].wVideoEsPID;
			m_ServiceList[ServiceNum].AudioEsList = m_pProgDatabase->m_ServiceList[Index].AudioEsList;
			m_ServiceList[ServiceNum].wSubtitleEsPID = m_pProgDatabase->m_ServiceList[Index].wSubtitleEsPID;
			m_ServiceList[ServiceNum].szServiceName[0] = TEXT('\0');
			ServiceNum++;
		}
	}

	TRACE(TEXT("CProgManager::OnServiceListUpdated()\n"));

	SendDecoderEvent(EID_SERVICE_LIST_UPDATED);
}


void CProgManager::OnServiceInfoUpdated(void)
{
	// サービス名を更新する
	for (size_t Index = 0 ; Index < m_ServiceList.size() ; Index++) {
		const WORD wServiceIndex = m_pProgDatabase->GetServiceIndexByID(m_ServiceList[Index].wServiceID);

		if (wServiceIndex != 0xFFFFU) {
			if (m_pProgDatabase->m_ServiceList[wServiceIndex].szServiceName[0]) {
				::lstrcpy(m_ServiceList[Index].szServiceName,
						  m_pProgDatabase->m_ServiceList[wServiceIndex].szServiceName);
			} else {
				::wsprintf(m_ServiceList[Index].szServiceName, TEXT("サービス%d"), Index + 1);
			}
		}
	}

	TRACE(TEXT("CProgManager::OnServiceInfoUpdated()\n"));

	SendDecoderEvent(EID_SERVICE_INFO_UPDATED);
}


void CProgManager::OnPcrTimestampUpdated(void)
{
	// PCRを更新する
	for (size_t Index = 0 ; Index < m_ServiceList.size() ; Index++) {
		const WORD wServiceIndex = m_pProgDatabase->GetServiceIndexByID(m_ServiceList[Index].wServiceID);

		if (wServiceIndex != 0xFFFFU) {
			m_ServiceList[Index].u64TimeStamp = m_pProgDatabase->m_ServiceList[wServiceIndex].u64TimeStamp;
		}
	}

//	TRACE(TEXT("CProgManager::OnPcrTimeStampUpdated()\n"));

	SendDecoderEvent(EID_PCR_TIMESTAMP_UPDATED);
}


//////////////////////////////////////////////////////////////////////
// CProgDatabase 構築/消滅
//////////////////////////////////////////////////////////////////////

CProgManager::CProgDatabase::CProgDatabase(CProgManager &ProgManager)
	: m_ProgManager(ProgManager)
	, m_PidMapManager(ProgManager.m_PidMapManager)
	, m_wTransportStreamID(0x0000U)
{
	Reset();
}


CProgManager::CProgDatabase::~CProgDatabase()
{
	UnmapTable();
}


void CProgManager::CProgDatabase::Reset(void)
{
	// 全テーブルアンマップ
	UnmapTable();

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0000U, new CPatTable, CProgDatabase::OnPatUpdated, this);

	// NITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0010U, new CNitTable, CProgDatabase::OnNitUpdated, this);
	::ZeroMemory(&m_NitInfo, sizeof(m_NitInfo));

	// EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0012U, new CHEitTable, NULL, this);
}


void CProgManager::CProgDatabase::UnmapTable(void)
{
	// 全PMT PIDアンマップ
	for (size_t Index = 0 ; Index < m_ServiceList.size() ; Index++) {
		m_PidMapManager.UnmapTarget(m_ServiceList[Index].wPmtTablePID);
	}

	// サービスリストクリア
	m_ServiceList.clear();

	// トランスポートストリームID初期化
	m_wTransportStreamID = 0x0000U;

	// PATテーブルリセット
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(m_PidMapManager.GetMapTarget(0x0000U));
	if (pPatTable)
		pPatTable->Reset();
}


const WORD CProgManager::CProgDatabase::GetServiceIndexByID(const WORD wServiceID)
{
	// プログラムIDからサービスインデックスを検索する
	for (size_t Index = 0 ; Index < m_ServiceList.size() ; Index++) {
		if (m_ServiceList[Index].wServiceID == wServiceID)
			return Index;
	}

	// プログラムIDが見つからない
	return 0xFFFFU;
}


void CALLBACK CProgManager::CProgDatabase::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CProgDatabase *pThis = static_cast<CProgDatabase *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);

	// トランスポートストリームID更新
	pThis->m_wTransportStreamID = pPatTable->m_CurSection.GetTableIdExtension();

	// 現PMT/PCRのPIDをアンマップする
	for (size_t Index = 0 ; Index < pThis->m_ServiceList.size() ; Index++) {
		WORD wPID;
		wPID = pThis->m_ServiceList[Index].wPmtTablePID;
		pMapManager->UnmapTarget(wPID);
		wPID = pThis->m_ServiceList[Index].wPcrPID;
		pMapManager->UnmapTarget(wPID);
	}

	// 新PMTをストアする
	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t Index = 0 ; Index < pThis->m_ServiceList.size() ; Index++) {
		// サービスリスト更新
		pThis->m_ServiceList[Index].bIsUpdated = false;
		pThis->m_ServiceList[Index].wServiceID = pPatTable->GetProgramID(Index);
		pThis->m_ServiceList[Index].wPmtTablePID = pPatTable->GetPmtPID(Index);
		pThis->m_ServiceList[Index].wVideoEsPID = 0xFFFFU;
		pThis->m_ServiceList[Index].AudioEsList.clear();
		pThis->m_ServiceList[Index].wSubtitleEsPID = 0xFFFFU;
		pThis->m_ServiceList[Index].wPcrPID = 0xFFFFU;
		pThis->m_ServiceList[Index].byVideoComponentTag = 0xFFU;
		//pThis->m_ServiceList[Index].byAudioComponentTag = 0xFFU;
		pThis->m_ServiceList[Index].byServiceType = 0xFFU;
		pThis->m_ServiceList[Index].byRunningStatus = 0xFFU;
		pThis->m_ServiceList[Index].bIsCaService = false;
		pThis->m_ServiceList[Index].szServiceName[0] = TEXT('\0');

		// PMTのPIDをマップ
		pMapManager->MapTarget(pPatTable->GetPmtPID(Index), new CPmtTable, CProgDatabase::OnPmtUpdated, pParam);
	}
}


void CALLBACK CProgManager::CProgDatabase::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CProgDatabase *pThis = static_cast<CProgDatabase *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);

	// サービスインデックスを検索
	const WORD wServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if(wServiceIndex == 0xFFFFU)return;

	// ビデオESのPIDをストア
	pThis->m_ServiceList[wServiceIndex].wVideoEsPID = 0xFFFFU;
	for (WORD wEsIndex = 0U ; wEsIndex < pPmtTable->GetEsInfoNum() ; wEsIndex++) {
		// 「ITU-T Rec. H.262|ISO/IEC 13818-2 Video or ISO/IEC 11172-2」のストリームタイプを検索
		if (pPmtTable->GetStreamTypeID(wEsIndex) == 0x02U) {
			pThis->m_ServiceList[wServiceIndex].wVideoEsPID = pPmtTable->GetEsPID(wEsIndex);
			break;
		}
	}

	// オーディオESのPIDをストア
	pThis->m_ServiceList[wServiceIndex].AudioEsList.clear();
	for (WORD wEsIndex = 0U ; wEsIndex < pPmtTable->GetEsInfoNum() ; wEsIndex++) {
		// 「ISO/IEC 13818-7 Audio (ADTS Transport Syntax)」のストリームタイプを検索
		if (pPmtTable->GetStreamTypeID(wEsIndex) == 0x0FU) {
			BYTE ComponentTag=0;
			const CDescBlock *pDescBlock=pPmtTable->GetItemDesc(wEsIndex);

			if (pDescBlock) {
				const CStreamIdDesc *pStreamIdDesc=dynamic_cast<const CStreamIdDesc*>(pDescBlock->GetDescByTag(CStreamIdDesc::DESC_TAG));

				if (pStreamIdDesc)
					ComponentTag=pStreamIdDesc->GetComponentTag();
			}
			pThis->m_ServiceList[wServiceIndex].AudioEsList.push_back(
						EsInfo(pPmtTable->GetEsPID(wEsIndex),ComponentTag));
		}
	}

	// 字幕ESのPIDをストア
	pThis->m_ServiceList[wServiceIndex].wSubtitleEsPID = 0xFFFFU;
	for (WORD wEsIndex = 0U ; wEsIndex < pPmtTable->GetEsInfoNum() ; wEsIndex++) {
		// 「ITU-T Rec. H.222|ISO/IEC 13818-1 PES packets containing private data」のストリームタイプを検索
		if (pPmtTable->GetStreamTypeID(wEsIndex) == 0x06U) {
			pThis->m_ServiceList[wServiceIndex].wSubtitleEsPID = pPmtTable->GetEsPID(wEsIndex);
			break;
		}
	}

	WORD wPcrPID = pPmtTable->GetPcrPID();
	if (wPcrPID < 0x1FFFU) {
		pThis->m_ServiceList[wServiceIndex].wPcrPID = wPcrPID;
		CTsPidMapTarget *pMap = pMapManager->GetMapTarget(wPcrPID);
		if (!pMap) {
			// 新規Map
			pMapManager->MapTarget(wPcrPID, new CPcrTable(wServiceIndex), CProgDatabase::OnPcrUpdated, pParam);
		} else {
			// 既存Map
			CPcrTable *pPcrTable = dynamic_cast<CPcrTable*>(pMap);
			if(pPcrTable) {
				// サービス追加
				pPcrTable->AddServiceIndex(wServiceIndex);
			}
		}
	}

	// 更新済みマーク
	pThis->m_ServiceList[wServiceIndex].bIsUpdated = true;

/*
	放送局によってはPATに含まれるサービス全てのPMTを流していない場合がある。
　　(何度もハンドラが呼び出されるのを防止するため全ての情報がそろった段階で呼び出したかった)

	// 他のPMTの更新状況を調べる
	for (WORD wIndex = 0U ; wIndex < pThis->m_ServiceList.size() ; wIndex++) {
		if(!pThis->m_ServiceList[wIndex].bIsUpdated)
			return;
	}
*/

	// SDTテーブルを再マップする
	pMapManager->MapTarget(0x0011U, new CSdtTable, CProgDatabase::OnSdtUpdated, pParam);

	// イベントハンドラ呼び出し
	pThis->m_ProgManager.OnServiceListUpdated();
}


void CALLBACK CProgManager::CProgDatabase::OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// SDTが更新された
	CProgDatabase *pThis = static_cast<CProgDatabase *>(pParam);
	CSdtTable *pSdtTable = dynamic_cast<CSdtTable *>(pMapTarget);

	for (WORD wSdtIndex = 0U ; wSdtIndex < pSdtTable->GetServiceNum() ; wSdtIndex++) {
		// サービスIDを検索
		const WORD wServiceIndex = pThis->GetServiceIndexByID(pSdtTable->GetServiceID(wSdtIndex));
		if (wServiceIndex == 0xFFFFU)
			continue;

		// サービス情報更新
		pThis->m_ServiceList[wServiceIndex].byRunningStatus = pSdtTable->GetRunningStatus(wSdtIndex);
		pThis->m_ServiceList[wServiceIndex].bIsCaService = pSdtTable->GetFreeCaMode(wSdtIndex);

		// サービス名更新
		pThis->m_ServiceList[wServiceIndex].szServiceName[0] = TEXT('\0');

		const CDescBlock *pDescBlock = pSdtTable->GetItemDesc(wSdtIndex);
		const CServiceDesc *pServiceDesc = dynamic_cast<const CServiceDesc *>(pDescBlock->GetDescByTag(CServiceDesc::DESC_TAG));

		if (pServiceDesc) {
			pServiceDesc->GetServiceName(pThis->m_ServiceList[wServiceIndex].szServiceName, 256);
			pThis->m_ServiceList[wServiceIndex].byServiceType = pServiceDesc->GetServiceType();
		}
	}

	// イベントハンドラ呼び出し
	pThis->m_ProgManager.OnServiceInfoUpdated();
}


void CALLBACK CProgManager::CProgDatabase::OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PCRが更新された
	CProgDatabase *pThis = static_cast<CProgDatabase *>(pParam);
	CPcrTable *pPcrTable = dynamic_cast<CPcrTable *>(pMapTarget);

	const unsigned __int64 u64TimeStamp = pPcrTable->GetPcrTimeStamp();

	WORD wServiceIndex;
	for (WORD wIndex = 0 ; pPcrTable->GetServiceIndex(&wServiceIndex,wIndex); wIndex++) {
		if (wServiceIndex < pThis->m_ServiceList.size()) {
			pThis->m_ServiceList[wServiceIndex].u64TimeStamp = u64TimeStamp;
		}
	}

	// イベントハンドラ呼び出し
	pThis->m_ProgManager.OnPcrTimestampUpdated();
}


void CALLBACK CProgManager::CProgDatabase::OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam){

	CProgDatabase *pThis = static_cast<CProgDatabase*>(pParam);
	CNitTable *pNitTable = dynamic_cast<CNitTable*>(pMapTarget);

	pThis->m_NitInfo.wNetworkID = pNitTable->GetNetworkID();

	const CDescBlock *pDescBlock;
	pDescBlock = pNitTable->GetNetworkDesc();
	if (pDescBlock) {
		const CNetworkNameDesc *pNetworkDesc = dynamic_cast<const CNetworkNameDesc *>(pDescBlock->GetDescByTag(CNetworkNameDesc::DESC_TAG));
		if(pNetworkDesc) {
			pNetworkDesc->GetNetworkName(pThis->m_NitInfo.szNetworkName,
										 sizeof(pThis->m_NitInfo.szNetworkName) / sizeof(TCHAR));
		}
		const CSystemManageDesc *pSysManageDesc = dynamic_cast<const CSystemManageDesc *>(pDescBlock->GetDescByTag(CSystemManageDesc::DESC_TAG));
		if (pSysManageDesc) {
			pThis->m_NitInfo.byBroadcastingID = pSysManageDesc->GetBroadcastingID();
		}
	}

	pDescBlock = pNitTable->GetItemDesc(0);
	if (pDescBlock) {
		const CTSInfoDesc *pTsInfoDesc = dynamic_cast<const CTSInfoDesc *>(pDescBlock->GetDescByTag(CTSInfoDesc::DESC_TAG));
		if (pTsInfoDesc) {
			pTsInfoDesc->GetTSName(pThis->m_NitInfo.szTSName,
								   sizeof(pThis->m_NitInfo.szTSName) / sizeof(TCHAR));
			pThis->m_NitInfo.byRemoteControlKeyID = pTsInfoDesc->GetRemoteControlKeyID();
		}
	}
}


/*
void CALLBACK CProgManager::CProgDatabase::OnEitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	CProgDatabase *pThis = static_cast<CProgDatabase*>(pParam);
	CHEitTable *pEitTable = dynamic_cast<CHEitTable*>(pMapTarget);

	for (DWORD Index = 0 ; Index < pEitTable->GetServiceNum() ; Index++) {
		const WORD ServiceIndex = pThis->GetServiceIndexByID(pEitTable->GetServiceID(Index));

		if (ServiceIndex == 0xFFFFU)
			continue;

		const CDescBlock *pDescBlock=pEitTable->GetItemDesc(Index,0);
		if (pDescBlock) {
			const CAudioComponentDesc *pAudioDesc=dynamic_cast<const CAudioComponentDesc*>(pDescBlock->GetDescByTag(CAudioComponentDesc::DESC_TAG));

			if (pAudioDesc) {
				
			}
		}
	}
}
*/
