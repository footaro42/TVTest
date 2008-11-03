#include "StdAfx.h"
#include "AacDecFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// 5.1chダウンミックス設定
#define DMR_CENTER			0.5		// 50%
#define DMR_FRONT			1.0		// 100%
#define DMR_REAR			0.7		// 70%
#define DMR_LFE				0.7		// 70%

/*
	整数演算によるダウンミックス
	テストでは処理時間が1/8ぐらいになった(ただしSSE等によって変わるはず)
*/
#define DOWNMIX_INT

#ifdef DOWNMIX_INT
#define IDOWNMIX_DENOM	4096
#define IDOWNMIX_CENTER	((int)(DMR_CENTER*IDOWNMIX_DENOM+0.5))
#define IDOWNMIX_FRONT	((int)(DMR_FRONT*IDOWNMIX_DENOM+0.5))
#define IDOWNMIX_REAR	((int)(DMR_REAR*IDOWNMIX_DENOM+0.5))
#define IDOWNMIX_LFE	((int)(DMR_LFE*IDOWNMIX_DENOM+0.5))
#endif




CAacDecFilter::CAacDecFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CTransformFilter(AACDECFILTER_NAME, pUnk, CLSID_AACDECFILTER)
	, m_AdtsParser(&m_AacDecoder)
	, m_AacDecoder(this)
	, m_pOutSample(NULL)
	, m_byCurChannelNum(0)
	, m_bNormalize(false)
{
	TRACE(TEXT("CAacDecFilter::CAacDecFilter %p\n"),this);

	*phr=S_OK;
}

CAacDecFilter::~CAacDecFilter(void)
{
	TRACE(TEXT("CAacDecFilter::~CAacDecFilter\n"));
}

