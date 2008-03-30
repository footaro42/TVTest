// AacConverter.h: CAacConverter クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "AacDecoder.h"


/////////////////////////////////////////////////////////////////////////////
// AACデコーダ(AACをPCMにデコードする)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CAdtsFrame		入力データ
// Output	#0	: CMediaData		出力データ
/////////////////////////////////////////////////////////////////////////////

class CAacConverter :	public CMediaDecoder,
						protected CAacDecoder::IPcmHandler
{
public:
	CAacConverter(IEventHandler *pEventHandler = NULL);
	virtual ~CAacConverter();

// IMediaDecoder
	virtual void Reset(void);
	virtual const DWORD GetInputNum(void) const;
	virtual const DWORD GetOutputNum(void) const;
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CAacConverter
	const BYTE GetLastChannelNum(void) const;

protected:
	virtual void OnPcmFrame(const CAacDecoder *pAacDecoder, const BYTE *pData, const DWORD dwSamples, const BYTE byChannel);

	CAacDecoder m_AacDecoder;
	CMediaData m_PcmBuffer;

	BYTE m_byLastChannelNum;

private:
	static const DWORD DownMixMono(short *pDst, const short *pSrc, const DWORD dwSamples);
	static const DWORD DownMixStreao(short *pDst, const short *pSrc, const DWORD dwSamples);
	static const DWORD DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples);
};
