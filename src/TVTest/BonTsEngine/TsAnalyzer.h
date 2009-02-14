#pragma once

#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"


class CTsAnalyzer : public CMediaDecoder
{
public:
	enum { PID_INVALID = 0xFFFF };

	CTsAnalyzer(IEventHandler *pEventHandler = NULL);
	virtual ~CTsAnalyzer();

	// CMediaDecoder
	virtual void Reset();
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

	// CTsAnalyzer
	WORD GetServiceNum();
	bool GetServiceID(const WORD Index, WORD *pServiceID);
	int GetServiceIndexByID(const WORD ServiceID);
	bool GetPmtPID(const WORD Index, WORD *pPmtPID);
	bool GetVideoEsPID(const WORD Index, WORD *pVideoPID);
	bool GetVideoStreamType(const WORD Index, BYTE *pStreamType);
	WORD GetAudioEsNum(const WORD Index);
	bool GetAudioEsPID(const WORD Index, const WORD AudioIndex, WORD *pAudioPID);
	BYTE GetAudioComponentTag(const WORD Index, const WORD AudioIndex);
	//BYTE GetAudioComponentType(const WORD Index, const WORD AudioIndex);
	WORD GetSubtitleEsNum(const WORD Index);
	bool GetSubtitleEsPID(const WORD Index, const WORD SubtitleIndex, WORD *pSubtitlePID);
	WORD GetDataCarrouselEsNum(const WORD Index);
	bool GetDataCarrouselEsPID(const WORD Index, const WORD DataCarrouselIndex, WORD *pDataCarrouselPID);
	bool GetPcrPID(const WORD Index, WORD *pPcrPID);
	bool GetPcrTimeStamp(const WORD Index, ULONGLONG *pTimeStamp);
	bool GetEcmPID(const WORD Index, WORD *pEcmPID);
	int GetServiceName(const WORD Index, LPTSTR pszName, const int MaxLength);
	WORD GetTransportStreamID() const;

	WORD GetNetworkID() const;
	BYTE GetBroadcastingID() const;
	int GetNetworkName(LPTSTR pszName, int MaxLength);
	BYTE GetRemoteControlKeyID() const;
	int GetTsName(LPTSTR pszName, int MaxLength);

	class CEventHandler {
	public:
		CEventHandler();
		virtual ~CEventHandler();
		virtual void OnEvent(CTsAnalyzer *pAnalyzer) = 0;
		virtual void OnReset(CTsAnalyzer *pAnalyzer);
	};
	enum EventType {
		EVENT_PAT_UPDATE,
		EVENT_PMT_UPDATE,
		EVENT_PCR_UPDATE,
		EVENT_NIT_UPDATE,
		EVENT_LAST = EVENT_NIT_UPDATE
	};
	bool AddEventHandler(EventType Type, CEventHandler *pHandler);
	bool RemoveEventHandler(CEventHandler *pHandler);

protected:
	void CallEventHandler(EventType Type);
	void NotifyResetEvent(EventType Type);

	CTsPidMapManager m_PidMapManager;

	struct EsInfo {
		WORD PID;
		BYTE ComponentTag;
		EsInfo(WORD pid, BYTE Tag = 0) : PID(pid), ComponentTag(Tag) {}
	};

	struct ServiceInfo {
		bool bIsUpdated;
		WORD ServiceID;
		WORD PmtPID;
		BYTE VideoStreamType;
		WORD VideoEsPID;
		std::vector<EsInfo> AudioEsList;
		std::vector<EsInfo> SubtitleEsList;
		std::vector<EsInfo> DataCarrouselEsList;
		WORD PcrPID;
		ULONGLONG PcrTimeStamp;
		WORD EcmPID;
		BYTE RunningStatus;
		bool bIsCaService;
		TCHAR szServiceName[256];
		BYTE ServiceType;
	};

	std::vector<ServiceInfo> m_ServiceList;
	WORD m_TransportStreamID;

	struct NitInfo {
		WORD NetworkID;
		BYTE BroadcastingID;
		BYTE RemoteControlKeyID;
		TCHAR szNetworkName[32];
		TCHAR szTSName[32];
	};
	NitInfo m_NitInfo;

	struct EventHandlerInfo {
		EventType Type;
		CEventHandler *pHandler;
	};
	std::vector<EventHandlerInfo> m_EventHandlerList[EVENT_LAST + 1];

private:
	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
};
