#include "stdafx.h"
#include <shlwapi.h>
#include <shlobj.h>
#include "TVTest.h"
#include "AppMain.h"
#include "EpgOptions.h"
#include "DialogUtil.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




CEpgOptions::CEpgOptions(CCoreEngine *pCoreEngine)
{
	::lstrcpy(m_szEpgDataCapDllPath,TEXT("EpgDataCap2.dll"));
	m_fSaveEpgFile=true;
	::lstrcpy(m_szEpgFileName,TEXT("EpgData"));
	m_fUpdateWhenStandby=false;
	m_fUseEpgData=false;
	if (::SHGetSpecialFolderPath(NULL,m_szEpgDataFolder,CSIDL_PERSONAL,FALSE))
		::PathAppend(m_szEpgDataFolder,TEXT("EpgTimerBon\\EpgData"));
	else
		m_szEpgDataFolder[0]='\0';
	m_pCoreEngine=pCoreEngine;
	m_hLoadThread=NULL;
	m_pEpgDataLoader=NULL;
	m_EpgDataLoaderEventHandler.SetPacketParser(&pCoreEngine->m_DtvEngine.m_TsPacketParser);
}


CEpgOptions::~CEpgOptions()
{
	Finalize();
}


void CEpgOptions::Finalize()
{
	if (m_hLoadThread) {
		::WaitForSingleObject(m_hLoadThread,INFINITE);
		::CloseHandle(m_hLoadThread);
		m_hLoadThread=NULL;
	}
	SAFE_DELETE(m_pEpgDataLoader);
}


bool CEpgOptions::Read(CSettings *pSettings)
{
	pSettings->Read(TEXT("EpgDataCapDll"),m_szEpgDataCapDllPath,lengthof(m_szEpgDataCapDllPath));
	pSettings->Read(TEXT("SaveEpgData"),&m_fSaveEpgFile);
	pSettings->Read(TEXT("EpgDataFileName"),m_szEpgFileName,lengthof(m_szEpgFileName));
	pSettings->Read(TEXT("EpgUpdateWhenStandby"),&m_fUpdateWhenStandby);
	pSettings->Read(TEXT("UseEpgData"),&m_fUseEpgData);
	pSettings->Read(TEXT("EpgDataFolder"),m_szEpgDataFolder,lengthof(m_szEpgDataFolder));
	return true;
}


bool CEpgOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("EpgDataCapDll"),m_szEpgDataCapDllPath);
	pSettings->Write(TEXT("SaveEpgData"),m_fSaveEpgFile);
	pSettings->Write(TEXT("EpgDataFileName"),m_szEpgFileName);
	pSettings->Write(TEXT("EpgUpdateWhenStandby"),m_fUpdateWhenStandby);
	pSettings->Write(TEXT("UseEpgData"),m_fUseEpgData);
	pSettings->Write(TEXT("EpgDataFolder"),m_szEpgDataFolder);
	return true;
}


bool CEpgOptions::InitializeEpgDataCap()
{
	if (m_szEpgDataCapDllPath[0]=='\0')
		return true;
	return m_pCoreEngine->m_DtvEngine.m_TsPacketParser.InitializeEpgDataCap(m_szEpgDataCapDllPath);
}


bool CEpgOptions::GetEpgFileFullPath(LPTSTR pszFileName)
{
	if (::PathIsFileSpec(m_szEpgFileName)) {
		::GetModuleFileName(NULL,pszFileName,MAX_PATH);
		::lstrcpy(::PathFindFileName(pszFileName),m_szEpgFileName);
	} else {
		::lstrcpy(pszFileName,m_szEpgFileName);
	}
	return true;
}


bool CEpgOptions::LoadEpgFile(CEpgProgramList *pEpgList)
{
	bool fOK=true;

	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		GetEpgFileFullPath(szFileName);
		if (::PathFileExists(szFileName))
			fOK=pEpgList->LoadFromFile(szFileName);
	}
	return fOK;
}


struct EpgLoadInfo {
	CEpgProgramList *pList;
	TCHAR szFileName[MAX_PATH];
};

bool CEpgOptions::AsyncLoadEpgFile(CEpgProgramList *pEpgList)
{
	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		GetEpgFileFullPath(szFileName);
		if (::PathFileExists(szFileName)) {
			EpgLoadInfo *pInfo=new EpgLoadInfo;

			pInfo->pList=pEpgList;
			::lstrcpy(pInfo->szFileName,szFileName);
			m_hLoadThread=::CreateThread(NULL,0,LoadThread,pInfo,0,NULL);
			if (m_hLoadThread==NULL) {
				delete pInfo;
				return pEpgList->LoadFromFile(szFileName);
			}
		}
	}
	return true;
}


