#include "stdafx.h"
#include "CommandLine.h"




CCommandLineParser::CCommandLineParser()
{
	m_szDriverName[0]='\0';
	m_fNoDescramble=false;
	m_fUseNetworkRemocon=false;
	m_UDPPort=1234;
	m_Channel=0;
	m_ControllerChannel=0;
	m_fRecord=false;
	m_fFullscreen=false;
	m_fNoView=false;
	m_fSchedule=false;
	m_fInitialSettings=false;
}


/*
	利用可能なコマンドラインオプション

	/ch		チャンネル (e.g. /ch 13)
	/d		ドライバの指定 (e.g. /d BonDriver.dll)
	/f		フルスクリーン
	/init	初期設定ダイアログを表示する
	/nd		スクランブル解除しない
	/nr		ネットワークリモコンを使用する
	/p		UDP のポート番号 (e.g. /p 1234)
	/rch	リモコンチャンネル
	/rec	録画
	/s		複数起動しない
*/
void CCommandLineParser::Parse(LPCWSTR pszCmdLine)
{
	LPWSTR *ppszArgList;
	int Args;

	ppszArgList=::CommandLineToArgvW(pszCmdLine,&Args);
	if (ppszArgList!=NULL) {
		int i;

		for (i=0;i<Args;i++) {
			if (ppszArgList[i][0]=='/' || ppszArgList[i][0]=='-') {
				if (::lstrcmpi(ppszArgList[i]+1,TEXT("ch"))==0) {
					i++;
					if (i<Args)
						m_Channel=::_wtoi(ppszArgList[i]);
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("d"))==0) {
					i++;
					if (i<Args && ::lstrlen(ppszArgList[i])<MAX_PATH)
						::lstrcpy(m_szDriverName,ppszArgList[i]);
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("fullscreen"))==0
						|| ::lstrcmpi(ppszArgList[i]+1,TEXT("f"))==0) {
					m_fFullscreen=true;
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("init"))==0) {
					m_fInitialSettings=true;
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("nd"))==0) {
					m_fNoDescramble=true;
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("noview"))==0) {
					m_fNoView=true;
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("nr"))==0) {
					m_fUseNetworkRemocon=true;
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("p"))==0
						|| ::lstrcmpi(ppszArgList[i]+1,TEXT("port"))==0) {
					i++;
					if (i<Args)
						m_UDPPort=::_wtoi(ppszArgList[i]);
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("rec"))==0) {
					m_fRecord=true;
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("rch"))==0) {
					i++;
					if (i<Args)
						m_ControllerChannel=::_wtoi(ppszArgList[i]);
				} else if (::lstrcmpi(ppszArgList[i]+1,TEXT("schedule"))==0
						|| ::lstrcmpi(ppszArgList[i]+1,TEXT("s"))==0) {
					m_fSchedule=true;
				}
			} else if (::wcsncmp(ppszArgList[i],L"udp://@:",8)==0) {
				// なぜかudp://@:1234のようにポートを指定できると思っている人が多いので、対応しておく
				m_UDPPort=::_wtoi(ppszArgList[i]+8);
			}
		}
		::LocalFree(ppszArgList);
	}
}
