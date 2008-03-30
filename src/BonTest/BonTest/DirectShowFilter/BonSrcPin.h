#pragma once


#include "MediaData.h"
#include "Source.h"


class CBonSrcFilter;

class CBonSrcPin : public CBaseOutputPin
{
public:
	CBonSrcPin(HRESULT *phr, CBonSrcFilter *pFilter);
	virtual ~CBonSrcPin();

// CBasePin
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT CheckMediaType(const CMediaType *pMediaType);

	HRESULT Active(void);
	HRESULT Inactive(void);
	HRESULT Run(REFERENCE_TIME tStart);

// CBaseOutputPin
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);

// CBonSrcPin
	const bool InputMedia(CMediaData *pMediaData);

protected:
	CBonSrcFilter* m_pFilter;
};
