#include "StdAfx.h"
#include "BonSrcFilter.h"


CBonSrcFilter::CBonSrcFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(TEXT("Bon Source Filter"), pUnk, &m_cStateLock, CLSID_BONSOURCE)
	, m_pSrcPin(NULL)
{
	// ピンのインスタンス生成
	m_pSrcPin = new CBonSrcPin(phr, this);
		
	*phr = (m_pSrcPin)? S_OK : E_OUTOFMEMORY;
}

CBonSrcFilter::~CBonSrcFilter()
{
	// ピンのインスタンスを削除する
	if(m_pSrcPin)delete m_pSrcPin;
}

CUnknown * WINAPI CBonSrcFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
	// インスタンスを作成する
	CBonSrcFilter *pNewFilter = new CBonSrcFilter(pUnk, phr);
	if(!pNewFilter)*phr = E_OUTOFMEMORY;
	
	return dynamic_cast<CUnknown *>(pNewFilter);
}

int CBonSrcFilter::GetPinCount(void)
{
	// ピン数を返す
	return 1;
}

CBasePin * CBonSrcFilter::GetPin(int n)
{
	// ピンのインスタンスを返す
	return (n == 0)? m_pSrcPin : NULL;
}

STDMETHODIMP CBonSrcFilter::Run(REFERENCE_TIME tStart)
{
	TRACE(L"■CBonSrcFilter::Run()\n");

	return CBaseFilter::Run(tStart);
}

STDMETHODIMP CBonSrcFilter::Pause(void)
{
	TRACE(L"■CBonSrcFilter::Pause()\n");

	return CBaseFilter::Pause();
}

STDMETHODIMP CBonSrcFilter::Stop(void)
{
	TRACE(L"■CBonSrcFilter::Stop()\n");

	return CBaseFilter::Stop();
}

const bool CBonSrcFilter::InputMedia(CMediaData *pMediaData)
{
	if(!m_pSrcPin)return false;
	else return m_pSrcPin->InputMedia(pMediaData);
}
