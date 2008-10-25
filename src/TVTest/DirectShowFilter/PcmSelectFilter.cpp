#include "StdAfx.h"
#include "PcmSelectFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CPcmSelectFilter::CPcmSelectFilter(LPUNKNOWN pUnk, HRESULT *phr)
	//: CTransformFilter(PCMSELFILTER_NAME, pUnk, CLSID_PCMSELFILTER)
	: CTransInPlaceFilter(PCMSELFILTER_NAME, pUnk, CLSID_PCMSELFILTER, phr, TRUE)
	, m_iStereoMode(Pcm_Stereo)
{
	TRACE(TEXT("CPcmSelectFilter::CPcmSelectFilter %p\n"),this);

	*phr=S_OK;
}


CPcmSelectFilter::~CPcmSelectFilter(void)
{
	TRACE(TEXT("CPcmSelectFilter::~CPcmSelectFilter\n"));
}


IBaseFilter* WINAPI CPcmSelectFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr, CPcmSelectFilter **ppClassIf)
{
	// インスタンスを作成する
	if(ppClassIf) *ppClassIf = NULL;
	CPcmSelectFilter *pNewFilter = new CPcmSelectFilter(pUnk, phr);
	/*
	if(!pNewFilter) {
		*phr = E_OUTOFMEMORY;
		return NULL;
	}
	*/

	IBaseFilter *pFilter;
	*phr=pNewFilter->QueryInterface(IID_IBaseFilter,(void**)&pFilter);
	if (FAILED(*phr)) {
		delete pNewFilter;
		return NULL;
	}
	if(ppClassIf) *ppClassIf = pNewFilter;
	return pFilter;
}


HRESULT CPcmSelectFilter::CheckInputType(const CMediaType* mtIn)
{
    CheckPointer(mtIn, E_POINTER);

	if(*mtIn->Type() == MEDIATYPE_Audio){
		if(*mtIn->Subtype() == MEDIASUBTYPE_PCM){

				return S_OK;
			}
		}
	return VFW_E_TYPE_NOT_ACCEPTED;
}


/*
HRESULT CPcmSelectFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
	CheckPointer(mtIn, E_POINTER);
	CheckPointer(mtOut, E_POINTER);

	if(*mtOut->Type() == MEDIATYPE_Audio){
		if(*mtOut->Subtype() == MEDIASUBTYPE_PCM){
			return S_OK;
			}
		}

	return VFW_E_TYPE_NOT_ACCEPTED;
}
*/


HRESULT CPcmSelectFilter::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
	return S_OK;
}


/*
HRESULT CPcmSelectFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
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
*/


HRESULT CPcmSelectFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pMediaType, E_POINTER);

	if(iPosition < 0)return E_INVALIDARG;
	if(iPosition > 0)return VFW_S_NO_MORE_ITEMS;
	*pMediaType=this->m_pInput->CurrentMediaType();
	return S_OK;
}


/*
HRESULT CPcmSelectFilter::StartStreaming(void)
{
	return S_OK;
}


HRESULT CPcmSelectFilter::StopStreaming(void)
{
	return S_OK;
}


HRESULT CPcmSelectFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	// 入力データポインタを取得する
	BYTE *pInData = NULL;
	pIn->GetPointer(&pInData);
	long lDataSize = pIn->GetActualDataLength();

	// 出力ポインタ取得
	BYTE *pOutBuff = NULL;
	pOut->GetPointer(&pOutBuff);

	// データを複製
	short *p=(short*)pInData,*q=(short*)pOutBuff;
	short *pEnd=p+lDataSize/2;	// 16bit 2ch入力
	switch (m_iStereoMode) {
	case Pcm_Left:
		// 左のみ
		while (p<pEnd) {
			short Value=p[0];
			*q++=Value;	// L
			*q++=Value;	// R
			p+=2;
		}
		break;
	case Pcm_Right:
		// 右のみ
		while (p<pEnd) {
			short Value=p[1];
			*q++=Value;	// L
			*q++=Value;	// R
			p+=2;
		}
		break;
	case Pcm_Mix:
		// 左右ミックス
		while (p<pEnd) {
			long Value;

			Value=(p[0]+p[1])/2;
			if (Value<-32768) Value=-32768;
			else if (Value>32767) Value=32767;
			*q++=(short)Value;	// L
			*q++=(short)Value;	// R
			p+=2;
		}
		break;
	default:
		// そのまま出力
		::CopyMemory(pOutBuff,pInData,lDataSize);
		break;
	}

	pOut->SetActualDataLength(lDataSize);

	return S_OK;
}
*/


HRESULT CPcmSelectFilter::Transform(IMediaSample *pSample)
{
	BYTE *pData = NULL;
	pSample->GetPointer(&pData);
	long lDataSize = pSample->GetActualDataLength();

	// データを複製
	short *p=reinterpret_cast<short*>(pData);
	short *pEnd=p+lDataSize/2;	// 16bit 2ch入力
	switch (m_iStereoMode) {
	case Pcm_Left:
		// 左のみ
		while (p<pEnd) {
			p[1]=p[0];
			p+=2;
		}
		break;
	case Pcm_Right:
		// 右のみ
		while (p<pEnd) {
			p[0]=p[1];
			p+=2;
		}
		break;
	case Pcm_Mix:
		// 左右ミックス
		while (p<pEnd) {
			long Value;

			Value=(p[0]+p[1])/2;
			if (Value<-32768) Value=-32768;
			else if (Value>32767) Value=32767;
			*p++=(short)Value;	// L
			*p++=(short)Value;	// R
		}
		break;
	}

	return S_OK;
}


HRESULT CPcmSelectFilter::Receive(IMediaSample *pSample)
{
	Transform(pSample);
	return m_pOutput->Deliver(pSample);
}


bool CPcmSelectFilter::SetStereoMode(int iMode)
{
	switch (iMode) {
	case Pcm_Stereo:
	case Pcm_Left:
	case Pcm_Right:
	case Pcm_Mix:
		m_iStereoMode = iMode;
		return true;
	}
	return false;
}
