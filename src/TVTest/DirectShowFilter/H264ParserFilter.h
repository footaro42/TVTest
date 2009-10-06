#pragma once

#include "MediaData.h"
#include "TsMedia.h"


#define H264PARSERFILTER_NAME TEXT("H264 Parser Filter")

// {46941C5F-AD0A-47fc-A35A-155ECFCEB4BA}
DEFINE_GUID(CLSID_H264ParserFilter, 0x46941c5f, 0xad0a, 0x47fc, 0xa3, 0x5a, 0x15, 0x5e, 0xcf, 0xce, 0xb4, 0xba);

class CMpeg2VideoInfo
{
public:
	WORD m_OrigWidth,m_OrigHeight;
	WORD m_DisplayWidth,m_DisplayHeight;
	WORD m_PosX,m_PosY;
	BYTE m_AspectRatioX,m_AspectRatioY;
	CMpeg2VideoInfo()
	{
		m_OrigWidth=0;
		m_OrigHeight=0;
		m_DisplayWidth=0;
		m_DisplayHeight=0;
		m_PosX=0;
		m_PosY=0;
		m_AspectRatioX=0;
		m_AspectRatioY=0;
	}
	CMpeg2VideoInfo(WORD OrigWidth,WORD OrigHeight,WORD DisplayWidth,WORD DisplayHeight,BYTE AspectX,BYTE AspectY)
	{
		m_OrigWidth=OrigWidth;
		m_OrigHeight=OrigHeight;
		m_DisplayWidth=DisplayWidth;
		m_DisplayHeight=DisplayHeight;
		m_PosX=(OrigWidth-DisplayWidth)/2;
		m_PosY=(OrigHeight-DisplayHeight)/2;
		m_AspectRatioX=AspectX;
		m_AspectRatioY=AspectY;
	}
	bool operator==(const CMpeg2VideoInfo &Info) const
	{
		return m_OrigWidth==Info.m_OrigWidth
			&& m_OrigHeight==Info.m_OrigHeight
			&& m_DisplayWidth==Info.m_DisplayWidth
			&& m_DisplayHeight==Info.m_DisplayHeight
			&& m_AspectRatioX==Info.m_AspectRatioX
			&& m_AspectRatioY==Info.m_AspectRatioY;
	}
	bool operator!=(const CMpeg2VideoInfo &Info) const
	{
		return !(*this==Info);
	}
};

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
	//HRESULT Receive(IMediaSample *pSample);

// CH264ParserFilter
	typedef void (CALLBACK *VideoInfoCallback)(const CMpeg2VideoInfo *pVideoInfo,const LPVOID pParam);
	void SetVideoInfoCallback(VideoInfoCallback pCallback, const PVOID pParam = NULL);
	bool SetAdjustTime(bool bAdjust);

protected:
// CTransformFilter
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);

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
	bool m_bAdjustTime;
	REFERENCE_TIME m_PrevTime;
	REFERENCE_TIME m_BaseTime;
	DWORD m_SampleCount;
};
