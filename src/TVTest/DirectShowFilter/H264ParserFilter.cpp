#include "StdAfx.h"
#include "H264ParserFilter.h"
#include "DirectShowUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// REFERENCE_TIMEの一秒
#define REFERENCE_TIME_SECOND 10000000LL

#define FRAME_RATE (REFERENCE_TIME_SECOND * 15000 / 1001)
#define FRAME_TIME(time) ((LONGLONG)(time) * REFERENCE_TIME_SECOND * 1001 / 15000)

inline LONGLONG llabs(LONGLONG val)
{
	return val<0?-val:val;
}


CH264ParserFilter::CH264ParserFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(H264PARSERFILTER_NAME, pUnk, CLSID_H264ParserFilter)
	, m_pfnVideoInfoCallback(NULL)
	, m_pCallbackParam(NULL)
	, m_H264Parser(this)
	, m_bAdjustTime(false)
{
	TRACE(TEXT("CH264ParserFilter::CH264ParserFilter %p\n"),this);

	m_MediaType.InitMediaType();
	m_MediaType.SetType(&MEDIATYPE_Video);
	m_MediaType.SetSubtype(&MEDIASUBTYPE_H264);
	m_MediaType.SetTemporalCompression(TRUE);
	m_MediaType.SetSampleSize(0);
	m_MediaType.SetFormatType(&FORMAT_VideoInfo);
	VIDEOINFOHEADER *pvih = reinterpret_cast<VIDEOINFOHEADER*>(m_MediaType.AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	if (!pvih) {
		*phr = E_OUTOFMEMORY;
		return;
	}
	::ZeroMemory(pvih, sizeof(VIDEOINFOHEADER));
	pvih->dwBitRate = 128000;
	pvih->AvgTimePerFrame = FRAME_TIME(1);
	pvih->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvih->bmiHeader.biWidth = 320;
	pvih->bmiHeader.biHeight = 240;
	pvih->bmiHeader.biCompression = MAKEFOURCC('h','2','6','4');

	*phr=S_OK;
}

CH264ParserFilter::~CH264ParserFilter(void)
{
	//TRACE(TEXT("CH264ParserFilter::~CH264ParserFilter\n"));
}

IBaseFilter* WINAPI CH264ParserFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr, CH264ParserFilter **ppClassIf)
{
	// インスタンスを作成する
	if(ppClassIf) *ppClassIf = NULL;
	CH264ParserFilter *pNewFilter = new CH264ParserFilter(pUnk, phr);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}

	IBaseFilter *pFilter;
	*phr = pNewFilter->QueryInterface(IID_IBaseFilter, (void**)&pFilter);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}
	if(ppClassIf) *ppClassIf = pNewFilter;
	return pFilter;
}

HRESULT CH264ParserFilter::CheckInputType(const CMediaType* mtIn)
{
	CheckPointer(mtIn, E_POINTER);

	if (*mtIn->Type() == MEDIATYPE_Video)
		return S_OK;

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CH264ParserFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if (*mtOut->Type() == MEDIATYPE_Video
			&& (*mtOut->Subtype() == MEDIASUBTYPE_H264
				|| *mtOut->Subtype() == MEDIASUBTYPE_h264
				|| *mtOut->Subtype() == MEDIASUBTYPE_H264_bis
				|| *mtOut->Subtype() == MEDIASUBTYPE_AVC1
				|| *mtOut->Subtype() == MEDIASUBTYPE_avc1)) {
		return S_OK;
	}

	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CH264ParserFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pAllocator, E_POINTER);
	CheckPointer(pprop, E_POINTER);

	// バッファは1個あればよい
	if(!pprop->cBuffers)pprop->cBuffers = 1L;

	// とりあえず1MB確保
	if(pprop->cbBuffer < 0x100000L)pprop->cbBuffer = 0x100000L;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAllocator->SetProperties(pprop, &Actual);
	if(FAILED(hr))return hr;

	// 要求を受け入れられたか判定
	if(Actual.cbBuffer < pprop->cbBuffer)return E_FAIL;

	return S_OK;
}

HRESULT CH264ParserFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pMediaType, E_POINTER);

	if(iPosition < 0)return E_INVALIDARG;
	if(iPosition > 0)return VFW_S_NO_MORE_ITEMS;

	*pMediaType = m_MediaType;

	return S_OK;
}

HRESULT CH264ParserFilter::StartStreaming(void)
{
	CAutoLock AutoLock(m_pLock);

	m_PrevTime = -1;
	return S_OK;
}

HRESULT CH264ParserFilter::StopStreaming(void)
{
	return S_OK;
}


