#ifndef __EPG_DATA_CAP_DLL_UTIL_2_H__
#define __EPG_DATA_CAP_DLL_UTIL_2_H__

#include "EpgDataCap2.h"

class CEpgDataCapDllUtil2
{
public:
	CEpgDataCapDllUtil2(void);
	~CEpgDataCapDllUtil2(void);

	//DLLの初期化
	//戻り値：エラーコード
	DWORD Initialize(
		LPCTSTR pszFileName, //[IN] ファイルパス(by HDUSTestの中の人)
		BOOL bAsyncMode //[IN]TRUE:非同期モード、FALSE:同期モード
		);

	//DLLの開放
	//戻り値：なし
	void UnInitialize(
		);

	//ロード済みか取得(by HDUSTestの中の人)
	bool IsLoaded() const;

	//解析対象のTSパケットを読み込ませる
	//戻り値：エラーコード
	DWORD AddTSPacket(
		BYTE* pbData, //[IN] TSパケット
		DWORD dwSize //[IN] pbDataのサイズ（188の倍数）
		);

	//解析データのクリア
	//戻り値：なし
	void Clear(
		);

	//PFデータの取得
	//戻り値：エラーコード
	DWORD GetPFData(
		WORD wSID, //[IN] 番組情報取得するSID
		EPG_DATA_INFO** pstEpgData, //[OUT] 番組情報（次にGetPFDataEP呼ぶまで有効）
		BOOL bNextData //[IN] TRUE:次の番組情報、FALSE:現在の番組情報
		);

	//現在の番組のEventIDを取得
	//戻り値：EventID（-1でまだ解析できるパケットが入っていない）
	int GetNowEventID(
		WORD wSID //[IN] 取得するSID
		);

	//EPG解析モードの設定
	//戻り値：なし
	void SetPFOnly(
		BOOL bPFOnly //[IN] TRUE:PFデータのみ解析、FALSE:1週間分のデータも解析
		);

	//EPG解析モードの設定
	//戻り値：なし
	void SetBasicMode(
		BOOL bBasicOnly //[IN] TRUE:基本情報のみで蓄積判定、FALSE:拡張情報も蓄積判定
		);

	//現在のストリームのTSIDを取得
	//戻り値：TSID（-1でまだ解析できるパケットが入っていない）
	int GetTSID(
		);

	//現在のストリームのPMTのPIDリストを取得
	//戻り値：なし
	DWORD GetPmtPIDList(
		DWORD** pdwPIDList, //[OUT] PMTのPIDリスト（次にGetPmtPIDListEP呼ぶまで有効）
		DWORD* dwListCount, //[OUT] pdwPIDListの個数
		DWORD* dwVersion //[OUT] PATのバージョン
		);

	//現在のストリームの指定SIDのPMTのPIDを取得
	//戻り値：PMTのPID（-1でまだ解析できるパケットが入っていない）
	int GetPmtPID(
		DWORD dwSID, //[IN] ServiceID
		DWORD* dwVersion //[OUT] PATのバージョン
		);

	//現在のストリームの指定SIDのPMTのPIDを取得
	//戻り値：エラーコード
	DWORD GetESInfo(
		DWORD dwSID, //[IN] ServiceID
		DWORD* pdwPcrPID, //[OUT] PCRのPID
		DWORD* pdwEcmPID, //[OUT] 当該サービス全体のECMのPID 存在しない場合は0x1FFF ESで個別の場合はELEMENT_INFO内のもので
		ELEMENT_INFO** pstESList, //[OUT]ES情報のリスト（次にGetESInfoEP呼ぶまで有効）
		DWORD* dwListCount, //[OUT] pstESListの個数
		DWORD* dwVersion //[OUT] PMTのバージョン
		);

	//現在のストリームのEMMのPIDを取得
	//戻り値：EMMのPID（-1でまだ解析できるパケットが入っていない）
	int GetEmmPID(
		DWORD* dwVersion //[OUT] CATのバージョン
		);

	//現在のストリームのサービスIDリストを取得
	//戻り値：エラーコード
	DWORD GetServiceList(
		SERVICE_INFO** pstList, //[OUT] サービス一覧（次にGetServiceList or GetServiceListDB呼ぶまで有効）
		DWORD* dwListSize //[OUT] pstListの個数
		);

	//現在のストリーム内で最後に解析できた時間を取得
	//戻り値：FILETIMEを__int64にシフトしたもの（-1でまだ解析できるパケットが入っていない）
	__int64 GetNowTime(
		);

