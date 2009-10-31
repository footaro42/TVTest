#include "stdafx.h"
#include "TsTable.h"
#include "TsAnalyzer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifdef _DEBUG
#define TABLE_DEBUG	true
#else
#define TABLE_DEBUG
#endif


#ifndef TVH264
#define VIEWABLE_STREAM_TYPE	0x02	// MPEG-2
#else
#define VIEWABLE_STREAM_TYPE	0x1B	// H.264
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
	NotifyResetEvent();

	// 全テーブルアンマップ
	m_PidMapManager.UnmapAllTarget();

	// サービスリストクリア
	m_ServiceList.clear();

	// トランスポートストリームID初期化
	m_TransportStreamID = 0x0000;

	// PATテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0000, new CPatTable(TABLE_DEBUG), OnPatUpdated, this);

	// NITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0010, new CNitTable, OnNitUpdated, this);
	::ZeroMemory(&m_NitInfo, sizeof(m_NitInfo));

#ifdef TS_ANALYZER_EIT_SUPPORT
	// H-EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0012, new CHEitTable);

#ifdef TVH264
	// L-EITテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0027, new CLEitTable);
#endif
#endif

	// TOTテーブルPIDマップ追加
	m_PidMapManager.MapTarget(0x0014, new CTotTable);
}


WORD CTsAnalyzer::GetServiceNum()
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス数を返す
	return (WORD)m_ServiceList.size();
}


bool CTsAnalyzer::GetServiceID(const int Index, WORD *pServiceID)
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	// サービスIDを取得する
	if (Index < 0) {
#ifndef TVH264
		if (m_ServiceList.size() == 0 || !m_ServiceList[0].bIsUpdated)
			return false;
		*pServiceID = m_ServiceList[0].ServiceID;
#else
		size_t i;
		for (i = 0; i < m_ServiceList.size(); i++) {
			if (m_ServiceList[i].PmtPID == 0x1FC8)
				break;
		}
		if (i == m_ServiceList.size() || !m_ServiceList[i].bIsUpdated)
			return false;
		*pServiceID = m_ServiceList[i].ServiceID;
#endif
	} else if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
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


WORD CTsAnalyzer::GetViewableServiceNum()
{
	CBlockLock Lock(&m_DecoderLock);
	WORD Count = 0;

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].VideoStreamType == VIEWABLE_STREAM_TYPE)
			Count++;
	}
	return Count;
}


bool CTsAnalyzer::GetViewableServiceID(const int Index, WORD *pServiceID)
{
	if (pServiceID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	int j = 0;
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].VideoStreamType == VIEWABLE_STREAM_TYPE) {
			if (j == Index) {
				*pServiceID = m_ServiceList[i].ServiceID;
				return true;
			}
			j++;
		}
	}
	return false;
}


bool CTsAnalyzer::GetFirstViewableServiceID(WORD *pServiceID)
{
	if (pServiceID == NULL)
		return false;

#ifndef TVH264
	CBlockLock Lock(&m_DecoderLock);

	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (!m_ServiceList[i].bIsUpdated)
			return false;
		if (m_ServiceList[i].VideoStreamType == VIEWABLE_STREAM_TYPE) {
			*pServiceID = m_ServiceList[i].ServiceID;
			return true;
		}
	}
	return false;
#else
	return GetServiceID(-1, pServiceID);
#endif
}


int CTsAnalyzer::GetViewableServiceIndexByID(const WORD ServiceID)
{
	CBlockLock Lock(&m_DecoderLock);

	int j = 0;
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].VideoStreamType == VIEWABLE_STREAM_TYPE) {
			if (m_ServiceList[i].ServiceID == ServiceID)
				return j;
			j++;
		}
	}
	return -1;
}


bool CTsAnalyzer::GetServiceInfo(const int Index, ServiceInfo *pInfo)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pInfo != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pInfo = m_ServiceList[Index];
		return true;
	}
	return false;
}


bool CTsAnalyzer::IsServiceUpdated(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		return m_ServiceList[Index].bIsUpdated;
	}
	return false;
}


