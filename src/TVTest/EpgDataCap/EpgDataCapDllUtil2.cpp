#include "stdafx.h"
#include "EpgDataCapDllUtil2.h"

CEpgDataCapDllUtil2::CEpgDataCapDllUtil2(void)
{
	m_hModule = NULL;
	m_iID = -1;
}

CEpgDataCapDllUtil2::~CEpgDataCapDllUtil2(void)
{
	UnLoadDll();
}

BOOL CEpgDataCapDllUtil2::LoadDll(LPCTSTR pszFileName)
{
	if( m_hModule != NULL ){
		return FALSE;
	}

	pfnInitializeEP = NULL;
	pfnUnInitializeEP = NULL;
	pfnAddTSPacketEP = NULL;
	pfnClearEP = NULL;
	pfnGetPFDataEP = NULL;
	pfnSetPFOnlyEP = NULL;
	pfnSetBasicModeEP = NULL;
	pfnGetTSIDEP = NULL;
	pfnGetNowTimeEP = NULL;
	pfnGetServiceList = NULL;
	pfnGetTsNameEP = NULL;
	pfnGetPmtPIDList = NULL;
	pfnGetServiceListDB = NULL;
	pfnGetEpgDataListDB = NULL;
	pfnGetPmtPID = NULL;
	pfnGetESInfo = NULL;
	pfnGetEmmPID = NULL;
	pfnGetNowEventID = NULL;
	pfnGetEpgDataDB = NULL;
	pfnClearDB = NULL;
	pfnGetEpgDataListDB2 = NULL;
	pfnGetEpgDataDB2 = NULL;
	pfnGetPFData2 = NULL;

	BOOL bRet = TRUE;

	m_hModule = ::LoadLibrary(pszFileName);

	if( m_hModule == NULL ){
//		::MessageBox( NULL, L"EpgDataCap.dll ‚Ìƒ[ƒh‚ÉŽ¸”s‚µ‚Ü‚µ‚½", NULL, MB_OK);
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
	pfnClearEP = ( ClearEP ) ::GetProcAddress( m_hModule , "ClearEP");
	if( !pfnClearEP ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetPFDataEP = ( GetPFDataEP ) ::GetProcAddress( m_hModule , "GetPFDataEP");
	if( !pfnGetPFDataEP ){
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
	pfnGetPmtPIDList = ( GetPmtPIDListEP ) ::GetProcAddress( m_hModule , "GetPmtPIDListEP");
	if( !pfnGetPmtPIDList ){
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
	pfnGetPmtPID = ( GetPmtPIDEP ) ::GetProcAddress( m_hModule , "GetPmtPIDEP");
	if( !pfnGetPmtPID ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetESInfo = ( GetESInfoEP ) ::GetProcAddress( m_hModule , "GetESInfoEP");
	if( !pfnGetESInfo ){
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
	pfnGetEpgDataDB = ( GetEpgDataDBEP ) ::GetProcAddress( m_hModule , "GetEpgDataDBEP");
	if( !pfnGetEpgDataDB ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetEpgDataListDB2 = ( GetEpgDataListDB2EP ) ::GetProcAddress( m_hModule , "GetEpgDataListDB2EP");
	if( !pfnGetEpgDataListDB2 ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetEpgDataDB2 = ( GetEpgDataDB2EP ) ::GetProcAddress( m_hModule , "GetEpgDataDB2EP");
	if( !pfnGetEpgDataDB2 ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnGetPFData2 = ( GetPFData2EP ) ::GetProcAddress( m_hModule , "GetPFData2EP");
	if( !pfnGetPFData2 ){
		bRet = FALSE;
		goto ERR_END;
	}
	pfnClearDB = ( ClearDBEP ) ::GetProcAddress( m_hModule , "ClearDBEP");
	if( !pfnClearDB ){
		bRet = FALSE;
		goto ERR_END;
	}

ERR_END:
	if( bRet == FALSE ){
		::FreeLibrary( m_hModule );
		m_hModule=NULL;
//		::MessageBox( NULL, L"GetProcAddress ‚ÉŽ¸”s‚µ‚Ü‚µ‚½", NULL, MB_OK);
	}
	return bRet;
}

BOOL CEpgDataCapDllUtil2::UnLoadDll(void)
{
	if( m_hModule != NULL ){
		if( m_iID != -1 ){
			pfnUnInitializeEP(m_iID);
		}
		::FreeLibrary( m_hModule );
		m_iID = -1;
	}
	m_hModule=NULL;
	return TRUE;
}

DWORD CEpgDataCapDllUtil2::Initialize(LPCTSTR pszFileName, BOOL bAsyncMode)
{
	if( LoadDll(pszFileName) == FALSE ){
		return ERR_INIT;
	}
	m_iID = pfnInitializeEP(bAsyncMode);
	if( m_iID == -1 ){
		return ERR_INIT;
	}
	return NO_ERR;
}

void CEpgDataCapDllUtil2::UnInitialize()
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ;
	}
	DWORD dwRet = pfnUnInitializeEP(m_iID);
	m_iID = -1;
	UnLoadDll();
	return ;
}

bool CEpgDataCapDllUtil2::IsLoaded() const
{
	return m_hModule != NULL && m_iID >= 0;
}

DWORD CEpgDataCapDllUtil2::AddTSPacket(
	BYTE* pbData,
	DWORD dwSize
	)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ERR_INIT;
	}
	return pfnAddTSPacketEP(m_iID, pbData,dwSize);
}

void CEpgDataCapDllUtil2::Clear()
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ;
	}
	pfnClearEP(m_iID);
}

DWORD CEpgDataCapDllUtil2::GetPFData(
	WORD wSID,
	EPG_DATA_INFO** pstEpgData,
	BOOL bNextData
	)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ERR_INIT;
	}

	return pfnGetPFDataEP(m_iID, wSID,pstEpgData,bNextData);
}

void CEpgDataCapDllUtil2::SetPFOnly( BOOL bPFOnly )
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ;
	}

	pfnSetPFOnlyEP(m_iID, bPFOnly);
}

