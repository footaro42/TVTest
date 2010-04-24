#include "stdafx.h"
#include <map>
#include "LogoDownloader.h"
#include "TsDownload.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


class CLogoDataModule : public CDataModule
{
public:
	struct ServiceInfo {
		WORD OriginalNetworkID;
		WORD TransportStreamID;
		WORD ServiceID;
	};

	struct LogoInfo {
		BYTE LogoType;
		WORD LogoID;
		std::vector<ServiceInfo> ServiceList;
		WORD DataSize;
		const BYTE *pData;
	};

	class ABSTRACT_CLASS_DECL IEventHandler {
	public:
		virtual ~IEventHandler() {}
		virtual void OnLogoData(const LogoInfo *pInfo) = 0;
	};

	CLogoDataModule(DWORD DownloadID, WORD BlockSize, WORD ModuleID, DWORD ModuleSize, BYTE ModuleVersion,
					IEventHandler *pHandler)
		: CDataModule(DownloadID, BlockSize, ModuleID, ModuleSize, ModuleVersion)
		, m_pEventHandler(pHandler)
	{
	}

private:
// CDataModule
	virtual void OnComplete(const BYTE *pData, const DWORD ModuleSize);

	IEventHandler *m_pEventHandler;
};

void CLogoDataModule::OnComplete(const BYTE *pData, const DWORD ModuleSize)
{
	if (ModuleSize < 3)
		return;

	LogoInfo Info;

	Info.LogoType = pData[0];
	if (Info.LogoType > 0x05)
		return;

	const WORD NumberOfLoop = ((WORD)pData[1] << 8) | (WORD)pData[2];

	DWORD Pos = 3;
	for (WORD i = 0; i < NumberOfLoop; i++) {
		if (Pos + 3 >= ModuleSize)
			return;

		Info.LogoID = ((WORD)(pData[Pos + 0] & 0x01) << 8) | (WORD)pData[Pos + 1];
		const BYTE NumberOfServices = pData[Pos + 2];
		Pos += 3;
		if (NumberOfServices == 0
				|| Pos + 6 * NumberOfServices + 2 >= ModuleSize)
			return;

		Info.ServiceList.resize(NumberOfServices);

		TRACE(TEXT("[%d/%d] Logo ID %04X / %d Services\n"),i+1,NumberOfLoop,Info.LogoID,NumberOfServices);

		for (BYTE j = 0; j < NumberOfServices; j++) {
			Info.ServiceList[j].OriginalNetworkID = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
			Info.ServiceList[j].TransportStreamID = ((WORD)pData[Pos + 2] << 8) | (WORD)pData[Pos + 3];
			Info.ServiceList[j].ServiceID = ((WORD)pData[Pos + 4] << 8) | (WORD)pData[Pos + 5];
			Pos += 6;

			TRACE(TEXT("[%d:%2d/%2d] Network ID %04X / TSID %04X / Service ID %04X\n"),
				  i+1,j+1,NumberOfServices,
				  Info.ServiceList[j].OriginalNetworkID,
				  Info.ServiceList[j].TransportStreamID,
				  Info.ServiceList[j].ServiceID);
		}

		const WORD DataSize = ((WORD)pData[Pos + 0] << 8) | (WORD)pData[Pos + 1];
		Pos+=2;
		if (DataSize == 0 || Pos + DataSize > ModuleSize)
			return;

		Info.DataSize = DataSize;
		Info.pData = &pData[Pos];

		m_pEventHandler->OnLogoData(&Info);

		Pos += DataSize;
	}
}


class CDsmccSection : public CPsiStreamTable
					, public CDownloadInfoIndicationParser::IEventHandler
					, public CDownloadDataBlockParser::IEventHandler
					, public CLogoDataModule::IEventHandler
{
public:
	typedef void (CALLBACK *LogoDataCallback)(const CLogoDownloader::LogoData *pData, void *pParam);

	CDsmccSection(LogoDataCallback pCallback, void *pCallbackParam);
	~CDsmccSection();

private:
// CPsiStreamTable
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection);

// CDownloadInfoIndicationParser::IEventHandler
	virtual void OnDataModule(const CDownloadInfoIndicationParser::MessageInfo *pMessageInfo,
							  const CDownloadInfoIndicationParser::ModuleInfo *pModuleInfo);

// CDownloadDataBlockParser::IEventHandler
	virtual void OnDataBlock(const CDownloadDataBlockParser::DataBlockInfo *pDataBlock);

