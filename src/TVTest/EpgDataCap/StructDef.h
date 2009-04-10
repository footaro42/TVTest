#ifndef __STRUCT_DEF_H__
#define __STRUCT_DEF_H__

#include "Util.h"

typedef struct _CH_DATA1{
	int iSpace;
	int iCh;
	wstring strName;
	BOOL bEpgCap;
	//=オペレーターの処理
	_CH_DATA1 & operator= (const _CH_DATA1 & o) {
		iSpace = o.iSpace;
		iCh = o.iCh;
		strName = o.strName;
		bEpgCap = o.bEpgCap;
		return *this;
	}
} CH_DATA1;

typedef struct _CH_DATA2{
	int iSpace;
	int iCh;
	int iTSID;
	int iSID;
	int iONID;
	DWORD dwServiceType;
	BOOL bUse;
	wstring strName;
	//=オペレーターの処理
	_CH_DATA2 & operator= (const _CH_DATA2 & o) {
		iSpace = o.iSpace;
		iCh = o.iCh;
		iTSID = o.iTSID;
		iSID = o.iSID;
		iONID = o.iONID;
		dwServiceType = o.dwServiceType;
		bUse = o.bUse;
		strName = o.strName;
		return *this;
	}
} CH_DATA2;

typedef struct _CH_DATA3{
	int iTSID;
	int iSID;
	int iONID;
	wstring strName;
	DWORD dwServiceType;
	BOOL bEpgCap;
	BOOL bSearch;
	//=オペレーターの処理
	_CH_DATA3 & operator= (const _CH_DATA3 & o) {
		iTSID = o.iTSID;
		iSID = o.iSID;
		iONID = o.iONID;
		strName = o.strName;
		dwServiceType = o.dwServiceType;
		bEpgCap = o.bEpgCap;
		bSearch = o.bSearch;
		return *this;
	}
} CH_DATA3;

//EpgDataCap.dll用
typedef struct _EPG_DATA_INFO{
	DWORD dwServiceID;
	DWORD dwEventID;
	WCHAR* lpwszEventName;
	DWORD dwEventNameLength;
	WCHAR* lpwszEventText;
	DWORD dwEventTextLength;
	WCHAR* lpwszEventExtText;
	DWORD dwEventExtTextLength;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	BOOL bUnkownStart; //開始時間未定
	BOOL bUnkownDure; //総時間未定
}EPG_DATA_INFO;

typedef struct _SERVICE_INFO{
	DWORD dwTSID;
	DWORD dwOriginalNID;
	DWORD dwServiceID;
	WCHAR* lpwszServiceName;
	DWORD dwServiceNameLength;
	DWORD dwServiceType;
}SERVICE_INFO;

typedef struct _EPG_DATA_INFO2{
	DWORD dwOriginalNID;
	DWORD dwTSID;
	DWORD dwServiceID;
	DWORD dwEventID;
	WCHAR* lpwszEventName;
	DWORD dwEventNameLength;
	WCHAR* lpwszEventText;
	DWORD dwEventTextLength;
	WCHAR* lpwszEventExtText;
	DWORD dwEventExtTextLength;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	unsigned char ucComponentType;
	DWORD dwComponentTypeTextLength;
	WCHAR* lpwszComponentTypeText;
	unsigned char ucAudioComponentType;
	unsigned char ucESMultiLangFlag;
	unsigned char ucMainComponentFlag;
	unsigned char ucSamplingRate;
	DWORD dwAudioComponentTypeTextLength;
	WCHAR* lpwszAudioComponentTypeText;
	DWORD dwContentNibbleListCount;
	DWORD* dwContentNibbleList;
}EPG_DATA_INFO2;

typedef struct _EVENT_ID_INFO{
	DWORD dwOriginalNID;
	DWORD dwTSID;
	DWORD dwServiceID;
	DWORD dwEventID;
}EVENT_ID_INFO;

