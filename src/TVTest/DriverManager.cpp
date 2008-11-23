#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "DriverManager.h"
#include "IBonDriver2.h"


typedef IBonDriver *(*CreateBonDriverFunc)();




CDriverInfo::CDriverInfo(LPCTSTR pszFileName)
{
	m_pszFileName=DuplicateString(pszFileName);
	m_pszTunerName=NULL;
	m_fChannelFileLoaded=false;
}


CDriverInfo::~CDriverInfo()
{
	delete [] m_pszFileName;
	delete [] m_pszTunerName;
}


bool CDriverInfo::LoadTuningSpaceList(bool fUseDriver)
{
	if (!m_fChannelFileLoaded) {
		TCHAR szFileName[MAX_PATH];

		if (::PathIsFileSpec(m_pszFileName)) {
			GetAppClass().GetAppDirectory(szFileName);
			::PathAppend(szFileName,m_pszFileName);
		} else {
			::lstrcpy(szFileName,m_pszFileName);
		}
		::PathRenameExtension(szFileName,TEXT(".ch2"));
		if (!m_TuningSpaceList.LoadFromFile(szFileName))
			return false;
		if (fUseDriver) {
			HMODULE hLib=::LoadLibrary(m_pszFileName);

			if (hLib!=NULL) {
				CreateBonDriverFunc pCreate=reinterpret_cast<CreateBonDriverFunc>(::GetProcAddress(hLib,"CreateBonDriver"));
				IBonDriver *pBonDriver;

				if (pCreate!=NULL && (pBonDriver=pCreate())!=NULL) {
					IBonDriver2 *pBonDriver2=dynamic_cast<IBonDriver2*>(pBonDriver);

					if (pBonDriver2!=NULL) {
						ReplaceString(&m_pszTunerName,pBonDriver2->GetTunerName());
						for (int i=0;i<m_TuningSpaceList.NumSpaces();i++) {
							CTuningSpaceInfo *pTuningSpaceInfo=m_TuningSpaceList.GetTuningSpaceInfo(i);

							pTuningSpaceInfo->SetName(pBonDriver2->EnumTuningSpace(i));

							/*
							CChannelList *pChannelList=pTuningSpaceInfo->GetChannelList();
							if (pChannelList->NumChannels()==0) {
								LPCTSTR pszName;

								for (int j=0;(pszName=pBonDriver2->EnumChannelName(i,j))!=NULL;j++) {
									pChannelList->AddChannel(i,0,j,j+1,0,pszName);
								}
							}
							*/
						}
					}
					pBonDriver->Release();
				}
				::FreeLibrary(hLib);
			}
		}
		m_fChannelFileLoaded=true;
	}
	return true;
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