bool CTsAnalyzer::GetPmtPID(const int Index, WORD *pPmtPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pPmtPID != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pPmtPID = m_ServiceList[Index].PmtPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoEsPID(const int Index, WORD *pVideoPID)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pVideoPID != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].VideoEs.PID != PID_INVALID) {
		*pVideoPID = m_ServiceList[Index].VideoEs.PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetVideoStreamType(const int Index, BYTE *pStreamType)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pStreamType != NULL && Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pStreamType = m_ServiceList[Index].VideoStreamType;
		return true;
	}
	return false;
}


BYTE CTsAnalyzer::GetVideoComponentTag(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return m_ServiceList[Index].VideoEs.ComponentTag;
	return COMPONENTTAG_INVALID;
}


WORD CTsAnalyzer::GetAudioEsNum(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].AudioEsList.size();
	return 0;
}


bool CTsAnalyzer::GetAudioEsPID(const int Index, const int AudioIndex, WORD *pAudioPID)
{
	if (pAudioPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		*pAudioPID = m_ServiceList[Index].AudioEsList[AudioIndex].PID;
		return true;
	}
	return false;
}


BYTE CTsAnalyzer::GetAudioComponentTag(const int Index, const int AudioIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		return m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag;
	}
	return COMPONENTTAG_INVALID;
}


#ifdef TS_ANALYZER_EIT_SUPPORT

BYTE CTsAnalyzer::GetVideoComponentType(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			const CComponentDesc *pComponentDesc = dynamic_cast<const CComponentDesc*>(pDescBlock->GetDescByTag(CComponentDesc::DESC_TAG));

			if (pComponentDesc != NULL)
				return pComponentDesc->GetComponentType();
		}
	}
	return 0;
}


BYTE CTsAnalyzer::GetAudioComponentType(const int Index, const int AudioIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& AudioIndex >= 0 && (size_t)AudioIndex < m_ServiceList[Index].AudioEsList.size()) {
		const CDescBlock *pDescBlock = GetHEitItemDesc(Index);

		if (pDescBlock) {
			for (WORD i = 0 ; i < pDescBlock->GetDescNum() ; i++) {
				const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

				if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
					const CAudioComponentDesc *pAudioDesc = dynamic_cast<const CAudioComponentDesc*>(pDesc);

					if (pAudioDesc != NULL
							&& pAudioDesc->GetComponentTag() == m_ServiceList[Index].AudioEsList[AudioIndex].ComponentTag)
						return pAudioDesc->GetComponentType();
				}
			}
		}
	}
	return 0;
}

#endif	// TS_ANALYZER_EIT_SUPPORT


WORD CTsAnalyzer::GetSubtitleEsNum(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].SubtitleEsList.size();
	return 0;
}


bool CTsAnalyzer::GetSubtitleEsPID(const int Index, const WORD SubtitleIndex, WORD *pSubtitlePID)
{
	if (pSubtitlePID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& (size_t)SubtitleIndex < m_ServiceList[Index].SubtitleEsList.size()) {
		*pSubtitlePID = m_ServiceList[Index].SubtitleEsList[SubtitleIndex].PID;
		return true;
	}
	return false;
}


WORD CTsAnalyzer::GetDataCarrouselEsNum(const int Index)
{
	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size())
		return (WORD)m_ServiceList[Index].DataCarrouselEsList.size();
	return 0;
}