typedef struct _EPG_DATA_INFO3{
	DWORD dwOriginalNID;
	DWORD dwTSID;
	DWORD dwServiceID;
	DWORD dwEventID;
	WCHAR* lpwszEventName;
	DWORD dwEventNameLength;
	WCHAR* lpwszEventText;
	DWORD dwEventTextLength;
	WCHAR* lpwszEventExtText;
	DWORD dwEventExtTextLength;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	unsigned char ucComponentType;
	DWORD dwComponentTypeTextLength;
	WCHAR* lpwszComponentTypeText;
	unsigned char ucAudioComponentType;
	unsigned char ucESMultiLangFlag;
	unsigned char ucMainComponentFlag;
	unsigned char ucSamplingRate;
	DWORD dwAudioComponentTypeTextLength;
	WCHAR* lpwszAudioComponentTypeText;
	DWORD dwContentNibbleListCount;
	DWORD* dwContentNibbleList;
	DWORD dwEventRelayListCount;
	EVENT_ID_INFO* pstEventRelayList;
	DWORD dwEventGroupListCount;
	EVENT_ID_INFO* pstEventGroupList;
}EPG_DATA_INFO3;

typedef struct _ELEMENT_INFO{
	unsigned char ucStreamType;
	unsigned short usElementaryPID;
	unsigned short usComponentTag;
	unsigned char ucQuality;
	DWORD dwECMPID; //当該ESのみ対象のECM 存在しない場合は0x1FFF
}ELEMENT_INFO;

//EpgDB用
typedef struct _SERVICE_INFO_DATA{
	WORD wOriginalNID;
	WORD wTSID;
	WORD wServiceID;
	wstring strServiceName;
	WORD wServiceType;
	_SERVICE_INFO_DATA & operator= (const _SERVICE_INFO_DATA & o) {
		wOriginalNID = o.wOriginalNID;
		wTSID = o.wTSID;
		wServiceID = o.wServiceID;
		strServiceName = o.strServiceName;
		wServiceType = o.wServiceType;
		return *this;
	};
}SERVICE_INFO_DATA;

typedef struct _NIBBLE_DATA{
	unsigned char ucContentNibbleLv1; //content_nibble_level_1
	unsigned char ucContentNibbleLv2; //content_nibble_level_2
	unsigned char ucUserNibbleLv1; //user_nibble
	unsigned char ucUserNibbleLv2; //user_nibble
	_NIBBLE_DATA & operator= (const _NIBBLE_DATA & o) {
		ucContentNibbleLv1 = o.ucContentNibbleLv1;
		ucContentNibbleLv2 = o.ucContentNibbleLv2;
		ucUserNibbleLv1 = o.ucUserNibbleLv1;
		ucUserNibbleLv2 = o.ucUserNibbleLv2;
		return *this;
	};
}NIBBLE_DATA;

typedef struct _EVENT_INFO_DATA{
	WORD wOriginalNID;
	WORD wTSID;
	WORD wServiceID;
	WORD wEventID;
	wstring strEventName;
	wstring strEventText;
	wstring strEventExtText;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	unsigned char ucComponentType;
	wstring strComponentTypeText;
	unsigned char ucAudioComponentType;
	unsigned char ucESMultiLangFlag;
	unsigned char ucMainComponentFlag;
	unsigned char ucSamplingRate;
	wstring strAudioComponentTypeText;
	vector<NIBBLE_DATA> NibbleList;

	wstring strSearchTitle;
	wstring strSearchInfo;

	_EVENT_INFO_DATA & operator= (const _EVENT_INFO_DATA & o) {
		wOriginalNID = o.wOriginalNID;
		wTSID = o.wTSID;
		wServiceID = o.wServiceID;
		wEventID = o.wEventID;
		strEventName = o.strEventName;
		strEventText = o.strEventText;
		strEventExtText = o.strEventExtText;
		stStartTime = o.stStartTime;
		dwDurationSec = o.dwDurationSec;
		ucComponentType = o.ucComponentType;
		strComponentTypeText = o.strComponentTypeText;
		ucAudioComponentType = o.ucAudioComponentType;
		ucESMultiLangFlag = o.ucESMultiLangFlag;
		ucMainComponentFlag = o.ucMainComponentFlag;
		ucSamplingRate = o.ucSamplingRate;
		strAudioComponentTypeText = o.strAudioComponentTypeText;
		NibbleList = o.NibbleList;

		strSearchTitle = o.strSearchTitle;
		strSearchInfo = o.strSearchInfo;
		return *this;
	};
}EVENT_INFO_DATA;