// CLogoDataModule::IEventHandler
	virtual void OnLogoData(const CLogoDataModule::LogoInfo *pInfo);

	CDownloadInfoIndicationParser m_DII;
	CDownloadDataBlockParser m_DDB;

	typedef std::map<WORD, CLogoDataModule*> LogoDataMap;
	LogoDataMap m_LogoDataMap;

	LogoDataCallback m_pLogoDataCallback;
	void *m_pLogoDataCallbackParam;
};

CDsmccSection::CDsmccSection(LogoDataCallback pCallback, void *pCallbackParam)
	: CPsiStreamTable(NULL, true, true)
	, m_DII(this)
	, m_DDB(this)
	, m_pLogoDataCallback(pCallback)
	, m_pLogoDataCallbackParam(pCallbackParam)
{
}

CDsmccSection::~CDsmccSection()
{
	for (LogoDataMap::iterator itr = m_LogoDataMap.begin(); itr != m_LogoDataMap.end(); itr++) {
		delete itr->second;
	}
}

const bool CDsmccSection::OnTableUpdate(const CPsiSection *pCurSection)
{
	const WORD DataSize = pCurSection->GetPayloadSize();
	const BYTE *pData = pCurSection->GetPayloadData();

	if (pCurSection->GetTableID() == 0x3B) {
		// DII
		return m_DII.ParseData(pData, DataSize);
	} else if (pCurSection->GetTableID() == 0x3C) {
		// DDB
		return m_DDB.ParseData(pData, DataSize);
	}

	return false;
}

void CDsmccSection::OnDataModule(const CDownloadInfoIndicationParser::MessageInfo *pMessageInfo,
								 const CDownloadInfoIndicationParser::ModuleInfo *pModuleInfo)
{
	if (!pModuleInfo->ModuleDesc.Name.pText
			|| (pModuleInfo->ModuleDesc.Name.Length != 7
				&& pModuleInfo->ModuleDesc.Name.Length != 10)
			|| (pModuleInfo->ModuleDesc.Name.Length == 7
				&& ::StrCmpNA(pModuleInfo->ModuleDesc.Name.pText, "LOGO-0", 6) != 0)
			|| (pModuleInfo->ModuleDesc.Name.Length == 10
				&& ::StrCmpNA(pModuleInfo->ModuleDesc.Name.pText, "CS_LOGO-0", 9) != 0))
		return;

	TRACE(TEXT("DII Logo Data : Download ID %08lX / Module ID %04X / Module size %lu\n"),
		  pMessageInfo->DownloadID, pModuleInfo->ModuleID, pModuleInfo->ModuleSize);

	LogoDataMap::iterator itr = m_LogoDataMap.find(pModuleInfo->ModuleID);
	if (itr == m_LogoDataMap.end()) {
		m_LogoDataMap.insert(std::pair<WORD, CLogoDataModule*>(pModuleInfo->ModuleID,
			new CLogoDataModule(pMessageInfo->DownloadID, pMessageInfo->BlockSize,
								pModuleInfo->ModuleID, pModuleInfo->ModuleSize, pModuleInfo->ModuleVersion, this)));
	} else if (itr->second->GetDownloadID() != pMessageInfo->DownloadID
			|| itr->second->GetBlockSize() != pMessageInfo->BlockSize
			|| itr->second->GetModuleSize() != pModuleInfo->ModuleSize
			|| itr->second->GetModuleVersion() != pModuleInfo->ModuleVersion) {
		delete itr->second;
		m_LogoDataMap[pModuleInfo->ModuleID] = new CLogoDataModule(
			pMessageInfo->DownloadID, pMessageInfo->BlockSize,
			pModuleInfo->ModuleID, pModuleInfo->ModuleSize, pModuleInfo->ModuleVersion,
			this);
	}
}

void CDsmccSection::OnDataBlock(const CDownloadDataBlockParser::DataBlockInfo *pDataBlock)
{
	LogoDataMap::iterator itr = m_LogoDataMap.find(pDataBlock->ModuleID);
	if (itr != m_LogoDataMap.end()) {
		if (itr->second->GetDownloadID() == pDataBlock->DownloadID
				&& itr->second->GetModuleVersion() == pDataBlock->ModuleVersion) {
			itr->second->StoreBlock(pDataBlock->BlockNumber, pDataBlock->pData, pDataBlock->DataSize);
		}
	}
}

