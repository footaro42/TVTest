#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H


class CCommandLineParser {
public:
	CCommandLineParser();
	void Parse(LPCWSTR pszCmdLine);
//private:
	TCHAR m_szDriverName[MAX_PATH];
	bool m_fNoDescramble;
	bool m_fUseNetworkRemocon;
	DWORD m_UDPPort;
	int m_Channel;
	int m_ControllerChannel;
	int m_ServiceID;
	bool m_fRecord;
	DWORD m_RecordDelay;
	DWORD m_RecordDuration;
	TCHAR m_szRecordFileName[MAX_PATH];
	bool m_fFullscreen;
	bool m_fNoView;
	bool m_fSchedule;
	bool m_fInitialSettings;
	bool m_fSaveLog;
};


#endif
