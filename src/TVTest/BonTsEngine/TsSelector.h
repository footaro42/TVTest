#pragma once


#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsTable.h"
#include "TsUtilClass.h"


class CTsSelector : public CMediaDecoder
{
public:
	CTsSelector(IEventHandler *pEventHandler = NULL);
	virtual ~CTsSelector();

// CMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsSelector
	bool SetTargetServiceID(WORD ServiceID=0);
	ULONGLONG GetInputPacketCount() const;
	ULONGLONG GetOutputPacketCount() const;

protected:
	bool IsTargetPID(WORD PID);
	bool AddTargetPID(WORD PID);
	bool MakePat(const CTsPacket *pSrcPacket, CTsPacket *pDstPacket);

	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnCatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	CTsPidMapManager m_PidMapManager;

	WORD m_TargetServiceID;
	std::vector<WORD> m_TargetPIDList;
	WORD m_TargetPmtPID;

	ULONGLONG m_InputPacketCount;
	ULONGLONG m_OutputPacketCount;

	CTsPacket m_PatPacket;
	WORD m_LastTSID;
	WORD m_LastPmtPID;
	BYTE m_LastVersion;
	BYTE m_Version;
};
