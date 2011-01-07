#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H


#include <vector>


class CCommandLineParser
{
public:
	CCommandLineParser();
	void Parse(LPCWSTR pszCmdLine);
	bool IsChannelSpecified() const;
//private:
	CDynamicString m_IniFileName;
	CDynamicString m_DriverName;
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
	FILETIME m_RecordStartTime;
	int m_RecordDelay;
	int m_RecordDuration;
	CDynamicString m_RecordFileName;
	bool m_fRecordCurServiceOnly;
	bool m_fExitOnRecordEnd;
	bool m_fFullscreen;
	bool m_fMinimize;
	bool m_fMaximize;
	bool m_fNoDriver;
	bool m_fStandby;
	bool m_fNoView;
	bool m_fNoDirectShow;
	bool m_fSilent;
	bool m_fNoPlugin;
	std::vector<CDynamicString> m_NoLoadPlugins;
	CDynamicString m_PluginsDirectory;
	bool m_fSingleTask;
	bool m_fInitialSettings;
	bool m_fSaveLog;
	bool m_fRecordOnly;
	bool m_fNoEpg;
	int m_TvRockDID;
	int m_Volume;
	bool m_fMute;
};


#endif