HRESULT CH264ParserFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CAutoLock AutoLock(m_pLock);

	// 入力データポインタを取得する
	BYTE *pInData = NULL;
	HRESULT hr = pIn->GetPointer(&pInData);
	if (FAILED(hr))
		return hr;
	LONG DataSize = pIn->GetActualDataLength();

	// タイムスタンプ設定
	REFERENCE_TIME StartTime;
	REFERENCE_TIME EndTime;
	if (pIn->GetTime(&StartTime, &EndTime) == S_OK) {
		m_BaseTime = StartTime;
	} else {
		m_BaseTime = -1;
		m_PrevTime = -1;
	}

	// 出力メディアサンプル設定
	if (m_bAdjustTime) {
		m_pOutSample = pOut;
		pOut->SetActualDataLength(0UL);

		m_SampleCount = 0;

		// タイムスタンプ設定
		if (m_BaseTime >= 0) {
			if (m_PrevTime >= 0
					&& llabs(m_PrevTime - StartTime) <= REFERENCE_TIME_SECOND / 5LL)
				m_BaseTime = StartTime = m_PrevTime;
			EndTime = StartTime + FRAME_TIME(1);
			pOut->SetTime(&StartTime, &EndTime);
		}
	} else {
		BYTE *pOutData = NULL;
		HRESULT hr = pOut->GetPointer(&pOutData);
		if (SUCCEEDED(hr)) {
			hr = pOut->SetActualDataLength(DataSize);
			if (SUCCEEDED(hr))
				::CopyMemory(pOutData, pInData, DataSize);
		}
	}

	m_ParserLock.Lock();
	m_H264Parser.StoreEs(pInData, DataSize);
	m_ParserLock.Unlock();

	if (pOut->GetActualDataLength() == 0)
		return S_FALSE;

	return S_OK;
}

/*
HRESULT CH264ParserFilter::Receive(IMediaSample *pSample)
{
	IMediaSample *pOutSample;
	HRESULT hr;

	hr = InitializeOutputSample(pSample, &pOutSample);
	if (FAILED(hr)) {
		return hr;
	}
	hr = Transform(pSample, pOutSample);
	if (hr == NOERROR) {
		hr = m_pOutput->Deliver(pOutSample);
	}
	pOutSample->Release();
	return hr;
}
*/


void CH264ParserFilter::SetVideoInfoCallback(VideoInfoCallback pCallback, const PVOID pParam)
{
	CAutoLock Lock(m_pLock);

	m_pfnVideoInfoCallback = pCallback;
	m_pCallbackParam = pParam;
}


bool CH264ParserFilter::SetAdjustTime(bool bAdjust)
{
	CAutoLock Lock(m_pLock);

	m_bAdjustTime = bAdjust;
	return true;
}


void CH264ParserFilter::OnAccessUnit(const CH264Parser *pParser, const CH264AccessUnit *pAccessUnit)
{
	if (m_bAdjustTime) {
		/*
			1フレーム単位でタイムスタンプを設定しないとかくつく
			ただしこんなやり方が許されるのかは知らない
			(とりあえずうまくいっているようだが...)
		*/
		BYTE *pOutData = NULL;
		HRESULT hr = m_pOutSample->GetPointer(&pOutData);
		if (SUCCEEDED(hr)) {
			hr = m_pOutSample->SetActualDataLength(pAccessUnit->GetSize());
			if (SUCCEEDED(hr)) {
				::CopyMemory(pOutData, pAccessUnit->GetData(), pAccessUnit->GetSize());
				m_pOutput->Deliver(m_pOutSample);
			}
			m_pOutSample->SetActualDataLength(0);

			// タイムスタンプ設定
			if (m_BaseTime >= 0) {
				REFERENCE_TIME StartTime = m_BaseTime + FRAME_TIME(m_SampleCount + 1);
				REFERENCE_TIME EndTime = m_BaseTime + FRAME_TIME(m_SampleCount + 2);
				m_pOutSample->SetTime(&StartTime, &EndTime);
				m_PrevTime = StartTime;
			}
		}
		m_SampleCount++;
	}

	// 全然できていません
	BYTE AspectX,AspectY;
	WORD OrigWidth,OrigHeight;
	WORD DisplayWidth,DisplayHeight;

	OrigWidth=pAccessUnit->GetHorizontalSize();
	OrigHeight=pAccessUnit->GetVerticalSize();

	AspectX = AspectY = 0;
	if (OrigWidth == 320) {
		if (OrigHeight == 180) {
			AspectX = 16;
			AspectY = 9;
		} else if (OrigHeight == 240) {
			AspectX = 4;
			AspectY = 3;
		}
	}

	DisplayWidth = OrigWidth;
	DisplayHeight = OrigHeight;

	CMpeg2VideoInfo Info(OrigWidth, OrigHeight, DisplayWidth, DisplayHeight, AspectX, AspectY);

	if (Info != m_VideoInfo) {
		// 映像のサイズ及びフレームレートが変わった
		m_VideoInfo = Info;
		// 通知
		if (m_pfnVideoInfoCallback)
			m_pfnVideoInfoCallback(&m_VideoInfo, m_pCallbackParam);
	}
}
