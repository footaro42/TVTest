#include "stdafx.h"
#include "TsTable.h"
#include "TsAnalyzer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CTsAnalyzer::CTsAnalyzer(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)
{
	Reset();
}


CTsAnalyzer::~CTsAnalyzer()
{
}


const bool CTsAnalyzer::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	/*
	if (dwInputIndex >= GetInputNum())
		return false;

	CTsPacket *pTsPacket = dynamic_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if (!pTsPacket)
		return false;
	*/

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// PIDルーティング
	m_PidMapManager.StorePacket(pTsPacket);

	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}


void CTsAnalyzer::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	// イベントハンドラにリセットを通知する
	for (int i = 0 ; i <= EVENT_LAST ; i++)
		NotifyResetEvent((EventType)i);

	// 全テーブルアンマップ
	m_PidMapManager.UnmapAllTarget();

	// サービスリストクリア
	m_ServiceList.clear();

	// トランスポートストリームID初期化
	m_TransportStreamID = 0x0000;

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0000, new CPatTable, OnPatUpdated, this);

	// NITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0010, new CNitTable, OnNitUpdated, this);
	::ZeroMemory(&m_NitInfo, sizeof(m_NitInfo));

#ifdef TS_ANALYZER_EIT_SUPPORT
	// EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0012, new CHEitTable, NULL, this);
#endif
}


WORD CTsAnalyzer::GetServiceNum()
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス数を返す
	return (WORD)m_ServiceList.size();
}


bool CTsAnalyzer::GetServiceID(const WORD Index, WORD *pServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pServiceID == NULL)
		return false;

	// サービスIDを取得する
	if (Index == 0xFFFF) {
		if (m_ServiceList.size() == 0 || !m_ServiceList[0].bIsUpdated)
			return false;
		*pServiceID = m_ServiceList[0].ServiceID;
	} else if ((size_t)Index < m_ServiceList.size()) {
		*pServiceID = m_ServiceList[Index].ServiceID;
	} else {
		return false;
	}
	return true;
}


int CTsAnalyzer::GetServiceIndexByID(const WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	// プログラムIDからサービスインデックスを検索する
	for (size_t Index = 0 ; Index < m_ServiceList.size() ; Index++) {
		if (m_ServiceList[Index].ServiceID == ServiceID)
			return Index;
	}

	// プログラムIDが見つからない
	return -1;
}


bool CTsAnalyzer::IsServiceUpdated(const WORD Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size()) {
		return m_ServiceList[Index].bIsUpdated;
	}
	return false;
}


bool CTsAnalyzer::GetPmtPID(const WORD Index, WORD *pPmtPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pPmtPID != NULL && (size_t)Index < m_ServiceList.size()) {
		*pPmtPID = m_ServiceList[Index].PmtPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoEsPID(const WORD Index, WORD *pVideoPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pVideoPID != NULL && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].VideoEsPID != PID_INVALID) {
		*pVideoPID = m_ServiceList[Index].VideoEsPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoStreamType(const WORD Index, BYTE *pStreamType)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pStreamType != NULL && (size_t)Index < m_ServiceList.size()) {
		*pStreamType = m_ServiceList[Index].VideoStreamType;
		return true;
	}
	return false;
}


WORD CTsAnalyzer::GetAudioEsNum(const WORD Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].AudioEsList.size();
	return 0;
}


bool CTsAnalyzer::GetAudioEsPID(const WORD Index, const WORD AudioIndex, WORD *pAudioPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pAudioPID != NULL && (size_t)Index < m_ServiceList.size()
			&& (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		*pAudioPID = m_ServiceList[Index].AudioEsList[AudioIndex].PID;
		return true;
	}
	return false;
}


BYTE CTsAnalyzer::GetAudioComponentTag(const WORD Index, const WORD AudioIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size()
			&& (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		return m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag;
	}
	return 0;
}


