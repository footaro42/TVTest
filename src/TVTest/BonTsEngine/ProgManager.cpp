// ProgManager.cpp: CProgManager クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsTable.h"
#include "TsProgGuide.h"
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
	// サービスリストをクリア
	m_ServiceList.clear();

	// プログラムデータベースリセット
	m_pProgDatabase->Reset();

	// 下位デコーダをリセット
	CMediaDecoder::Reset();
}

const bool CProgManager::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if(dwInputIndex >= GetInputNum())return false;

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	// 入力メディアデータは互換性がない
	if(!pTsPacket)return false;

	// PIDルーティング
	m_PidMapManager.StorePacket(pTsPacket);


	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}

const WORD CProgManager::GetServiceNum(void) const
{
	// サービス数を返す
	return m_ServiceList.size();
}

const bool CProgManager::GetServiceID(WORD *pwServiceID, const WORD wIndex) const
{
	// サービスIDを取得する
	if(wIndex==0xFFFF){
		if(m_pProgDatabase){
			if(m_pProgDatabase->m_ServiceList.size()==0
					|| !m_pProgDatabase->m_ServiceList[0].bIsUpdated)
				return false;
			*pwServiceID = m_pProgDatabase->m_ServiceList[0].wServiceID;
			return true;
			}
		}
	if((wIndex < GetServiceNum()) && pwServiceID){
		*pwServiceID = m_ServiceList[wIndex].wServiceID;
		return true;		
		}
		
	return false;
}

const bool CProgManager::GetServiceEsPID(WORD *pwVideoPID, WORD *pwAudioPID, const WORD wIndex) const
{
	// ESのPIDを取得する
	if((wIndex < GetServiceNum()) && (pwVideoPID || pwAudioPID)){
		if(pwVideoPID)*pwVideoPID = m_ServiceList[wIndex].wVideoEsPID;
		if(pwVideoPID)*pwAudioPID = m_ServiceList[wIndex].wAudioEsPID;
		return true;
		}

	return false;
}

const bool CProgManager::GetPcrTimeStamp(unsigned __int64 *pu64PcrTimeStamp, const WORD wIndex) const
{
	// PCRを取得する
	if((wIndex < GetServiceNum()) && pu64PcrTimeStamp){
		*pu64PcrTimeStamp = m_ServiceList[wIndex].u64TimeStamp; 
		return true;
		}
	return false;	
}

const DWORD CProgManager::GetServiceName(LPTSTR lpszDst, const WORD wIndex) const
{
	// サービス名を取得する
	if((wIndex < GetServiceNum()) && lpszDst){
		const WORD wNameLen = ::lstrlen(m_ServiceList[wIndex].szServiceName);
		if(wNameLen)::lstrcpy(lpszDst, m_ServiceList[wIndex].szServiceName);
		return wNameLen;		
		}
		
	return 0U;
}

const WORD CProgManager::GetTransportStreamID()
{
	if(m_pProgDatabase){
		return m_pProgDatabase->m_wTransportStreamID;
	} else {
		return 0;
		}
}

WORD CProgManager::GetNetworkID(void)
{
	if (m_pProgDatabase==NULL)
		return 0;
	return m_pProgDatabase->m_NitInfo.wNetworkID;
}

BYTE CProgManager::GetBroadcastingID(void)
{
	if (m_pProgDatabase==NULL)
		return 0;
	return m_pProgDatabase->m_NitInfo.byBroadcastingID;
}

DWORD CProgManager::GetNetworkName(LPTSTR pszName,int MaxLength)
{
	if (m_pProgDatabase==NULL)
		return 0;
	if (pszName)
		::lstrcpyn(pszName,m_pProgDatabase->m_NitInfo.szNetworkName,MaxLength);
	return ::lstrlen(m_pProgDatabase->m_NitInfo.szNetworkName);
}

BYTE CProgManager::GetRemoteControlKeyID(void)
{
	if (m_pProgDatabase==NULL)
		return 0;
	return m_pProgDatabase->m_NitInfo.byRemoteControlKeyID;
}

DWORD CProgManager::GetTSName(LPTSTR pszName,int MaxLength)
{
	if (m_pProgDatabase==NULL)
		return 0;
	if (pszName)
		::lstrcpyn(pszName,m_pProgDatabase->m_NitInfo.szTSName,MaxLength);
	return ::lstrlen(m_pProgDatabase->m_NitInfo.szTSName);
}