bool CTsAnalyzer::GetDataCarrouselEsPID(const int Index, const WORD DataCarrouselIndex, WORD *pDataCarrouselPID)
{
	if (pDataCarrouselPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& (size_t)DataCarrouselIndex < m_ServiceList[Index].DataCarrouselEsList.size()) {
		*pDataCarrouselPID = m_ServiceList[Index].DataCarrouselEsList[DataCarrouselIndex].PID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrPID(const int Index, WORD *pPcrPID)
{
	if (pPcrPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].PcrPID != PID_INVALID) {
		*pPcrPID = m_ServiceList[Index].PcrPID;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetPcrTimeStamp(const int Index, ULONGLONG *pTimeStamp)
{
	if (pTimeStamp == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	// PCRを取得する
	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		*pTimeStamp = m_ServiceList[Index].PcrTimeStamp;
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetEcmPID(const int Index, WORD *pEcmPID)
{
	if (pEcmPID == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (Index >= 0 && (size_t)Index < m_ServiceList.size()
			&& m_ServiceList[Index].EcmPID != PID_INVALID) {
		*pEcmPID = m_ServiceList[Index].EcmPID;
		return true;
	}
	return false;
}


int CTsAnalyzer::GetServiceName(const int Index, LPTSTR pszName, const int MaxLength)
{
	CBlockLock Lock(&m_DecoderLock);

	// サービス名を取得する
	if (Index >= 0 && (size_t)Index < m_ServiceList.size()) {
		if (pszName != NULL && MaxLength > 0)
			::lstrcpyn(pszName, m_ServiceList[Index].szServiceName, MaxLength);
		return ::lstrlen(m_ServiceList[Index].szServiceName);
	}
	return 0;
}


bool CTsAnalyzer::GetServiceList(CServiceList *pList)
{
	CBlockLock Lock(&m_DecoderLock);

	pList->m_ServiceList = m_ServiceList;
	return true;
}


bool CTsAnalyzer::GetViewableServiceList(CServiceList *pList)
{
	CBlockLock Lock(&m_DecoderLock);

	pList->m_ServiceList.clear();
	for (size_t i = 0 ; i < m_ServiceList.size() ; i++) {
		if (m_ServiceList[i].VideoStreamType == VIEWABLE_STREAM_TYPE)
			pList->m_ServiceList.push_back(m_ServiceList[i]);
	}
	return true;
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


WORD CTsAnalyzer::GetEventID(const int ServiceIndex, const bool fNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable=dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index=pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index>=0)
				return pEitTable->GetEventID(Index,fNext?1:0);
		}

#ifdef TVH264
		const CLEitTable *pLEitTable=dynamic_cast<const CLEitTable*>(m_PidMapManager.GetMapTarget(0x0027));

		if (pLEitTable) {
			int Index=pLEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index>=0)
				return pLEitTable->GetEventID(Index,fNext?1:0);
		}
#endif
	}
	return 0;
}


bool CTsAnalyzer::GetEventStartTime(const int ServiceIndex, SYSTEMTIME *pSystemTime, const bool bNext)
{
	if (pSystemTime == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
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

#ifdef TVH264
		const CLEitTable *pLEitTable = dynamic_cast<const CLEitTable*>(m_PidMapManager.GetMapTarget(0x0027));

		if (pLEitTable) {
			int Index = pLEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0) {
				const SYSTEMTIME *pStartTime = pLEitTable->GetStartTime(Index, bNext ? 1 : 0);
				if (pStartTime)
					*pSystemTime = *pStartTime;
				return true;
			}
		}
#endif
	}
	return false;
}


DWORD CTsAnalyzer::GetEventDuration(const int ServiceIndex, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetDuration(Index, bNext ? 1 : 0);
		}

#ifdef TVH264
		const CLEitTable *pLEitTable = dynamic_cast<const CLEitTable*>(m_PidMapManager.GetMapTarget(0x0027));

		if (pLEitTable) {
			int Index = pLEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pLEitTable->GetDuration(Index, bNext ? 1 : 0);
		}
#endif
	}
	return 0;
}


int CTsAnalyzer::GetEventName(const int ServiceIndex, LPTSTR pszName, int MaxLength, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}

#ifdef TVH264
	pDescBlock = GetLEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventName(pszName, MaxLength);
	}
#endif
	return 0;
}


int CTsAnalyzer::GetEventText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bNext)
{
	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}

#ifdef TVH264
	pDescBlock = GetLEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CShortEventDesc *pShortEvent = dynamic_cast<const CShortEventDesc *>(pDescBlock->GetDescByTag(CShortEventDesc::DESC_TAG));

		if (pShortEvent)
			return pShortEvent->GetEventDesc(pszText, MaxLength);
	}
#endif
	return 0;
}


