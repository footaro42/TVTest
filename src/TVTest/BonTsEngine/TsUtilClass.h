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
	DWORD GetRefCount(void) const;

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
// イベントクラス
/////////////////////////////////////////////////////////////////////////////

class CLocalEvent
{
	HANDLE m_hEvent;
public:
	CLocalEvent();
	~CLocalEvent();
	bool Create(bool bManual = false, bool bInitialState = false);
	bool IsCreated() const;
	void Close();
	bool Set();
	bool Reset();
	DWORD Wait(DWORD Timeout = INFINITE);
	DWORD SignalAndWait(HANDLE hHandle, DWORD Timeout = INFINITE, bool bAlertable = false);
	DWORD SignalAndWait(CLocalEvent *pEvent, DWORD Timeout = INFINITE);
	bool IsSignaled();
};


/////////////////////////////////////////////////////////////////////////////
// 日時クラス
/////////////////////////////////////////////////////////////////////////////

class CDateTime
{
	SYSTEMTIME m_Time;
public:
	CDateTime();
	CDateTime(const SYSTEMTIME &Time);
	CDateTime &operator=(const SYSTEMTIME &Time);
	CDateTime &operator=(const FILETIME &Time);
	void LocalTime();
	void UTCTime();
	bool LocalToUTC();
	bool UTCToLocal();
	bool Offset(LONGLONG Milliseconds);
	void Set(const SYSTEMTIME &Time) { m_Time = Time; }
	const SYSTEMTIME &Get() const { return m_Time; }
	void Get(SYSTEMTIME *pTime) const;
	int GetYear() const { return m_Time.wYear; }
	int GetMonth() const { return m_Time.wMonth; }
	int GetDay() const { return m_Time.wDay; }
	int GetDayOfWeek() const { return m_Time.wDayOfWeek; }
	int GetHour() const { return m_Time.wHour; }
	int GetMinute() const { return m_Time.wMinute; }
	int GetSecond() const { return m_Time.wSecond; }
	DWORD GetMilliseconds() const { return m_Time.wMilliseconds; }
	static LONGLONG SECONDS(int Sec) { return Sec * 1000LL; }
	static LONGLONG MINUTES(int Min) { return Min * (1000LL * 60LL); }
	static LONGLONG HOURS(int Hours) { return Hours * (1000LL * 60LL * 60LL); }
};


/////////////////////////////////////////////////////////////////////////////
// トレースクラス
/////////////////////////////////////////////////////////////////////////////

class CTracer
{
	TCHAR m_szBuffer[256];
public:
	virtual ~CTracer() {}
	void Trace(LPCTSTR pszOutput, ...);
	void TraceV(LPCTSTR pszOutput,va_list Args);
protected:
	virtual void OnTrace(LPCTSTR pszOutput)=0;
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
