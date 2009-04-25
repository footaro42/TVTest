/*
	TVTest プラグインサンプル

	指定時間後にスリープする
*/


#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlwapi.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "resource.h"


#pragma comment(lib,"comctl32.lib")
#pragma comment(lib,"shlwapi.lib")


// ウィンドウクラス名
#define SLEEPTIMER_WINDOW_CLASS TEXT("TVTest SleepTimer Window")

// スリープを実行するタイマーの識別子
#define SLEEP_TIMER_ID 1


// プラグインクラス
class CSleepTimer : public TVTest::CTVTestPlugin
{
	TCHAR m_szIniFileName[MAX_PATH];	// INIファイルのパス
	DWORD m_SleepTime;					// スリープまでの時間(秒単位)
	enum SleepMode {
		MODE_EXIT,		// TVTest終了
		MODE_POWEROFF,	// 電源オフ
		MODE_LOGOFF,	// ログオフ
		MODE_SUSPEND,	// サスペンド
		MODE_HIBERNATE	// ハイバネート
	};
	SleepMode m_Mode;					// スリープ時の動作
	bool m_fForce;						// 強制
	bool m_fConfirm;					// 確認を取る
	bool m_fShowSettings;				// プラグイン有効時に設定表示
	HWND m_hwnd;						// ウィンドウハンドル
	bool m_fEnabled;					// プラグインが有効か?
	int m_ConfirmTimeout;				// 確認のタイムアウト時間(秒単位)
	int m_ConfirmTimerCount;			// 確認のタイマー
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static CSleepTimer *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	static BOOL CALLBACK ConfirmDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
	bool DoSleep();
public:
	CSleepTimer();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();
};


CSleepTimer::CSleepTimer()
	: m_SleepTime(600)
	, m_Mode(MODE_EXIT)
	, m_fForce(false)
	, m_fConfirm(true)
	, m_fShowSettings(true)
	, m_hwnd(NULL)
	, m_fEnabled(false)
	, m_ConfirmTimeout(10)
{
}


bool CSleepTimer::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS;
	pInfo->pszPluginName  = L"SleepTimer";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"指定時間後にスリープする";
	return true;
}


bool CSleepTimer::Initialize()
{
	// 初期化処理

	// 設定の読み込み
	::GetModuleFileName(g_hinstDLL,m_szIniFileName,MAX_PATH);
	::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	m_SleepTime=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Time"),m_SleepTime,m_szIniFileName);
	m_Mode=(SleepMode)::GetPrivateProfileInt(TEXT("Settings"),TEXT("Mode"),m_Mode,m_szIniFileName);
	m_fForce=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Force"),m_fForce,m_szIniFileName)!=0;
	m_fConfirm=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Confirm"),m_fConfirm,m_szIniFileName)!=0;
	m_fShowSettings=::GetPrivateProfileInt(TEXT("Settings"),TEXT("ShowSettings"),m_fShowSettings,m_szIniFileName)!=0;

	// ウィンドウクラスの登録
	WNDCLASS wc;
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=g_hinstDLL;
	wc.hIcon=NULL;
	wc.hCursor=NULL;
	wc.hbrBackground=NULL;
	wc.lpszMenuName=NULL;
	wc.lpszClassName=SLEEPTIMER_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	// ウィンドウの作成
	m_hwnd=::CreateWindowEx(0,SLEEPTIMER_WINDOW_CLASS,NULL,WS_POPUP,
							0,0,0,0,
							m_pApp->GetAppWindow(),NULL,g_hinstDLL,this);
	if (m_hwnd==NULL)
		return false;

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	return true;
}


BOOL WritePrivateProfileInt(LPCTSTR pszAppName,LPCTSTR pszKeyName,int Value,LPCTSTR pszFileName)
{
	TCHAR szValue[16];

	wsprintf(szValue,TEXT("%d"),Value);
	return WritePrivateProfileString(pszAppName,pszKeyName,szValue,pszFileName);
}


bool CSleepTimer::Finalize()
{
	// 終了処理

	m_pApp->EnablePlugin(false);	// 次回起動時に有効にならないように

	// ウィンドウの破棄
	::DestroyWindow(m_hwnd);

	// 設定の保存
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("Time"),m_SleepTime,m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("Mode"),m_Mode,m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("Force"),m_fForce,m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("Confirm"),m_fConfirm,m_szIniFileName);
	::WritePrivateProfileInt(TEXT("Settings"),TEXT("ShowSettings"),m_fShowSettings,m_szIniFileName);

	return true;
}


