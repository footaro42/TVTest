#pragma once


#include "MediaDecoder.h"
#include "TsTable.h"


class CLogoDownloader : public CMediaDecoder
{
public:
	CLogoDownloader(IEventHandler *pEventHandler = NULL);
	virtual ~CLogoDownloader();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CLogoDownloader
	struct LogoData {
		WORD OriginalNetworkID;
		WORD LogoID;
		WORD LogoVersion;
		BYTE LogoType;
		WORD DataSize;
		const BYTE *pData;
	};

	class __declspec(novtable) ILogoHandler
	{
	public:
		virtual ~ILogoHandler() {}
		virtual void OnLogo(const LogoData *pData) = 0;
	};

	void SetLogoHandler(ILogoHandler *pHandler);

private:
	CCdtTable m_CdtTable;
	ILogoHandler *m_pLogoHandler;
};