typedef struct _EVENT_INFO_DATA3{
	WORD wOriginalNID;
	WORD wTSID;
	WORD wServiceID;
	WORD wEventID;
	wstring strEventName;
	wstring strEventText;
	wstring strEventExtText;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	unsigned char ucComponentType;
	wstring strComponentTypeText;
	unsigned char ucAudioComponentType;
	unsigned char ucESMultiLangFlag;
	unsigned char ucMainComponentFlag;
	unsigned char ucSamplingRate;
	wstring strAudioComponentTypeText;
	vector<NIBBLE_DATA> NibbleList;
	vector<EVENT_ID_INFO> EventRelayList;

	wstring strSearchTitle;
	wstring strSearchInfo;
	vector<EVENT_ID_INFO> EventGroupList;

	_EVENT_INFO_DATA3 & operator= (const _EVENT_INFO_DATA3 & o) {
		wOriginalNID = o.wOriginalNID;
		wTSID = o.wTSID;
		wServiceID = o.wServiceID;
		wEventID = o.wEventID;
		strEventName = o.strEventName;
		strEventText = o.strEventText;
		strEventExtText = o.strEventExtText;
		stStartTime = o.stStartTime;
		dwDurationSec = o.dwDurationSec;
		ucComponentType = o.ucComponentType;
		strComponentTypeText = o.strComponentTypeText;
		ucAudioComponentType = o.ucAudioComponentType;
		ucESMultiLangFlag = o.ucESMultiLangFlag;
		ucMainComponentFlag = o.ucMainComponentFlag;
		ucSamplingRate = o.ucSamplingRate;
		strAudioComponentTypeText = o.strAudioComponentTypeText;
		NibbleList = o.NibbleList;
		EventRelayList = o.EventRelayList;

		strSearchTitle = o.strSearchTitle;
		strSearchInfo = o.strSearchInfo;
		EventGroupList = o.EventGroupList;
		return *this;
	};
}EVENT_INFO_DATA3;

typedef struct _EVENT_INFO_DATA2{
	WORD wOriginalNID;
	WORD wTSID;
	WORD wServiceID;
	WORD wEventID;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	BOOL bUnkownStart; //開始時間未定
	BOOL bUnkownDure; //総時間未定
	_EVENT_INFO_DATA2 & operator= (const _EVENT_INFO_DATA2 & o) {
		wOriginalNID = o.wOriginalNID;
		wTSID = o.wTSID;
		wServiceID = o.wServiceID;
		wEventID = o.wEventID;
		stStartTime = o.stStartTime;
		dwDurationSec = o.dwDurationSec;
		bUnkownStart = o.bUnkownStart;
		bUnkownDure = o.bUnkownDure;
		return *this;
	};
}EVENT_INFO_DATA2;

typedef struct _EVENT_INFO_DATA4{
	WORD wOriginalNID;
	WORD wTSID;
	WORD wServiceID;
	WORD wEventID;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	BOOL bUnkownStart; //開始時間未定
	BOOL bUnkownDure; //総時間未定
	wstring strEventName;
	vector<EVENT_ID_INFO> EventRelayList;
	_EVENT_INFO_DATA4 & operator= (const _EVENT_INFO_DATA4 & o) {
		wOriginalNID = o.wOriginalNID;
		wTSID = o.wTSID;
		wServiceID = o.wServiceID;
		wEventID = o.wEventID;
		stStartTime = o.stStartTime;
		dwDurationSec = o.dwDurationSec;
		bUnkownStart = o.bUnkownStart;
		bUnkownDure = o.bUnkownDure;
		strEventName = o.strEventName;
		EventRelayList = o.EventRelayList;
		return *this;
	};
}EVENT_INFO_DATA4;

typedef struct _EVENT_INFO_LIST{
	map<WORD, EVENT_INFO_DATA3> EventDataMap; //キー EventID
	_EVENT_INFO_LIST & operator= (const _EVENT_INFO_LIST & o) {
		EventDataMap = o.EventDataMap;
		return *this;
	};
}EVENT_INFO_LIST;

