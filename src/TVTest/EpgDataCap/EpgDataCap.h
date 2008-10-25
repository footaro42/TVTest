#ifndef __EPG_DATA_CAP_H__
#define __EPG_DATA_CAP_H__

#include "EpgDataCapDef.h"

//DLLの初期化
//戻り値：エラーコード
typedef DWORD (WINAPI *InitializeEP)(
	BOOL bAsyncMode
	);

//DLLの開放
//戻り値：エラーコード
typedef DWORD (WINAPI *UnInitializeEP)();

//TSパケットを読み込ませる
//戻り値：エラーコード
typedef DWORD (WINAPI *AddTSPacketEP)(
	BYTE* pbData,
	DWORD dwSize
	);

//PFデータの取得
//戻り値：エラーコード
typedef DWORD (WINAPI *GetPFDataEP)(
	WORD wSID,
	EPG_DATA_INFO* pstEpgData,
	BOOL bNextData
	);

//PFデータの取得に使用したメモリの開放
//戻り値：エラーコード
typedef DWORD (WINAPI *ReleasePFDataEP)(
	EPG_DATA_INFO* pstEpgData
	);

typedef void (WINAPI *ClearDataEP)(
	);

typedef void (WINAPI *SetPFOnlyEP)(
	BOOL bPFOnly
	);

typedef void (WINAPI *SetBasicModeEP)(
	BOOL bBasicOnly
	);

typedef int (WINAPI *GetTSIDEP)();

typedef __int64 (WINAPI *GetNowTimeEP)();

typedef DWORD (WINAPI *GetEpgCapStatusEP)();

typedef int (WINAPI *GetDefSIDEP)();

typedef const WCHAR* (WINAPI *GetTsNameEP)();

typedef DWORD (WINAPI *GetServiceListEP)(
	SERVICE_INFO** pstList,
	DWORD* dwListSize
	);

typedef void (WINAPI *ReleaseServiceListEP)();

typedef void (WINAPI *ClearBuffEP)();

typedef int (WINAPI *GetPmtPIDListEP)(DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);

typedef int (WINAPI *GetPCRPIDListEP)(DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);

typedef int (WINAPI *GetElementIDEP)(DWORD dwSID, DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);

typedef int (WINAPI *GetPCRPIDEP)(DWORD dwSID, DWORD* dwPID, DWORD* dwVersion);

typedef int (WINAPI *GetPmtPIDEP)(DWORD dwSID, DWORD* dwPID, DWORD* dwVersion);

typedef int (WINAPI *GetEmmPIDEP)();

typedef int (WINAPI *GetNowEventIDEP)(DWORD dwSID);

typedef BOOL (WINAPI *GetServiceListDBEP)(SERVICE_INFO** pstList, DWORD* dwListSizeD);

typedef BOOL (WINAPI *GetEpgDataListDBEP)(DWORD dwONID, DWORD dwTSID, DWORD dwSID, EPG_DATA_INFO2** pstList, DWORD* dwListSize);

typedef int (WINAPI *GetLowQualityElementIDEP)(DWORD dwSID, DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);

#endif