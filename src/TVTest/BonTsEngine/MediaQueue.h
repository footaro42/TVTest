// MediaQueue.h: CMediaQueue クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "MediaDecoder.h"
#include "TsUtilClass.h"
#include <queue>


using std::queue;


/////////////////////////////////////////////////////////////////////////////
// メディアキュー(CMediaDataをキューイングして非同期に取り出す手段を提供する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CMediaData		入力データ
/////////////////////////////////////////////////////////////////////////////

#define MAXQUEUECOUNT	10UL

class CMediaQueue : public CMediaDecoder  
{
public:
	CMediaQueue(IEventHandler *pEventHandler = NULL);
	virtual ~CMediaQueue();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CMediaQueue
	void EnableQueuing(const DWORD dwMaxQueueCount = MAXQUEUECOUNT);
	void DisableQueuing(void);

	CMediaData * PeekMediaData(void);
	CMediaData * GetMediaData(void);

	void PurgeMediaData(void);

protected:
	queue<CMediaData *> m_MediaQueue;
	DWORD m_dwMaxQueueCount;

	CCriticalLock m_CriticalLock;
	HANDLE m_hMediaEvent;

	bool m_bIsQueuing;
	bool m_bIsPreCharge;
};