bool CTsAnalyzer::GetEventVideoInfo(const int ServiceIndex, EventVideoInfo *pInfo, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		const CComponentDesc *pComponentDesc = dynamic_cast<const CComponentDesc *>(pDescBlock->GetDescByTag(CComponentDesc::DESC_TAG));

		if (pComponentDesc) {
			pInfo->StreamContent = pComponentDesc->GetStreamContent();
			pInfo->ComponentType = pComponentDesc->GetComponentType();
			pInfo->ComponentTag = pComponentDesc->GetComponentTag();
			pInfo->LanguageCode = pComponentDesc->GetLanguageCode();
			pComponentDesc->GetText(pInfo->szText, EventVideoInfo::MAX_TEXT);
			return true;
		}
	}
	return false;
}


bool CTsAnalyzer::GetEventAudioList(const int ServiceIndex, EventAudioList *pList, const bool bNext)
{
	if (pList == NULL)
		return false;

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock) {
		pList->clear();
		for (WORD i = 0; i < pDescBlock->GetDescNum(); i++) {
			const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);

			if (pDesc->GetTag() == CAudioComponentDesc::DESC_TAG) {
				const CAudioComponentDesc *pAudioComponent = dynamic_cast<const CAudioComponentDesc*>(pDesc);

				if (pAudioComponent) {
					EventAudioInfo AudioInfo;

					AudioInfo.StreamContent = pAudioComponent->GetStreamContent();
					AudioInfo.ComponentType = pAudioComponent->GetComponentType();
					AudioInfo.ComponentTag = pAudioComponent->GetComponentTag();
					AudioInfo.SimulcastGroupTag = pAudioComponent->GetSimulcastGroupTag();
					AudioInfo.bESMultiLingualFlag = pAudioComponent->GetESMultiLingualFlag();
					AudioInfo.bMainComponentFlag = pAudioComponent->GetMainComponentFlag();
					AudioInfo.QualityIndicator = pAudioComponent->GetQualityIndicator();
					AudioInfo.SamplingRate = pAudioComponent->GetSamplingRate();
					AudioInfo.LanguageCode = pAudioComponent->GetLanguageCode();
					AudioInfo.LanguageCode2 = pAudioComponent->GetLanguageCode2();
					pAudioComponent->GetText(AudioInfo.szText, EventAudioInfo::MAX_TEXT);
					pList->push_back(AudioInfo);
				}
			}
		}
		return true;
	}
	return false;
}


bool CTsAnalyzer::GetEventContentNibble(const int ServiceIndex, EventContentNibble *pInfo, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock != NULL) {
		const CContentDesc *pContentDesc = dynamic_cast<const CContentDesc *>(pDescBlock->GetDescByTag(CContentDesc::DESC_TAG));

		if (pContentDesc) {
			pInfo->NibbleCount = pContentDesc->GetNibbleCount();
			for (int i = 0; i < pInfo->NibbleCount; i++)
				pContentDesc->GetNibble(i, &pInfo->NibbleList[i]);
			return true;
		}
	}
	return false;
}


#include "TsEncode.h"

int CTsAnalyzer::GetEventExtendedText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bUseEventGroup, const bool bNext)
{
	if (pszText == NULL || MaxLength < 1)
		return 0;

	pszText[0] = '\0';

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock == NULL)
		return 0;
	if (pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL) {
		if (!bUseEventGroup)
			return 0;

		// イベント共有の参照先から情報を取得する
#if 0
		const CEventGroupDesc *pEventGroup = dynamic_cast<const CEventGroupDesc*>(pDescBlock->GetDescByTag(CEventGroupDesc::DESC_TAG));
		if (pEventGroup == NULL
				|| pEventGroup->GetGroupType() != CEventGroupDesc::GROUPTYPE_COMMON
				|| pEventGroup->GetEventNum() < 1)
			return 0;
		const WORD EventID = GetEventID(ServiceIndex, bNext);
		int i;
		// 自己の記述がない場合は参照元
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)
					&& EventInfo.ServiceID == m_ServiceList[ServiceIndex].ServiceID
					&& EventInfo.EventID == EventID)
				return 0;
		}
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)) {
				int Index = GetServiceIndexByID(EventInfo.ServiceID);
				if (Index >= 0) {
					if (pEitTable->GetEventID(pEitTable->GetServiceIndexByID(EventInfo.ServiceID), bNext ? 1 : 0) != EventInfo.EventID
							|| (pDescBlock = GetHEitItemDesc(Index, bNext)) == NULL
							|| pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL)
						return 0;
					break;
				}
			}
		}
		if (i == pEventGroup->GetEventNum())
			return 0;
