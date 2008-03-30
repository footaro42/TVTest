#pragma once


#include "MediaData.h"
#include "BonSrcPin.h"


// ‚±‚ÌƒtƒBƒ‹ƒ^‚ÌGUID {DCA86296-964A-4e64-857D-8D140E630707}
DEFINE_GUID(CLSID_BONSOURCE, 0xdca86296, 0x964a, 0x4e64, 0x85, 0x7d, 0x8d, 0x14, 0x0e, 0x63, 0x07, 0x07);


class CBonSrcFilter : public CBaseFilter
{
friend CBonSrcPin;

public:
	DECLARE_IUNKNOWN

	CBonSrcFilter(LPUNKNOWN pUnk, HRESULT *phr);
	virtual ~CBonSrcFilter();
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

// CBaseFilter
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause(void);
	STDMETHODIMP Stop(void);
    int GetPinCount(void);
    CBasePin *GetPin(int n);

// CBonSrcFilter
	const bool InputMedia(CMediaData *pMediaData);

protected:
	CBonSrcPin *m_pSrcPin;
	CCritSec m_cStateLock;
};
