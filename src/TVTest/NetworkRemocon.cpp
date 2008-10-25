#include "stdafx.h"
#include <shlobj.h>
#include <shlwapi.h>
#include "TVTest.h"
#include "AppMain.h"
#include "NetworkRemocon.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




class CSendStringInfo {
public:
	CNetworkRemocon *m_pRemocon;
	char *m_pBuffer;
	int m_Length;
	CNetworkRemoconReciver *m_pReciver;
	CSendStringInfo(CNetworkRemocon *pRemocon,const char *pBuffer,int Length,
					CNetworkRemoconReciver *pReciver) {
		m_pRemocon=pRemocon;
		m_pBuffer=new char[Length+1];
		CopyMemory(m_pBuffer,pBuffer,Length);
		m_pBuffer[Length]='\0';
		m_Length=Length;
		m_pReciver=pReciver;
	}
	~CSendStringInfo() {
		delete [] m_pBuffer;
	}
};




CNetworkRemocon::CNetworkRemocon()
{
	m_fInitialized=false;
	m_Address=INADDR_NONE;
	m_Port=0;
	m_hThread=NULL;
	m_Socket=INVALID_SOCKET;
	m_fConnected=false;
}


CNetworkRemocon::~CNetworkRemocon()
{
	if (m_fInitialized) {
		Shutdown();
		WSACleanup();
	}
}


bool CNetworkRemocon::Init(LPCSTR pszAddress,WORD Port)
{
	int Err;

	if (!m_fInitialized) {
		Err=WSAStartup(MAKEWORD(2,0),&m_WSAData);
		if (Err!=0)
			return false;
		m_fInitialized=true;
	} else {
		if (m_Address==inet_addr(pszAddress) && m_Port==Port)
			return true;
		Shutdown();
	}
	m_Address=inet_addr(pszAddress);
	if (m_Address==INADDR_NONE)
		return false;
	m_Port=Port;
	return true;
}


bool CNetworkRemocon::Shutdown()
{
	if (m_fInitialized) {
		if (m_hThread!=NULL) {
			if (WaitForSingleObject(m_hThread,5000)==WAIT_TIMEOUT)
				TerminateThread(m_hThread,FALSE);
			CloseHandle(m_hThread);
			m_hThread=NULL;
		}
		if (m_Socket!=INVALID_SOCKET) {
			shutdown(m_Socket,1);
			closesocket(m_Socket);
			m_Socket=INVALID_SOCKET;
		}
	}
	return true;
}


DWORD CNetworkRemocon::SendProc(LPVOID pParam)
{
	CSendStringInfo *pInfo=(CSendStringInfo*)pParam;
	int Result;
	char Buffer[1024];

	if (pInfo->m_pRemocon->m_Socket==INVALID_SOCKET) {
		pInfo->m_pRemocon->m_Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (pInfo->m_pRemocon->m_Socket==INVALID_SOCKET)
			return FALSE;
	}
	if (!pInfo->m_pRemocon->m_fConnected) {
		struct sockaddr_in sockaddri;

		sockaddri.sin_family=AF_INET;
		sockaddri.sin_port=htons(pInfo->m_pRemocon->m_Port);
		sockaddri.sin_addr.S_un.S_addr=pInfo->m_pRemocon->m_Address;
		ZeroMemory(sockaddri.sin_zero,sizeof(sockaddri.sin_zero));
		Result=connect(pInfo->m_pRemocon->m_Socket,
							(struct sockaddr*)&sockaddri,sizeof(sockaddr_in));
		if (Result!=0) {
			delete pInfo;
			return FALSE;
		}
		pInfo->m_pRemocon->m_fConnected=true;
	}
	Result=send(pInfo->m_pRemocon->m_Socket,pInfo->m_pBuffer,pInfo->m_Length,0);
	if (Result==SOCKET_ERROR) {
		closesocket(pInfo->m_pRemocon->m_Socket);
		pInfo->m_pRemocon->m_Socket=INVALID_SOCKET;
		pInfo->m_pRemocon->m_fConnected=false;
		return FALSE;
	}
	Result=recv(pInfo->m_pRemocon->m_Socket,Buffer,sizeof(Buffer)-1,0);
	if (Result!=SOCKET_ERROR && pInfo->m_pReciver!=NULL) {
		Buffer[Result]='\0';
		pInfo->m_pReciver->OnRecive(Buffer);
	}
	delete pInfo;
	if (Result==SOCKET_ERROR || Result==0) {
		closesocket(pInfo->m_pRemocon->m_Socket);
		pInfo->m_pRemocon->m_Socket=INVALID_SOCKET;
		pInfo->m_pRemocon->m_fConnected=false;
		return Result==SOCKET_ERROR;
	}
	return TRUE;
}