#else
		/*
			参照先にしかイベントグループ記述子が無かったり、
			参照元なのに自己のService_idとevent_idが記述してあったりするので(なぜ?)
			取りあえず先頭サービスが参照先になっている場合のみ想定する
		*/
		WORD ServiceID;
		if (!GetFirstViewableServiceID(&ServiceID)
				|| ServiceID == m_ServiceList[ServiceIndex].ServiceID)
			return 0;
		pDescBlock = GetHEitItemDesc(GetServiceIndexByID(ServiceID), bNext);
		if (pDescBlock == NULL)
			return false;
		const CEventGroupDesc *pEventGroup = dynamic_cast<const CEventGroupDesc*>(pDescBlock->GetDescByTag(CEventGroupDesc::DESC_TAG));
		if (pEventGroup == NULL
				|| pEventGroup->GetGroupType() != CEventGroupDesc::GROUPTYPE_COMMON
				|| pEventGroup->GetEventNum() <= 1)
			return 0;
		const WORD EventID = GetEventID(ServiceIndex, bNext);
		int i;
		for (i = 0; i < pEventGroup->GetEventNum(); i++) {
			CEventGroupDesc::EventInfo EventInfo;
			if (pEventGroup->GetEventInfo(i, &EventInfo)
					&& EventInfo.ServiceID == m_ServiceList[ServiceIndex].ServiceID
					&& EventInfo.EventID == EventID) {
				break;
			}
		}
		if (i == pEventGroup->GetEventNum())
			return 0;
		int Index = GetServiceIndexByID(ServiceID);
		if (Index < 0
				|| (pDescBlock = GetHEitItemDesc(Index, bNext)) == NULL
				|| pDescBlock->GetDescByTag(CExtendedEventDesc::DESC_TAG) == NULL)
			return 0;
#endif
	}

	std::vector<const CExtendedEventDesc *> DescList;
	for (int i = 0; i < pDescBlock->GetDescNum(); i++) {
		const CBaseDesc *pDesc = pDescBlock->GetDescByIndex(i);
		if (pDesc != NULL && pDesc->GetTag() == CExtendedEventDesc::DESC_TAG) {
			const CExtendedEventDesc *pExtendedEvent = dynamic_cast<const CExtendedEventDesc *>(pDesc);
			if (pExtendedEvent != NULL) {
				DescList.push_back(pExtendedEvent);
			}
		}
	}
	if (DescList.size() == 0)
		return 0;

	// descriptor_number 順にソートする
	for (int i = (int)DescList.size() - 2; i >= 0; i--) {
		const CExtendedEventDesc *pKey = DescList[i];
		int j;
		for (j = i + 1; j < (int)DescList.size() && DescList[j]->GetDescriptorNumber() < pKey->GetDescriptorNumber(); j++)
			DescList[j - 1] = DescList[j];
		DescList[j - 1] = pKey;
	}

	struct ItemInfo {
		BYTE DescriptorNumber;
		LPCTSTR pszDescription;
		int Data1Length;
		const BYTE *pData1;
		int Data2Length;
		const BYTE *pData2;
	};
	std::vector<ItemInfo> ItemList;
	for (int i = 0; i < (int)DescList.size(); i++) {
		const CExtendedEventDesc *pExtendedEvent = DescList[i];
		for (int j = 0; j < pExtendedEvent->GetItemCount(); j++) {
			const CExtendedEventDesc::ItemInfo *pItem = pExtendedEvent->GetItem(j);
			if (pItem == NULL)
				continue;
			if (pItem->szDescription[0] != '\0') {
				// 新規項目
				ItemInfo Item;
				Item.DescriptorNumber = pExtendedEvent->GetDescriptorNumber();
				Item.pszDescription = pItem->szDescription;
				Item.Data1Length = pItem->ItemLength;
				Item.pData1 = pItem->ItemChar;
				Item.Data2Length = 0;
				Item.pData2 = NULL;
				ItemList.push_back(Item);
			} else if (ItemList.size() > 0) {
				// 前の項目の続き
				ItemInfo &Item = ItemList[ItemList.size() - 1];
				if (Item.DescriptorNumber == pExtendedEvent->GetDescriptorNumber() - 1
						&& Item.pData2 == NULL) {
					Item.Data2Length = pItem->ItemLength;
					Item.pData2 = pItem->ItemChar;
				}
			}
		}
	}

	TCHAR szText[1024];
	int Length;
	int Pos = 0;
	for (int i = 0; i < (int)ItemList.size(); i++) {
		ItemInfo &Item = ItemList[i];
		Length = ::lstrlen(Item.pszDescription);
		if (Length + 2 >= MaxLength - Pos)
			break;
		::lstrcpy(&pszText[Pos], Item.pszDescription);
		Pos += Length;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
		if (Item.pData2 == NULL) {
			CAribString::AribToString(szText, 1024, Item.pData1, Item.Data1Length);
		} else {
			BYTE Buffer[220 * 2];
			::CopyMemory(Buffer, Item.pData1, Item.Data1Length);
			::CopyMemory(Buffer + Item.Data1Length, Item.pData2, Item.Data2Length);
			CAribString::AribToString(szText, 1024, Buffer, Item.Data1Length + Item.Data2Length);
		}
		LPTSTR p = szText;
		while (*p != '\0') {
			if (Pos >= MaxLength - 1)
				break;
			pszText[Pos++] = *p;
			if (*p == '\r') {
				if (*(p + 1) != '\n') {
					if (Pos == MaxLength - 1)
						break;
					pszText[Pos++] = '\n';
				}
			}
			p++;
		}
		if (Pos + 2 >= MaxLength)
			break;
		pszText[Pos++] = '\r';
		pszText[Pos++] = '\n';
	}
	pszText[Pos] = '\0';

	return Pos;
}


