#include "stdafx.h"
#include "Common.h"
#include "LogoDownloader.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


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

	m_CdtTable.Reset();
}


const bool CLogoDownloader::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	CTsPacket *pTsPacket = static_cast<CTsPacket *>(pMediaData);

	if (pTsPacket->GetPID() == PID_CDT) {
		if (m_CdtTable.StorePacket(pTsPacket)
				&& m_pLogoHandler
				&& m_CdtTable.GetDataType() == CCdtTable::DATATYPE_LOGO) {
			const WORD DataSize = m_CdtTable.GetDataModuleSize();
			const BYTE *pData = m_CdtTable.GetDataModuleByte();

			if (DataSize > 7 && pData) {
				LogoData Data;

				Data.OriginalNetworkID = m_CdtTable.GetOriginalNetworkId();
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

	// 次のフィルタにデータを渡す
	OutputMedia(pMediaData);

	return true;
}


void CLogoDownloader::SetLogoHandler(ILogoHandler *pHandler)
{
	CBlockLock Lock(&m_DecoderLock);

	m_pLogoHandler = pHandler;
}