void CDsmccSection::OnLogoData(const CLogoDataModule::LogoInfo *pInfo)
{
	CLogoDownloader::LogoData LogoData;

	LogoData.OriginalNetworkID = pInfo->ServiceList[0].OriginalNetworkID;
	LogoData.ServiceList.resize(pInfo->ServiceList.size());
	for (size_t i = 0; i < pInfo->ServiceList.size(); i++) {
		LogoData.ServiceList[i].OriginalNetworkID = pInfo->ServiceList[i].OriginalNetworkID;
		LogoData.ServiceList[i].TransportStreamID = pInfo->ServiceList[i].TransportStreamID;
		LogoData.ServiceList[i].ServiceID = pInfo->ServiceList[i].ServiceID;
	}
	LogoData.LogoID = pInfo->LogoID;
	LogoData.LogoVersion = 0;	// これはどこから持ってくるんだろう…(共通バージョンを使う?)
	LogoData.LogoType = pInfo->LogoType;
	LogoData.DataSize = pInfo->DataSize;
	LogoData.pData = pInfo->pData;

	m_pLogoDataCallback(&LogoData, m_pLogoDataCallbackParam);
}




CLogoDownloader::CLogoDownloader(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler, 1, 1)
	, m_pLogoHandler(NULL)
{
	Reset();
}


CLogoDownloader::~CLogoDownloader()
{
}


void CLogoDownloader::Reset()
{
	CBlockLock Lock(&m_DecoderLock);

	m_PidMapManager.UnmapAllTarget();
	m_PidMapManager.MapTarget(PID_PAT, new CPatTable, OnPatUpdated, this);
	m_PidMapManager.MapTarget(PID_CDT, new CCdtTable(this));

	m_ServiceList.clear();
}


const bool CLogoDownloader::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	m_PidMapManager.StorePacket(pTsPacket);

	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}


void CLogoDownloader::SetLogoHandler(ILogoHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	m_pLogoHandler = pHandler;
}


void CLogoDownloader::OnSection(CPsiStreamTable *pTable, const CPsiSection *pSection)
{
	// CDTからロゴ取得
	const CCdtTable *pCdtTable = dynamic_cast<const CCdtTable*>(pTable);

	if (m_pLogoHandler
			&& pCdtTable->GetDataType() == CCdtTable::DATATYPE_LOGO) {
		const WORD DataSize = pCdtTable->GetDataModuleSize();
		const BYTE *pData = pCdtTable->GetDataModuleByte();

		if (DataSize > 7 && pData) {
			LogoData Data;

			Data.OriginalNetworkID = pCdtTable->GetOriginalNetworkId();
			Data.LogoID = ((WORD)(pData[1] & 0x01) << 8) | (WORD)pData[2];
			Data.LogoVersion = ((WORD)(pData[3] & 0x0F) << 8) | (WORD)pData[4];
			Data.LogoType = pData[0];
			Data.DataSize = ((WORD)pData[5] << 8) | (WORD)pData[6];
			Data.pData = &pData[7];
			if (Data.LogoType <= 0x05
					&& Data.DataSize <= DataSize - 7)
				m_pLogoHandler->OnLogo(&Data);
		}
	}
}


void CALLBACK CLogoDownloader::OnLogoDataModule(const LogoData *pData, void *pParam)
{
	CLogoDownloader *pThis = static_cast<CLogoDownloader*>(pParam);

	if (pThis->m_pLogoHandler)
		pThis->m_pLogoHandler->OnLogo(pData);
}


void CALLBACK CLogoDownloader::OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PATが更新された
	CLogoDownloader *pThis = static_cast<CLogoDownloader *>(pParam);
	CPatTable *pPatTable = dynamic_cast<CPatTable *>(pMapTarget);
	if (pPatTable == NULL)
		return;

	for (size_t i = 0; i < pThis->m_ServiceList.size(); i++) {
		pMapManager->UnmapTarget(pThis->m_ServiceList[i].PmtPID);
		if (pThis->m_ServiceList[i].ServiceType == SERVICE_TYPE_ENGINEERING) {
			pThis->UnmapDataEs((int)i);
		}
	}

	pThis->m_ServiceList.resize(pPatTable->GetProgramNum());

	for (size_t i = 0; i < pThis->m_ServiceList.size(); i++) {
		pThis->m_ServiceList[i].ServiceID = pPatTable->GetProgramID((WORD)i);
		pThis->m_ServiceList[i].PmtPID = pPatTable->GetPmtPID((WORD)i);
		pThis->m_ServiceList[i].ServiceType = SERVICE_TYPE_INVALID;
		pThis->m_ServiceList[i].EsList.clear();

		pMapManager->MapTarget(pPatTable->GetPmtPID((WORD)i), new CPmtTable, OnPmtUpdated, pParam);
	}

	pMapManager->MapTarget(PID_NIT, new CNitTable, OnNitUpdated, pThis);
}