// スリープ実行
bool CSleepTimer::DoSleep()
{
	if (m_Mode!=MODE_EXIT && m_Mode!=MODE_LOGOFF) {
		// 権限設定
		HANDLE hToken;

		if (::OpenProcessToken(::GetCurrentProcess(),
							   TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,&hToken)) {
			LUID ld;
			LUID_AND_ATTRIBUTES la;
			TOKEN_PRIVILEGES tp;

			::LookupPrivilegeValue(NULL,SE_SHUTDOWN_NAME,&ld);
			la.Luid=ld;
			la.Attributes=SE_PRIVILEGE_ENABLED;
			tp.PrivilegeCount=1;
			tp.Privileges[0]=la;
			::AdjustTokenPrivileges(hToken,FALSE,&tp,0,NULL,NULL);
			::CloseHandle(hToken);
		}
	}
	switch (m_Mode) {
	case MODE_EXIT:
		if (!m_pApp->Close(m_fForce?TVTest::CLOSE_EXIT:0))
			return false;
		break;
	case MODE_POWEROFF:
		if (!::ExitWindowsEx((m_fForce?EWX_FORCE:0) | EWX_POWEROFF,0))
			return false;
		break;
	case MODE_LOGOFF:
		if (!::ExitWindowsEx((m_fForce?EWX_FORCE:0) | EWX_LOGOFF,0))
			return false;
		break;
	case MODE_SUSPEND:
		if (!::SetSystemPowerState(TRUE,m_fForce))
			return FALSE;
		break;
	case MODE_HIBERNATE:
		if (!::SetSystemPowerState(FALSE,m_fForce))
			return false;
		break;
	default:
		return false;
	}
	return true;
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CSleepTimer::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CSleepTimer *pThis=static_cast<CSleepTimer*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		if (lParam1!=0 && pThis->m_fShowSettings) {
			if (::DialogBoxParam(g_hinstDLL,MAKEINTRESOURCE(IDD_SETTINGS),
								 pThis->m_pApp->GetAppWindow(),SettingsDlgProc,
								 reinterpret_cast<LPARAM>(pThis))!=IDOK)
				return FALSE;
		}
		pThis->m_fEnabled=lParam1!=0;
		if (pThis->m_fEnabled)
			::SetTimer(pThis->m_hwnd,SLEEP_TIMER_ID,pThis->m_SleepTime*1000,NULL);
		else
			::KillTimer(pThis->m_hwnd,SLEEP_TIMER_ID);
		return TRUE;

	case TVTest::EVENT_PLUGINSETTINGS:
		// プラグインの設定を行う
		return ::DialogBoxParam(g_hinstDLL,MAKEINTRESOURCE(IDD_SETTINGS),
								reinterpret_cast<HWND>(lParam1),SettingsDlgProc,
								reinterpret_cast<LPARAM>(pThis))==IDOK;
	}
	return 0;
}


