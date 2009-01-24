// ProgManager.h: CProgManager クラスのインターフェイス
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <vector>
#include "MediaDecoder.h"
#include "TsStream.h"


using std::vector;


struct EsInfo {
	WORD PID;
	BYTE ComponentTag;
	EsInfo(WORD pid,BYTE Tag) : PID(pid), ComponentTag(Tag) {}
};


/////////////////////////////////////////////////////////////////////////////
// 番組情報管理クラス
/////////////////////////////////////////////////////////////////////////////
// Input	#0	: CTsPacket	TSパケット
// Output	#0	: CTsPacket	TSパケット
/////////////////////////////////////////////////////////////////////////////

class CProgManager : public CMediaDecoder
{
public:
	enum EVENTID
	{
		EID_SERVICE_LIST_UPDATED,	// サービスリスト更新
		EID_SERVICE_INFO_UPDATED,	// サービス情報更新
		EID_PCR_TIMESTAMP_UPDATED	// PCRタイムスタンプ更新(かなり頻繁)
	};

	CProgManager(IEventHandler *pEventHandler = NULL);
	virtual ~CProgManager();

// IMediaDecoder
	virtual void Reset(void);
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);

// CProgManager
	const WORD GetServiceNum(void);
	const bool GetServiceID(WORD *pwServiceID, const WORD wIndex = 0U);
	const WORD GetServiceIndexByID(const WORD ServiceID);
	const bool GetVideoEsPID(WORD *pwVideoPID, const WORD wIndex = 0U);
	const bool GetAudioEsPID(WORD *pwAudioPID, const WORD wAudioIndex = 0U, const WORD wIndex = 0U);
	const BYTE GetAudioComponentTag(const WORD wAudioIndex = 0U,const WORD wIndex = 0U);
	const BYTE GetAudioComponentType(const WORD wAudioIndex = 0U,const WORD wIndex = 0U);
	const WORD GetAudioEsNum(const WORD wIndex = 0U);
	const bool GetSubtitleEsPID(WORD *pwSubtitlePID, const WORD wIndex = 0U);
	const bool GetPcrTimeStamp(unsigned __int64 *pu64PcrTimeStamp, const WORD wServiceID = 0U);
	const DWORD GetServiceName(LPTSTR lpszDst, const WORD wIndex = 0U);
	const WORD GetTransportStreamID() const;

	WORD GetNetworkID(void) const;
	BYTE GetBroadcastingID(void) const;
	DWORD GetNetworkName(LPTSTR pszName,int MaxLength);
	BYTE GetRemoteControlKeyID(void) const;
	DWORD GetTSName(LPTSTR pszName,int MaxLength);

	const WORD GetEventID(const WORD ServiceIndex, const bool fNext = false);
	const bool GetStartTime(const WORD ServiceIndex, SYSTEMTIME *pSystemTime, const bool fNext = false);
	const DWORD GetDuration(const WORD ServiceIndex, const bool fNext = false);
	const int GetEventName(const WORD ServiceIndex, LPTSTR pszName, int MaxLength, const bool fNext = false);
	const int GetEventText(const WORD ServiceIndex, LPTSTR pszText, int MaxLength, const bool fNext = false);

protected:
	class CProgDatabase;

	void OnServiceListUpdated(void);
	void OnServiceInfoUpdated(void);
	void OnPcrTimestampUpdated(void);

	const CDescBlock *GetHEitItemDesc(const WORD ServiceIndex, const bool fNext = false) const;

	struct TAG_SERVICEINFO
	{
		WORD wServiceID;
		WORD wVideoEsPID;
		vector<EsInfo> AudioEsList;
		WORD wSubtitleEsPID;
		TCHAR szServiceName[256];

		// タイムスタンプ
		unsigned __int64 u64TimeStamp;
	};

	vector<TAG_SERVICEINFO> m_ServiceList;

	CTsPidMapManager m_PidMapManager;
	CProgDatabase *m_pProgDatabase;
};


/////////////////////////////////////////////////////////////////////////////
// 番組情報データベースクラス
/////////////////////////////////////////////////////////////////////////////

class CProgManager::CProgDatabase
{
public:
	CProgDatabase(CProgManager &ProgManager);
	virtual ~CProgDatabase();

	void Reset(void);
	void UnmapTable(void);

	const WORD GetServiceIndexByID(const WORD wServiceID);

	// CProgManagerと情報がダブっているので見直すべき
	struct TAG_SERVICEINFO
	{
		WORD wServiceID;
		BYTE VideoStreamType;
		WORD wVideoEsPID;
		vector<EsInfo> AudioEsList;
		WORD wSubtitleEsPID;
		BYTE byServiceType;
		TCHAR szServiceName[256];

		bool bIsUpdated;

		// 下記は情報として特に不要？
		BYTE byVideoComponentTag;

		WORD wPmtTablePID;
		BYTE byRunningStatus;
		bool bIsCaService;

		// タイムスタンプ
		WORD wPcrPID;
		unsigned __int64 u64TimeStamp;
	};

	vector<TAG_SERVICEINFO> m_ServiceList;
	WORD m_wTransportStreamID;

	struct TAG_NITINFO
	{
		WORD wNetworkID;
		BYTE byBroadcastingID;
		BYTE byRemoteControlKeyID;
		TCHAR szNetworkName[32];
		TCHAR szTSName[32];
	};
	TAG_NITINFO m_NitInfo;

private:
	static void CALLBACK OnPatUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPmtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnPcrUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);
	static void CALLBACK OnSdtUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);	
	static void CALLBACK OnNitUpdated(const WORD wPID, CTsPidMapTarget *pMapTarget, CTsPidMapManager *pMapManager, const PVOID pParam);

	CProgManager &m_ProgManager;
	CTsPidMapManager &m_PidMapManager;
};