#ifdef TS_ANALYZER_EIT_SUPPORT
BYTE CTsAnalyzer::GetAudioComponentType(const WORD Index, const WORD AudioIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size()
			&& (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(wIndex);

		if (pDescBlock) {
			for (WORD i = 0 ; i < pDescBlock->GetDescNum() ; i++) {
				const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

				if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
					const CAudioComponentDesc *pAudioDesc = dynamic_cast<const CAudioComponentDesc*>(pDesc);

					if (pAudioDesc->GetComponentTag() == m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag)
						return pAudioDesc->GetComponentType();
				}
			}
		}
	}
	return 0;
}
#endif


WORD CTsAnalyzer::GetSubtitleEsNum(const WORD Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].SubtitleEsList.size();
	return 0;
}


bool CTsAnalyzer::GetSubtitleEsPID(const WORD Index, const WORD SubtitleIndex, WORD *pSubtitlePID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pSubtitlePID != NULL && (size_t)Index < m_ServiceList.size()
			&& (size_t)SubtitleIndex < m_ServiceList[Index].SubtitleEsList.size()) {
		*pSubtitlePID = m_ServiceList[Index].SubtitleEsList[SubtitleIndex].PID;
		return true;
	}
	return false;
}


WORD CTsAnalyzer::GetDataCarrouselEsNum(const WORD Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].DataCarrouselEsList.size();
	return 0;
}


bool CTsAnalyzer::GetDataCarrouselEsPID(const WORD Index, const WORD DataCarrouselIndex, WORD *pDataCarrouselPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pDataCarrouselPID != NULL && (size_t)Index < m_ServiceList.size()
			&& (size_t)DataCarrouselIndex < m_ServiceList[Index].DataCarrouselEsList.size()) {
		*pDataCarrouselPID = m_ServiceList[Index].DataCarrouselEsList[DataCarrouselIndex].PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrPID(const WORD Index, WORD *pPcrPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pPcrPID != NULL && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].PcrPID != PID_INVALID) {
		*pPcrPID = m_ServiceList[Index].PcrPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrTimeStamp(const WORD Index, ULONGLONG *pTimeStamp)
{
	CBlockLock Lock(&m_DecoderLock);

	// PCRを取得する
	if ((size_t)Index < m_ServiceList.size() && pTimeStamp != NULL) {
		*pTimeStamp = m_ServiceList[Index].PcrTimeStamp;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetEcmPID(const WORD Index, WORD *pEcmPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pEcmPID != NULL && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].EcmPID != PID_INVALID) {
		*pEcmPID = m_ServiceList[Index].EcmPID;
		return true;
	}
	return false;
}


int CTsAnalyzer::GetServiceName(const WORD Index, LPTSTR pszName, const int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス名を取得する
	if ((size_t)Index < m_ServiceList.size()) {
		if (pszName != NULL && MaxLength > 0)
			::lstrcpyn(pszName, m_ServiceList[Index].szServiceName, MaxLength);
		return ::lstrlen(m_ServiceList[Index].szServiceName);
	}
	return 0;
}


WORD CTsAnalyzer::GetTransportStreamID() const
{
	return m_TransportStreamID;
}


WORD CTsAnalyzer::GetNetworkID() const
{
	return m_NitInfo.NetworkID;
}


BYTE CTsAnalyzer::GetBroadcastingID() const
{
	return m_NitInfo.BroadcastingID;
}


int CTsAnalyzer::GetNetworkName(LPTSTR pszName, int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName != NULL && MaxLength > 0)
		::lstrcpyn(pszName, m_NitInfo.szNetworkName, MaxLength);
	return ::lstrlen(m_NitInfo.szNetworkName);
}


BYTE CTsAnalyzer::GetRemoteControlKeyID() const
{
	return m_NitInfo.RemoteControlKeyID;
}


int CTsAnalyzer::GetTsName(LPTSTR pszName,int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pszName != NULL && MaxLength > 0)
		::lstrcpyn(pszName, m_NitInfo.szTSName, MaxLength);
	return ::lstrlen(m_NitInfo.szTSName);
}


#ifdef TS_ANALYZER_EIT_SUPPORT