void CProgManager::OnServiceListUpdated(void)
{
	// サービスリストクリア、リサイズ
	m_ServiceList.clear();

	// サービスリスト構築
	for(WORD wIndex = 0U, wServiceNum = 0U ; wIndex < m_pProgDatabase->m_ServiceList.size() ; wIndex++){
		if(m_pProgDatabase->m_ServiceList[wIndex].wVideoEsPID != 0xFFFFU){
			// MPEG2映像のみ(ワンセグ、データ放送以外)
			m_ServiceList.resize(wServiceNum + 1);
			m_ServiceList[wServiceNum].wServiceID = m_pProgDatabase->m_ServiceList[wIndex].wServiceID;
			m_ServiceList[wServiceNum].wVideoEsPID = m_pProgDatabase->m_ServiceList[wIndex].wVideoEsPID;
			m_ServiceList[wServiceNum].wAudioEsPID = m_pProgDatabase->m_ServiceList[wIndex].wAudioEsPID;
			m_ServiceList[wServiceNum].szServiceName[0] = TEXT('\0');
			wServiceNum++;
			}
		}

	TRACE(TEXT("CProgManager::OnServiceListUpdated()\n"));

	SendDecoderEvent(EID_SERVICE_LIST_UPDATED);
}

void CProgManager::OnServiceInfoUpdated(void)
{
	// サービス名を更新する
	for(WORD wIndex = 0U, wServiceNum = 0U ; wIndex < GetServiceNum() ; wIndex++){
		const WORD wServiceIndex = m_pProgDatabase->GetServiceIndexByID(m_ServiceList[wIndex].wServiceID);

		if(wServiceIndex != 0xFFFFU){
			if(m_pProgDatabase->m_ServiceList[wIndex].szServiceName[0]){
				::lstrcpy(m_ServiceList[wIndex].szServiceName, m_pProgDatabase->m_ServiceList[wServiceIndex].szServiceName);
				}
			else{
				::wsprintf(m_ServiceList[wIndex].szServiceName, TEXT("サービス%u"), wIndex + 1U);
				}
			}
		}

	TRACE(TEXT("CProgManager::OnServiceInfoUpdated()\n"));
	
	SendDecoderEvent(EID_SERVICE_INFO_UPDATED);
}