bool CTsAnalyzer::GetEventSeriesInfo(const int ServiceIndex, EventSeriesInfo *pInfo, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	const CDescBlock *pDescBlock = GetHEitItemDesc(ServiceIndex, bNext);
	if (pDescBlock != NULL) {
		const CSeriesDesc *pSeriesDesc = dynamic_cast<const CSeriesDesc *>(pDescBlock->GetDescByTag(CSeriesDesc::DESC_TAG));

		if (pSeriesDesc) {
			pInfo->SeriesID = pSeriesDesc->GetSeriesID();
			pInfo->RepeatLabel = pSeriesDesc->GetRepeatLabel();
			pInfo->ProgramPattern = pSeriesDesc->GetProgramPattern();
			pInfo->bIsExpireDateValid = pSeriesDesc->IsExpireDateValid()
				&& pSeriesDesc->GetExpireDate(&pInfo->ExpireDate);
			pInfo->EpisodeNumber = pSeriesDesc->GetEpisodeNumber();
			pInfo->LastEpisodeNumber = pSeriesDesc->GetLastEpisodeNumber();
			pSeriesDesc->GetSeriesName(pInfo->szSeriesName, CSeriesDesc::MAX_SERIES_NAME);
			return true;
		}
	}
	return false;
}


bool CTsAnalyzer::GetEventInfo(const int ServiceIndex, EventInfo *pInfo, const bool bUseEventGroup, const bool bNext)
{
	if (pInfo == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	pInfo->EventID = GetEventID(ServiceIndex, bNext);
	if (pInfo->EventID == 0)
		return false;
	pInfo->bValidStartTime = GetEventStartTime(ServiceIndex, &pInfo->StartTime, bNext);
	pInfo->Duration = GetEventDuration(ServiceIndex, bNext);
	if (pInfo->pszEventName != NULL && pInfo->MaxEventName > 0) {
		pInfo->pszEventName[0] = '\0';
		GetEventName(ServiceIndex, pInfo->pszEventName, pInfo->MaxEventName, bNext);
	}
	if (pInfo->pszEventText != NULL && pInfo->MaxEventText > 0) {
		pInfo->pszEventText[0] = '\0';
		GetEventText(ServiceIndex, pInfo->pszEventText, pInfo->MaxEventText, bNext);
	}
	if (pInfo->pszEventExtendedText != NULL && pInfo->MaxEventExtendedText > 0) {
		pInfo->pszEventExtendedText[0] = '\0';
		GetEventExtendedText(ServiceIndex, pInfo->pszEventExtendedText, pInfo->MaxEventExtendedText, bUseEventGroup, bNext);
	}

	::ZeroMemory(&pInfo->Video, sizeof(EventVideoInfo));
	GetEventVideoInfo(ServiceIndex, &pInfo->Video, bNext);

	pInfo->Audio.clear();
	GetEventAudioList(ServiceIndex, &pInfo->Audio, bNext);

	if (!GetEventContentNibble(ServiceIndex, &pInfo->ContentNibble, bNext))
		pInfo->ContentNibble.NibbleCount = 0;

	return true;
}


const CDescBlock *CTsAnalyzer::GetHEitItemDesc(const int ServiceIndex, const bool bNext) const
{
	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CHEitTable *pEitTable = dynamic_cast<const CHEitTable*>(m_PidMapManager.GetMapTarget(0x0012));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetItemDesc(Index, bNext ? 1 : 0);
		}
	}
	return NULL;
}