WORD CTsAnalyzer::GetEventID(const WORD ServiceIndex, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable=dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index=pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index>=0)
				return pEitTable->GetEventID(Index,fNext?1:0);
		}
	}
	return 0;
}


bool CTsAnalyzer::GetEventStartTime(const WORD ServiceIndex, SYSTEMTIME *pSystemTime, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pSystemTime == NULL)
		return false;
	if ((size_t)ServiceIndex < m_ServiceList.size() && pSystemTime) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0) {
				const SYSTEMTIME *pStartTime = pEitTable->GetStartTime(Index, bNext ? 1 : 0);
				if (pStartTime) {
					*pSystemTime = *pStartTime;
					return true;
				}
			}
		}
	}
	return false;
}


DWORD CTsAnalyzer::GetEventDuration(const WORD ServiceIndex, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if ((size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetDuration(Index, bNext ? 1 : 0);
		}
	}
	return 0;
}


int CTsAnalyzer::GetEventName(const WORD ServiceIndex, LPTSTR pszName, int MaxLength, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}
	return 0;
}


int CTsAnalyzer::GetEventText(const WORD ServiceIndex, LPTSTR pszText, int MaxLength, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}
	return 0;
}


CDescBlock *CTsAnalyzer::GetHEitItemDesc(const WORD ServiceIndex, const bool bNext) const
{
	if ((size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetItemDesc(Index, bNext ? 1 : 0);
		}
	}
	return NULL;
}


#endif	// TS_ANALYZER_EIT_SUPPORT


bool CTsAnalyzer::AddEventHandler(EventType Type, CEventHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Type < 0 || Type > EVENT_LAST || pHandler == NULL)
		return false;

	EventHandlerInfo Info;
	Info.Type = Type;
	Info.pHandler = pHandler;
	m_EventHandlerList[Type].push_back(Info);

	return true;
}


bool CTsAnalyzer::RemoveEventHandler(CEventHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	for (int i = 0 ; i <= EVENT_LAST ; i++) {
		std::vector<EventHandlerInfo>::iterator itr;

		for (itr = m_EventHandlerList[i].begin() ; itr != m_EventHandlerList[i].end() ; itr++) {
			if (itr->pHandler == pHandler) {
				m_EventHandlerList[i].erase(itr);
				return true;
			}
		}
	}
	return false;
}


void CTsAnalyzer::CallEventHandler(EventType Type)
{
	for (size_t i = 0 ; i < m_EventHandlerList[Type].size() ; i++) {
		m_EventHandlerList[Type][i].pHandler->OnEvent(this);
	}
}


void CTsAnalyzer::NotifyResetEvent(EventType Type)
{
	for (size_t i = 0 ; i < m_EventHandlerList[Type].size() ; i++) {
		m_EventHandlerList[Type][i].pHandler->OnReset(this);
	}
}


void CALLBACK CTsAnalyzer::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	TRACE(TEXT("CTsAnalyzer::OnPatUpdated()\n"));

	// PATが更新された
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);
	if (pPatTable == NULL)
		return;

	// トランスポートストリームID更新
	pThis->m_TransportStreamID = pPatTable->m_CurSection.GetTableIdExtension();

	// 現PMT/PCRのPIDをアンマップする
	for (size_t Index = 0 ; Index < pThis->m_ServiceList.size() ; Index++) {
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PmtPID);
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PcrPID);
	}
	pThis->NotifyResetEvent(EVENT_PMT_UPDATED);
	pThis->NotifyResetEvent(EVENT_PCR_UPDATED);

	// 新PMTをストアする
	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t Index = 0 ; Index < pThis->m_ServiceList.size() ; Index++) {
		// サービスリスト更新
		pThis->m_ServiceList[Index].bIsUpdated = false;
		pThis->m_ServiceList[Index].ServiceID = pPatTable->GetProgramID(Index);
		pThis->m_ServiceList[Index].PmtPID = pPatTable->GetPmtPID(Index);
		pThis->m_ServiceList[Index].VideoStreamType = 0xFF;
		pThis->m_ServiceList[Index].VideoEsPID = PID_INVALID;
		pThis->m_ServiceList[Index].AudioEsList.clear();
		pThis->m_ServiceList[Index].SubtitleEsList.clear();
		pThis->m_ServiceList[Index].DataCarrouselEsList.clear();
		pThis->m_ServiceList[Index].PcrPID = PID_INVALID;
		pThis->m_ServiceList[Index].EcmPID = PID_INVALID;
		pThis->m_ServiceList[Index].RunningStatus = 0xFF;
		pThis->m_ServiceList[Index].bIsCaService = false;
		pThis->m_ServiceList[Index].szServiceName[0] = '\0';
		pThis->m_ServiceList[Index].ServiceType = 0xFF;

		// PMTのPIDをマップ
		pMapManager->MapTarget(pPatTable->GetPmtPID(Index), new CPmtTable, OnPmtUpdated, pParam);
	}

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_PAT_UPDATED);
}


