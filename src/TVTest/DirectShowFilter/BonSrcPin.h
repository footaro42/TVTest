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

	void Reset();
	void Flush();
	void SetOutputWhenPaused(bool bOutput) { m_bOutputWhenPaused=bOutput; }
protected:
	void EndStreamThread();
	static DWORD WINAPI StreamThread(LPVOID lpParameter);

	CBonSrcFilter* m_pFilter;

	HANDLE m_hThread;
	volatile bool m_bKillSignal;
	BYTE *m_pBuffer;
	DWORD m_BufferLength;
	DWORD m_BufferUsed;
	DWORD m_BufferPos;
	CCritSec m_StreamLock;

	bool m_bOutputWhenPaused;
};