typedef struct _SEARCH_KEY{
	wstring strAnd;
	wstring strNot;
	BOOL bTitle;
	int iJanru;
	int iSH;
	int iSM;
	int iEH;
	int iEM;
	BOOL bChkMon;
	BOOL bChkTue;
	BOOL bChkWed;
	BOOL bChkThu;
	BOOL bChkFri;
	BOOL bChkSat;
	BOOL bChkSun;
	vector<__int64> CHIDList; //ONID<<24 | TSID<<16 | SID
	//以下自動予約登録時関係のみ使用
	int iAutoAddID; //自動予約登録一覧の識別用キー
	int iPriority;
	int iTuijyuu;
	int iRecMode;
	int iPittari;
	wstring strBat;
	wstring strRecFolder;
	WORD wSuspendMode;
	BOOL bReboot;
	BOOL bUseMargine;
	int iStartMargine;
	int iEndMargine;
	DWORD dwServiceMode;

	BOOL bRegExp;
	wstring strPattern;
	//=オペレーターの処理
	_SEARCH_KEY & operator= (const _SEARCH_KEY & o) {
		strAnd = o.strAnd;
		strNot = o.strNot;
		bTitle = o.bTitle;
		iJanru = o.iJanru;
		iSH = o.iSH;
		iSM = o.iSM;
		iEH = o.iEH;
		iEM = o.iEM;
		bChkMon = o.bChkMon;
		bChkTue = o.bChkTue;
		bChkWed = o.bChkWed;
		bChkThu = o.bChkThu;
		bChkFri = o.bChkFri;
		bChkSat = o.bChkSat;
		bChkSun = o.bChkSun;
		CHIDList = o.CHIDList;
		iAutoAddID = o.iAutoAddID;
		iPriority = o.iPriority;
		iTuijyuu = o.iTuijyuu;
		iRecMode = o.iRecMode;
		iPittari = o.iPittari;
		strBat = o.strBat;
		strRecFolder = o.strRecFolder;
		wSuspendMode = o.wSuspendMode;
		bReboot = o.bReboot;
		bUseMargine = o.bUseMargine;
		iStartMargine = o.iStartMargine;
		iEndMargine = o.iEndMargine;
		dwServiceMode = o.dwServiceMode;

		bRegExp = o.bRegExp;
		strPattern = o.strPattern;
		return *this;
	};
} SEARCH_KEY;

typedef struct _RESERVE_DATA{
	wstring strTitle;
	SYSTEMTIME StartTime;
	DWORD dwDurationSec;
	wstring strStationName;
	unsigned short usONID;
	unsigned short usTSID;
	unsigned short usServiceID;
	unsigned short usEventID;
	unsigned char ucPriority;
	unsigned char ucTuijyuu;
	wstring strComment;
	DWORD dwRecMode;
	BOOL bPittari;
	wstring strBatPath;
	DWORD dwReserveID; //同一番組判別用ID
	BOOL bSetWait; //予約待機入った？
	DWORD dwPiledUpMode; //かぶり状態 1:かぶってチューナー足りない予約あり 2:チューナー足りなくて予約できない
	wstring strRecFolder;
	WORD wSuspendMode;
	BOOL bReboot;
	wstring strRecFilePath;
	BOOL bUseMargine;
	int iStartMargine;
	int iEndMargine;
	DWORD dwServiceMode;
	//=オペレーターの処理
	_RESERVE_DATA & operator= (const _RESERVE_DATA & o) {
		strTitle=o.strTitle;
		StartTime = o.StartTime;
		dwDurationSec = o.dwDurationSec;
		strStationName = o.strStationName;
		usONID = o.usONID;
		usTSID = o.usTSID;
		usServiceID = o.usServiceID;
		usEventID = o.usEventID;
		ucPriority = o.ucPriority;
		ucTuijyuu = o.ucTuijyuu;
		strComment = o.strComment;
		dwRecMode = o.dwRecMode;
		bPittari = o.bPittari;
		strBatPath = o.strBatPath;
		dwReserveID = o.dwReserveID;
		bSetWait = o.bSetWait;
		dwPiledUpMode = o.dwPiledUpMode;
		strRecFolder = o.strRecFolder;
		wSuspendMode = o.wSuspendMode;
		bReboot = o.bReboot;
		strRecFilePath = o.strRecFilePath;
		bUseMargine = o.bUseMargine;
		iStartMargine = o.iStartMargine;
		iEndMargine = o.iEndMargine;
		dwServiceMode = o.dwServiceMode;
		return *this;
	};
} RESERVE_DATA;

typedef struct _TUNER_RESERVE_DATA{
	wstring strBonDriver;
	DWORD dwTunerID;
	vector<DWORD> ReserveIDList;
	//=オペレーターの処理
	_TUNER_RESERVE_DATA & operator= (const _TUNER_RESERVE_DATA & o) {
		strBonDriver = o.strBonDriver;
		dwTunerID = o.dwTunerID;
		ReserveIDList = o.ReserveIDList;
		return *this;
	};
} TUNER_RESERVE_DATA;