void CALLBACK CTsAnalyzer::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	TRACE(TEXT("CTsAnalyzer::OnPmtUpdated()\n"));

	// PMTが更新された
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);
	if (pPmtTable == NULL)
		return;

	// サービスインデックスを検索
	const int ServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = pThis->m_ServiceList[ServiceIndex];

	// ESのPIDをストア
	Info.VideoStreamType = 0xFF;
	Info.VideoEsPID = PID_INVALID;
	Info.AudioEsList.clear();
	Info.SubtitleEsList.clear();
	Info.DataCarrouselEsList.clear();
	for (WORD EsIndex = 0 ; EsIndex < pPmtTable->GetEsInfoNum() ; EsIndex++) {
		const BYTE StreamType = pPmtTable->GetStreamTypeID(EsIndex);
		const WORD EsPID = pPmtTable->GetEsPID(EsIndex);

		switch (StreamType) {
		case 0x02:	// ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2
			if (Info.VideoEsPID == PID_INVALID
					|| Info.VideoStreamType != 0x02) {
				Info.VideoStreamType = StreamType;
				Info.VideoEsPID = EsPID;
			}
			break;

		case 0x06:	// ITU-T Rec.H.222 | ISO/IEC 13818-1
			Info.SubtitleEsList.push_back(EsInfo(EsPID));
			break;

		case 0x0D:
			Info.DataCarrouselEsList.push_back(EsInfo(EsPID));
			break;

		case 0x0F:	// ISO/IEC 13818-7 Audio (ADTS Transport Syntax)
			{
				BYTE ComponentTag = 0;
				const CDescBlock *pDescBlock = pPmtTable->GetItemDesc(EsIndex);

				if (pDescBlock) {
					const CStreamIdDesc *pStreamIdDesc = dynamic_cast<const CStreamIdDesc*>(pDescBlock->GetDescByTag(CStreamIdDesc::DESC_TAG));

					if (pStreamIdDesc)
						ComponentTag = pStreamIdDesc->GetComponentTag();
				}
				Info.AudioEsList.push_back(EsInfo(EsPID, ComponentTag));
			}
			break;

		case 0x1B:	// ITU-T Rec.H.264 | ISO/IEC 14496-10Video
			if (Info.VideoEsPID == PID_INVALID) {
				Info.VideoStreamType = StreamType;
				Info.VideoEsPID = EsPID;
			}
			break;
		}
	}

	WORD PcrPID = pPmtTable->GetPcrPID();
	if (PcrPID < 0x1FFFU) {
		Info.PcrPID = PcrPID;
		CTsPidMapTarget *pMap = pMapManager->GetMapTarget(PcrPID);
		if (!pMap) {
			// 新規Map
			pMapManager->MapTarget(PcrPID, new CPcrTable(ServiceIndex), OnPcrUpdated, pParam);
		} else {
			// 既存Map
			CPcrTable *pPcrTable = dynamic_cast<CPcrTable*>(pMap);
			if(pPcrTable) {
				// サービス追加
				pPcrTable->AddServiceIndex(ServiceIndex);
			}
		}
	}

	Info.EcmPID = pPmtTable->GetEcmPID();

	// 更新済みマーク
	Info.bIsUpdated = true;

	// SDTテーブルを再マップする
	pMapManager->MapTarget(0x0011, new CSdtTable, OnSdtUpdated, pParam);

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_PMT_UPDATED);
}


