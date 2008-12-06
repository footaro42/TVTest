#include "stdafx.h"
#include "TVTest.h"
#include "ResidentManager.h"
#include "AppMain.h"
#include "resource.h"




CResidentManager::CResidentManager()
{
	m_hwnd=NULL;
	m_fResident=false;
	m_fMinimizeToTray=false;
	m_Status=0;
	m_TaskbarCreatedMessage=::RegisterWindowMessage(TEXT("TaskbarCreated"));
}


CResidentManager::~CResidentManager()
{
	Finalize();
}


bool CResidentManager::Initialize(HWND hwnd)
{
	m_hwnd=hwnd;
	if (IsTrayIconVisible()) {
		if (!AddTrayIcon())
			return false;
	}
	return true;
}


void CResidentManager::Finalize()
{
	if (IsTrayIconVisible())
		RemoveTrayIcon();
}


bool CResidentManager::SetResident(bool fResident)
{
	if (m_fResident!=fResident) {
		if (m_hwnd!=NULL) {
			if (!IsTrayIconVisible()) {
				if (fResident) {
					if (!AddTrayIcon())
						return false;
				}
			} else {
				if (!m_fMinimizeToTray || (m_Status&STATUS_MINIMIZED)==0)
					RemoveTrayIcon();
			}
		}
		m_fResident=fResident;
	}
	return true;
}


bool CResidentManager::SetMinimizeToTray(bool fMinimizeToTray)
{
	if (m_fMinimizeToTray!=fMinimizeToTray) {
		if (m_hwnd!=NULL) {
			if (!IsTrayIconVisible()) {
				if (fMinimizeToTray && (m_Status&STATUS_MINIMIZED)!=0) {
					if (!AddTrayIcon())
						return false;
				}
			} else {
				if (!m_fResident)
					RemoveTrayIcon();
			}
		}
		m_fMinimizeToTray=fMinimizeToTray;
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
	nid.hIcon=LoadIcon(GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE((m_Status&STATUS_RECORDING)!=0?IDI_TRAY_RECORDING:IDI_TRAY));
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
	nid.hIcon=LoadIcon(GetAppClass().GetResourceInstance(),
		MAKEINTRESOURCE((m_Status&STATUS_RECORDING)!=0?IDI_TRAY_RECORDING:IDI_TRAY));
	return Shell_NotifyIcon(NIM_MODIFY,&nid)!=FALSE;
}


bool CResidentManager::IsTrayIconVisible() const
{
	return m_hwnd!=NULL && (m_fResident || (m_fMinimizeToTray && (m_Status&STATUS_MINIMIZED)!=0));
}


bool CResidentManager::SetStatus(UINT Status,UINT Mask)
{
	Status=(m_Status&~Mask)|(Status&Mask);
	if (m_Status!=Status) {
		bool fChangeIcon=false;

		if ((m_Status&STATUS_RECORDING)!=(Status&STATUS_RECORDING))
			fChangeIcon=true;
		if ((m_Status&STATUS_MINIMIZED)!=(Status&STATUS_MINIMIZED)) {
			if (m_fMinimizeToTray && m_hwnd!=NULL) {
				::ShowWindow(m_hwnd,(Status&STATUS_MINIMIZED)!=0?SW_HIDE:SW_SHOW);
				if (!IsTrayIconVisible() && (Status&STATUS_MINIMIZED)!=0)
					AddTrayIcon();
			}
			if (IsTrayIconVisible() && !m_fResident && (Status&STATUS_MINIMIZED)==0)
				RemoveTrayIcon();
		}
		if (m_fResident) {
			if (fChangeIcon)
				ChangeTrayIcon();
		}
		m_Status=Status;
	}
	return true;
}


bool CResidentManager::HandleMessage(UINT Message,WPARAM wParam,LPARAM lParam)
{
	// ÉVÉFÉãÇ™çƒãNìÆÇµÇΩéûÇÃëŒçÙ
	if (Message==m_TaskbarCreatedMessage) {
		if (IsTrayIconVisible())
			AddTrayIcon();
		return true;
	}
	return false;
}
