#include "stdafx.h"
#include "TVTest.h"
#include "CommandLine.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif




class CArgsParser {
	LPWSTR *m_ppszArgList;
	int m_Args;
	int m_CurPos;
public:
	CArgsParser(LPCWSTR pszCmdLine);
	~CArgsParser();
	bool IsSwitch() const;
	bool IsOption(LPCWSTR pszOption) const;
	bool GetOption(LPCWSTR pszOption,bool *pValue);
	bool GetOption(LPCWSTR pszOption,CDynamicString *pValue);
	bool GetOption(LPCWSTR pszOption,LPTSTR pszValue,int MaxLength);
	bool GetOption(LPCWSTR pszOption,int *pValue);
	bool GetOption(LPCWSTR pszOption,DWORD *pValue);
	bool GetDurationOption(LPCWSTR pszOption,DWORD *pValue);
	bool IsEnd() const { return m_CurPos>=m_Args; }
	bool Next();
	LPCWSTR GetText() const;
	bool GetText(LPWSTR pszText,int MaxLength) const;
	bool GetValue(int *pValue) const;
	bool GetValue(DWORD *pValue) const;
	bool GetDurationValue(DWORD *pValue) const;
};


CArgsParser::CArgsParser(LPCWSTR pszCmdLine)
{
	m_ppszArgList=::CommandLineToArgvW(pszCmdLine,&m_Args);
	if (m_ppszArgList==0)
		m_Args=0;
	m_CurPos=0;
}


CArgsParser::~CArgsParser()
{
	if (m_ppszArgList)
		::LocalFree(m_ppszArgList);
}


bool CArgsParser::IsSwitch() const
{
	if (IsEnd())
		return false;
	return m_ppszArgList[m_CurPos][0]=='-' || m_ppszArgList[m_CurPos][0]=='/';
}


bool CArgsParser::IsOption(LPCWSTR pszOption) const
{
	if (IsEnd())
		return false;
	return ::lstrcmpi(m_ppszArgList[m_CurPos]+1,pszOption)==0;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,bool *pValue)
{
	if (IsOption(pszOption)) {
		*pValue=true;
		return true;
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,CDynamicString *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return pValue->Set(GetText());
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,LPTSTR pszValue,int MaxLength)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetText(pszValue,MaxLength);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,int *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetOption(LPCWSTR pszOption,DWORD *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetValue(pValue);
	}
	return false;
}


bool CArgsParser::GetDurationOption(LPCWSTR pszOption,DWORD *pValue)
{
	if (IsOption(pszOption)) {
		if (Next())
			return GetDurationValue(pValue);
	}
	return false;
}


bool CArgsParser::Next()
{
	m_CurPos++;
	return m_CurPos<m_Args;
}


LPCWSTR CArgsParser::GetText() const
{
	if (IsEnd())
		return TEXT("");
	return m_ppszArgList[m_CurPos];
}


bool CArgsParser::GetText(LPWSTR pszText,int MaxLength) const
{
	if (IsEnd())
		return false;
	if (::lstrlen(m_ppszArgList[m_CurPos])>=MaxLength)
		return false;
	::lstrcpy(pszText,m_ppszArgList[m_CurPos]);
	return true;
}


bool CArgsParser::GetValue(int *pValue) const
{
	if (IsEnd())
		return false;
	*pValue=_wtoi(m_ppszArgList[m_CurPos]);
	return true;
}


bool CArgsParser::GetValue(DWORD *pValue) const
{
	if (IsEnd())
		return false;
	*pValue=_wtoi(m_ppszArgList[m_CurPos]);
	return true;
}