void CALLBACK CTsAnalyzer::OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	TRACE(TEXT("CTsAnalyzer::OnSdtUpdated()\n"));

	// SDTが更新された
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CSdtTable *pSdtTable = dynamic_cast<CSdtTable *>(pMapTarget);
	if (pSdtTable == NULL)
		return;

	for (WORD SdtIndex = 0 ; SdtIndex < pSdtTable->GetServiceNum() ; SdtIndex++) {
		// サービスIDを検索
		const int ServiceIndex = pThis->GetServiceIndexByID(pSdtTable->GetServiceID(SdtIndex));
		if (ServiceIndex < 0)
			continue;

		// サービス情報更新
		pThis->m_ServiceList[ServiceIndex].RunningStatus = pSdtTable->GetRunningStatus(SdtIndex);
		pThis->m_ServiceList[ServiceIndex].bIsCaService = pSdtTable->GetFreeCaMode(SdtIndex);

		// サービス名更新
		pThis->m_ServiceList[ServiceIndex].szServiceName[0] = '\0';

		const CDescBlock *pDescBlock = pSdtTable->GetItemDesc(SdtIndex);
		const CServiceDesc *pServiceDesc = dynamic_cast<const CServiceDesc *>(pDescBlock->GetDescByTag(CServiceDesc::DESC_TAG));

		if (pServiceDesc) {
			pServiceDesc->GetServiceName(pThis->m_ServiceList[ServiceIndex].szServiceName, 256);
			pThis->m_ServiceList[ServiceIndex].ServiceType = pServiceDesc->GetServiceType();
		}
	}
}


void CALLBACK CTsAnalyzer::OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PCRが更新された
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPcrTable *pPcrTable = dynamic_cast<CPcrTable *>(pMapTarget);
	if (pPcrTable == NULL)
		return;

	const ULONGLONG TimeStamp = pPcrTable->GetPcrTimeStamp();

	WORD ServiceIndex;
	for (WORD Index = 0 ; pPcrTable->GetServiceIndex(&ServiceIndex, Index); Index++) {
		if (ServiceIndex < pThis->m_ServiceList.size()) {
			pThis->m_ServiceList[ServiceIndex].PcrTimeStamp = TimeStamp;
		}
	}

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_PCR_UPDATED);
}


void CALLBACK CTsAnalyzer::OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam){

	TRACE(TEXT("CTsAnalyzer::OnNitUpdated()\n"));

	CTsAnalyzer *pThis = static_cast<CTsAnalyzer*>(pParam);
	CNitTable *pNitTable = dynamic_cast<CNitTable*>(pMapTarget);
	if (pNitTable == NULL)
		return;

	pThis->m_NitInfo.NetworkID = pNitTable->GetNetworkID();

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
			pThis->m_NitInfo.BroadcastingID = pSysManageDesc->GetBroadcastingID();
		}
	}

	pDescBlock = pNitTable->GetItemDesc(0);
	if (pDescBlock) {
		const CTSInfoDesc *pTsInfoDesc = dynamic_cast<const CTSInfoDesc *>(pDescBlock->GetDescByTag(CTSInfoDesc::DESC_TAG));
		if (pTsInfoDesc) {
			pTsInfoDesc->GetTSName(pThis->m_NitInfo.szTSName,
								   sizeof(pThis->m_NitInfo.szTSName) / sizeof(TCHAR));
			pThis->m_NitInfo.RemoteControlKeyID = pTsInfoDesc->GetRemoteControlKeyID();
		}
	}

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_NIT_UPDATED);
}




CTsAnalyzer::CEventHandler::CEventHandler()
{
}


CTsAnalyzer::CEventHandler::~CEventHandler()
{
}


void CTsAnalyzer::CEventHandler::OnReset(CTsAnalyzer *pAnalyzer)
{
}
