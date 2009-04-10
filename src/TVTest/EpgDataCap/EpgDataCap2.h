#ifndef __EPG_DATA_CAP_2_H__
#define __EPG_DATA_CAP_2_H__

#include "EpgBonCommonDef.h"

//DLLの初期化
//戻り値：識別ID（-1でエラー）
typedef int (WINAPI *InitializeEP)(
	BOOL bAsyncMode //[IN]TRUE:非同期モード、FALSE:同期モード
	);

//DLLの開放
//戻り値：エラーコード
typedef DWORD (WINAPI *UnInitializeEP)(
	int iID //[IN] InitializeEPの戻り値
	);

//解析対象のTSパケットを読み込ませる
//戻り値：エラーコード
typedef DWORD (WINAPI *AddTSPacketEP)(
	int iID, //[IN] InitializeEPの戻り値
	BYTE* pbData, //[IN] TS1パケット
	DWORD dwSize //[IN] pbDataのサイズ188固定
	);

//解析データのクリア
//戻り値：なし
typedef void (WINAPI *ClearEP)(
	int iID //[IN] InitializeEPの戻り値
	);

//PFデータの取得
//戻り値：エラーコード
typedef DWORD (WINAPI *GetPFDataEP)(
	int iID, //[IN] InitializeEPの戻り値
	WORD wSID, //[IN] 番組情報取得するSID
	EPG_DATA_INFO** pstEpgData, //[OUT] 番組情報（次にGetPFDataEP呼ぶまで有効）
	BOOL bNextData //[IN] TRUE:次の番組情報、FALSE:現在の番組情報
	);

//現在の番組のEventIDを取得
//戻り値：EventID（-1でまだ解析できるパケットが入っていない）
typedef int (WINAPI *GetNowEventIDEP)(
	int iID, //[IN] InitializeEPの戻り値
	WORD wSID //[IN] 取得するSID
	);

//EPG解析モードの設定
//戻り値：なし
typedef void (WINAPI *SetPFOnlyEP)(
	int iID, //[IN] InitializeEPの戻り値
	BOOL bPFOnly //[IN] TRUE:PFデータのみ解析、FALSE:1週間分のデータも解析
	);

//EPG解析モードの設定
//戻り値：なし
typedef void (WINAPI *SetBasicModeEP)(
	int iID, //[IN] InitializeEPの戻り値
	BOOL bBasicOnly //[IN] TRUE:基本情報のみで蓄積判定、FALSE:拡張情報も蓄積判定
	);

//現在のストリームのTSIDを取得
//戻り値：TSID（-1でまだ解析できるパケットが入っていない）
typedef int (WINAPI *GetTSIDEP)(
	int iID //[IN] InitializeEPの戻り値
	);

//現在のストリームのPMTのPIDリストを取得
//戻り値：なし
typedef DWORD (WINAPI *GetPmtPIDListEP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD** pdwPIDList, //[OUT] PMTのPIDリスト（次にGetPmtPIDListEP呼ぶまで有効）
	DWORD* dwListCount, //[OUT] pdwPIDListの個数
	DWORD* dwVersion //[OUT] PATのバージョン
	);

//現在のストリームのPMTのPIDリストを取得
//戻り値：なし
typedef DWORD (WINAPI *GetPmtPIDListEP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD** pdwPIDList, //[OUT] PMTのPIDリスト（次にGetPmtPIDListEP呼ぶまで有効）
	DWORD* dwListCount, //[OUT] pdwPIDListの個数
	DWORD* dwVersion //[OUT] PATのバージョン
	);

//現在のストリームの指定SIDのPMTのPIDを取得
//戻り値：PMTのPID（-1でまだ解析できるパケットが入っていない）
typedef int (WINAPI *GetPmtPIDEP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD dwSID, //[IN] ServiceID
	DWORD* dwVersion //[OUT] PATのバージョン
	);

//現在のストリームの指定SIDのPMTのPIDを取得
//戻り値：エラーコード
typedef DWORD (WINAPI *GetESInfoEP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD dwSID, //[IN] ServiceID
	DWORD* pdwPcrPID, //[OUT] PCRのPID
	DWORD* pdwEcmPID, //[OUT] 当該サービス全体のECMのPID 存在しない場合は0x1FFF ESで個別の場合はELEMENT_INFO内のもので
	ELEMENT_INFO** pstESList, //[OUT]ES情報のリスト（次にGetESInfoEP呼ぶまで有効）
	DWORD* dwListCount, //[OUT] pstESListの個数
	DWORD* dwVersion //[OUT] PMTのバージョン
	);

