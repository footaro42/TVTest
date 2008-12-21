// TsPacketParser.h: CTsPacketParser クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "TsStream.h"
#include "Epg.h"


/////////////////////////////////////////////////////////////////////////////
// TSパケット抽出デコーダ(バイナリデータからTSパケットを抽出する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData	TSパケットを含むバイナリデータ
// Output	#0	: CTsPacket		TSパケット
/////////////////////////////////////////////////////////////////////////////

class CTsPacketParser : public CMediaDecoder  
{
public:
	CTsPacketParser(IEventHandler *pEventHandler = NULL);
	virtual ~CTsPacketParser();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsPacketParser
	void SetOutputNullPacket(const bool bEnable = true);
	const DWORD GetInputPacketCount(void) const;
	const DWORD GetOutputPacketCount(void) const;
	const DWORD GetErrorPacketCount(void) const;
	const DWORD GetContinuityErrorPacketCount(void) const;
	void ResetErrorPacketCount(void);

	// Append by HDUSTestの中の人
	bool InitializeEpgDataCap(LPCTSTR pszDllFileName);
	bool UnInitializeEpgDataCap();
	CEpgDataInfo *GetEpgDataInfo(WORD wSID,bool bNext);
	CEpgDataCapDllUtil *GetEpgDataCapDllUtil() { return &m_EpgCap; }
private:
	void inline SyncPacket(const BYTE *pData, const DWORD dwSize);
	bool inline ParsePacket(void);

	CTsPacket m_TsPacket;

	bool m_bOutputNullPacket;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_OutputPacketCount;
	ULONGLONG m_ErrorPacketCount;
	ULONGLONG m_ContinuityErrorPacketCount;
	BYTE m_abyContCounter[0x1FFF];

	// Append by HDUSTestの中の人
	CEpgDataCapDllUtil m_EpgCap;
};
