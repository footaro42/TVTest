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
	bool m_fRecord;
	bool m_fFullscreen;
	bool m_fNoView;
	bool m_fSchedule;
	bool m_fInitialSettings;
};


#endif