DWORD WINAPI CEpgOptions::LoadThread(LPVOID lpParameter)
{
	EpgLoadInfo *pInfo=static_cast<EpgLoadInfo*>(lpParameter);

	::SetThreadPriority(::GetCurrentThread(),THREAD_PRIORITY_LOWEST);
	bool fOK=pInfo->pList->LoadFromFile(pInfo->szFileName);
	delete pInfo;
	return fOK;
}


bool CEpgOptions::SaveEpgFile(CEpgProgramList *pEpgList)
{
	bool fOK=true;

	if (m_fSaveEpgFile) {
		TCHAR szFileName[MAX_PATH];

		GetEpgFileFullPath(szFileName);
		if (pEpgList->NumServices()>0) {
			fOK=pEpgList->SaveToFile(szFileName);
			if (!fOK)
				::DeleteFile(szFileName);
		} else {
			if (::DeleteFile(szFileName)
					|| ::GetLastError()==ERROR_FILE_NOT_FOUND)
				fOK=true;
		}
	}
	return fOK;
}


bool CEpgOptions::LoadEpgData()
{
	bool fOK=true;

	if (m_fUseEpgData && m_szEpgDataFolder[0]!='\0'
			&& m_pCoreEngine->m_DtvEngine.m_TsPacketParser.IsEpgDataCapLoaded()) {
		CEpgDataLoader Loader(m_pCoreEngine->m_DtvEngine.m_TsPacketParser.GetEpgDataCapDllUtil());

		fOK=Loader.Load(m_szEpgDataFolder);
	}
	return fOK;
}


bool CEpgOptions::AsyncLoadEpgData(CEpgLoadEventHandler *pEventHandler)
{
	bool fOK=true;

	if (m_fUseEpgData && m_szEpgDataFolder[0]!='\0'
			&& m_pCoreEngine->m_DtvEngine.m_TsPacketParser.IsEpgDataCapLoaded()) {
		delete m_pEpgDataLoader;
		m_pEpgDataLoader=
			new CEpgDataLoader(m_pCoreEngine->m_DtvEngine.m_TsPacketParser.GetEpgDataCapDllUtil());

		m_EpgDataLoaderEventHandler.SetEventHandler(pEventHandler);
		fOK=m_pEpgDataLoader->LoadAsync(m_szEpgDataFolder,&m_EpgDataLoaderEventHandler);
	}
	return fOK;
}


bool CEpgOptions::IsEpgDataLoading() const
{
	return m_EpgDataLoaderEventHandler.IsLoading();
}


CEpgOptions *CEpgOptions::GetThis(HWND hDlg)
{
	return static_cast<CEpgOptions*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CEpgOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CEpgOptions *pThis=dynamic_cast<CEpgOptions*>(OnInitDialog(hDlg,lParam));

			::SendDlgItemMessage(hDlg,IDC_EPGOPTIONS_EPGDATACAPDLLPATH,
								 EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATACAPDLLPATH,
												pThis->m_szEpgDataCapDllPath);
			::CheckDlgButton(hDlg,IDC_EPGOPTIONS_SAVEEPGFILE,
							 pThis->m_fSaveEpgFile?BST_CHECKED:BST_UNCHECKED);
			::EnableDlgItems(hDlg,IDC_EPGOPTIONS_EPGFILENAME_LABEL,
					IDC_EPGOPTIONS_EPGFILENAME_BROWSE,pThis->m_fSaveEpgFile);
			::SendDlgItemMessage(hDlg,IDC_EPGOPTIONS_EPGFILENAME,
								 EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,
							 pThis->m_szEpgFileName);
			::CheckDlgButton(hDlg,IDC_EPGOPTIONS_UPDATEWHENSTANDBY,
						pThis->m_fUpdateWhenStandby?BST_CHECKED:BST_UNCHECKED);
			DlgCheckBox_Check(hDlg,IDC_EPGOPTIONS_USEEPGDATA,pThis->m_fUseEpgData);
			::SendDlgItemMessage(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,EM_LIMITTEXT,MAX_PATH-1,0);
			::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,pThis->m_szEpgDataFolder);
			EnableDlgItems(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER_LABEL,
								IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE,
						   pThis->m_fUseEpgData);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_EPGOPTIONS_EPGDATACAPDLLPATH_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitialDir[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATACAPDLLPATH,
								 szFileName,lengthof(szFileName));
				ofn.lStructSize=sizeof(OPENFILENAME);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("EpgDataCap2.dll\0EpgDataCap2.dll\0")
								TEXT("DLLファイル(*.dll)\0*.dll\0")
								TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrCustomFilter=NULL;
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrFileTitle=NULL;
				if (szFileName[0]=='\0' || ::PathIsFileSpec(szFileName)) {
					GetAppClass().GetAppDirectory(szInitialDir);
					ofn.lpstrInitialDir=szInitialDir;
				} else
					ofn.lpstrInitialDir=NULL;
				ofn.lpstrTitle=TEXT("DLLファイル名");
				ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;
				ofn.lpstrDefExt=TEXT("dll");