IBaseFilter* WINAPI CAacDecFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr, CAacDecFilter **ppClassIf)
{
	// インスタンスを作成する
	if(ppClassIf) *ppClassIf = NULL;
	CAacDecFilter *pNewFilter = new CAacDecFilter(pUnk, phr);
	/*
	if (!pNewFilter) {
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

HRESULT CAacDecFilter::CheckInputType(const CMediaType* mtIn)
{
    CheckPointer(mtIn, E_POINTER);

	// 何でもOK

	return S_OK;
}

HRESULT CAacDecFilter::CheckTransform(const CMediaType* mtIn, const CMediaType* mtOut)
{
    CheckPointer(mtIn, E_POINTER);
    CheckPointer(mtOut, E_POINTER);
    
	if(*mtOut->Type() == MEDIATYPE_Audio){
		if(*mtOut->Subtype() == MEDIASUBTYPE_PCM){

			// GUID_NULLではデバッグアサートが発生するのでダミーを設定して回避			
			CMediaType MediaType;
			MediaType.InitMediaType();
			MediaType.SetType(&MEDIATYPE_Stream);
			MediaType.SetSubtype(&MEDIASUBTYPE_None);
			
			m_pInput->SetMediaType(&MediaType);
			
			return S_OK;
			}
		}
    
	return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT CAacDecFilter::DecideBufferSize(IMemAllocator * pAllocator, ALLOCATOR_PROPERTIES *pprop)
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

HRESULT CAacDecFilter::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	CAutoLock AutoLock(m_pLock);
	CheckPointer(pMediaType, E_POINTER);

	if(iPosition < 0)return E_INVALIDARG;
	if(iPosition > 0)return VFW_S_NO_MORE_ITEMS;

	// メディアタイプ設定
	pMediaType->InitMediaType();
	pMediaType->SetType(&MEDIATYPE_Audio);
	pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
	pMediaType->SetTemporalCompression(FALSE);
	pMediaType->SetSampleSize(0);
	pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
		
	 // フォーマット構造体確保
	WAVEFORMATEX *pWaveInfo = reinterpret_cast<WAVEFORMATEX *>(pMediaType->AllocFormatBuffer(sizeof(WAVEFORMATEX)));
	if(!pWaveInfo)return E_OUTOFMEMORY;
	//::ZeroMemory(pWaveInfo, sizeof(WAVEFORMATEX));

	// WAVEFORMATEX構造体設定(48KHz 16bit ステレオ固定)
	pWaveInfo->wFormatTag = WAVE_FORMAT_PCM;
	pWaveInfo->nChannels = 2U;
	pWaveInfo->nSamplesPerSec = 48000UL;
	pWaveInfo->wBitsPerSample = 16U;
	pWaveInfo->nBlockAlign = pWaveInfo->wBitsPerSample * pWaveInfo->nChannels / 8U;
	pWaveInfo->nAvgBytesPerSec = pWaveInfo->nSamplesPerSec * pWaveInfo->nBlockAlign;

	return S_OK;
}

HRESULT CAacDecFilter::StartStreaming(void)
{
	CAutoLock AutoLock(m_pLock);

	// ADTSパーサ初期化
	m_AdtsParser.Reset();

	// AACデコーダオープン
	if(!m_AacDecoder.OpenDecoder())return E_FAIL;

	return S_OK;
}

HRESULT CAacDecFilter::StopStreaming(void)
{
	CAutoLock AutoLock(m_pLock);

	// AACデコーダクローズ
	m_AacDecoder.CloseDecoder();

	return S_OK;
}

const BYTE CAacDecFilter::GetCurrentChannelNum()
{
	return m_byCurChannelNum;
}

HRESULT CAacDecFilter::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	CAutoLock AutoLock(m_pLock);

	// 入力データポインタを取得する
	BYTE *pInData = NULL;
	pIn->GetPointer(&pInData);

	// 出力メディアサンプル設定
	m_pOutSample = pOut;
	pOut->SetActualDataLength(0UL);

	// ADTSパーサに入力
	m_AdtsParser.StoreEs(pInData, pIn->GetActualDataLength());

	if(!pOut->GetActualDataLength())
		//return S_FALSE;
		return S_OK;

	// タイムスタンプ設定
	REFERENCE_TIME StartTime = 0LL;
	REFERENCE_TIME EndTime = 0LL;

	// タイムスタンプ設定
	if (pIn->GetTime(&StartTime, &EndTime) == S_OK) {
		pOut->SetTime(&StartTime, &EndTime);
	}

	// ストリームタイム設定
	if (pIn->GetMediaTime(&StartTime, &EndTime) == S_OK) {
		pOut->SetMediaTime(&StartTime, &EndTime);
	}

	return S_OK;
}

/*
HRESULT CAacDecFilter::Receive(IMediaSample *pSample)
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

void CAacDecFilter::OnPcmFrame(const CAacDecoder *pAacDecoder, const BYTE *pData, const DWORD dwSamples, const BYTE byChannel)
{
	CAutoLock AutoLock(m_pLock);

	// 出力ポインタ取得
	const DWORD dwOffset = m_pOutSample->GetActualDataLength();
	DWORD dwOutSize = 0UL;

	BYTE *pOutBuff = NULL;
	m_pOutSample->GetPointer(&pOutBuff);
	pOutBuff = &pOutBuff[dwOffset];
	m_byCurChannelNum = byChannel;

	// ダウンミックス
	switch(byChannel){
		case 1U:	dwOutSize = DownMixMono((short *)pOutBuff, (const short *)pData, dwSamples);		break;
		case 2U:	dwOutSize = DownMixStreao((short *)pOutBuff, (const short *)pData, dwSamples);		break;
		case 6U:	dwOutSize = DownMixSurround((short *)pOutBuff, (const short *)pData, dwSamples);	break;
		}

	if (m_bNormalize)
		Normalize((short*)pOutBuff,dwOutSize/sizeof(short));

	// メディアサンプル有効サイズ設定
    m_pOutSample->SetActualDataLength(dwOffset + dwOutSize);
}

const DWORD CAacDecFilter::DownMixMono(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	// 1ch → 2ch 二重化
	for(register DWORD dwPos = 0UL ; dwPos < dwSamples ; dwPos++){
		pDst[dwPos * 2UL + 0UL] = pSrc[dwPos];	// L
		pDst[dwPos * 2UL + 1UL] = pSrc[dwPos];	// R
		}

	// バッファサイズを返す
	return dwSamples * 4UL;
}

const DWORD CAacDecFilter::DownMixStreao(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	// 2ch → 2ch スルー
	::CopyMemory(pDst, pSrc, dwSamples * 4UL);

	// バッファサイズを返す
	return dwSamples * 4UL;
}

#ifndef DOWNMIX_INT

/*
	浮動小数点数演算版(オリジナル)
*/
const DWORD CAacDecFilter::DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	// 5.1ch → 2ch ダウンミックス

	int iOutLch, iOutRch;

	for(register DWORD dwPos = 0UL ; dwPos < dwSamples ; dwPos++){
		// ダウンミックス
		iOutLch = (int)(
					(double)pSrc[dwPos * 6UL + 1UL]	* DMR_FRONT		+
					(double)pSrc[dwPos * 6UL + 3UL]	* DMR_REAR		+
					(double)pSrc[dwPos * 6UL + 0UL]	* DMR_CENTER	+
					(double)pSrc[dwPos * 6UL + 5UL]	* DMR_LFE
					);

		iOutRch = (int)(
					(double)pSrc[dwPos * 6UL + 2UL]	* DMR_FRONT		+
					(double)pSrc[dwPos * 6UL + 4UL]	* DMR_REAR		+
					(double)pSrc[dwPos * 6UL + 0UL]	* DMR_CENTER	+
					(double)pSrc[dwPos * 6UL + 5UL]	* DMR_LFE
					);

		// クリップ
		if(iOutLch > 32767L)iOutLch = 32767L;
		else if(iOutLch < -32768L)iOutLch = -32768L;

		if(iOutRch > 32767L)iOutRch = 32767L;
		else if(iOutRch < -32768L)iOutRch = -32768L;

		pDst[dwPos * 2UL + 0UL] = (short)iOutLch;	// L
		pDst[dwPos * 2UL + 1UL] = (short)iOutRch;	// R
		}

	// バッファサイズを返す
	return dwSamples * 4UL;
}