void CALLBACK CLogoDownloader::OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// PMTが更新された
	CLogoDownloader *pThis = static_cast<CLogoDownloader *>(pParam);
	CPmtTable *pPmtTable = dynamic_cast<CPmtTable *>(pMapTarget);
	if (pPmtTable == NULL)
		return;

	const int ServiceIndex = pThis->GetServiceIndexByID(pPmtTable->m_CurSection.GetTableIdExtension());
	if (ServiceIndex < 0)
		return;
	ServiceInfo &Info = pThis->m_ServiceList[ServiceIndex];

	if (Info.ServiceType == SERVICE_TYPE_ENGINEERING) {
		pThis->UnmapDataEs(ServiceIndex);
	}

	Info.EsList.clear();

	for (WORD EsIndex = 0; EsIndex < pPmtTable->GetEsInfoNum(); EsIndex++) {
		if (pPmtTable->GetStreamTypeID(EsIndex) == STREAM_TYPE_DATACARROUSEL) {
			const CDescBlock *pDescBlock = pPmtTable->GetItemDesc(EsIndex);
			if (pDescBlock) {
				const CStreamIdDesc *pStreamIdDesc = dynamic_cast<const CStreamIdDesc*>(pDescBlock->GetDescByTag(CStreamIdDesc::DESC_TAG));

				if (pStreamIdDesc
						&& (pStreamIdDesc->GetComponentTag() == 0x79
							|| pStreamIdDesc->GetComponentTag() == 0x7A)) {
					// 全受信機共通データ
					Info.EsList.push_back(pPmtTable->GetEsPID(EsIndex));
				}
			}
		}
	}

	if (Info.ServiceType == SERVICE_TYPE_ENGINEERING) {
		pThis->MapDataEs(ServiceIndex);
	}
}


void CALLBACK CLogoDownloader::OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam)
{
	// NITが更新された
	CLogoDownloader *pThis = static_cast<CLogoDownloader*>(pParam);
	CNitTable *pNitTable = dynamic_cast<CNitTable*>(pMapTarget);
	if (pNitTable == NULL)
		return;

	for (WORD i = 0; i < pNitTable->GetTransportStreamNum(); i++) {
		const CDescBlock *pDescBlock = pNitTable->GetItemDesc(i);

		if (pDescBlock) {
			const CServiceListDesc *pServiceListDesc = dynamic_cast<const CServiceListDesc*>(pDescBlock->GetDescByTag(CServiceListDesc::DESC_TAG));
			if (pServiceListDesc) {
				for (int j = 0; j < pServiceListDesc->GetServiceNum(); j++) {
					CServiceListDesc::ServiceInfo Info;

					if (pServiceListDesc->GetServiceInfo(j, &Info)) {
						int Index = pThis->GetServiceIndexByID(Info.ServiceID);
						if (Index >= 0) {
							const BYTE ServiceType = Info.ServiceType;
							if (pThis->m_ServiceList[Index].ServiceType != ServiceType) {
								if (ServiceType == SERVICE_TYPE_ENGINEERING) {
									pThis->MapDataEs(Index);
								} else if (pThis->m_ServiceList[Index].ServiceType == SERVICE_TYPE_ENGINEERING) {
									pThis->UnmapDataEs(Index);
								}
								pThis->m_ServiceList[Index].ServiceType = ServiceType;
							}
						}
					}
				}
			}
		}
	}
}


int CLogoDownloader::GetServiceIndexByID(const WORD ServiceID) const
{
	for (size_t i = 0; i < m_ServiceList.size(); i++) {
		if (m_ServiceList[i].ServiceID == ServiceID)
			return (int)i;
	}
	return -1;
}


bool CLogoDownloader::MapDataEs(const int Index)
{
	if (Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	ServiceInfo &Info = m_ServiceList[Index];

	TRACE(TEXT("CLogoDownloader::MapDataEs() : SID %04X / %lu stream(s)\n"),
		  Info.ServiceID, (ULONG)Info.EsList.size());

	for (size_t i = 0; i < Info.EsList.size(); i++) {
		m_PidMapManager.MapTarget(Info.EsList[i], new CDsmccSection(OnLogoDataModule, this));
	}

	return true;
}


bool CLogoDownloader::UnmapDataEs(const int Index)
{
	if (Index < 0 || (size_t)Index >= m_ServiceList.size())
		return false;

	ServiceInfo &Info = m_ServiceList[Index];
	for (size_t i = 0; i < Info.EsList.size(); i++) {
		m_PidMapManager.UnmapTarget(Info.EsList[i]);
	}

	return true;
}