#if _WIN32_WINNT>=0x500
				ofn.pvReserved=NULL;
				ofn.dwReserved=0;
				ofn.FlagsEx=0;
#endif
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATACAPDLLPATH,szFileName);
			}
			return TRUE;

		case IDC_EPGOPTIONS_SAVEEPGFILE:
			::EnableDlgItems(hDlg,IDC_EPGOPTIONS_EPGFILENAME_LABEL,
									IDC_EPGOPTIONS_EPGFILENAME_BROWSE,
				::IsDlgButtonChecked(hDlg,IDC_EPGOPTIONS_SAVEEPGFILE)==BST_CHECKED);
			return TRUE;

		case IDC_EPGOPTIONS_EPGFILENAME_BROWSE:
			{
				OPENFILENAME ofn;
				TCHAR szFileName[MAX_PATH],szInitialDir[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,
								 szFileName,lengthof(szFileName));
				ofn.lStructSize=sizeof(OPENFILENAME);
				ofn.hwndOwner=hDlg;
				ofn.lpstrFilter=TEXT("すべてのファイル\0*.*\0");
				ofn.lpstrCustomFilter=NULL;
				ofn.nFilterIndex=1;
				ofn.lpstrFile=szFileName;
				ofn.nMaxFile=lengthof(szFileName);
				ofn.lpstrFileTitle=NULL;
				if (szFileName[0]=='\0' || ::PathIsFileSpec(szFileName)) {
					::GetModuleFileName(NULL,szInitialDir,lengthof(szInitialDir));
					*(::PathFindFileName(szInitialDir)-1)='\0';
					ofn.lpstrInitialDir=szInitialDir;
				} else
					ofn.lpstrInitialDir=NULL;
				ofn.lpstrTitle=TEXT("EPGファイル名");
				ofn.Flags=OFN_EXPLORER | OFN_HIDEREADONLY;
				ofn.lpstrDefExt=NULL;
#if _WIN32_WINNT>=0x500
				ofn.pvReserved=NULL;
				ofn.dwReserved=0;
				ofn.FlagsEx=0;
#endif
				if (::GetOpenFileName(&ofn))
					::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,szFileName);
			}
			return TRUE;

		case IDC_EPGOPTIONS_USEEPGDATA:
			EnableDlgItems(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER_LABEL,
								IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE,
						   DlgCheckBox_IsChecked(hDlg,IDC_EPGOPTIONS_USEEPGDATA));
			return TRUE;

		case IDC_EPGOPTIONS_EPGDATAFOLDER_BROWSE:
			{
				TCHAR szFolder[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,szFolder,lengthof(szFolder));
				if (BrowseFolderDialog(hDlg,szFolder,TEXT("EPGデータのフォルダ")))
					::SetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,szFolder);
			}
			return TRUE;
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CEpgOptions *pThis=GetThis(hDlg);
				TCHAR szPath[MAX_PATH];

				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATACAPDLLPATH,
								 szPath,lengthof(szPath));
				if (::lstrcmpi(szPath,pThis->m_szEpgDataCapDllPath)!=0) {
					pThis->m_pCoreEngine->m_DtvEngine.m_TsPacketParser.UnInitializeEpgDataCap();
					if (szPath[0]!='\0') {
						pThis->m_pCoreEngine->m_DtvEngine.m_TsPacketParser.InitializeEpgDataCap(szPath);
					}
					::lstrcpy(pThis->m_szEpgDataCapDllPath,szPath);
				}
				pThis->m_fSaveEpgFile=
					::IsDlgButtonChecked(hDlg,IDC_EPGOPTIONS_SAVEEPGFILE)==BST_CHECKED;
				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGFILENAME,
					 pThis->m_szEpgFileName,lengthof(pThis->m_szEpgFileName));
				pThis->m_fUpdateWhenStandby=
					::IsDlgButtonChecked(hDlg,IDC_EPGOPTIONS_UPDATEWHENSTANDBY)==BST_CHECKED;
				bool fUseEpgData=
					DlgCheckBox_IsChecked(hDlg,IDC_EPGOPTIONS_USEEPGDATA);
				::GetDlgItemText(hDlg,IDC_EPGOPTIONS_EPGDATAFOLDER,
					pThis->m_szEpgDataFolder,lengthof(pThis->m_szEpgDataFolder));
				if (!pThis->m_fUseEpgData && fUseEpgData) {
					pThis->m_fUseEpgData=fUseEpgData;
					pThis->AsyncLoadEpgData();
				}
				pThis->m_fUseEpgData=fUseEpgData;
			}
			break;
		}
		break;

	case WM_DESTROY:
		{
			CEpgOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
