#include "stdafx.h"
#include <shlwapi.h>
#include "TVTest.h"
#include "EpgDataLoader.h"




CEpgDataLoader::CEpgDataLoader(CEpgDataCapDllUtil2 *pEpgDataCap)
	: m_pEpgDataCap(pEpgDataCap)
	, m_hThread(NULL)
	, m_pszFolder(NULL)
	, m_pEventHandler(NULL)
{
}


CEpgDataLoader::~CEpgDataLoader()
{
	if (m_hThread) {
		if (::WaitForSingleObject(m_hThread,10000)==WAIT_TIMEOUT)
			::TerminateThread(m_hThread, 1);
		::CloseHandle(m_hThread);
	}
	delete [] m_pszFolder;
}


bool CEpgDataLoader::LoadFromFile(LPCTSTR pszFileName)
{
	static const DWORD BUFFER_LENGTH=1024;
	HANDLE hFile;
	LARGE_INTEGER FileSize;
	LONGLONG RemainSize;
	BYTE Buffer[188*BUFFER_LENGTH];
	DWORD Size,Read;

	m_pEpgDataCap->SetBasicMode(TRUE);
	m_pEpgDataCap->SetPFOnly(FALSE);
	hFile=::CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
					   OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	if (!::GetFileSizeEx(hFile,&FileSize)) {
		::CloseHandle(hFile);
		return false;
	}
	for (RemainSize=FileSize.QuadPart;RemainSize>0;RemainSize-=Size) {
		if (RemainSize<188*BUFFER_LENGTH)
			Size=(DWORD)RemainSize;
		else
			Size=188*BUFFER_LENGTH;
		if (!::ReadFile(hFile,Buffer,Size,&Read,NULL))
			break;
		for (DWORD i=0;i<Read/188;i++) {
			WORD PID=((WORD)(Buffer[i*188+1]&0x1F)<<8) | Buffer[i*188+2];
			if (PID==0x0000 || PID==0x0010 || PID == 0x0011 || PID==0x0012
					|| PID==0x0014 || PID==0x0026 || PID==0x0027) {
				m_pEpgDataCap->AddTSPacket(&Buffer[i*188],188);
			}
		}
		if (Read<Size)
			break;
	}
	::CloseHandle(hFile);
	return true;
}


bool CEpgDataLoader::Load(LPCTSTR pszFolder)
{
	TCHAR szFileMask[MAX_PATH];
	HANDLE hFind;
	WIN32_FIND_DATA fd;

	::PathCombine(szFileMask,pszFolder,TEXT("*_epg.dat"));
	hFind=::FindFirstFile(szFileMask,&fd);
	if (hFind==INVALID_HANDLE_VALUE)
		return false;
	do {
		if ((fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)==0) {
			TCHAR szFilePath[MAX_PATH];

			::PathCombine(szFilePath,pszFolder,fd.cFileName);
			LoadFromFile(szFilePath);
		}
	} while (::FindNextFile(hFind,&fd));
	::FindClose(hFind);
	return true;
}


bool CEpgDataLoader::LoadAsync(LPCTSTR pszFolder,CEventHandler *pEventHandler)
{
	if (m_hThread) {
		if (::WaitForSingleObject(m_hThread,5000)==WAIT_TIMEOUT)
			return false;
		::CloseHandle(m_hThread);
		m_hThread=NULL;
	}
	ReplaceString(&m_pszFolder,pszFolder);
	m_pEventHandler=pEventHandler;
	m_hThread=CreateThread(NULL,0,LoadThread,this,0,NULL);
	if (m_hThread==NULL)
		return false;
	return true;
}


DWORD WINAPI CEpgDataLoader::LoadThread(LPVOID lpParameter)
{
	CEpgDataLoader *pThis=static_cast<CEpgDataLoader*>(lpParameter);

	if (pThis->m_pEventHandler!=NULL)
		pThis->m_pEventHandler->OnStart();
	::SetThreadPriority(::GetCurrentThread(),THREAD_PRIORITY_LOWEST);
	bool fSuccess=pThis->Load(pThis->m_pszFolder);
	if (pThis->m_pEventHandler!=NULL)
		pThis->m_pEventHandler->OnEnd(fSuccess);
	SAFE_DELETE(pThis->m_pszFolder);
	return 0;
}
