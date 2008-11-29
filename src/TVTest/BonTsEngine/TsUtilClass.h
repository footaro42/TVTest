// TsUtilClass.h: TSユーティリティークラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


/////////////////////////////////////////////////////////////////////////////
// ダイナミックリファレンス管理ベースクラス
/////////////////////////////////////////////////////////////////////////////

class CDynamicReferenceable
{
public:
	CDynamicReferenceable();
	virtual ~CDynamicReferenceable();

	void AddRef(void);
	void ReleaseRef(void);

private:
	DWORD m_dwRefCount;
};


/////////////////////////////////////////////////////////////////////////////
// クリティカルセクションラッパークラス
/////////////////////////////////////////////////////////////////////////////

class CCriticalLock
{
public:
	CCriticalLock();
	virtual ~CCriticalLock();

	void Lock(void);
	void Unlock(void);
	bool TryLock(DWORD TimeOut=0);
private:
	CRITICAL_SECTION m_CriticalSection;
};


/////////////////////////////////////////////////////////////////////////////
// ブロックスコープロッククラス
/////////////////////////////////////////////////////////////////////////////

class CBlockLock
{
public:
	CBlockLock(CCriticalLock *pCriticalLock);
	virtual ~CBlockLock();
		
private:
	CCriticalLock *m_pCriticalLock;
};

class CTryBlockLock
{
public:
	CTryBlockLock(CCriticalLock *pCriticalLock);
	bool TryLock(DWORD TimeOut=0);
	~CTryBlockLock();
private:
	CCriticalLock *m_pCriticalLock;
	bool m_bLocked;
};


/////////////////////////////////////////////////////////////////////////////
// CRC計算クラス
/////////////////////////////////////////////////////////////////////////////

class CCrcCalculator
{
public:
	static WORD CalcCrc16(const BYTE *pData, DWORD DataSize, WORD wCurCrc = 0xFFFF);
	static DWORD CalcCrc32(const BYTE *pData, DWORD DataSize, DWORD dwCurCrc = 0xFFFFFFFFUL);
};