#ifdef TVH264
const CDescBlock *CTsAnalyzer::GetLEitItemDesc(const int ServiceIndex, const bool bNext) const
{
	if (ServiceIndex >= 0 && (size_t)ServiceIndex < m_ServiceList.size()) {
		const CLEitTable *pEitTable = dynamic_cast<const CLEitTable*>(m_PidMapManager.GetMapTarget(0x0027));

		if (pEitTable) {
			int Index = pEitTable->GetServiceIndexByID(m_ServiceList[ServiceIndex].ServiceID);

			if (Index >= 0)
				return pEitTable->GetItemDesc(Index, bNext ? 1 : 0);
		}
	}
	return NULL;
}
#endif


#endif	// TS_ANALYZER_EIT_SUPPORT


bool CTsAnalyzer::GetTotTime(SYSTEMTIME *pTime)
{
	CBlockLock Lock(&m_DecoderLock);

	if (pTime == NULL)
		return false;

	const CTotTable *pTotTable = dynamic_cast<const CTotTable*>(m_PidMapManager.GetMapTarget(0x0014));
	if (pTotTable)
		return pTotTable->GetDateTime(pTime);
	return false;
}


bool CTsAnalyzer::AddEventHandler(IAnalyzerEventHandler *pHandler)
{
	if (pHandler == NULL)
		return false;

	CBlockLock Lock(&m_DecoderLock);

	m_EventHandlerList.push_back(pHandler);

	return true;
}


bool CTsAnalyzer::RemoveEventHandler(IAnalyzerEventHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	for (std::vector<IAnalyzerEventHandler*>::iterator itr = m_EventHandlerList.begin(); itr != m_EventHandlerList.end(); itr++) {
		if (*itr == pHandler) {
			m_EventHandlerList.erase(itr);
			return true;
		}
	}
	return false;
}


void CTsAnalyzer::CallEventHandler(EventType Type)
{
	for (size_t i = 0; i < m_EventHandlerList.size(); i++) {
		m_EventHandlerList[i]->OnEvent(this, Type);
	}

	SendDecoderEvent((DWORD)Type);
}


