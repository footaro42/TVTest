#pragma once

#include "TransFrm.h"
#include "MediaData.h"
#include "TsMedia.h"
#include "AacDecoder.h"


// テンプレート名
#define AACDECFILTER_NAME	(L"AAC Decoder Filter")


// このフィルタのGUID {8D1E3E25-D92B-4849-8D38-C787DA78352C}
DEFINE_GUID(CLSID_AACDECFILTER, 0x8d1e3e25, 0xd92b, 0x4849, 0x8d, 0x38, 0xc7, 0x87, 0xda, 0x78, 0x35, 0x2c);


// フィルタ情報の外部参照宣言
extern const AMOVIESETUP_FILTER g_AacDecFilterInfo;


class CAacDecFilter :	public CTransformFilter,
						protected CAacDecoder::IPcmHandler
{

public:
	DECLARE_IUNKNOWN

	CAacDecFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CAacDecFilter(void);
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CTransformFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);

protected:
	HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);

// CAacDecoder::IPcmHandler
	virtual void OnPcmFrame(const CAacDecoder *pAacDecoder, const BYTE *pData, const DWORD dwSamples, const BYTE byChannel);
	
// CAacDecFilter
	CAdtsParser m_AdtsParser;
	CAacDecoder m_AacDecoder;
	IMediaSample *m_pOutSample;
	
private:
	static const DWORD DownMixMono(short *pDst, const short *pSrc, const DWORD dwSamples);
	static const DWORD DownMixStreao(short *pDst, const short *pSrc, const DWORD dwSamples);
	static const DWORD DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples);
};