	//地デジの場合、現在のストリームの放送局名
	//戻り値：NULLでまだ解析できるパケットが入っていない or BS/CS
	const WCHAR* GetTsName(
		);

	//蓄積済みのサービスIDリストを取得
	//戻り値：エラーコード
	DWORD GetServiceListDB(
		SERVICE_INFO** pstList, //[OUT] サービス一覧（次にGetServiceList or GetServiceListDB呼ぶまで有効）
		DWORD* dwListSize //[OUT] pstListの個数
		);

	//蓄積済みの番組情報を取得
	//戻り値：TRUE:成功、FALSE:失敗
	BOOL GetEpgDataListDB(
		DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
		DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
		DWORD dwSID, //[IN] 取得したいサービスのServiceID
		EPG_DATA_INFO2** pstList, //[OUT] 番組情報一覧（次にGetEpgDataListDBEP呼ぶまで有効）
		DWORD* dwListSize //[OUT] pstListの個数
		);

	//蓄積済みの番組情報から指定EventIDの番組情報を取得
	//戻り値：TRUE:成功、FALSE:失敗
	BOOL GetEpgDataDB(
		DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
		DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
		DWORD dwSID, //[IN] 取得したいサービスのServiceID
		DWORD dwEID, //[IN] 取得したいサービスのEventID
		EPG_DATA_INFO2** pstEpgData //[OUT] 番組情報（次にGetEpgDataDBEP呼ぶまで有効）
		);

	//蓄積済みの番組情報を取得
	//戻り値：TRUE:成功、FALSE:失敗
	BOOL GetEpgDataListDB2(
		DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
		DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
		DWORD dwSID, //[IN] 取得したいサービスのServiceID
		EPG_DATA_INFO3** pstList, //[OUT] 番組情報一覧（次にGetEpgDataListDB2EP呼ぶまで有効）
		DWORD* dwListSize //[OUT] pstListの個数
		);

	//蓄積済みの番組情報から指定EventIDの番組情報を取得
	//戻り値：TRUE:成功、FALSE:失敗
	BOOL GetEpgDataDB2(
		DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
		DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
		DWORD dwSID, //[IN] 取得したいサービスのServiceID
		DWORD dwEID, //[IN] 取得したいサービスのEventID
		EPG_DATA_INFO3** pstEpgData //[OUT] 番組情報（次にGetEpgDataDB2EP呼ぶまで有効）
		);

	//PFデータの取得
	//戻り値：エラーコード
	DWORD GetPFData2(
		WORD wSID, //[IN] 番組情報取得するSID
		EPG_DATA_INFO3** pstEpgData, //[OUT] 番組情報（次にGetPFData2EP呼ぶまで有効）
		BOOL bNextData //[IN] TRUE:次の番組情報、FALSE:現在の番組情報
		);

	//蓄積済みの番組情報のクリア
	//戻り値：なし
	void ClearDB(
		);
protected:
	HMODULE m_hModule;
	int m_iID;

	InitializeEP pfnInitializeEP;
	UnInitializeEP pfnUnInitializeEP;
	AddTSPacketEP pfnAddTSPacketEP;
	ClearEP pfnClearEP;
	GetPFDataEP pfnGetPFDataEP;
	SetPFOnlyEP pfnSetPFOnlyEP;
	SetBasicModeEP pfnSetBasicModeEP;
	GetTSIDEP pfnGetTSIDEP;
	GetNowTimeEP pfnGetNowTimeEP;
	GetTsNameEP pfnGetTsNameEP;
	GetServiceListEP pfnGetServiceList;
	GetPmtPIDListEP pfnGetPmtPIDList;
	GetServiceListDBEP pfnGetServiceListDB;
	GetEpgDataListDBEP pfnGetEpgDataListDB;
	GetPmtPIDEP pfnGetPmtPID;
	GetESInfoEP pfnGetESInfo;
	GetEmmPIDEP pfnGetEmmPID;
	GetNowEventIDEP pfnGetNowEventID;
	GetEpgDataDBEP pfnGetEpgDataDB;
	ClearDBEP pfnClearDB;
	GetEpgDataListDB2EP pfnGetEpgDataListDB2;
	GetEpgDataDB2EP pfnGetEpgDataDB2;
	GetPFData2EP pfnGetPFData2;


protected:
	BOOL LoadDll(LPCTSTR pszFileName);
	BOOL UnLoadDll(void);
};

#endif
