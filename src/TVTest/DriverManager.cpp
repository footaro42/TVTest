#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "DriverManager.h"




CDriverInfo::CDriverInfo(LPCTSTR pszFileName)
{
	m_pszFileName=DuplicateString(pszFileName);
}


CDriverInfo::~CDriverInfo()
{
	delete [] m_pszFileName;
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


int CDriverManager::CompareDriverFileName(const CDriverInfo *pDriver1,const CDriverInfo *pDriver2)
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
