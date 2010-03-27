/*
	TVTest プラグインサンプル

	ストリームの各種情報を表示する
*/


#include <windows.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"




// プラグインクラス
class CTSInfo : public TVTest::CTVTestPlugin
{
	HWND m_hwnd;
	HBRUSH m_hbrBack;
	COLORREF m_crTextColor;
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CTSInfo *GetThis(HWND hDlg);
	static INT_PTR CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CTSInfo()
	{
		m_hwnd=NULL;
		m_hbrBack=NULL;
	}
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


bool CTSInfo::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = 0;
	pInfo->pszPluginName  = L"TSInfo";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"ストリームの情報を表示します";
	return true;
}


bool CTSInfo::Initialize()
{
	// 初期化処理

	if (::CreateDialogParam(g_hinstDLL,MAKEINTRESOURCE(IDD_MAIN),
							m_pApp->GetAppWindow(),DlgProc,reinterpret_cast<LPARAM>(this))==NULL)
		return false;

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


bool CTSInfo::Finalize()
{
	// 終了処理

	::DestroyWindow(m_hwnd);

	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CTSInfo::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CTSInfo *pThis=static_cast<CTSInfo*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		{
			bool fEnable=lParam1!=0;

			::ShowWindow(pThis->m_hwnd,fEnable?SW_SHOW:SW_HIDE);
			if (fEnable)
				::SetTimer(pThis->m_hwnd,1,1000,NULL);
			else
				::KillTimer(pThis->m_hwnd,1);
		}
		return TRUE;

	case TVTest::EVENT_COLORCHANGE:
		// 色の設定が変化した
		{
			HBRUSH hbrBack=::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));

			if (hbrBack!=NULL) {
				if (pThis->m_hbrBack!=NULL)
					::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack=hbrBack;
			}
			pThis->m_crTextColor=pThis->m_pApp->GetColor(L"PanelText");
			::InvalidateRect(pThis->m_hwnd,NULL,TRUE);
		}
		return TRUE;
	}
	return 0;
}


CTSInfo *CTSInfo::GetThis(HWND hDlg)
{
	return static_cast<CTSInfo*>(::GetProp(hDlg,TEXT("This")));
}


INT_PTR CALLBACK CTSInfo::DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CTSInfo *pThis=reinterpret_cast<CTSInfo*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			pThis->m_hwnd=hDlg;
			pThis->m_hbrBack=::CreateSolidBrush(pThis->m_pApp->GetColor(L"PanelBack"));
			pThis->m_crTextColor=pThis->m_pApp->GetColor(L"PanelText");
		}
		return TRUE;

	case WM_TIMER:
		{
			// 情報更新
			CTSInfo *pThis=GetThis(hDlg);
			TVTest::ChannelInfo ChannelInfo;
			int CurService,NumServices;
			TVTest::ServiceInfo ServiceInfo;
			TCHAR szText[256];

			if (pThis->m_pApp->GetCurrentChannelInfo(&ChannelInfo)) {
				TCHAR szSpaceName[32];

				if (pThis->m_pApp->GetTuningSpaceName(ChannelInfo.Space,szSpaceName,32)==0)
					::lstrcpy(szSpaceName,TEXT("???"));
				::wsprintf(szText,TEXT("%d (%s)"),ChannelInfo.Space,szSpaceName);
				::SetDlgItemText(hDlg,IDC_SPACE,szText);
				::wsprintf(szText,TEXT("%d (%s)"),ChannelInfo.Channel,ChannelInfo.szChannelName);
				::SetDlgItemText(hDlg,IDC_CHANNEL,szText);
				::wsprintf(szText,TEXT("0x%x"),ChannelInfo.NetworkID);
				::SetDlgItemText(hDlg,IDC_NETWORKID,szText);
				::SetDlgItemText(hDlg,IDC_NETWORKNAME,ChannelInfo.szNetworkName);
				::wsprintf(szText,TEXT("0x%x"),ChannelInfo.TransportStreamID);
				::SetDlgItemText(hDlg,IDC_TRANSPORTSTREAMID,szText);
				::SetDlgItemText(hDlg,IDC_TRANSPORTSTREAMNAME,ChannelInfo.szTransportStreamName);
				::SetDlgItemInt(hDlg,IDC_REMOTECONTROLKEYID,ChannelInfo.RemoteControlKeyID,FALSE);
			}
			CurService=pThis->m_pApp->GetService(&NumServices);
			if (CurService>=0
					&& pThis->m_pApp->GetServiceInfo(CurService,&ServiceInfo)) {
				::wsprintf(szText,TEXT("%d / %d"),CurService+1,NumServices);
				::SetDlgItemText(hDlg,IDC_SERVICE,szText);
				::wsprintf(szText,TEXT("0x%x"),ServiceInfo.ServiceID);
				::SetDlgItemText(hDlg,IDC_SERVICEID,szText);
				::SetDlgItemText(hDlg,IDC_SERVICENAME,ServiceInfo.szServiceName);
				::wsprintf(szText,TEXT("0x%x"),ServiceInfo.VideoPID);
				::SetDlgItemText(hDlg,IDC_VIDEOPID,szText);
				::wsprintf(szText,TEXT("0x%x"),ServiceInfo.AudioPID[0]);
				if (ServiceInfo.NumAudioPIDs>1) {
					::wsprintf(szText+::lstrlen(szText),TEXT(" / 0x%x"),ServiceInfo.AudioPID[1]);
				}
				::SetDlgItemText(hDlg,IDC_AUDIOPID,szText);
				if (ServiceInfo.SubtitlePID!=0)
					::wsprintf(szText,TEXT("0x%x"),ServiceInfo.SubtitlePID);
				else
					::lstrcpy(szText,TEXT("<none>"));
				::SetDlgItemText(hDlg,IDC_SUBTITLEPID,szText);
			}
		}
		return TRUE;

	case WM_CTLCOLORSTATIC:
		{
			CTSInfo *pThis=GetThis(hDlg);
			HDC hdc=reinterpret_cast<HDC>(wParam);

			::SetBkMode(hdc,TRANSPARENT);
			::SetTextColor(hdc,pThis->m_crTextColor);
			return reinterpret_cast<BOOL>(pThis->m_hbrBack);
		}

	case WM_CTLCOLORDLG:
		{
			CTSInfo *pThis=GetThis(hDlg);

			return reinterpret_cast<BOOL>(pThis->m_hbrBack);
		}

	case WM_COMMAND:
		if (LOWORD(wParam)==IDCANCEL) {
			// 閉じる時はプラグインを無効にする
			CTSInfo *pThis=GetThis(hDlg);

			pThis->m_pApp->EnablePlugin(false);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		{
			CTSInfo *pThis=GetThis(hDlg);

			::KillTimer(hDlg,1);
			if (pThis->m_hbrBack!=NULL) {
				::DeleteObject(pThis->m_hbrBack);
				pThis->m_hbrBack=NULL;
			}
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CTSInfo;
}
