#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H


class CCommandLineParser {
public:
	CCommandLineParser();
	void Parse(LPCWSTR pszCmdLine);
	bool IsChannelSpecified() const;
//private:
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szDriverName[MAX_PATH];
	bool m_fNoDescramble;
	bool m_fUseNetworkRemocon;
	DWORD m_UDPPort;
	int m_Channel;
	int m_ControllerChannel;
	int m_TuningSpace;
	int m_ServiceID;
	int m_NetworkID;
	int m_TransportStreamID;
	bool m_fRecord;
	bool m_fRecordStop;
	DWORD m_RecordDelay;
	DWORD m_RecordDuration;
	TCHAR m_szRecordFileName[MAX_PATH];
	bool m_fExitOnRecordEnd;
	bool m_fFullscreen;
	bool m_fMinimize;
	bool m_fNoDriver;
	bool m_fStandby;
	bool m_fNoView;
	bool m_fNoDirectShow;
	bool m_fSilent;
	bool m_fNoPlugin;
	bool m_fSchedule;
	bool m_fInitialSettings;
	bool m_fSaveLog;
	bool m_fRecordOnly;
};


#endif