// ウィンドウハンドルからthisを取得する
CSleepTimer *CSleepTimer::GetThis(HWND hwnd)
{
	return reinterpret_cast<CSleepTimer*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


// ウィンドウプロシージャ
// 単にタイマーを処理するだけ
LRESULT CALLBACK CSleepTimer::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CSleepTimer *pThis=static_cast<CSleepTimer*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis=GetThis(hwnd);
			TVTest::RecordStatusInfo Info;

			pThis->m_pApp->EnablePlugin(false);	// タイマーは一回限り有効
			::KillTimer(hwnd,wParam);			// EventCallbackで呼ばれるはずだが、念のため

			if (pThis->m_fConfirm) {
				// 確認ダイアログを表示
				if (::DialogBoxParam(g_hinstDLL,MAKEINTRESOURCE(IDD_CONFIRM),
									 pThis->m_pApp->GetAppWindow(),ConfirmDlgProc,
									 reinterpret_cast<LPARAM>(pThis))!=IDOK)
					return 0;
			}

			// 録画中はスリープ実行しない
			if (!pThis->m_pApp->GetRecordStatus(&Info)
					|| Info.Status!=TVTest::RECORD_STATUS_NOTRECORDING) {
				pThis->m_pApp->AddLog(L"録画中なのでスリープがキャンセルされました。");
				return 0;
			}

			// スリープ実行
			pThis->DoSleep();
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


// 設定ダイアログプロシージャ
BOOL CALLBACK CSleepTimer::SettingsDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis=reinterpret_cast<CSleepTimer*>(lParam);
			static LPCTSTR ModeList[] = {
				TEXT("終了"),
				TEXT("電源オフ"),
				TEXT("ログオフ"),
				TEXT("サスペンド"),
				TEXT("ハイバネート"),
			};

			::SetProp(hDlg,TEXT("This"),pThis);
			::SetDlgItemInt(hDlg,IDC_SETTINGS_TIME_HOURS,pThis->m_SleepTime/(60*60),FALSE);
			::SendDlgItemMessage(hDlg,IDC_SETTINGS_TIME_HOURS_UD,UDM_SETRANGE,0,MAKELPARAM(UD_MAXVAL,0));
			::SetDlgItemInt(hDlg,IDC_SETTINGS_TIME_MINUTES,pThis->m_SleepTime/60%60,FALSE);
::SendDlgItemMessage(hDlg,IDC_SETTINGS_TIME_MINUTES_UD,UDM_SETRANGE,0,MAKELPARAM(59,0));
			::SetDlgItemInt(hDlg,IDC_SETTINGS_TIME_SECONDS,pThis->m_SleepTime%60,FALSE);
::SendDlgItemMessage(hDlg,IDC_SETTINGS_TIME_SECONDS_UD,UDM_SETRANGE,0,MAKELPARAM(59,0));
			for (int i=0;i<sizeof(ModeList)/sizeof(LPCTSTR);i++)
				::SendDlgItemMessage(hDlg,IDC_SETTINGS_MODE,CB_ADDSTRING,0,(LPARAM)ModeList[i]);
			::SendDlgItemMessage(hDlg,IDC_SETTINGS_MODE,CB_SETCURSEL,(WPARAM)pThis->m_Mode,0);
			::CheckDlgButton(hDlg,IDC_SETTINGS_FORCE,pThis->m_fForce?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_SETTINGS_CONFIRM,pThis->m_fConfirm?BST_CHECKED:BST_UNCHECKED);
			::CheckDlgButton(hDlg,IDC_SETTINGS_SHOWSETTINGS,pThis->m_fShowSettings?BST_CHECKED:BST_UNCHECKED);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			{
				CSleepTimer *pThis=static_cast<CSleepTimer*>(::GetProp(hDlg,TEXT("This")));

				ULONGLONG Time=
					(ULONGLONG)::GetDlgItemInt(hDlg,IDC_SETTINGS_TIME_HOURS,NULL,FALSE)*(60*60)+
					(ULONGLONG)::GetDlgItemInt(hDlg,IDC_SETTINGS_TIME_MINUTES,NULL,FALSE)*60ULL+
					(ULONGLONG)::GetDlgItemInt(hDlg,IDC_SETTINGS_TIME_SECONDS,NULL,FALSE);
				if (Time>USER_TIMER_MAXIMUM) {
					::MessageBox(hDlg,TEXT("スリープまでの時間が未来すぎるよ。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				if (Time==0) {
					::MessageBox(hDlg,TEXT("時間の指定がおかしいよ。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				pThis->m_SleepTime=(DWORD)Time;
				pThis->m_Mode=(SleepMode)::SendDlgItemMessage(hDlg,IDC_SETTINGS_MODE,CB_GETCURSEL,0,0);
				pThis->m_fForce=::IsDlgButtonChecked(hDlg,IDC_SETTINGS_FORCE)==BST_CHECKED;
				pThis->m_fConfirm=::IsDlgButtonChecked(hDlg,IDC_SETTINGS_CONFIRM)==BST_CHECKED;
				pThis->m_fShowSettings=::IsDlgButtonChecked(hDlg,IDC_SETTINGS_SHOWSETTINGS)==BST_CHECKED;
				if (pThis->m_fEnabled)
					::SetTimer(pThis->m_hwnd,SLEEP_TIMER_ID,pThis->m_SleepTime*1000,NULL);
			}
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
			return TRUE;
		}
		return TRUE;

	case WM_NCDESTROY:
		::RemoveProp(hDlg,TEXT("This"));
		return TRUE;
	}
	return FALSE;
}


// 確認ダイアログプロシージャ
BOOL CALLBACK CSleepTimer::ConfirmDlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		{
			CSleepTimer *pThis=reinterpret_cast<CSleepTimer*>(lParam);

			::SetProp(hDlg,TEXT("This"),pThis);
			::SetDlgItemInt(hDlg,IDC_CONFIRM_TIMEOUT,pThis->m_ConfirmTimeout,TRUE);
			pThis->m_ConfirmTimerCount=0;
			::SetTimer(hDlg,1,1000,NULL);
		}
		return TRUE;

	case WM_TIMER:
		{
			CSleepTimer *pThis=static_cast<CSleepTimer*>(::GetProp(hDlg,TEXT("This")));

			pThis->m_ConfirmTimerCount++;
			if (pThis->m_ConfirmTimerCount<pThis->m_ConfirmTimeout) {
				::SetDlgItemInt(hDlg,IDC_CONFIRM_TIMEOUT,pThis->m_ConfirmTimeout-pThis->m_ConfirmTimerCount,TRUE);
			} else {
				::EndDialog(hDlg,IDOK);
			}
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
		case IDCANCEL:
			::EndDialog(hDlg,LOWORD(wParam));
		}
		return TRUE;
	}
	return FALSE;
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CSleepTimer;
}
