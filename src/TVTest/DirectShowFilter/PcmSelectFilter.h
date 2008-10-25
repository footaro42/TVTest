#pragma once

#include "TransFrm.h"

// テンプレート名
#define PCMSELFILTER_NAME	(L"Pcm Channel Select Filter")

// このフィルタのGUID {529E09D3-BF03-47f8-8D52-DE13239F9E2C}
DEFINE_GUID(CLSID_PCMSELFILTER, 0x529e09d3, 0xbf03, 0x47f8, 0x8d, 0x52, 0xde, 0x13, 0x23, 0x9f, 0x9e, 0x2c);

class CPcmSelectFilter : public CTransInPlaceFilter
{

public:
	DECLARE_IUNKNOWN

	CPcmSelectFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CPcmSelectFilter(void);
	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr, CPcmSelectFilter **ppClassIf);

/*
// CTransformFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);
*/
//CTransInPlaceFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT Receive(IMediaSample *pSample);

// CPcmSelectFilter
	enum { Pcm_Stereo, Pcm_Left, Pcm_Right, Pcm_Mix };
	bool SetStereoMode(int iMode);

protected:
	//HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);
	HRESULT Transform(IMediaSample *pSample);

// CPcmSelectFilter
	int m_iStereoMode;
};