#else

/*
	整数演算版
*/
const DWORD CAacDecFilter::DownMixSurround(short *pDst, const short *pSrc, const DWORD dwSamples)
{
	// 5.1ch → 2ch ダウンミックス

	int iOutLch, iOutRch;

	for (register DWORD dwPos = 0UL ; dwPos < dwSamples ; dwPos++) {
		// ダウンミックス
		iOutLch =	((int)pSrc[dwPos * 6UL + 1UL] * IDOWNMIX_FRONT	+
					 (int)pSrc[dwPos * 6UL + 3UL] * IDOWNMIX_REAR	+
					 (int)pSrc[dwPos * 6UL + 0UL] * IDOWNMIX_CENTER	+
					 (int)pSrc[dwPos * 6UL + 5UL] * IDOWNMIX_LFE)/
					IDOWNMIX_DENOM;

		iOutRch =	((int)pSrc[dwPos * 6UL + 2UL] * IDOWNMIX_FRONT	+
					 (int)pSrc[dwPos * 6UL + 4UL] * IDOWNMIX_REAR	+
					 (int)pSrc[dwPos * 6UL + 0UL] * IDOWNMIX_CENTER	+
					 (int)pSrc[dwPos * 6UL + 5UL] * IDOWNMIX_LFE)/
					IDOWNMIX_DENOM;

		// クリップ
		if(iOutLch > 32767)iOutLch = 32767;
		else if(iOutLch < -32768L)iOutLch = -32768;

		if(iOutRch > 32767)iOutRch = 32767;
		else if(iOutRch < -32768)iOutRch = -32768;

		pDst[dwPos * 2UL + 0UL] = (short)iOutLch;	// L
		pDst[dwPos * 2UL + 1UL] = (short)iOutRch;	// R
	}

	// バッファサイズを返す
	return dwSamples * 4UL;
}

#endif




bool CAacDecFilter::SetNormalize(bool bNormalize,float Level)
{
	m_bNormalize=bNormalize;
	m_NormalizeLevel=Level;
	return true;
}


#define NORMALIZE_SHIFT_BITS	12
#define NORMALIZE_DENOM			(1<<NORMALIZE_SHIFT_BITS)


/*
	本当はダウンミックスと同時に行った方が良い
*/
void CAacDecFilter::Normalize(short *pBuffer,DWORD Samples)
{
	int Level=(int)(m_NormalizeLevel*(float)NORMALIZE_DENOM);
	short *p,*pEnd;
	int Value;

	p=pBuffer;
	pEnd=p+Samples;
	while (p<pEnd) {
		Value=(*p*Level)/NORMALIZE_DENOM;
		*p++=Value>32767?32767:Value<-32768?-32768:Value;
	}
}
