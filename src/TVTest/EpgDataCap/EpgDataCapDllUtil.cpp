/*
	EpgDataCapの中の人氏が書いたコードに、EpgDataCap.dllのパスを指定できるように
	小改造したものです
*/

#include "StdAfx.h"
#include "EpgDataCapDllUtil.h"

CEpgDataCapDllUtil::CEpgDataCapDllUtil(void)
{
	m_hModule = NULL;
}

CEpgDataCapDllUtil::~CEpgDataCapDllUtil(void)
{
	UnLoadDll();
}

BOOL CEpgDataCapDllUtil::LoadDll(LPCTSTR pszDllFileName)
{
	if( m_hModule != NULL ){
		return FALSE;
	}

	pfnInitializeEP = NULL;
	pfnUnInitializeEP = NULL;
	pfnAddTSPacketEP = NULL;
	pfnGetPFDataEP = NULL;
	pfnReleasePFDataEP = NULL;
	pfnClearDataEP = NULL;
	pfnSetPFOnlyEP = NULL;
	pfnSetBasicModeEP = NULL;
	pfnGetTSIDEP = NULL;
	pfnGetNowTimeEP = NULL;
	pfnGetEpgCapStatusEP = NULL;
	pfnGetDefSIDEP = NULL;
	pfnGetServiceList = NULL;
	pfnReleaseServiceList = NULL;
	pfnGetTsNameEP = NULL;
	pfnGetPmtPIDList = NULL;
	pfnGetPCRPIDList = NULL;
	pfnGetElementID = NULL;
	pfnGetNowEventID = NULL;
	pfnGetServiceListDB = NULL;
	pfnGetEpgDataListDB = NULL;
	pfnGetLowQualityElementIDEP = NULL;


	BOOL bRet = TRUE;

	m_hModule = ::LoadLibrary(pszDllFileName);

	if( m_hModule == NULL ){
		//AfxMessageBox( L"EpgDataCap.dll のロードに失敗しました");
		return FALSE;
	}

	pfnInitializeEP = ( InitializeEP ) ::GetProcAddress( m_hModule , "InitializeEP");
	if( !pfnInitializeEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnUnInitializeEP = ( UnInitializeEP ) ::GetProcAddress( m_hModule , "UnInitializeEP");
	if( !pfnUnInitializeEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnAddTSPacketEP = ( AddTSPacketEP ) ::GetProcAddress( m_hModule , "AddTSPacketEP");
	if( !pfnAddTSPacketEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetPFDataEP = ( GetPFDataEP ) ::GetProcAddress( m_hModule , "GetPFDataEP");
	if( !pfnGetPFDataEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnReleasePFDataEP = ( ReleasePFDataEP ) ::GetProcAddress( m_hModule , "ReleasePFDataEP");
	if( !pfnReleasePFDataEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnClearDataEP = ( ClearDataEP ) ::GetProcAddress( m_hModule , "ClearDataEP");
	if( !pfnClearDataEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnSetPFOnlyEP = ( SetPFOnlyEP ) ::GetProcAddress( m_hModule , "SetPFOnlyEP");
	if( !pfnSetPFOnlyEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnSetBasicModeEP = ( SetBasicModeEP ) ::GetProcAddress( m_hModule , "SetBasicModeEP");
	if( !pfnSetBasicModeEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetTSIDEP = ( GetTSIDEP ) ::GetProcAddress( m_hModule , "GetTSIDEP");
	if( !pfnGetTSIDEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetNowTimeEP = ( GetNowTimeEP ) ::GetProcAddress( m_hModule , "GetNowTimeEP");
	if( !pfnGetNowTimeEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetEpgCapStatusEP = ( GetEpgCapStatusEP ) ::GetProcAddress( m_hModule , "GetEpgCapStatusEP");
	if( !pfnGetEpgCapStatusEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetDefSIDEP = ( GetDefSIDEP ) ::GetProcAddress( m_hModule , "GetDefSIDEP");
	if( !pfnGetDefSIDEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetTsNameEP = ( GetTsNameEP ) ::GetProcAddress( m_hModule , "GetTsNameEP");
	if( !pfnGetTsNameEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetServiceList = ( GetServiceListEP ) ::GetProcAddress( m_hModule , "GetServiceListEP");
	if( !pfnGetServiceList ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnReleaseServiceList = ( ReleaseServiceListEP ) ::GetProcAddress( m_hModule , "ReleaseServiceListEP");
	if( !pfnReleaseServiceList ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnClearBuff = ( ClearBuffEP ) ::GetProcAddress( m_hModule , "ClearBuffEP");
	if( !pfnClearBuff ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetPmtPIDList = ( GetPmtPIDListEP ) ::GetProcAddress( m_hModule , "GetPmtPIDListEP");
	if( !pfnGetPmtPIDList ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetPCRPIDList = ( GetPCRPIDListEP ) ::GetProcAddress( m_hModule , "GetPCRPIDListEP");
	if( !pfnGetPCRPIDList ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetElementID = ( GetElementIDEP ) ::GetProcAddress( m_hModule , "GetElementIDEP");
	if( !pfnGetElementID ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetPCRPID = ( GetPCRPIDEP ) ::GetProcAddress( m_hModule , "GetPCRPIDEP");
	if( !pfnGetElementID ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetPmtPID = ( GetPmtPIDEP ) ::GetProcAddress( m_hModule , "GetPmtPIDEP");
	if( !pfnGetPmtPID ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetEmmPID = ( GetEmmPIDEP ) ::GetProcAddress( m_hModule , "GetEmmPIDEP");
	if( !pfnGetEmmPID ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetNowEventID = ( GetNowEventIDEP ) ::GetProcAddress( m_hModule , "GetNowEventIDEP");
	if( !pfnGetNowEventID ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetServiceListDB = ( GetServiceListDBEP ) ::GetProcAddress( m_hModule , "GetServiceListDBEP");
	if( !pfnGetServiceListDB ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetEpgDataListDB = ( GetEpgDataListDBEP ) ::GetProcAddress( m_hModule , "GetEpgDataListDBEP");
	if( !pfnGetEpgDataListDB ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetLowQualityElementIDEP = ( GetLowQualityElementIDEP ) ::GetProcAddress( m_hModule , "GetLowQualityElementIDEP");
	if( !pfnGetLowQualityElementIDEP ){
		bRet = FALSE;
		goto ERR_END;
	}

ERR_END:
	if( bRet == FALSE ){
		::FreeLibrary( m_hModule );
		m_hModule=NULL;
		//AfxMessageBox( L"GetProcAddress に失敗しました");
	}
	return bRet;
}

BOOL CEpgDataCapDllUtil::UnLoadDll(void)
{
	if( m_hModule != NULL ){
		pfnUnInitializeEP();
		::FreeLibrary( m_hModule );
	}
	m_hModule=NULL;
	return TRUE;
}

DWORD CEpgDataCapDllUtil::Initialize(LPCTSTR pszDllFileName,BOOL bAsyncMode)
{
	if( LoadDll(pszDllFileName) == FALSE ){
		return ERR_INIT;
	}
	return pfnInitializeEP(bAsyncMode);
}

DWORD CEpgDataCapDllUtil::UnInitialize()
{
	if( m_hModule == NULL ){
		return ERR_INIT;
	}
	DWORD dwRet = pfnUnInitializeEP();
	UnLoadDll();
	return dwRet;
}

DWORD CEpgDataCapDllUtil::AddTSPacket(
	BYTE* pbData,
	DWORD dwSize
	)
{
	if( m_hModule == NULL ){
		return ERR_INIT;
	}
	return pfnAddTSPacketEP(pbData,dwSize);
}

DWORD CEpgDataCapDllUtil::GetPFData(
	WORD wSID,
	EPG_DATA_INFO* pstEpgData,
	BOOL bNextData
	)
{
	if( m_hModule == NULL ){
		return ERR_INIT;
	}

	return pfnGetPFDataEP(wSID,pstEpgData,bNextData);
}

DWORD CEpgDataCapDllUtil::ReleasePFData(
	EPG_DATA_INFO* pstEpgData
	)
{
	if( m_hModule == NULL ){
		return ERR_INIT;
	}

	return pfnReleasePFDataEP(pstEpgData);
}

void CEpgDataCapDllUtil::ClearData()
{
	if( m_hModule == NULL ){
		return ;
	}

	pfnClearDataEP();
}

void CEpgDataCapDllUtil::SetPFOnly( BOOL bPFOnly )
{
	if( m_hModule == NULL ){
		return ;
	}

	pfnSetPFOnlyEP(bPFOnly);
}

void CEpgDataCapDllUtil::SetBasicMode( BOOL bBasicOnly )
{
	if( m_hModule == NULL ){
		return ;
	}

	pfnSetBasicModeEP(bBasicOnly);
}

int CEpgDataCapDllUtil::GetTSID()
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetTSIDEP();
}

__int64 CEpgDataCapDllUtil::GetNowTime()
{
	if( m_hModule == NULL ){
		return ERR_INIT;
	}
	return pfnGetNowTimeEP();
}

DWORD CEpgDataCapDllUtil::GetEpgCapStatus()
{
	if( m_hModule == NULL ){
		return ERR_INIT;
	}
	return pfnGetEpgCapStatusEP();
}

int CEpgDataCapDllUtil::GetDefSID()
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetDefSIDEP();
}

const WCHAR* CEpgDataCapDllUtil::GetTsName()
{
	if( m_hModule == NULL ){
		return NULL;
	}
	return pfnGetTsNameEP();
}

DWORD CEpgDataCapDllUtil::GetServiceList(
	SERVICE_INFO** pstList,
	DWORD* dwListSize
	)
{
	if( m_hModule == NULL ){
		return ERR_INIT;
	}
	return pfnGetServiceList(pstList,dwListSize);
}

void CEpgDataCapDllUtil::ReleaseServiceList()
{
	if( m_hModule == NULL ){
		return ;
	}
	pfnReleaseServiceList();
}

void CEpgDataCapDllUtil::ClearBuff()
{
	if( m_hModule == NULL ){
		return ;
	}
	pfnClearBuff();
}


int CEpgDataCapDllUtil::GetPmtPIDList(DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetPmtPIDList(dwPIDList, dwListCount,dwVersion);
}

int CEpgDataCapDllUtil::GetPCRPIDList(DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetPCRPIDList(dwPIDList,dwListCount,dwVersion);
}

int CEpgDataCapDllUtil::GetElementID(DWORD dwSID, DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetElementID(dwSID, dwPIDList,dwListCount,dwVersion);
}

int CEpgDataCapDllUtil::GetPCRPID(DWORD dwSID, DWORD* dwPID, DWORD* dwVersion)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetPCRPID(dwSID, dwPID,dwVersion);
}

int CEpgDataCapDllUtil::GetPmtPID(DWORD dwSID, DWORD* dwPID, DWORD* dwVersion)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetPmtPID(dwSID, dwPID,dwVersion);
}

int CEpgDataCapDllUtil::GetEmmPID()
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetEmmPID();
}

int CEpgDataCapDllUtil::GetNowEventID(DWORD dwSID)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetNowEventID(dwSID);
}

int CEpgDataCapDllUtil::GetServiceListDB(SERVICE_INFO** pstList, DWORD* dwListSize)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetServiceListDB(pstList,dwListSize);
}

int CEpgDataCapDllUtil::GetEpgDataListDB(DWORD dwONID, DWORD dwTSID, DWORD dwSID, EPG_DATA_INFO2** pstList, DWORD* dwListSize)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetEpgDataListDB(dwONID, dwTSID, dwSID, pstList, dwListSize);
}

int CEpgDataCapDllUtil::GetLowQualityElementID(DWORD dwSID, DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion)
{
	if( m_hModule == NULL ){
		return -1;
	}
	return pfnGetLowQualityElementIDEP(dwSID, dwPIDList,dwListCount,dwVersion);
}