typedef struct _REC_EVENT_INFO{
	wstring strRecFilePath;
	BOOL bUseSID;//wONIDとwTSIDとwSIDとwEventIDの値が使用できるかどうか
	WORD wONID;
	WORD wTSID;
	WORD wSID;
	WORD wEventID;
	BOOL bUseBonCh;//dwSpaceとdwChの値が使用できるかどうか
	DWORD dwSpace;
	DWORD dwCh;
	SYSTEMTIME stStartTime;
	DWORD dwDurationSec;
	DWORD dwRecMode;
	BOOL bSavePgInfo;
	BOOL bSaveErrLog;
	BOOL bPittari;
	DWORD dwServiceMode;
	//=オペレーターの処理
	_REC_EVENT_INFO & operator= (const _REC_EVENT_INFO & o) {
		strRecFilePath=o.strRecFilePath;
		bUseSID = o.bUseSID;
		wONID = o.wONID;
		wTSID = o.wTSID;
		wSID = o.wSID;
		wEventID = o.wEventID;
		bUseBonCh = o.bUseBonCh;
		dwSpace = o.dwSpace;
		dwCh = o.dwCh;
		stStartTime = o.stStartTime;
		dwDurationSec = o.dwDurationSec;
		dwRecMode = o.dwRecMode;
		bSavePgInfo = o.bSavePgInfo;
		bSaveErrLog = o.bSaveErrLog;
		bPittari = o.bPittari;
		dwServiceMode = o.dwServiceMode;
		return *this;
	};
}REC_EVENT_INFO;

typedef struct _MANUAL_ADD_KEY{
	WORD wHH;
	WORD wMM;
	WORD wSS;
	DWORD dwDureSec;
	WORD wONID;
	WORD wTSID;
	WORD wSID;
	wstring strTitle;
	BOOL bChkMon;
	BOOL bChkTue;
	BOOL bChkWed;
	BOOL bChkThu;
	BOOL bChkFri;
	BOOL bChkSat;
	BOOL bChkSun;
	WORD wPri;
	WORD wRecMode;
	wstring strBat;
	wstring strRecFolder;
	WORD wSuspendMode;
	BOOL bReboot;
	DWORD dwAddKeyID;

	BOOL bUseMargine;
	int iStartMargine;
	int iEndMargine;
	DWORD dwServiceMode;
	//=オペレーターの処理
	_MANUAL_ADD_KEY & operator= (const _MANUAL_ADD_KEY & o) {
		wHH = o.wHH;
		wMM = o.wMM;
		wSS = o.wSS;
		dwDureSec = o.dwDureSec;
		wONID = o.wONID;
		wTSID = o.wTSID;
		wSID = o.wSID;
		strTitle = o.strTitle;
		bChkMon = o.bChkMon;
		bChkTue = o.bChkTue;
		bChkWed = o.bChkWed;
		bChkThu = o.bChkThu;
		bChkFri = o.bChkFri;
		bChkSat = o.bChkSat;
		bChkSun = o.bChkSun;
		wPri = o.wPri;
		wRecMode = o.wRecMode;
		strBat = o.strBat;
		strRecFolder = o.strRecFolder;
		wSuspendMode = o.wSuspendMode;
		bReboot = o.bReboot;
		dwAddKeyID = o.dwAddKeyID;

		bUseMargine = o.bUseMargine;
		iStartMargine = o.iStartMargine;
		iEndMargine = o.iEndMargine;
		dwServiceMode = o.dwServiceMode;
		return *this;
	};
}MANUAL_ADD_KEY;

//コマンド送信用
typedef struct _SET_CH_INFO{
	BOOL bUseSID;//wONIDとwTSIDとwSIDの値が使用できるかどうか
	WORD wONID;
	WORD wTSID;
	WORD wSID;
	BOOL bUseBonCh;//dwSpaceとdwChの値が使用できるかどうか
	DWORD dwSpace;
	DWORD dwCh;
	//=オペレーターの処理
	_SET_CH_INFO & operator= (const _SET_CH_INFO & o) {
		bUseSID = o.bUseSID;
		wONID = o.wONID;
		wTSID = o.wTSID;
		wSID = o.wSID;
		bUseBonCh = o.bUseBonCh;
		dwSpace = o.dwSpace;
		dwCh = o.dwCh;
		return *this;
	};
}SET_CH_INFO;

#endif
