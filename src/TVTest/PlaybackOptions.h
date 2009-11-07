#ifndef PLAYBACK_OPTIONS_H
#define PLAYBACK_OPTIONS_H


#include "Options.h"


class CPlaybackOptions : public COptions
{
	enum {
		UPDATE_PACKETBUFFERING	=0x00000001UL
	};
	enum { MAX_AUDIO_DEVICE_NAME=128 };
	TCHAR m_szAudioDeviceName[MAX_AUDIO_DEVICE_NAME];
	bool m_fDownMixSurround;
	bool m_fRestoreMute;
	bool m_fUseAudioRendererClock;
	bool m_fAdjustAudioStreamTime;
	bool m_fMinTimerResolution;
	bool m_fPacketBuffering;
	DWORD m_PacketBufferLength;
	int m_PacketBufferPoolPercentage;

	static CPlaybackOptions *GetThis(HWND hDlg);

public:
	CPlaybackOptions();
	~CPlaybackOptions();
	LPCTSTR GetAudioDeviceName() const { return m_szAudioDeviceName; }
	bool GetDownMixSurround() const { return m_fDownMixSurround; }
	bool GetRestoreMute() const { return m_fRestoreMute; }
	bool GetUseAudioRendererClock() const { return m_fUseAudioRendererClock; }
	bool GetAdjustAudioStreamTime() const { return m_fAdjustAudioStreamTime; }
	bool GetMinTimerResolution() const { return m_fMinTimerResolution; }
	bool GetPacketBuffering() const { return m_fPacketBuffering; }
	bool SetPacketBuffering(bool fBuffering);
	DWORD GetPacketBufferLength() const { return m_PacketBufferLength; }
	int GetPacketBufferPoolPercentage() const { return m_PacketBufferPoolPercentage; }
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
// COptions
	bool Apply(DWORD Flags);
	bool Read(CSettings *pSettings);
	bool Write(CSettings *pSettings) const;
};


#endif