//現在のストリームのEMMのPIDを取得
//戻り値：EMMのPID（-1でまだ解析できるパケットが入っていない）
typedef int (WINAPI *GetEmmPIDEP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD* dwVersion //[OUT] CATのバージョン
	);

//現在のストリームのサービスIDリストを取得
//戻り値：エラーコード
typedef DWORD (WINAPI *GetServiceListEP)(
	int iID, //[IN] InitializeEPの戻り値
	SERVICE_INFO** pstList, //[OUT] サービス一覧（次にGetServiceListEP or GetServiceListDBEP呼ぶまで有効）
	DWORD* dwListSize //[OUT] pstListの個数
	);

//現在のストリーム内で最後に解析できた時間を取得
//戻り値：FILETIMEを__int64にシフトしたもの（-1でまだ解析できるパケットが入っていない）
typedef __int64 (WINAPI *GetNowTimeEP)(
	int iID //[IN] InitializeEPの戻り値
	);

//地デジの場合、現在のストリームの放送局名
//戻り値：NULLでまだ解析できるパケットが入っていない or BS/CS
typedef const WCHAR* (WINAPI *GetTsNameEP)(
	int iID //[IN] InitializeEPの戻り値
	);

//蓄積済みのサービスIDリストを取得
//戻り値：エラーコード
typedef DWORD (WINAPI *GetServiceListDBEP)(
	int iID, //[IN] InitializeEPの戻り値
	SERVICE_INFO** pstList, //[OUT] サービス一覧（次にGetServiceListEP or GetServiceListDBEP呼ぶまで有効）
	DWORD* dwListSize //[OUT] pstListの個数
	);

//蓄積済みの番組情報を取得
//戻り値：TRUE:成功、FALSE:失敗
typedef BOOL (WINAPI *GetEpgDataListDBEP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
	DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
	DWORD dwSID, //[IN] 取得したいサービスのServiceID
	EPG_DATA_INFO2** pstList, //[OUT] 番組情報一覧（次にGetEpgDataListDBEP呼ぶまで有効）
	DWORD* dwListSize //[OUT] pstListの個数
	);

//蓄積済みの番組情報から指定EventIDの番組情報を取得
//戻り値：TRUE:成功、FALSE:失敗
typedef BOOL (WINAPI *GetEpgDataDBEP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
	DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
	DWORD dwSID, //[IN] 取得したいサービスのServiceID
	DWORD dwEID, //[IN] 取得したいサービスのEventID
	EPG_DATA_INFO2** pstEpgData //[OUT] 番組情報（次にGetEpgDataDBEP呼ぶまで有効）
	);

//蓄積済みの番組情報を取得
//戻り値：TRUE:成功、FALSE:失敗
typedef BOOL (WINAPI *GetEpgDataListDB2EP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
	DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
	DWORD dwSID, //[IN] 取得したいサービスのServiceID
	EPG_DATA_INFO3** pstList, //[OUT] 番組情報一覧（次にGetEpgDataListDB2EP呼ぶまで有効）
	DWORD* dwListSize //[OUT] pstListの個数
	);

//蓄積済みの番組情報から指定EventIDの番組情報を取得
//戻り値：TRUE:成功、FALSE:失敗
typedef BOOL (WINAPI *GetEpgDataDB2EP)(
	int iID, //[IN] InitializeEPの戻り値
	DWORD dwONID, //[IN] 取得したいサービスのOriginalNetworkID
	DWORD dwTSID, //[IN] 取得したいサービスのTransportStreamID
	DWORD dwSID, //[IN] 取得したいサービスのServiceID
	DWORD dwEID, //[IN] 取得したいサービスのEventID
	EPG_DATA_INFO3** pstEpgData //[OUT] 番組情報（次にGetEpgDataDB2EP呼ぶまで有効）
	);

//PFデータの取得
//戻り値：エラーコード
typedef DWORD (WINAPI *GetPFData2EP)(
	int iID, //[IN] InitializeEPの戻り値
	WORD wSID, //[IN] 番組情報取得するSID
	EPG_DATA_INFO3** pstEpgData, //[OUT] 番組情報（次にGetPFData2EP呼ぶまで有効）
	BOOL bNextData //[IN] TRUE:次の番組情報、FALSE:現在の番組情報
	);

//蓄積済みの番組情報のクリア
//戻り値：なし
typedef void (WINAPI *ClearDBEP)(
	int iID //[IN] InitializeEPの戻り値
	);

#endif