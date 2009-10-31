#pragma once

#include "MediaData.h"
#include "TsMedia.h"
#include "AacDecoder.h"


// テンプレート名
#define AACDECFILTER_NAME	(L"AAC Decoder Filter")


// このフィルタのGUID {8D1E3E25-D92B-4849-8D38-C787DA78352C}
DEFINE_GUID(CLSID_AACDECFILTER, 0x8d1e3e25, 0xd92b, 0x4849, 0x8d, 0x38, 0xc7, 0x87, 0xda, 0x78, 0x35, 0x2c);

// フィルタ情報の外部参照宣言
//extern const AMOVIESETUP_FILTER g_AacDecFilterInfo;

class CAacDecFilter :	public CTransformFilter,
						protected CAacDecoder::IPcmHandler
{
public:
	DECLARE_IUNKNOWN

	CAacDecFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CAacDecFilter(void);
	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr, CAacDecFilter **ppClassIf);

// CTransformFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);
	HRESULT BeginFlush(void);
	HRESULT Receive(IMediaSample *pSample);

// CAacDecFilter
	const BYTE GetCurrentChannelNum();

	// Append by HDUSTestの中の人
	bool ResetDecoder();
	enum { STEREOMODE_STEREO, STEREOMODE_LEFT, STEREOMODE_RIGHT };
	bool SetStereoMode(int StereoMode);
	int GetStereoMode() const { return m_StereoMode; }
	bool SetDownMixSurround(bool bDownMix);
	bool GetDownMixSurround() const { return m_bDownMixSurround; }
	bool SetNormalize(bool bNormalize, float Level=1.0f);
	bool GetNormalize(float *pLevel) const;

	bool SetAdjustStreamTime(bool bAdjust);

	typedef void (CALLBACK *StreamCallback)(short *pData, DWORD Samples, int Channels, void *pParam);
	bool SetStreamCallback(StreamCallback pCallback, void *pParam = NULL);

protected:
	CCritSec m_cStateLock;
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);

// CAacDecoder::IPcmHandler
	virtual void OnPcmFrame(const CAacDecoder *pAacDecoder, const BYTE *pData, const DWORD dwSamples, const BYTE byChannel);

// CAacDecFilter
	CAdtsParser m_AdtsParser;
	CAacDecoder m_AacDecoder;
	CMediaType m_MediaType;
	IMediaSample *m_pOutSample;
	BYTE m_byCurChannelNum;

private:
	const DWORD DownMixMono(short *pDst, const short *pSrc, const DWORD dwSamples);
	const DWORD DownMixStereo(short *pDst, const short *pSrc, const DWORD dwSamples);
	const DWORD DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples);
	const DWORD MapSurroundChannels(short *pDst, const short *pSrc, const DWORD dwSamples);

	// Append by HDUSTestの中の人
	int m_StereoMode;
	bool m_bDownMixSurround;
	bool m_bNormalize;
	float m_NormalizeLevel;
	void Normalize(short *pBuffer, DWORD Samples);

	StreamCallback m_pStreamCallback;
	void *m_pStreamCallbackParam;

	bool m_bAdjustStreamTime;
	REFERENCE_TIME m_StartTime;
	LONGLONG m_SampleCount;
};
