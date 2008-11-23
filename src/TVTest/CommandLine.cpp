#include "stdafx.h"
#include "CommandLine.h"




class CArgsParser {
	LPWSTR *m_ppszArgList;
	int m_Args;
	int m_CurPos;
public:
	CArgsParser(LPCWSTR pszCmdLine);
	~CArgsParser();
	bool IsSwitch() const;
	bool IsOption(LPCWSTR pszOption) const;
	bool IsEnd() const { return m_CurPos>=m_Args; }
	bool Next();
	LPCWSTR GetText() const;
	bool GetText(LPWSTR pszText,int MaxLength) const;
	bool GetValue(int *pValue) const;
	bool GetValue(DWORD *pValue) const;
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




CCommandLineParser::CCommandLineParser()
{
	m_szDriverName[0]='\0';
	m_fNoDescramble=false;
	m_fUseNetworkRemocon=false;
	m_UDPPort=1234;
	m_Channel=0;
	m_ControllerChannel=0;
	m_ServiceID=0;
	m_NetworkID=0;
	m_TransportStreamID=0;
	m_fRecord=false;
	m_RecordDelay=0;
	m_RecordDuration=0;
	m_szRecordFileName[0]='\0';
	m_fFullscreen=false;
	m_fNoView=false;
	m_fSilent=false;
	m_fNoDriver=false;
	m_fNoDirectShow=false;
	m_fNoPlugin=false;
	m_fSchedule=false;
	m_fInitialSettings=false;
	m_fSaveLog=false;
}


/*
	利用可能なコマンドラインオプション

	/ch				チャンネル (e.g. /ch 13)
	/d				ドライバの指定 (e.g. /d BonDriver.dll)
	/f				フルスクリーン
	/init			初期設定ダイアログを表示する
	/log			終了時にログを保存する
	/nd				スクランブル解除しない
	/nid			ネットワークID
	/nodriver		BonDriverを読み込まない
	/nodshow		DirectShowの初期化をしない
	/noplugin		プラグインを読み込まない
	/noview			プレビュー無効
	/nr				ネットワークリモコンを使用する
	/p				UDP のポート番号 (e.g. /p 1234)
	/rch			リモコンチャンネル
	/rec			録画
	/recdelay		録画までの時間(秒)
	/recduration	録画時間(秒)
	/recfile		録画ファイル名
	/s				複数起動しない
	/sid			サービスID
	/silent			エラー時にダイアログを表示しない
	/tsid			トランスポートストリームID
*/
void CCommandLineParser::Parse(LPCWSTR pszCmdLine)
{
	CArgsParser Args(pszCmdLine);

	if (Args.IsEnd())
		return;
	do {
		if (Args.IsSwitch()) {
			if (Args.IsOption(TEXT("ch"))) {
				if (Args.Next())
					Args.GetValue(&m_Channel);
			} else if (Args.IsOption(TEXT("d"))) {
				if (Args.Next())
					Args.GetText(m_szDriverName,MAX_PATH);
			} else if (Args.IsOption(TEXT("fullscreen"))
					|| Args.IsOption(TEXT("f"))) {
				m_fFullscreen=true;
			} else if (Args.IsOption(TEXT("init"))) {
				m_fInitialSettings=true;
			} else if (Args.IsOption(TEXT("log"))) {
				m_fSaveLog=true;
			} else if (Args.IsOption(TEXT("nd"))) {
				m_fNoDescramble=true;
			} else if (Args.IsOption(TEXT("nid"))) {
				if (Args.Next())
					Args.GetValue(&m_NetworkID);
			} else if (Args.IsOption(TEXT("nodriver"))) {
				m_fNoDriver=true;
			} else if (Args.IsOption(TEXT("nodshow"))) {
				m_fNoDirectShow=true;
			} else if (Args.IsOption(TEXT("noplugin"))) {
				m_fNoPlugin=true;
			} else if (Args.IsOption(TEXT("noview"))) {
				m_fNoView=true;
			} else if (Args.IsOption(TEXT("nr"))) {
				m_fUseNetworkRemocon=true;
			} else if (Args.IsOption(TEXT("p"))
					|| Args.IsOption(TEXT("port"))) {
				if (Args.Next())
					Args.GetValue(&m_UDPPort);
			} else if (Args.IsOption(TEXT("rec"))) {
				m_fRecord=true;
			} else if (Args.IsOption(TEXT("recdelay"))) {
				if (Args.Next())
					Args.GetValue(&m_RecordDelay);
			} else if (Args.IsOption(TEXT("recduration"))) {
				if (Args.Next())
					Args.GetValue(&m_RecordDuration);
			} else if (Args.IsOption(TEXT("recfile"))) {
				if (Args.Next())
					Args.GetText(m_szRecordFileName,MAX_PATH);
			} else if (Args.IsOption(TEXT("rch"))) {
				if (Args.Next())
					Args.GetValue(&m_ControllerChannel);
			} else if (Args.IsOption(TEXT("schedule"))
					|| Args.IsOption(TEXT("s"))) {
				m_fSchedule=true;
			} else if (Args.IsOption(TEXT("sid"))) {
				if (Args.Next())
					Args.GetValue(&m_ServiceID);
			} else if (Args.IsOption(TEXT("silent"))) {
				m_fSilent=true;
			} else if (Args.IsOption(TEXT("tsid"))) {
				if (Args.Next())
					Args.GetValue(&m_TransportStreamID);
			}
		} else {
			// なぜかudp://@:1234のようにポートを指定できると思っている人が多いので、対応しておく
			if (::wcsncmp(Args.GetText(),L"udp://@:",8)==0)
				m_UDPPort=::_wtoi(Args.GetText()+8);
		}
	} while (Args.Next());
}


bool CCommandLineParser::IsChannelSpecified() const
{
	return m_Channel>0 || m_ControllerChannel>0 || m_ServiceID>0
		|| m_NetworkID>0 || m_TransportStreamID>0;
}
