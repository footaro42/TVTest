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


class CEcmProcessor;
class CEmmProcessor;

class CBcasAccess {
protected:
	BYTE m_Data[256];
	DWORD m_DataSize;

public:
	CBcasAccess(const BYTE *pData, DWORD Size);
	CBcasAccess(const CBcasAccess &BcasAccess);
	virtual ~CBcasAccess();
	CBcasAccess &operator=(const CBcasAccess &BcasAccess);
	virtual bool Process() = 0;
};

class CEcmAccess : public CBcasAccess {
	CEcmProcessor *m_pEcmProcessor;

public:
	CEcmAccess(CEcmProcessor *pEcmProcessor, const BYTE *pData, DWORD Size);
	CEcmAccess(const CEcmAccess &BcasAccess);
	~CEcmAccess();
	CEcmAccess &operator=(const CEcmAccess &EcmAccess);
	bool Process();
};

class CEmmAccess : public CBcasAccess {
	CEmmProcessor *m_pEmmProcessor;

public:
	CEmmAccess(CEmmProcessor *pEmmProcessor, const BYTE *pData, DWORD Size);
	CEmmAccess(const CEmmAccess &EmmAccess);
	~CEmmAccess();
	CEmmAccess &operator=(const CEmmAccess &EmmAccess);
	bool Process();
};

class CBcasAccessQueue : public CBonBaseClass {
	std::deque<CBcasAccess*> m_Queue;
	CBcasCard *m_pBcasCard;
	CCardReader::ReaderType m_ReaderType;
	HANDLE m_hThread;
	CLocalEvent m_Event;
	volatile bool m_bKillEvent;
	volatile bool m_bStartEvent;
	CCriticalLock m_Lock;
	static DWORD CALLBACK BcasAccessThread(LPVOID lpParameter);

public:
	CBcasAccessQueue(CBcasCard *pBcasCard);
	~CBcasAccessQueue();
	void Clear();
	bool Enqueue(CEcmProcessor *pEcmProcessor, const BYTE *pData, DWORD Size);
	bool Enqueue(CEmmProcessor *pEmmProcessor, const BYTE *pData, DWORD Size);
	bool BeginBcasThread(CCardReader::ReaderType ReaderType);
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
	enum {
		EID_EMM_PROCESSED	= 0x00000001UL
	};

	CTsDescrambler(IEventHandler *pEventHandler = NULL);
	virtual ~CTsDescrambler();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsDescrambler
	const bool EnableDescramble(bool bDescramble);
	const bool EnableEmmProcess(bool bEnable);
	const bool OpenBcasCard(CCardReader::ReaderType ReaderType = CCardReader::READER_SCARD);
	void CloseBcasCard(void);
	const bool IsBcasCardOpen() const;
	const bool GetBcasCardID(BYTE *pCardID);
	LPCTSTR GetCardReaderName() const;
	int FormatBcasCardID(LPTSTR pszText,int MaxLength) const;
	char GetBcasCardManufacturerID() const;
	BYTE GetBcasCardVersion() const;
	const DWORD GetInputPacketCount(void) const;
	const DWORD GetScramblePacketCount(void) const;
	void ResetScramblePacketCount(void);
	bool SetTargetServiceID(WORD ServiceID=0);
	static bool IsSSE2Available();
	bool EnableSSE2(bool bEnable);

protected:
	class CEsProcessor;

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnCatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	int GetServiceIndexByID(WORD ServiceID) const;

#ifdef _DEBUG
	void PrintStatus(void) const;
#endif

	bool m_bDescramble;
	bool m_bProcessEmm;
	CTsPidMapManager m_PidMapManager;
	CBcasCard m_BcasCard;
	CBcasAccessQueue m_Queue;

	WORD m_CurTransportStreamID;
	WORD m_DescrambleServiceID;

	struct TAG_SERVICEINFO {
		bool bTarget;
		WORD ServiceID;
		WORD PmtPID;
		WORD EcmPID;
		std::vector<WORD> EsPIDList;
	};
	std::vector<TAG_SERVICEINFO> m_ServiceList;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_ScramblePacketCount;

	bool m_bEnableSSE2;

	WORD m_EmmPID;

	friend class CEcmProcessor;
	friend class CEmmProcessor;
	friend class CDescramblePmtTable;
};
