#pragma once

#include "EpgDataCap.h"

class CEpgDataCapDllUtil
{
public:
	CEpgDataCapDllUtil(void);
	~CEpgDataCapDllUtil(void);

	DWORD Initialize(LPCTSTR pszDllFileName,BOOL bAsyncMode=TRUE);
	DWORD UnInitialize();
	DWORD AddTSPacket(
		BYTE* pbData,
		DWORD dwSize
		);
	DWORD GetPFData(
		WORD wSID,
		EPG_DATA_INFO* pstEpgData,
		BOOL bNextData
		);
	DWORD ReleasePFData(
		EPG_DATA_INFO* pstEpgData
		);

	void ClearData();
	void SetPFOnly( BOOL bPFOnly );
	void SetBasicMode( BOOL bBasicOnly );
	int GetTSID();
	__int64 GetNowTime();

	DWORD GetEpgCapStatus();
	int GetDefSID();

	const WCHAR* GetTsName();
	
	DWORD GetServiceList(
		SERVICE_INFO** pstList,
		DWORD* dwListSize
		);
	void ReleaseServiceList();
	void ClearBuff();

	int GetPmtPIDList(DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);
	int GetPCRPIDList(DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);
	int GetElementID(DWORD dwSID, DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);

	int GetPCRPID(DWORD dwSID, DWORD* dwPID, DWORD* dwVersion);
	int GetPmtPID(DWORD dwSID, DWORD* dwPID, DWORD* dwVersion);
	int GetEmmPID();
	int GetNowEventID(DWORD dwSID);

	BOOL GetServiceListDB(SERVICE_INFO** pstList, DWORD* dwListSize);
	BOOL GetEpgDataListDB(DWORD dwONID, DWORD dwTSID, DWORD dwSID, EPG_DATA_INFO2** pstList, DWORD* dwListSize);

	int GetLowQualityElementID(DWORD dwSID, DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion);

protected:
	HMODULE m_hModule;

	InitializeEP pfnInitializeEP;
	UnInitializeEP pfnUnInitializeEP;
	AddTSPacketEP pfnAddTSPacketEP;
	GetPFDataEP pfnGetPFDataEP;
	ReleasePFDataEP pfnReleasePFDataEP;
	ClearDataEP pfnClearDataEP;
	SetPFOnlyEP pfnSetPFOnlyEP;
	SetBasicModeEP pfnSetBasicModeEP;
	GetTSIDEP pfnGetTSIDEP;
	GetNowTimeEP pfnGetNowTimeEP;
	GetEpgCapStatusEP pfnGetEpgCapStatusEP;
	GetDefSIDEP pfnGetDefSIDEP;
	GetTsNameEP pfnGetTsNameEP;
	GetServiceListEP pfnGetServiceList;
	ReleaseServiceListEP pfnReleaseServiceList;
	ClearBuffEP pfnClearBuff;
	GetPmtPIDListEP pfnGetPmtPIDList;
	GetPCRPIDListEP pfnGetPCRPIDList;
	GetElementIDEP pfnGetElementID;
	GetPCRPIDEP pfnGetPCRPID;
	GetPmtPIDEP pfnGetPmtPID;
	GetEmmPIDEP pfnGetEmmPID;
	GetNowEventIDEP pfnGetNowEventID;
	GetServiceListDBEP pfnGetServiceListDB;
	GetEpgDataListDBEP pfnGetEpgDataListDB;
	GetLowQualityElementIDEP pfnGetLowQualityElementIDEP;


protected:
	BOOL LoadDll(LPCTSTR pszDllFileName);
	BOOL UnLoadDll(void);
};