bool CNetworkRemocon::Send(const char *pBuffer,int Length,
											CNetworkRemoconReciver *pReciver)
{
	DWORD ThreadID;
	CSendStringInfo *pInfo;

	if (!m_fInitialized)
		return false;
	if (m_hThread!=NULL) {
		if (WaitForSingleObject(m_hThread,0)==WAIT_TIMEOUT)
			return false;
		CloseHandle(m_hThread);
	}
	pInfo=new CSendStringInfo(this,pBuffer,Length,pReciver);
	m_hThread=CreateThread(NULL,0,SendProc,pInfo,0,&ThreadID);
	if (m_hThread==NULL) {
		delete pInfo;
		return false;
	}
	return true;
}


bool CNetworkRemocon::SetChannel(int ChannelNo)
{
	char szText[16];
	int Length;

	if (m_ChannelList.NumChannels()>0) {
		int i;

		for (i=0;i<m_ChannelList.NumChannels();i++) {
			if (m_ChannelList.GetChannelNo(i)==ChannelNo+1) {
				Length=wsprintfA(szText,"SetCh:%d",i);
				return Send(szText,Length);
			}
		}
	} else {
		Length=wsprintfA(szText,"SetCh:%d",ChannelNo);
		return Send(szText,Length);
	}
	return false;
}


class CGetChannelReciver : public CNetworkRemoconReciver {
public:
	CNetworkRemoconReciver *m_pReciver;
	void OnRecive(LPCSTR pszText);
};

void CGetChannelReciver::OnRecive(LPCSTR pszText)
{
	LPCSTR p;
	char szChannel[16];
	int i;

	p=pszText;
	while (*p!='\t') {
		if (*p=='\0')
			return;
		p++;
	}
	p++;
	for (i=0;*p>='0' && *p<='9';i++) {
		if (i==sizeof(szChannel))
			return;
		szChannel[i]=*p++;
	}
	if (i==0)
		return;
	szChannel[i]='\0';
	m_pReciver->OnRecive(szChannel);
}


bool CNetworkRemocon::GetChannel(CNetworkRemoconReciver *pReciver)
{
	static CGetChannelReciver Reciver;

	Reciver.m_pReciver=pReciver;
	return Send("GetChList",9,&Reciver);
}


bool CNetworkRemocon::SetService(int Service)
{
	char szText[16];
	int Length;

	Length=wsprintfA(szText,"SetService:%d",Service);
	return Send(szText,Length);
}