void CProgManager::OnPcrTimestampUpdated(void)
{
	// PCRを更新する
	for(WORD wIndex = 0U, wServiceNum = 0U ; wIndex < GetServiceNum() ; wIndex++){
		const WORD wServiceIndex = m_pProgDatabase->GetServiceIndexByID(m_ServiceList[wIndex].wServiceID);

		if(wServiceIndex != 0xFFFFU){
			m_ServiceList[wIndex].u64TimeStamp = m_pProgDatabase->m_ServiceList[wServiceIndex].u64TimeStamp;
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
	::ZeroMemory(&m_NitInfo,sizeof(m_NitInfo));
}

void CProgManager::CProgDatabase::UnmapTable(void)
{
	// 全PMT PIDアンマップ
	for(WORD wIndex = 0U ; wIndex < m_ServiceList.size() ; wIndex++){
		m_PidMapManager.UnmapTarget(m_ServiceList[wIndex].wPmtTablePID);
		}

	// サービスリストクリア
	m_ServiceList.clear();

	// トランスポートストリームID初期化
	m_wTransportStreamID = 0xFFFFU;

	// PATテーブルリセット
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(m_PidMapManager.GetMapTarget(0x0000U));
	if(pPatTable)pPatTable->Reset();
}

const WORD CProgManager::CProgDatabase::GetServiceIndexByID(const WORD wServiceID)
{
	// プログラムIDからサービスインデックスを検索する
	for(WORD wIndex = 0U ; wIndex < m_ServiceList.size() ; wIndex++){
		if(m_ServiceList[wIndex].wServiceID == wServiceID)return wIndex;
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
	for(WORD wIndex = 0U ; wIndex < pThis->m_ServiceList.size() ; wIndex++){
		WORD wPID;
		wPID = pThis->m_ServiceList[wIndex].wPmtTablePID;
		pMapManager->UnmapTarget(wPID);
		wPID = pThis->m_ServiceList[wIndex].wPcrPID;
		pMapManager->UnmapTarget(wPID);
		}

	// 新PMTをストアする
	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for(WORD wIndex = 0U ; wIndex < pThis->m_ServiceList.size() ; wIndex++){
		// サービスリスト更新
		pThis->m_ServiceList[wIndex].bIsUpdated = false;
		pThis->m_ServiceList[wIndex].wServiceID = pPatTable->GetProgramID(wIndex);
		pThis->m_ServiceList[wIndex].wPmtTablePID = pPatTable->GetPmtPID(wIndex);

		pThis->m_ServiceList[wIndex].wVideoEsPID = 0xFFFFU;
		pThis->m_ServiceList[wIndex].wAudioEsPID = 0xFFFFU;
		pThis->m_ServiceList[wIndex].wPcrPID = 0xFFFFU;		
		pThis->m_ServiceList[wIndex].byVideoComponentTag = 0xFFU;
		pThis->m_ServiceList[wIndex].byAudioComponentTag = 0xFFU;
		pThis->m_ServiceList[wIndex].byServiceType = 0xFFU;
		pThis->m_ServiceList[wIndex].byRunningStatus = 0xFFU;
		pThis->m_ServiceList[wIndex].bIsCaService = false;
		pThis->m_ServiceList[wIndex].szServiceName[0] = TEXT('\0');
		
		// PMTのPIDをマップ
		pMapManager->MapTarget(pPatTable->GetPmtPID(wIndex), new CPmtTable, CProgDatabase::OnPmtUpdated, pParam);
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
	
	for(WORD wEsIndex = 0U ; wEsIndex < pPmtTable->GetEsInfoNum() ; wEsIndex++){
		// 「ITU-T Rec. H.262|ISO/IEC 13818-2 Video or ISO/IEC 11172-2」のストリームタイプを検索
		if(pPmtTable->GetStreamTypeID(wEsIndex) == 0x02U){
			pThis->m_ServiceList[wServiceIndex].wVideoEsPID = pPmtTable->GetEsPID(wEsIndex);
			break;
			}		
		}

	// オーディオESのPIDをストア
	pThis->m_ServiceList[wServiceIndex].wAudioEsPID = 0xFFFFU;
	
	for(WORD wEsIndex = 0U ; wEsIndex < pPmtTable->GetEsInfoNum() ; wEsIndex++){
		// 「ISO/IEC 13818-7 Audio (ADTS Transport Syntax)」のストリームタイプを検索
		if(pPmtTable->GetStreamTypeID(wEsIndex) == 0x0FU){
			pThis->m_ServiceList[wServiceIndex].wAudioEsPID = pPmtTable->GetEsPID(wEsIndex);
			break;
			}
		}

	WORD wPcrPID = pPmtTable->GetPcrPID();
	if(wPcrPID<0x1FFFU){
		pThis->m_ServiceList[wServiceIndex].wPcrPID = wPcrPID;
		CTsPidMapTarget *pMap = pMapManager->GetMapTarget(wPcrPID);
		if(!pMap){
			// 新規Map
			pMapManager->MapTarget(wPcrPID, new CPcrTable(wServiceIndex), CProgDatabase::OnPcrUpdated, pParam);
			} else {
			// 既存Map
			CPcrTable *pPcrTable = dynamic_cast<CPcrTable*>(pMap);
			if(pPcrTable){
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
	for(WORD wIndex = 0U ; wIndex < pThis->m_ServiceList.size() ; wIndex++){
		if(!pThis->m_ServiceList[wIndex].bIsUpdated)return;
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

	for(WORD wSdtIndex = 0U ; wSdtIndex < pSdtTable->GetServiceNum() ; wSdtIndex++){
		// サービスIDを検索
		const WORD wServiceIndex = pThis->GetServiceIndexByID(pSdtTable->GetServiceID(wSdtIndex));
		if(wServiceIndex == 0xFFFFU)continue;

		// サービス情報更新
		pThis->m_ServiceList[wServiceIndex].byRunningStatus = pSdtTable->GetRunningStatus(wSdtIndex);
		pThis->m_ServiceList[wServiceIndex].bIsCaService = pSdtTable->GetFreeCaMode(wSdtIndex);
		
		// サービス名更新
		pThis->m_ServiceList[wServiceIndex].szServiceName[0] = TEXT('\0');

		const CDescBlock *pDescBlock = pSdtTable->GetItemDesc(wSdtIndex);
		const CServiceDesc *pServiceDesc = dynamic_cast<const CServiceDesc *>(pDescBlock->GetDescByTag(CServiceDesc::DESC_TAG));

		if(pServiceDesc){
			pServiceDesc->GetServiceName(pThis->m_ServiceList[wServiceIndex].szServiceName);
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
	for(WORD wIndex = 0 ; pPcrTable->GetServiceIndex(&wServiceIndex,wIndex); wIndex++){
		if(wServiceIndex<pThis->m_ServiceList.size()) {
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
	pDescBlock = pNitTable->GetNetworkNameDesc();
	if (pDescBlock) {
		const CNetworkNameDesc *pNetworkDesc = dynamic_cast<const CNetworkNameDesc *>(pDescBlock->GetDescByTag(CNetworkNameDesc::DESC_TAG));
		if(pNetworkDesc) {
			pNetworkDesc->GetNetworkName(pThis->m_NitInfo.szNetworkName);
		}
	}

	pDescBlock = pNitTable->GetSystemManageDesc();
	if (pDescBlock) {
		const CSystemManageDesc *pSysManageDesc = dynamic_cast<const CSystemManageDesc *>(pDescBlock->GetDescByTag(CSystemManageDesc::DESC_TAG));
		if (pSysManageDesc) {
			pThis->m_NitInfo.byBroadcastingID = pSysManageDesc->GetBroadcastingID();
		}
	}

	pDescBlock = pNitTable->GetTSInfoDesc();
	if (pDescBlock) {
		const CTSInfoDesc *pTsInfoDesc = dynamic_cast<const CTSInfoDesc *>(pDescBlock->GetDescByTag(CTSInfoDesc::DESC_TAG));
		if (pTsInfoDesc) {
			pTsInfoDesc->GetTSName(pThis->m_NitInfo.szTSName);
			pThis->m_NitInfo.byRemoteControlKeyID = pTsInfoDesc->GetRemoteControlKeyID();
		}
	}
}
