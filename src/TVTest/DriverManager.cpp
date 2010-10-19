#include "stdafx.h"
#include "TVTest.h"
#include "AppMain.h"
#include "DriverManager.h"
#include "IBonDriver2.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


typedef IBonDriver *(*CreateBonDriverFunc)();




CDriverInfo::CDriverInfo(LPCTSTR pszFileName)
	: m_pszFileName(DuplicateString(pszFileName))
	, m_pszTunerName(NULL)
	, m_fChannelFileLoaded(false)
	, m_fDriverSpaceLoaded(false)
{
}


CDriverInfo::CDriverInfo(const CDriverInfo &Info)
	: m_pszFileName(NULL)
	, m_pszTunerName(NULL)
{
	*this=Info;
}


CDriverInfo::~CDriverInfo()
{
	delete [] m_pszFileName;
	delete [] m_pszTunerName;
}


CDriverInfo &CDriverInfo::operator=(const CDriverInfo &Src)
{
	if (&Src!=this) {
		ReplaceString(&m_pszFileName,Src.m_pszFileName);
		ReplaceString(&m_pszTunerName,Src.m_pszTunerName);
		m_fChannelFileLoaded=Src.m_fChannelFileLoaded;
		m_TuningSpaceList=Src.m_TuningSpaceList;
		m_fDriverSpaceLoaded=Src.m_fDriverSpaceLoaded;
		m_DriverSpaceList=Src.m_DriverSpaceList;
	}
	return *this;
}


bool CDriverInfo::LoadTuningSpaceList(LoadTuningSpaceListMode Mode)
{
	CAppMain &App=GetAppClass();

	bool fUseDriver;
	if (Mode==LOADTUNINGSPACE_NOLOADDRIVER) {
		fUseDriver=false;
	} else if (Mode==LOADTUNINGSPACE_USEDRIVER) {
		fUseDriver=true;
	} else {
		// チューナを開かずにチューニング空間とチャンネルを取得できない
		// ドライバはロードしないようにする
		fUseDriver=!::PathMatchSpec(m_pszFileName,TEXT("BonDriver_Spinel*.dll"))
				&& !::PathMatchSpec(m_pszFileName,TEXT("BonDriver_Friio*.dll"));
	}

	if (!m_fChannelFileLoaded) {
		TCHAR szFileName[MAX_PATH];

		App.GetChannelFileName(m_pszFileName,szFileName,lengthof(szFileName));
		if (m_TuningSpaceList.LoadFromFile(szFileName)) {
#if 0
			if (fUseDriver && Mode==LOADTUNINGSPACE_DEFAULT) {
				const int NumSpaces=m_TuningSpaceList.NumSpaces();
				int i;
				for (i=0;i<NumSpaces;i++) {
					if (m_TuningSpaceList.GetTuningSpaceName(i)==NULL
							|| m_TuningSpaceList.GetChannelList(i)->NumChannels()==0)
						break;
				}
				if (i==NumSpaces)
					fUseDriver=false;
			}
#else
			if (Mode==LOADTUNINGSPACE_DEFAULT)
				fUseDriver=false;
#endif
			m_fChannelFileLoaded=true;
		} else {
			if (!fUseDriver && !m_fDriverSpaceLoaded)
				return false;
		}
	}
	if (fUseDriver && !m_fDriverSpaceLoaded) {
		TCHAR szFileName[MAX_PATH];

		if (::PathIsRelative(m_pszFileName)) {
			TCHAR szTemp[MAX_PATH];
			GetAppClass().GetDriverDirectory(szTemp);
			::PathAppend(szTemp,m_pszFileName);
			::PathCanonicalize(szFileName,szTemp);
		} else {
			::lstrcpy(szFileName,m_pszFileName);
		}

		HMODULE hLib=::GetModuleHandle(szFileName);
		if (hLib!=NULL) {
			TCHAR szCurDriverPath[MAX_PATH];

			if (App.GetCoreEngine()->GetDriverPath(szCurDriverPath)
					&& ::lstrcmpi(szFileName,szCurDriverPath)==0) {
				m_DriverSpaceList=*App.GetChannelManager()->GetDriverTuningSpaceList();
				m_fDriverSpaceLoaded=true;
			}
		} else if ((hLib=::LoadLibrary(szFileName))!=NULL) {
			CreateBonDriverFunc pCreate=
				reinterpret_cast<CreateBonDriverFunc>(::GetProcAddress(hLib,"CreateBonDriver"));
			IBonDriver *pBonDriver;

			if (pCreate!=NULL && (pBonDriver=pCreate())!=NULL) {
				IBonDriver2 *pBonDriver2=dynamic_cast<IBonDriver2*>(pBonDriver);

				if (pBonDriver2!=NULL) {
					int NumSpaces;

					for (NumSpaces=0;pBonDriver2->EnumTuningSpace(NumSpaces)!=NULL;NumSpaces++);
					m_DriverSpaceList.Reserve(NumSpaces);
					ReplaceString(&m_pszTunerName,pBonDriver2->GetTunerName());
					for (int i=0;i<NumSpaces;i++) {
						CTuningSpaceInfo *pTuningSpaceInfo=m_DriverSpaceList.GetTuningSpaceInfo(i);
						LPCTSTR pszName=pBonDriver2->EnumTuningSpace(i);

						pTuningSpaceInfo->SetName(pszName);
						CChannelList *pChannelList=pTuningSpaceInfo->GetChannelList();
						for (int j=0;(pszName=pBonDriver2->EnumChannelName(i,j))!=NULL;j++) {
							pChannelList->AddChannel(i,0,j,j+1,0,pszName);
						}
					}
					m_fDriverSpaceLoaded=true;
				}
				pBonDriver->Release();
			}
			::FreeLibrary(hLib);
		}
		for (int i=0;i<m_TuningSpaceList.NumSpaces();i++) {
			if (m_TuningSpaceList.GetTuningSpaceName(i)==NULL)
				m_TuningSpaceList.GetTuningSpaceInfo(i)->SetName(m_DriverSpaceList.GetTuningSpaceName(i));
		}
	}
	if (!m_fChannelFileLoaded && !m_fDriverSpaceLoaded)
		return false;
	return true;
}