void CTsAnalyzer::NotifyResetEvent()
{
	for (size_t i = 0; i < m_EventHandlerList.size(); i++) {
		m_EventHandlerList[i]->OnReset(this);
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
	for (size_t Index = 0; Index < pThis->m_ServiceList.size(); Index++) {
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PmtPID);
		pMapManager->UnmapTarget(pThis->m_ServiceList[Index].PcrPID);
	}

	// 新PMTをストアする
	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t Index = 0; Index < pThis->m_ServiceList.size(); Index++) {
		// サービスリスト更新
		pThis->m_ServiceList[Index].bIsUpdated = false;
		pThis->m_ServiceList[Index].ServiceID = pPatTable->GetProgramID(Index);
		pThis->m_ServiceList[Index].PmtPID = pPatTable->GetPmtPID(Index);
		pThis->m_ServiceList[Index].VideoStreamType = 0xFF;
		pThis->m_ServiceList[Index].VideoEs.PID = PID_INVALID;
		pThis->m_ServiceList[Index].VideoEs.ComponentTag = COMPONENTTAG_INVALID;
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
		pMapManager->MapTarget(pPatTable->GetPmtPID(Index), new CPmtTable(TABLE_DEBUG), OnPmtUpdated, pParam);
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
	Info.VideoEs.PID = PID_INVALID;
	Info.VideoEs.ComponentTag = COMPONENTTAG_INVALID;
	Info.AudioEsList.clear();
	Info.SubtitleEsList.clear();
	Info.DataCarrouselEsList.clear();
	for (WORD EsIndex = 0; EsIndex < pPmtTable->GetEsInfoNum(); EsIndex++) {
		const BYTE StreamType = pPmtTable->GetStreamTypeID(EsIndex);
		const WORD EsPID = pPmtTable->GetEsPID(EsIndex);

		BYTE ComponentTag = COMPONENTTAG_INVALID;
		const CDescBlock *pDescBlock = pPmtTable->GetItemDesc(EsIndex);
		if (pDescBlock) {
			const CStreamIdDesc *pStreamIdDesc = dynamic_cast<const CStreamIdDesc*>(pDescBlock->GetDescByTag(CStreamIdDesc::DESC_TAG));

			if (pStreamIdDesc)
				ComponentTag = pStreamIdDesc->GetComponentTag();
		}

		switch (StreamType) {
		case 0x02:	// ITU-T Rec. H.262 | ISO/IEC 13818-2 Video or ISO/IEC 11172-2
			if (Info.VideoEs.PID == PID_INVALID
					|| Info.VideoStreamType != 0x02) {
				Info.VideoStreamType = StreamType;
				Info.VideoEs.PID = EsPID;
				Info.VideoEs.ComponentTag = ComponentTag;
			}
			break;

		case 0x06:	// ITU-T Rec.H.222 | ISO/IEC 13818-1
			Info.SubtitleEsList.push_back(EsInfo(EsPID, ComponentTag));
			break;

		case 0x0D:
			Info.DataCarrouselEsList.push_back(EsInfo(EsPID, ComponentTag));
			break;

		case 0x0F:	// ISO/IEC 13818-7 Audio (ADTS Transport Syntax)
			Info.AudioEsList.push_back(EsInfo(EsPID, ComponentTag));
			break;

		case 0x1B:	// ITU-T Rec.H.264 | ISO/IEC 14496-10Video
			if (Info.VideoEs.PID == PID_INVALID) {
				Info.VideoStreamType = StreamType;
				Info.VideoEs.PID = EsPID;
				Info.VideoEs.ComponentTag = ComponentTag;
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

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_SDT_UPDATED);
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


void CALLBACK CTsAnalyzer::OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PCRが更新された
	CTsAnalyzer *pThis = static_cast<CTsAnalyzer *>(pParam);
	CPcrTable *pPcrTable = dynamic_cast<CPcrTable *>(pMapTarget);
	if (pPcrTable == NULL)
		return;

	const ULONGLONG TimeStamp = pPcrTable->GetPcrTimeStamp();

	WORD ServiceIndex;
	for (WORD Index = 0; pPcrTable->GetServiceIndex(&ServiceIndex, Index); Index++) {
		if (ServiceIndex < pThis->m_ServiceList.size()) {
			pThis->m_ServiceList[ServiceIndex].PcrTimeStamp = TimeStamp;
		}
	}

	// イベントハンドラ呼び出し
	pThis->CallEventHandler(EVENT_PCR_UPDATED);
}




CTsAnalyzer::IAnalyzerEventHandler::IAnalyzerEventHandler()
{
}


CTsAnalyzer::IAnalyzerEventHandler::~IAnalyzerEventHandler()
{
}


void CTsAnalyzer::IAnalyzerEventHandler::OnReset(CTsAnalyzer *pAnalyzer)
{
}
