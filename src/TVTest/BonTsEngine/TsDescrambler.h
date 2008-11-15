// TsDescrambler.h: CTsDescrambler クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <deque>
#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsTable.h"
#include "TsUtilClass.h"
#include "BcasCard.h"
#include "Multi2Decoder.h"


class CEcmProcessor;

class CBcasAccess {
	CEcmProcessor *m_pEcmProcessor;
	BYTE m_EcmData[256];
	DWORD m_EcmSize;
public:
	CBcasAccess(CEcmProcessor *pEcmProcessor, const BYTE *pData, DWORD Size);
	CBcasAccess(const CBcasAccess &BcasAccess);
	~CBcasAccess();
	CBcasAccess &operator=(const CBcasAccess &BcasAccess);
	bool SetScrambleKey();
};

class CBcasAccessQueue {
	std::deque<CBcasAccess> m_Queue;
	HANDLE m_hThread;
	HANDLE m_hEvent;
	volatile bool m_bKillEvent;
	CCriticalLock m_Lock;
	static DWORD CALLBACK BcasAccessThread(LPVOID lpParameter);
public:
	CBcasAccessQueue();
	~CBcasAccessQueue();
	void Clear();
	bool Enqueue(CEcmProcessor *pEcmProcessor, const BYTE *pData, DWORD Size);
	bool BeginBcasThread();
	bool EndBcasThread();
};


/////////////////////////////////////////////////////////////////////////////
// MULTI2スクランブル解除(ECMによりペイロードのスクランブルを解除する)
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CTsPacket		暗号TSパケット
// Output	#0	: CTsPacket		平分TSパケット
/////////////////////////////////////////////////////////////////////////////

class CTsDescrambler : public CMediaDecoder
{
public:
	CTsDescrambler(IEventHandler *pEventHandler = NULL);
	virtual ~CTsDescrambler();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsDescrambler
	const bool OpenBcasCard(CCardReader::ReaderType ReaderType = CCardReader::READER_SCARD, DWORD *pErrorCode = NULL);
	void CloseBcasCard(void);
	const bool IsBcasCardOpen() const;
	const bool GetBcasCardID(BYTE *pCardID);
	LPCTSTR GetCardReaderName() const;
	const DWORD GetInputPacketCount(void) const;
	const DWORD GetScramblePacketCount(void) const;
	bool SetTargetPID(const WORD *pPIDList=NULL,int NumPIDs=0);
	bool IsTargetPID(WORD PID);
	bool SetTargetServiceID(WORD ServiceID=0);
	DWORD IncrementScramblePacketCount();

protected:
	class CEsProcessor;

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	CTsPidMapManager m_PidMapManager;
	CBcasCard m_BcasCard;
	CBcasAccessQueue m_Queue;

	std::vector<WORD> m_DescramblePIDList;
	CCriticalLock m_DescrambleListLock;
	WORD m_DescrambleServiceID;

	DWORD m_dwInputPacketCount;
	DWORD m_dwScramblePacketCount;
};


// ECM処理内部クラス
class CEcmProcessor :	public CDynamicReferenceable,
						public CPsiSingleTable
{
public:
	CEcmProcessor(CTsDescrambler *pDescrambler, CBcasCard *pBcasCard, CBcasAccessQueue *pQueue);

// CTsPidMapTarget
	virtual void OnPidMapped(const WORD wPID, const PVOID pParam);
	virtual void OnPidUnmapped(const WORD wPID);

	const bool DescramblePacket(CTsPacket *pTsPacket);
	const bool SetScrambleKey(const BYTE *pEcmData, DWORD EcmSize);

	const bool AddEsPID(WORD PID);

protected:
// CPsiSingleTable
	virtual const bool OnTableUpdate(const CPsiSection *pCurSection, const CPsiSection *pOldSection);

private:
	CTsDescrambler *m_pDescrambler;
	CMulti2Decoder m_Multi2Decoder;
	CBcasCard *m_pBcasCard;
	CBcasAccessQueue *m_pQueue;
	bool m_bInQueue;
	volatile bool m_bSetScrambleKey;
	CCriticalLock m_Multi2Lock;
	std::vector<WORD> m_EsPIDList;

	bool m_bLastEcmSucceed;
};


// ESスクランブル解除内部クラス
class CTsDescrambler::CEsProcessor : public CTsPidMapTarget
{
public:
	CEsProcessor(CEcmProcessor *pEcmProcessor);
	virtual ~CEsProcessor();

// CTsPidMapTarget
	virtual const bool StorePacket(const CTsPacket *pPacket);
	virtual void OnPidUnmapped(const WORD wPID);

private:
	CEcmProcessor *m_pEcmProcessor;
};
