#include "StdAfx.h"
#include "BonSrcPin.h"
#include "BonSrcFilter.h"


CBonSrcPin::CBonSrcPin(HRESULT *phr, CBonSrcFilter *pFilter)
	: CBaseOutputPin(TEXT("CBonSrcPin"), pFilter, pFilter->m_pLock, phr, L"TS")
	, m_pFilter(pFilter)
{
	*phr = S_OK;
}

CBonSrcPin::~CBonSrcPin(void)
{
	
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
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	return CBaseOutputPin::Active();
}

HRESULT CBonSrcPin::Inactive(void)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);

	DeliverEndOfStream();

	return CBaseOutputPin::Inactive();
}

HRESULT CBonSrcPin::Run(REFERENCE_TIME tStart)
{
	return CBaseOutputPin::Run(tStart);
}

HRESULT CBonSrcPin::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest)
{
	CAutoLock AutoLock(&m_pFilter->m_cStateLock);
	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pRequest, E_POINTER);

	// バッファは1個あればよい
	if(!pRequest->cBuffers)pRequest->cBuffers = 1024;

	// とりあえずTSパケット長確保
    if(pRequest->cbBuffer < 256)pRequest->cbBuffer = 256;

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

	if(IsStopped())return true;

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
	pSample->SetSyncPoint(FALSE);
	
	// サンプルを次のフィルタに渡す
	Deliver(pSample);

	pSample->Release();

	return true;
}