bool CNetworkRemocon::LoadChannelText(LPCTSTR pszFileName,
											const CChannelList *pChannelList)
{
	HANDLE hFile;
	DWORD FileSize,Read;
	char *pszText,*p;

	hFile=CreateFile(pszFileName,GENERIC_READ,FILE_SHARE_READ,NULL,
														OPEN_EXISTING,0,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	FileSize=GetFileSize(hFile,NULL);
	if (FileSize==INVALID_FILE_SIZE || FileSize==0) {
		CloseHandle(hFile);
		return false;
	}
	pszText=new char[FileSize+1];
	if (!ReadFile(hFile,pszText,FileSize,&Read,NULL) || Read!=FileSize) {
		CloseHandle(hFile);
		return false;
	}
	pszText[FileSize]='\0';
	CloseHandle(hFile);
	m_ChannelList.Clear();
	p=pszText;
	while (*p!='\0') {
		char *pszName;
		int Space,Channel;

		while (*p=='\r' || *p=='\n')
			p++;
		if (*p==';') {	// Comment
			while (*p!='\r' && *p!='\n' && *p!='\0')
				p++;
			continue;
		}
		pszName=p;
		while (*p!='\t' && *p!='\0')
			p++;
		if (*p=='\0')
			break;
		*p++='\0';
		Space=0;
		while (*p>='0' && *p<='9') {
			Space=Space*10+(*p-'0');
			p++;
		}
		if (*p!='\t')
			break;
		p++;
		Channel=0;
		while (*p>='0' && *p<='9') {
			Channel=Channel*10+(*p-'0');
			p++;
		}
		int i;
		for (i=0;i<pChannelList->NumChannels();i++) {
			const CChannelInfo *pChInfo=pChannelList->GetChannelInfo(i);
			if (pChInfo->GetSpace()==Space && pChInfo->GetChannelIndex()==Channel) {
				m_ChannelList.AddChannel(*pChInfo);
				break;
			}
		}
		if (i==pChannelList->NumChannels()) {
			TCHAR szName[MAX_CHANNEL_NAME];

			MultiByteToWideChar(CP_ACP,0,pszName,-1,szName,MAX_CHANNEL_NAME);
			m_ChannelList.AddChannel(Space,0,Channel,
				pChannelList->NumChannels()==0?
							m_ChannelList.NumChannels()+1:Channel+1,0,szName);
		}
		while (*p!='\r' && *p!='\n' && *p!='\0')
			p++;
	}
	delete [] pszText;
	return true;
}




CNetworkRemoconOptions::CNetworkRemoconOptions()
{
	m_fUseNetworkRemocon=false;
	::lstrcpyA(m_szAddress,"127.0.0.1");
	m_Port=1334;
	m_szChannelFileName[0]='\0';
	m_fTempEnable=false;
	m_TempPort=0;
}


CNetworkRemoconOptions::~CNetworkRemoconOptions()
{
}


bool CNetworkRemoconOptions::Read(CSettings *pSettings)
{
	TCHAR szText[16];

	pSettings->Read(TEXT("UseNetworkRemocon"),&m_fUseNetworkRemocon);
	if (pSettings->Read(TEXT("NetworkRemoconAddress"),szText,lengthof(szText)))
		::WideCharToMultiByte(CP_ACP,0,szText,-1,
							  m_szAddress,lengthof(m_szAddress),NULL,NULL);
	pSettings->Read(TEXT("NetworkRemoconPort"),&m_Port);
	pSettings->Read(TEXT("NetworkRemoconChFile"),
					m_szChannelFileName,lengthof(m_szChannelFileName));
	return true;
}


bool CNetworkRemoconOptions::Write(CSettings *pSettings) const
{
	pSettings->Write(TEXT("UseNetworkRemocon"),m_fUseNetworkRemocon);
	WCHAR szAddress[16];
	::MultiByteToWideChar(CP_ACP,0,m_szAddress,-1,szAddress,lengthof(szAddress));
	pSettings->Write(TEXT("NetworkRemoconAddress"),szAddress);
	pSettings->Write(TEXT("NetworkRemoconPort"),m_Port);
	pSettings->Write(TEXT("NetworkRemoconChFile"),m_szChannelFileName);
	return true;
}


bool CNetworkRemoconOptions::SetTempEnable(bool fEnable)
{
	m_fTempEnable=fEnable;
	return true;
}


bool CNetworkRemoconOptions::SetTempPort(unsigned int Port)
{
	if (m_fUseNetworkRemocon || m_fTempEnable)
		m_TempPort=Port;
	return true;
}


bool CNetworkRemoconOptions::IsSettingValid() const
{
	return m_Port>0;
}


bool CNetworkRemoconOptions::InitNetworkRemocon(CNetworkRemocon **ppNetworkRemocon,
		const CCoreEngine *pCoreEngine,CChannelManager *pChannelManager) const
{
	if ((m_fUseNetworkRemocon || m_fTempEnable)
			&& pCoreEngine->IsUDPDriver()) {
		TCHAR szChannelFile[MAX_PATH];

		if (*ppNetworkRemocon==NULL)
			*ppNetworkRemocon=new CNetworkRemocon;
		GetChannelFilePath(szChannelFile);
		if ((*ppNetworkRemocon)->LoadChannelText(szChannelFile,
								pChannelManager->GetFileAllChannelList())) {
			pChannelManager->SetNetworkRemoconMode(true,
									&(*ppNetworkRemocon)->GetChannelList());
			pChannelManager->SetNetworkRemoconCurrentChannel(-1);
			GetAppClass().UpdateChannelMenu();
		}
		(*ppNetworkRemocon)->Init(m_szAddress,m_TempPort>0?m_TempPort:m_Port);
	} else {
		if (*ppNetworkRemocon!=NULL) {
			delete *ppNetworkRemocon;
			*ppNetworkRemocon=NULL;
			GetAppClass().UpdateChannelMenu();
		}
	}
	return true;
}


void CNetworkRemoconOptions::GetChannelFilePath(LPTSTR pszPath) const
{
	if (m_szChannelFileName[0]=='\0' || ::PathIsFileSpec(m_szChannelFileName)) {
		::SHGetSpecialFolderPath(NULL,pszPath,CSIDL_PERSONAL,FALSE);
		::PathAppend(pszPath,TEXT("EpgTimerBon"));
		if (m_szChannelFileName[0]!='\0')
			::PathAppend(pszPath,m_szChannelFileName);
		else
			::PathAppend(pszPath,TEXT("BonDriver_HDUS(HDUS).ChSet.txt"));
	} else
		::lstrcpy(pszPath,m_szChannelFileName);
}


CNetworkRemoconOptions *CNetworkRemoconOptions::GetThis(HWND hDlg)
{
	return static_cast<CNetworkRemoconOptions*>(::GetProp(hDlg,TEXT("This")));
}


BOOL CALLBACK CNetworkRemoconOptions::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CNetworkRemoconOptions *pThis=dynamic_cast<CNetworkRemoconOptions*>(OnInitDialog(hDlg,lParam));

			::CheckDlgButton(hDlg,IDC_NETWORKREMOCON_USE,
						pThis->m_fUseNetworkRemocon?BST_CHECKED:BST_UNCHECKED);
			::SetDlgItemTextA(hDlg,IDC_NETWORKREMOCON_ADDRESS,pThis->m_szAddress);
			::SetDlgItemInt(hDlg,IDC_NETWORKREMOCON_PORT,pThis->m_Port,FALSE);
			::SetDlgItemText(hDlg,IDC_NETWORKREMOCON_CHANNELFILE,pThis->m_szChannelFileName);
		}
		return TRUE;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code) {
		case PSN_APPLY:
			{
				CNetworkRemoconOptions *pThis=GetThis(hDlg);
				bool fUse;
				char szAddress[16];
				unsigned int Port;
				TCHAR szChannelFile[MAX_PATH];

				fUse=::IsDlgButtonChecked(hDlg,IDC_NETWORKREMOCON_USE)==
																BST_CHECKED;
				::GetDlgItemTextA(hDlg,IDC_NETWORKREMOCON_ADDRESS,szAddress,
														lengthof(szAddress));
				Port=::GetDlgItemInt(hDlg,IDC_NETWORKREMOCON_PORT,NULL,FALSE);
				::GetDlgItemText(hDlg,IDC_NETWORKREMOCON_CHANNELFILE,
										szChannelFile,lengthof(szChannelFile));
				bool fUpdate=false;
				if (fUse!=pThis->m_fUseNetworkRemocon) {
					pThis->m_fUseNetworkRemocon=fUse;
					fUpdate=true;
				} else {
					if (pThis->m_fUseNetworkRemocon
							&& (pThis->m_Port!=Port
								|| ::lstrcmpiA(pThis->m_szAddress,szAddress)!=0
								|| ::lstrcmpi(pThis->m_szChannelFileName,szChannelFile)!=0)) {
						fUpdate=true;
					}
				}
				pThis->m_Port=Port;
				::lstrcpyA(pThis->m_szAddress,szAddress);
				::lstrcpy(pThis->m_szChannelFileName,szChannelFile);
				if (fUpdate)
					pThis->m_UpdateFlags|=UPDATE_NETWORKREMOCON;
			}
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CNetworkRemoconOptions *pThis=GetThis(hDlg);

			pThis->OnDestroy();
		}
		return TRUE;
	}
	return FALSE;
}
