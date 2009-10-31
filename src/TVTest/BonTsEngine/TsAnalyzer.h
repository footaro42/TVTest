#pragma once

#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"
#include "TsDescriptor.h"

#define TS_ANALYZER_EIT_SUPPORT


class CTsAnalyzer : public CMediaDecoder
{
public:
	enum {
		PID_INVALID = 0xFFFF,
		COMPONENTTAG_INVALID = 0xFF
	};

	struct EsInfo {
		WORD PID;
		BYTE ComponentTag;
		EsInfo() : PID(PID_INVALID), ComponentTag(COMPONENTTAG_INVALID) {}
		EsInfo(WORD pid, BYTE Tag) : PID(pid), ComponentTag(Tag) {}
	};

	struct ServiceInfo {
		bool bIsUpdated;
		WORD ServiceID;
		WORD PmtPID;
		BYTE VideoStreamType;
		EsInfo VideoEs;
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

	CTsAnalyzer(IEventHandler *pEventHandler = NULL);
	virtual ~CTsAnalyzer();

// CMediaDecoder
	virtual void Reset();
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CTsAnalyzer
	WORD GetServiceNum();
	bool GetServiceID(const int Index, WORD *pServiceID);
	int GetServiceIndexByID(const WORD ServiceID);
	WORD GetViewableServiceNum();
	bool GetViewableServiceID(const int Index, WORD *pServiceID);
	bool GetFirstViewableServiceID(WORD *pServiceID);
	int GetViewableServiceIndexByID(const WORD ServiceID);
	bool GetServiceInfo(const int Index, ServiceInfo *pInfo);
	bool IsServiceUpdated(const int Index);
	bool GetPmtPID(const int Index, WORD *pPmtPID);
	bool GetVideoEsPID(const int Index, WORD *pVideoPID);
	bool GetVideoStreamType(const int Index, BYTE *pStreamType);
	BYTE GetVideoComponentTag(const int Index);
	WORD GetAudioEsNum(const int Index);
	bool GetAudioEsPID(const int Index, const int AudioIndex, WORD *pAudioPID);
	BYTE GetAudioComponentTag(const int Index, const int AudioIndex);
#ifdef TS_ANALYZER_EIT_SUPPORT
	BYTE GetVideoComponentType(const int Index);
	BYTE GetAudioComponentType(const int Index, const int AudioIndex);
#endif
	WORD GetSubtitleEsNum(const int Index);
	bool GetSubtitleEsPID(const int Index, const WORD SubtitleIndex, WORD *pSubtitlePID);
	WORD GetDataCarrouselEsNum(const int Index);
	bool GetDataCarrouselEsPID(const int Index, const WORD DataCarrouselIndex, WORD *pDataCarrouselPID);
	bool GetPcrPID(const int Index, WORD *pPcrPID);
	bool GetPcrTimeStamp(const int Index, ULONGLONG *pTimeStamp);
	bool GetEcmPID(const int Index, WORD *pEcmPID);
	int GetServiceName(const int Index, LPTSTR pszName, const int MaxLength);

	class CServiceList {
	protected:
		std::vector<ServiceInfo> m_ServiceList;
	public:
		virtual ~CServiceList() {}
		void Clear() { m_ServiceList.clear(); }
		int NumServices() const { return m_ServiceList.size(); }
		const ServiceInfo *GetServiceInfo(int Index) const {
			if (Index<0 || Index>(int)m_ServiceList.size())
				return NULL;
			return &m_ServiceList[Index];
		}
		friend class CTsAnalyzer;
	};

	bool GetServiceList(CServiceList *pList);
	bool GetViewableServiceList(CServiceList *pList);

