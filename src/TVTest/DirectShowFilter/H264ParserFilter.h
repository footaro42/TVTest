#pragma once

#include "../BonTsEngine/MediaData.h"
#include "../BonTsEngine/TsMedia.h"
#include "VideoInfo.h"


#define H264PARSERFILTER_NAME L"H264 Parser Filter"

// {46941C5F-AD0A-47fc-A35A-155ECFCEB4BA}
DEFINE_GUID(CLSID_H264ParserFilter, 0x46941c5f, 0xad0a, 0x47fc, 0xa3, 0x5a, 0x15, 0x5e, 0xcf, 0xce, 0xb4, 0xba);

class CH264ParserFilter : public CTransformFilter,
						  protected CH264Parser::IAccessUnitHandler
{
public:
	DECLARE_IUNKNOWN

	CH264ParserFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CH264ParserFilter(void);
	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr, CH264ParserFilter **ppClassIf);

// CTransInplaceFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);
	HRESULT BeginFlush(void);

// CH264ParserFilter
	typedef void (CALLBACK *VideoInfoCallback)(const CMpeg2VideoInfo *pVideoInfo,const LPVOID pParam);
	void SetVideoInfoCallback(VideoInfoCallback pCallback, const PVOID pParam = NULL);
	bool SetAdjustTime(bool bAdjust);

protected:
// CTransformFilter
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT Receive(IMediaSample *pSample);

// CH264Parser::IAccessUnitHandler
	virtual void OnAccessUnit(const CH264Parser *pParser, const CH264AccessUnit *pAccessUnit);

// CH264ParserFilter
	VideoInfoCallback m_pfnVideoInfoCallback;
	LPVOID m_pCallbackParam;

	CMediaType m_MediaType;
	CH264Parser m_H264Parser;
	CMpeg2VideoInfo m_VideoInfo;
	CCritSec m_ParserLock;
	IMediaSample *m_pOutSample;
	HRESULT m_DeliverResult;
	bool m_bAdjustTime;
	REFERENCE_TIME m_PrevTime;
	REFERENCE_TIME m_BaseTime;
	DWORD m_SampleCount;
};
