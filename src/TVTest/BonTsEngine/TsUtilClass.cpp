// TsUtilClass.cpp: TSユーティリティークラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsUtilClass.h"


//////////////////////////////////////////////////////////////////////
// CDynamicReferenceable クラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CDynamicReferenceable::CDynamicReferenceable()
	: m_dwRefCount(0UL)
{

}

CDynamicReferenceable::~CDynamicReferenceable()
{

}

void CDynamicReferenceable::AddRef(void)
{
	// 参照カウントインクリメント
	m_dwRefCount++;
}

void CDynamicReferenceable::ReleaseRef(void)
{
	// 参照カウントデクリメント
	if(m_dwRefCount){
		// インスタンス開放
		if(!(--m_dwRefCount))delete this;
		}
#ifdef _DEBUG
	else{
		::DebugBreak();
		}
#endif
}


//////////////////////////////////////////////////////////////////////
// CCriticalLock クラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CCriticalLock::CCriticalLock()
{
	// クリティカルセクション初期化
	::InitializeCriticalSection(&m_CriticalSection);
}

CCriticalLock::~CCriticalLock()
{
	// クリティカルセクション削除
	::DeleteCriticalSection(&m_CriticalSection);
}

void CCriticalLock::Lock(void)
{
	// クリティカルセクション取得
	::EnterCriticalSection(&m_CriticalSection);
}

void CCriticalLock::Unlock(void)
{
	// クリティカルセクション開放
	::LeaveCriticalSection(&m_CriticalSection);
}

// 手抜きのためにTimeOutより実際の待ち時間は増える
bool CCriticalLock::TryLock(DWORD TimeOut)
{
	bool bLocked=false;

	if (TimeOut==0) {
		if (::TryEnterCriticalSection(&m_CriticalSection))
			bLocked=true;
	} else {
		for (DWORD i=TimeOut;i>0;i--) {
			if (::TryEnterCriticalSection(&m_CriticalSection)) {
				bLocked=true;
				break;
			}
			Sleep(1);
		}
	}
	return bLocked;
}


//////////////////////////////////////////////////////////////////////
// CBlockLock クラスの構築/消滅
//////////////////////////////////////////////////////////////////////

CBlockLock::CBlockLock(CCriticalLock *pCriticalLock)
	: m_pCriticalLock(pCriticalLock)
{
	// ロック取得
	m_pCriticalLock->Lock();
}

CBlockLock::~CBlockLock()
{
	// ロック開放
	m_pCriticalLock->Unlock();
}


CTryBlockLock::CTryBlockLock(CCriticalLock *pCriticalLock)
	: m_pCriticalLock(pCriticalLock)
	, m_bLocked(false)
{
}

CTryBlockLock::~CTryBlockLock()
{
	if (m_bLocked)
		m_pCriticalLock->Unlock();
}

bool CTryBlockLock::TryLock(DWORD TimeOut)
{
	if (m_pCriticalLock->TryLock(TimeOut))
		m_bLocked=true;
	return m_bLocked;
}