const CTuningSpaceList *CDriverInfo::GetAvailableTuningSpaceList() const
{
	if (m_fChannelFileLoaded)
		return &m_TuningSpaceList;
	if (m_fDriverSpaceLoaded)
		return &m_DriverSpaceList;
	return NULL;
}


const CChannelList *CDriverInfo::GetChannelList(int Space) const
{
	const CChannelList *pChannelList;

	pChannelList=m_TuningSpaceList.GetChannelList(Space);
	if (pChannelList==NULL) {
		pChannelList=m_DriverSpaceList.GetChannelList(Space);
	} else if (pChannelList->NumChannels()==0) {
		const CChannelList *pDriverChannelList=m_DriverSpaceList.GetChannelList(Space);

		if (pDriverChannelList!=NULL && pDriverChannelList->NumChannels()>0)
			pChannelList=pDriverChannelList;
	}
	return pChannelList;
}




CDriverManager::CDriverManager()
	: m_pszBaseDirectory(NULL)
{
}


CDriverManager::~CDriverManager()
{
	Clear();
	delete [] m_pszBaseDirectory;
}


void CDriverManager::Clear()
{
	m_DriverList.DeleteAll();
	m_DriverList.Clear();
}


int CDriverManager::CompareDriverFileName(const CDriverInfo *pDriver1,const CDriverInfo *pDriver2,void *pParam)
{
	return ::lstrcmpi(pDriver1->GetFileName(),pDriver2->GetFileName());
}


bool CDriverManager::Find(LPCTSTR pszDirectory)
{
	TCHAR szMask[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	Clear();
	::PathCombine(szMask,pszDirectory,TEXT("BonDriver*.dll"));
	hFind=::FindFirstFile(szMask,&wfd);
	if (hFind!=INVALID_HANDLE_VALUE) {
		do {
			if ((wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
				m_DriverList.Add(new CDriverInfo(wfd.cFileName));
			}
		} while (::FindNextFile(hFind,&wfd));
		::FindClose(hFind);
	}
	m_DriverList.Sort(CompareDriverFileName);
	ReplaceString(&m_pszBaseDirectory,pszDirectory);
	return true;
}


CDriverInfo *CDriverManager::GetDriverInfo(int Index)
{
	return m_DriverList.Get(Index);
}


const CDriverInfo *CDriverManager::GetDriverInfo(int Index) const
{
	return m_DriverList.Get(Index);
}
