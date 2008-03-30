// MediaQueue.cpp: CMediaQueue クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaQueue.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CMediaQueue::CMediaQueue(IEventHandler *pEventHandler)
	: CMediaDecoder(pEventHandler)
	, m_dwMaxQueueCount(MAXQUEUECOUNT)
	, m_bIsQueuing(false)
{
	// イベント作成
	m_hMediaEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
}

CMediaQueue::~CMediaQueue()
{
	// イベント削除
	if(m_hMediaEvent)::CloseHandle(m_hMediaEvent);
}

void CMediaQueue::Reset(void)
{
	// キューをパージする
	PurgeMediaData();

	CMediaDecoder::Reset();
}

const DWORD CMediaQueue::GetInputNum(void) const
{
	return 1UL;
}

const DWORD CMediaQueue::GetOutputNum(void) const
{
	return 0UL;
}

const bool CMediaQueue::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if(dwInputIndex > GetInputNum())return false;
	
	CBlockLock Lock(&m_CriticalLock);

	if(m_bIsQueuing){
		// 最大数を超えた、キューを空にする
		while(m_MediaQueue.size() >= m_dwMaxQueueCount){
			m_MediaQueue.front()->Delete();
			m_MediaQueue.pop();
			}

		// メディアデータをキューにプッシュする
		m_MediaQueue.push(new CMediaData(*pMediaData));

		// イベントをセットする
		::SetEvent(m_hMediaEvent);
		}

	return true;
}

void CMediaQueue::EnableQueuing(const DWORD dwMaxQueueCount)
{
	CBlockLock Lock(&m_CriticalLock);

	// 未処理のデータをパージする
	while(!m_MediaQueue.empty()){
		m_MediaQueue.front()->Delete();
		m_MediaQueue.pop();
		}
		
	// 最大数を保存
	m_dwMaxQueueCount = (dwMaxQueueCount)? dwMaxQueueCount : MAXQUEUECOUNT;

	// キューイングを有効にする
	m_bIsQueuing = true;

	m_bIsPreCharge = true;

	::ResetEvent(m_hMediaEvent);
}

void CMediaQueue::DisableQueuing(void)
{
	// イベントをセットする
	::SetEvent(m_hMediaEvent);	
	
	CBlockLock Lock(&m_CriticalLock);

	// キューイングを無効にする
	m_bIsQueuing = false;

	// 未処理のデータをパージする
	while(!m_MediaQueue.empty()){
		m_MediaQueue.front()->Delete();
		m_MediaQueue.pop();
		}
}

CMediaData * CMediaQueue::PeekMediaData(void)
{
	CMediaData *pMediaData = NULL;

	CBlockLock Lock(&m_CriticalLock);

	// データがあればキューから取り出す
	if(!m_MediaQueue.empty()){
		pMediaData = m_MediaQueue.front();
		m_MediaQueue.pop();
		
		// キューが空の場合はイベントをクリアする
		if(m_MediaQueue.empty()){
			::ResetEvent(m_hMediaEvent);
			}		
		}

	return pMediaData;
}

CMediaData * CMediaQueue::GetMediaData(void)
{
	// データの到着を待つ
	if(!m_bIsQueuing)return NULL;

	if(::WaitForSingleObject(m_hMediaEvent, INFINITE) != WAIT_OBJECT_0){
		// エラー
		DisableQueuing();
		return NULL;
		}

	return PeekMediaData();
}

void CMediaQueue::PurgeMediaData(void)
{
	CBlockLock Lock(&m_CriticalLock);
	
	// 未処理のデータをパージする
	while(!m_MediaQueue.empty()){
		m_MediaQueue.front()->Delete();
		m_MediaQueue.pop();
		}
}
