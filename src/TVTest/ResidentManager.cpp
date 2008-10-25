#include "stdafx.h"
#include "TVTest.h"
#include "ResidentManager.h"
#include "resource.h"




CResidentManager::CResidentManager()
{
	m_fResident=false;
	m_Status=0;
	m_hwnd=NULL;
	m_hinst=NULL;
	m_TaskbarCreatedMessage=::RegisterWindowMessage(TEXT("TaskbarCreated"));
}


CResidentManager::~CResidentManager()
{
	Finalize();
}


bool CResidentManager::Initialize(HWND hwnd,HINSTANCE hinst)
{
	m_hwnd=hwnd;
	m_hinst=hinst;
	if (m_fResident) {
		if (!AddTrayIcon())
			return false;
	}
	return true;
}


void CResidentManager::Finalize()
{
	if (m_hwnd!=NULL && m_fResident)
		RemoveTrayIcon();
	m_hwnd=NULL;
}


bool CResidentManager::SetResident(bool fResident)
{
	if (m_fResident!=fResident) {
		if (m_hwnd!=NULL) {
			if (fResident) {
				if (!AddTrayIcon())
					return false;
			} else {
				RemoveTrayIcon();
			}
		}
		m_fResident=fResident;
	}
	return true;
}


bool CResidentManager::AddTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=NIF_MESSAGE | NIF_ICON | NIF_TIP;
	nid.uCallbackMessage=WM_APP_TRAYICON;
	nid.hIcon=LoadIcon(m_hinst,MAKEINTRESOURCE((m_Status&STATUS_RECORDING)!=0?IDI_TRAY_RECORDING:IDI_TRAY));
	lstrcpy(nid.szTip,APP_NAME);
	if (!Shell_NotifyIcon(NIM_ADD,&nid))
		return false;
	/*
	nid.uVersion=NOTIFYICON_VERSION;
	Shell_NotifyIcon(NIM_SETVERSION,&nid);
	*/
	return true;
}


bool CResidentManager::RemoveTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=0;
	return Shell_NotifyIcon(NIM_DELETE,&nid)!=FALSE;
}


bool CResidentManager::ChangeTrayIcon()
{
	NOTIFYICONDATA nid;

	nid.cbSize=NOTIFYICONDATA_V2_SIZE;
	nid.hWnd=m_hwnd;
	nid.uID=1;
	nid.uFlags=NIF_ICON;
	nid.hIcon=LoadIcon(m_hinst,MAKEINTRESOURCE((m_Status&STATUS_RECORDING)!=0?IDI_TRAY_RECORDING:IDI_TRAY));
	return Shell_NotifyIcon(NIM_MODIFY,&nid)!=FALSE;
}


bool CResidentManager::SetStatus(UINT Status,UINT Mask)
{
	Status=(m_Status&~Mask)|(Status&Mask);
	if (m_Status!=Status) {
		bool fChangeIcon=false;

		if ((m_Status&STATUS_RECORDING)!=(Status&STATUS_RECORDING))
			fChangeIcon=true;
		m_Status=Status;
		if (m_fResident) {
			if (fChangeIcon)
				ChangeTrayIcon();
		}
	}
	return true;
}


bool CResidentManager::HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam)
{
	// ÉVÉFÉãÇ™çƒãNìÆÇµÇΩéûÇÃëŒçÙ
	if (Message==m_TaskbarCreatedMessage) {
		if (m_fResident && m_hwnd!=NULL)
			AddTrayIcon();
		return true;
	}
	return false;
}