bool CArgsParser::GetDurationValue(DWORD *pValue) const
{
	if (IsEnd())
		return false;

	// ?h?m?s 形式の時間指定をパースする
	// 単位の指定が無い場合は秒単位と解釈する
	LPCWSTR p=m_ppszArgList[m_CurPos];
	DWORD DurationSec=0,Duration=0;

	while (*p!='\0') {
		if (*p>='0' && *p<='9') {
			Duration=Duration*10+(*p-'0');
		} else {
			if (*p=='h') {
				DurationSec+=Duration*(60*60);
			} else if (*p=='m') {
				DurationSec+=Duration*60;
			} else if (*p=='s') {
				DurationSec+=Duration;
			}
			Duration=0;
		}
		p++;
	}
	DurationSec+=Duration;
	*pValue=DurationSec;

	return true;
}




CCommandLineParser::CCommandLineParser()
	: m_fNoDescramble(false)
	, m_fUseNetworkRemocon(false)
	, m_UDPPort(1234)
	, m_Channel(0)
	, m_ControllerChannel(0)
	, m_TuningSpace(-1)
	, m_ServiceID(0)
	, m_NetworkID(0)
	, m_TransportStreamID(0)
	, m_fRecord(false)
	, m_fRecordStop(false)
	, m_RecordDelay(0)
	, m_RecordDuration(0)
	, m_fRecordCurServiceOnly(false)
	, m_fExitOnRecordEnd(false)
	, m_fFullscreen(false)
	, m_fMinimize(false)
	, m_fMaximize(false)
	, m_fNoDriver(false)
	, m_fStandby(false)
	, m_fNoView(false)
	, m_fNoDirectShow(false)
	, m_fSilent(false)
	, m_fNoPlugin(false)
	, m_fSingleTask(false)
	, m_fInitialSettings(false)
	, m_fSaveLog(false)
	, m_fRecordOnly(false)
	, m_fNoEpg(false)
	, m_TvRockDID(0)
{
}


