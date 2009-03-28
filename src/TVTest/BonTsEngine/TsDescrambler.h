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

class CBcasAccessQueue : public CBonBaseClass {
	std::deque<CBcasAccess> m_Queue;
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
	CTsDescrambler(IEventHandler *pEventHandler = NULL);
	virtual ~CTsDescrambler();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsDescrambler
	const bool EnableDescramble(bool bDescramble);
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
protected:
	class CEsProcessor;

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	int GetServiceIndexByID(WORD ServiceID) const;

#ifdef _DEBUG
	void PrintStatus(void) const;
#endif

	bool m_bDescramble;
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

	friend class CEcmProcessor;
	friend class CDescramblePmtTable;
};
