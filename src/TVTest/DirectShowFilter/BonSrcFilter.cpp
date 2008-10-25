#include "StdAfx.h"
#include "BonSrcFilter.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CBonSrcFilter::CBonSrcFilter(LPUNKNOWN pUnk, HRESULT *phr)
	: CBaseFilter(TEXT("Bon Source Filter"), pUnk, &m_cStateLock, CLSID_BONSOURCE)
	, m_pSrcPin(NULL)
	, m_bOutputWhenPaused(false)
{
	TRACE(TEXT("CBonSrcFilter::CBonSrcFilter %p\n"),this);

	// ピンのインスタンス生成
	m_pSrcPin = new CBonSrcPin(phr, this);

	//*phr = (m_pSrcPin)? S_OK : E_OUTOFMEMORY;
	*phr=S_OK;
}

CBonSrcFilter::~CBonSrcFilter()
{
	// ピンのインスタンスを削除する
	if(m_pSrcPin)delete m_pSrcPin;
}

IBaseFilter* WINAPI CBonSrcFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr,CBonSrcFilter **ppBonSrcFilterIf)
{
	// インスタンスを作成する
	if(ppBonSrcFilterIf) *ppBonSrcFilterIf = NULL;
	CBonSrcFilter *pNewFilter = new CBonSrcFilter(pUnk, phr);
	/*
	if(!pNewFilter){
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
	if(ppBonSrcFilterIf) *ppBonSrcFilterIf = pNewFilter;
	return pFilter;
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

	CAutoLock Lock(m_pLock);

	return CBaseFilter::Run(tStart);
}


STDMETHODIMP CBonSrcFilter::Pause(void)
{
	TRACE(L"■CBonSrcFilter::Pause()\n");

	CAutoLock Lock(m_pLock);

	return CBaseFilter::Pause();
}

STDMETHODIMP CBonSrcFilter::Stop(void)
{
	TRACE(L"■CBonSrcFilter::Stop()\n");

	CAutoLock Lock(m_pLock);

	return CBaseFilter::Stop();
}


STDMETHODIMP CBonSrcFilter::GetState(DWORD dw, FILTER_STATE *pState)
{
	*pState = m_State;
	if (m_State==State_Paused && !m_bOutputWhenPaused)
		return VFW_S_CANT_CUE;
	return S_OK;
}


const bool CBonSrcFilter::InputMedia(CMediaData *pMediaData)
{
	if (!m_pSrcPin
			|| m_State==State_Stopped
			|| (m_State==State_Paused && !m_bOutputWhenPaused))
		return false;
	return m_pSrcPin->InputMedia(pMediaData);
}


void CBonSrcFilter::Flush()
{
	if (m_pSrcPin)
		m_pSrcPin->Flush();
}


void CBonSrcFilter::SetOutputWhenPaused(bool bOutput)
{
	m_bOutputWhenPaused=bOutput;
	if (m_pSrcPin)
		m_pSrcPin->SetOutputWhenPaused(bOutput);
}


// その場しのぎの関数
bool CBonSrcFilter::CheckHangUp(DWORD TimeOut)
{
	// 邪悪なキャスト
	CRITICAL_SECTION *pCritSec=(CRITICAL_SECTION*)&m_cStateLock;
	DWORD i;

	for (i=TimeOut;i>0;i--) {
		if (::TryEnterCriticalSection(pCritSec))
			break;
		Sleep(1);
	}
	if (i==0) {
		TRACE(TEXT("Filter graph is hang up.\n"));
		return true;
	}
	::LeaveCriticalSection(pCritSec);
	return false;
}