/*
	利用可能なコマンドラインオプション

	/ch				チャンネル (e.g. /ch 13)
	/chspace		チューニング空間 (e.g. /chspace 1)
	/d				ドライバの指定 (e.g. /d BonDriver.dll)
	/f /fullscreen	フルスクリーン
	/ini			INIファイル名
	/init			初期設定ダイアログを表示する
	/log			終了時にログを保存する
	/max			最大化状態で起動
	/min			最小化状態で起動
	/nd				スクランブル解除しない
	/nid			ネットワークID
	/nodriver		BonDriverを読み込まない
	/nodshow		DirectShowの初期化をしない
	/noepg			EPG 情報の取得を行わない
	/noplugin		プラグインを読み込まない
	/noview			プレビュー無効
	/nr				ネットワークリモコンを使用する
	/p /port		UDP のポート番号 (e.g. /p 1234)
	/plugin-		指定されたプラグインを読み込まない
	/plugindir		プラグインのフォルダ
	/rch			リモコンチャンネル
	/rec			録画
	/reccurservice	現在のサービスのみ録画
	/recdelay		録画までの時間(秒)
	/recduration	録画時間(秒)
	/recexit		録画終了時にプログラムを終了
	/recfile		録画ファイル名
	/reconly		録画専用モード
	/recstop		録画停止
	/s				複数起動しない
	/sid			サービスID
	/silent			エラー時にダイアログを表示しない
	/standby		待機状態で起動
	/tsid			トランスポートストリームID
*/
void CCommandLineParser::Parse(LPCWSTR pszCmdLine)
{
	CArgsParser Args(pszCmdLine);

	if (Args.IsEnd())
		return;
	do {
		if (Args.IsSwitch()) {
			if (!Args.GetOption(TEXT("ch"),&m_Channel)
					&& !Args.GetOption(TEXT("chspace"),&m_TuningSpace)
					&& !Args.GetOption(TEXT("d"),&m_DriverName)
					&& !Args.GetOption(TEXT("f"),&m_fFullscreen)
					&& !Args.GetOption(TEXT("fullscreen"),&m_fFullscreen)
					&& !Args.GetOption(TEXT("ini"),&m_IniFileName)
					&& !Args.GetOption(TEXT("init"),&m_fInitialSettings)
					&& !Args.GetOption(TEXT("log"),&m_fSaveLog)
					&& !Args.GetOption(TEXT("max"),&m_fMaximize)
					&& !Args.GetOption(TEXT("min"),&m_fMinimize)
					&& !Args.GetOption(TEXT("nd"),&m_fNoDescramble)
					&& !Args.GetOption(TEXT("nodriver"),&m_fNoDriver)
					&& !Args.GetOption(TEXT("nodshow"),&m_fNoDirectShow)
					&& !Args.GetOption(TEXT("noepg"),&m_fNoEpg)
					&& !Args.GetOption(TEXT("noplugin"),&m_fNoPlugin)
					&& !Args.GetOption(TEXT("noview"),&m_fNoView)
					&& !Args.GetOption(TEXT("nr"),&m_fUseNetworkRemocon)
					&& !Args.GetOption(TEXT("nid"),&m_NetworkID)
					&& !Args.GetOption(TEXT("p"),&m_UDPPort)
					&& !Args.GetOption(TEXT("port"),&m_UDPPort)
					&& !Args.GetOption(TEXT("plugindir"),&m_PluginsDirectory)
					&& !Args.GetOption(TEXT("pluginsdir"),&m_PluginsDirectory)
					&& !Args.GetOption(TEXT("rec"),&m_fRecord)
					&& !Args.GetOption(TEXT("reccurservice"),&m_fRecordCurServiceOnly)
					&& !Args.GetDurationOption(TEXT("recdelay"),&m_RecordDelay)
					&& !Args.GetDurationOption(TEXT("recduration"),&m_RecordDuration)
					&& !Args.GetOption(TEXT("recexit"),&m_fExitOnRecordEnd)
					&& !Args.GetOption(TEXT("recfile"),&m_RecordFileName)
					&& !Args.GetOption(TEXT("reconly"),&m_fRecordOnly)
					&& !Args.GetOption(TEXT("recstop"),&m_fRecordStop)
					&& !Args.GetOption(TEXT("rch"),&m_ControllerChannel)
					&& !Args.GetOption(TEXT("s"),&m_fSingleTask)
					&& !Args.GetOption(TEXT("sid"),&m_ServiceID)
					&& !Args.GetOption(TEXT("silent"),&m_fSilent)
					&& !Args.GetOption(TEXT("standby"),&m_fStandby)
					&& !Args.GetOption(TEXT("tsid"),&m_TransportStreamID)) {
				if (Args.IsOption(TEXT("plugin-"))) {
					if (Args.Next()) {
						TCHAR szPlugin[MAX_PATH];
						if (Args.GetText(szPlugin,MAX_PATH))
							m_NoLoadPlugins.push_back(CDynamicString(szPlugin));
					}
				} else if (Args.IsOption(TEXT("did"))) {
					if (Args.Next()) {
						const TCHAR DID=Args.GetText()[0];

						if (DID>='A' && DID<='Z')
							m_TvRockDID=DID-'A';
						else if (DID>='a' && DID<='z')
							m_TvRockDID=DID-'a';
					}
				}
#ifdef _DEBUG
				else {
					TRACE(TEXT("Unknown command line option %s\n"),Args.GetText());
				}
#endif
			}
		} else {
			// なぜかudp://@:1234のようにポートを指定できると思っている人が多いので、対応しておく
			if (::wcsncmp(Args.GetText(),L"udp://@:",8)==0)
				m_UDPPort=::_wtoi(Args.GetText()+8);
		}
	} while (Args.Next());
	if (m_fRecordOnly) {
		m_fNoDirectShow=true;
	}
}


bool CCommandLineParser::IsChannelSpecified() const
{
	return m_Channel>0 || m_ControllerChannel>0 || m_ServiceID>0
		|| m_NetworkID>0 || m_TransportStreamID>0;
}
