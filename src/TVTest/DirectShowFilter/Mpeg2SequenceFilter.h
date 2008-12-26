#pragma once

#include "TransFrm.h"
#include "TsMedia.h"


// テンプレート名
#define MPEG2SEQUENCEFILTER_NAME	(L"Mpeg2 Sequence Filter")

// このフィルタのGUID {3F8400DA-65F1-4694-BB05-303CDE739680}
DEFINE_GUID(CLSID_MPEG2SEQFILTER, 0x3f8400da, 0x65f1, 0x4694, 0xbb, 0x5, 0x30, 0x3c, 0xde, 0x73, 0x96, 0x80);

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

class CMpeg2SequenceFilter : public CTransInPlaceFilter,
							 protected CMpeg2Parser::ISequenceHandler
{

public:
	DECLARE_IUNKNOWN

	CMpeg2SequenceFilter(LPUNKNOWN pUnk, HRESULT *phr);
	~CMpeg2SequenceFilter(void);
	static IBaseFilter* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr,CMpeg2SequenceFilter **ppClassIf);

// CTransformFilter
	/*
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut);
	HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
	HRESULT DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT StartStreaming(void);
	HRESULT StopStreaming(void);
	*/
// CTransInPlaceFilter
	HRESULT CheckInputType(const CMediaType* mtIn);
	HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);

// CMpeg2SequenceFilter
	typedef void (CALLBACK MPEG2SEQUENCE_VIDEOINFO_FUNC)(const CMpeg2VideoInfo *pVideoInfo,const LPVOID pParam);
	void SetRecvCallback(MPEG2SEQUENCE_VIDEOINFO_FUNC pCallback, const PVOID pParam = NULL);
	HRESULT Receive(IMediaSample *pSample);

	const bool GetVideoSize(WORD *pX, WORD *pY) const;
	const bool GetAspectRatio(BYTE *pX,BYTE *pY) const;

	// Append by HDUSTestの中の人
	const bool GetOriginalVideoSize(WORD *pWidth,WORD *pHeight) const;
	const bool GetVideoInfo(CMpeg2VideoInfo *pInfo) const;
protected:
	//HRESULT Transform(IMediaSample * pIn, IMediaSample *pOut);
	HRESULT Transform(IMediaSample *pSample);

// CMpeg2Parser::ISequenceHandler
	virtual void OnMpeg2Sequence(const CMpeg2Parser *pMpeg2Parser, const CMpeg2Sequence *pSequence);

// CMpeg2SequenceFilter
	MPEG2SEQUENCE_VIDEOINFO_FUNC *m_pfnVideoInfoRecvFunc;
	LPVOID m_pCallbackParam;

	CMpeg2Parser m_Mpeg2Parser;

	CMpeg2VideoInfo m_VideoInfo;

	CCritSec m_ParserLock;
};
