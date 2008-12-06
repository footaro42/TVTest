#include "StdAfx.h"
#include "BonSrcPin.h"
#include "BonSrcFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CBonSrcPin::CBonSrcPin(HRESULT *phr, CBonSrcFilter *pFilter)
	: CBaseOutputPin(TEXT("CBonSrcPin"), pFilter, pFilter->m_pLock, phr, L"TS")
	, m_pFilter(pFilter)
	, m_bOutputWhenPaused(false)
{
	TRACE(TEXT("CBonSrcPin::CBonSrcPin() %p\n"),this);

	*phr = S_OK;
}

CBonSrcPin::~CBonSrcPin(void)
{
	TRACE(TEXT("CBonSrcPin::~CBonSrcPin()\n"));
}

HRESULT CBonSrcPin::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);
	CheckPointer(pMediaType, E_POINTER);
 
	if(iPosition < 0)return E_INVALIDARG;
	if(iPosition >0)return VFW_S_NO_MORE_ITEMS;
 
	// メディアタイプ設定
	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Stream);
	pMediaType->SetSubtype(&MEDIASUBTYPE_MPEG2_TRANSPORT);
	pMediaType->SetTemporalCompression(FALSE);
	pMediaType->SetSampleSize(188);

	return S_OK;
}

HRESULT CBonSrcPin::CheckMediaType(const CMediaType *pMediaType)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);
	CheckPointer(pMediaType, E_POINTER);

	// メディアタイプをチェックする
	CMediaType m_mt;
	GetMediaType(0, &m_mt);

	return (*pMediaType == m_mt)? S_OK : E_FAIL;
}

HRESULT CBonSrcPin::Active(void)
{
	TRACE(TEXT("CBonSrcPin::Active()\n"));

	return CBaseOutputPin::Active();
}

HRESULT CBonSrcPin::Inactive(void)
{
	TRACE(TEXT("CBonSrcPin::Inactive()\n"));

	DeliverEndOfStream();

	return CBaseOutputPin::Inactive();
}

HRESULT CBonSrcPin::Run(REFERENCE_TIME tStart)
{
	TRACE(TEXT("CBonSrcPin::Run()\n"));

	return CBaseOutputPin::Run(tStart);
}

HRESULT CBonSrcPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pRequest, E_POINTER);

	// バッファは1個あればよい
	if(!pRequest->cBuffers)pRequest->cBuffers = 1;

	// とりあえずTSパケット長確保
	if(pRequest->cbBuffer < 188)pRequest->cbBuffer = 188;

	// アロケータプロパティを設定しなおす
	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pAlloc->SetProperties(pRequest, &Actual);
	if(FAILED(hr))return hr;

	// 要求を受け入れられたか判定
	if(Actual.cbBuffer < pRequest->cbBuffer)return E_FAIL;

	return S_OK;
}


const bool CBonSrcPin::InputMedia(CMediaData *pMediaData)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	if (m_pFilter->m_State==State_Stopped
			&& (m_pFilter->m_State==State_Paused && !m_bOutputWhenPaused))
		return true;

	// 空のメディアサンプルを要求する
	IMediaSample *pSample = NULL;
	HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
	if(FAILED(hr))return false;

	// 書き込み先ポインタを取得する
	BYTE *pSampleData = NULL;
	pSample->GetPointer(&pSampleData);

	// 受け取ったデータをコピーする
	::CopyMemory(pSampleData, pMediaData->GetData(), pMediaData->GetSize());
    pSample->SetActualDataLength(pMediaData->GetSize());

	// サンプルを次のフィルタに渡す
	Deliver(pSample);

	pSample->Release();

	return true;
}

void CBonSrcPin::Flush()
{
	DeliverBeginFlush();
	DeliverEndFlush();
}