void CEpgDataCapDllUtil2::SetBasicMode( BOOL bBasicOnly )
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ;
	}

	pfnSetBasicModeEP(m_iID, bBasicOnly);
}

int CEpgDataCapDllUtil2::GetTSID()
{
	if( m_hModule == NULL || m_iID == -1 ){
		return -1;
	}
	return pfnGetTSIDEP(m_iID);
}

__int64 CEpgDataCapDllUtil2::GetNowTime()
{
	if( m_hModule == NULL || m_iID == -1 ){
		return -1;
	}
	return pfnGetNowTimeEP(m_iID);
}

const WCHAR* CEpgDataCapDllUtil2::GetTsName()
{
	if( m_hModule == NULL || m_iID == -1 ){
		return NULL;
	}
	return pfnGetTsNameEP(m_iID);
}

DWORD CEpgDataCapDllUtil2::GetServiceList(
	SERVICE_INFO** pstList,
	DWORD* dwListSize
	)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ERR_INIT;
	}
	return pfnGetServiceList(m_iID, pstList,dwListSize);
}

DWORD CEpgDataCapDllUtil2::GetPmtPIDList(DWORD** dwPIDList, DWORD* dwListCount, DWORD* dwVersion)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ERR_INIT;
	}
	return pfnGetPmtPIDList(m_iID, dwPIDList, dwListCount,dwVersion);
}

DWORD CEpgDataCapDllUtil2::GetServiceListDB(SERVICE_INFO** pstList, DWORD* dwListSize)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ERR_INIT;
	}
	return pfnGetServiceListDB(m_iID, pstList,dwListSize);
}

BOOL CEpgDataCapDllUtil2::GetEpgDataListDB(DWORD dwONID, DWORD dwTSID, DWORD dwSID, EPG_DATA_INFO2** pstList, DWORD* dwListSize)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return FALSE;
	}
	return pfnGetEpgDataListDB(m_iID, dwONID, dwTSID, dwSID, pstList, dwListSize);
}

int CEpgDataCapDllUtil2::GetPmtPID( DWORD dwSID, DWORD* dwVersion )
{
	if( m_hModule == NULL || m_iID == -1 ){
		return -1;
	}
	return pfnGetPmtPID(m_iID, dwSID, dwVersion);
}

DWORD CEpgDataCapDllUtil2::GetESInfo( DWORD dwSID, DWORD* pdwPcrPID, DWORD* pdwEcmPID, ELEMENT_INFO** pstESList, DWORD* dwListCount, DWORD* dwVersion )
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ERR_INIT;
	}
	return pfnGetESInfo(m_iID, dwSID, pdwPcrPID, pdwEcmPID, pstESList, dwListCount, dwVersion);
}

int CEpgDataCapDllUtil2::GetEmmPID( DWORD* dwVersion )
{
	if( m_hModule == NULL || m_iID == -1 ){
		return -1;
	}
	return pfnGetEmmPID(m_iID, dwVersion);
}

int CEpgDataCapDllUtil2::GetNowEventID( WORD wSID )
{
	if( m_hModule == NULL || m_iID == -1 ){
		return -1;
	}
	return pfnGetNowEventID(m_iID, wSID);
}

BOOL CEpgDataCapDllUtil2::GetEpgDataDB( DWORD dwONID, DWORD dwTSID, DWORD dwSID, DWORD dwEID, EPG_DATA_INFO2** pstEpgData)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return -1;
	}
	return pfnGetEpgDataDB(m_iID, dwONID, dwTSID, dwSID, dwEID, pstEpgData);
}

void CEpgDataCapDllUtil2::ClearDB()
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ;
	}
	pfnClearDB(m_iID);
}

BOOL CEpgDataCapDllUtil2::GetEpgDataListDB2(DWORD dwONID, DWORD dwTSID, DWORD dwSID, EPG_DATA_INFO3** pstList, DWORD* dwListSize)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return FALSE;
	}
	return pfnGetEpgDataListDB2(m_iID, dwONID, dwTSID, dwSID, pstList, dwListSize);
}

BOOL CEpgDataCapDllUtil2::GetEpgDataDB2( DWORD dwONID, DWORD dwTSID, DWORD dwSID, DWORD dwEID, EPG_DATA_INFO3** pstEpgData)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return FALSE;
	}
	return pfnGetEpgDataDB2(m_iID, dwONID, dwTSID, dwSID, dwEID, pstEpgData);
}

DWORD CEpgDataCapDllUtil2::GetPFData2(
	WORD wSID,
	EPG_DATA_INFO3** pstEpgData,
	BOOL bNextData
	)
{
	if( m_hModule == NULL || m_iID == -1 ){
		return ERR_INIT;
	}

	return pfnGetPFData2(m_iID, wSID,pstEpgData,bNextData);
}