	WORD GetTransportStreamID() const;
	WORD GetNetworkID() const;
	BYTE GetBroadcastingID() const;
	int GetNetworkName(LPTSTR pszName, int MaxLength);
	BYTE GetRemoteControlKeyID() const;
	int GetTsName(LPTSTR pszName, int MaxLength);

#ifdef TS_ANALYZER_EIT_SUPPORT
	WORD GetEventID(const int ServiceIndex, const bool bNext = false);
	bool GetEventStartTime(const int ServiceIndex, SYSTEMTIME *pSystemTime, const bool bNext = false);
	DWORD GetEventDuration(const int ServiceIndex, const bool bNext = false);
	int GetEventName(const int ServiceIndex, LPTSTR pszName, int MaxLength, const bool bNext = false);
	int GetEventText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bNext = false);
	int GetEventExtendedText(const int ServiceIndex, LPTSTR pszText, int MaxLength, const bool bUseEventGroup = true, const bool bNext = false);
	struct EventSeriesInfo {
		WORD SeriesID;
		BYTE RepeatLabel;
		BYTE ProgramPattern;
		bool bIsExpireDateValid;
		SYSTEMTIME ExpireDate;
		WORD EpisodeNumber;
		WORD LastEpisodeNumber;
		TCHAR szSeriesName[CSeriesDesc::MAX_SERIES_NAME];
	};
	bool GetEventSeriesInfo(const int ServiceIndex, EventSeriesInfo *pInfo, const bool bNext = false);
	struct EventVideoInfo {
		enum { MAX_TEXT = 64 };
		BYTE StreamContent;
		BYTE ComponentType;
		BYTE ComponentTag;
		DWORD LanguageCode;
		TCHAR szText[MAX_TEXT];
	};
	struct EventAudioInfo {
		enum { MAX_TEXT = 64 };
		BYTE StreamContent;
		BYTE ComponentType;
		BYTE ComponentTag;
		BYTE SimulcastGroupTag;
		bool bESMultiLingualFlag;
		bool bMainComponentFlag;
		BYTE QualityIndicator;
		BYTE SamplingRate;
		DWORD LanguageCode;
		DWORD LanguageCode2;
		TCHAR szText[MAX_TEXT];
	};
	typedef std::vector<EventAudioInfo> EventAudioList;
	struct EventContentNibble {
		int NibbleCount;
		CContentDesc::Nibble NibbleList[7];
	};
	struct EventInfo {
		WORD EventID;
		bool bValidStartTime;
		SYSTEMTIME StartTime;
		DWORD Duration;
		LPTSTR pszEventName;
		int MaxEventName;
		LPTSTR pszEventText;
		int MaxEventText;
		LPTSTR pszEventExtendedText;
		int MaxEventExtendedText;
		EventVideoInfo Video;
		EventAudioList Audio;
		EventContentNibble ContentNibble;
	};
	bool GetEventVideoInfo(const int ServiceIndex, EventVideoInfo *pInfo, const bool bNext = false);
	bool GetEventAudioList(const int ServiceIndex, EventAudioList *pList, const bool bNext = false);
	bool GetEventContentNibble(const int ServiceIndex, EventContentNibble *pInfo, const bool bNext = false);
	bool GetEventInfo(const int ServiceIndex, EventInfo *pInfo, const bool bUseEventGroup = true, const bool bNext = false);
#endif	// TS_ANALYZER_EIT_SUPPORT

	bool GetTotTime(SYSTEMTIME *pTime);

	enum EventType {
		EVENT_PAT_UPDATED,
		EVENT_PMT_UPDATED,
		EVENT_SDT_UPDATED,
		EVENT_NIT_UPDATED,
		EVENT_PCR_UPDATED,
		NUM_EVENTS,
		EVENT_LAST = NUM_EVENTS - 1
	};
	class IAnalyzerEventHandler {
	public:
		IAnalyzerEventHandler();
		virtual ~IAnalyzerEventHandler();
		virtual void OnEvent(CTsAnalyzer *pAnalyzer, EventType Type) = 0;
		virtual void OnReset(CTsAnalyzer *pAnalyzer);
	};
	bool AddEventHandler(IAnalyzerEventHandler *pHandler);
	bool RemoveEventHandler(IAnalyzerEventHandler *pHandler);

protected:
	void CallEventHandler(EventType Type);
	void NotifyResetEvent();

#ifdef TS_ANALYZER_EIT_SUPPORT
	const CDescBlock *GetHEitItemDesc(const int ServiceIndex, const bool bNext = false) const;
#ifdef TVH264
	const CDescBlock *GetLEitItemDesc(const int ServiceIndex, const bool bNext = false) const;
#endif
#endif

	CTsPidMapManager m_PidMapManager;

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

	std::vector<IAnalyzerEventHandler*> m_EventHandlerList;

private:
	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
};
